/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"

#include "postit.hxx"

#include <rtl/ustrbuf.hxx>
#include <unotools/useroptions.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdocapt.hxx>
#include <editeng/outlobj.hxx>
#include <editeng/editobj.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>

#include "scitems.hxx"
#include <svx/xlnstit.hxx>
#include <svx/xlnstwit.hxx>
#include <svx/xlnstcit.hxx>
#include <svx/sxcecitm.hxx>
#include <svx/xflclit.hxx>
#include <svx/svdlegacy.hxx>

#include "document.hxx"
#include "docpool.hxx"
#include "patattr.hxx"
#include "cell.hxx"
#include "drwlayer.hxx"
#include "userdat.hxx"
#include "detfunc.hxx"

using ::rtl::OUString;
using ::rtl::OUStringBuffer;

// ============================================================================

namespace {

const long SC_NOTECAPTION_WIDTH             =  2900;    /// Default width of note caption textbox.
const long SC_NOTECAPTION_MAXWIDTH_TEMP     = 12000;    /// Maximum width of temporary note caption textbox.
const long SC_NOTECAPTION_HEIGHT            =  1800;    /// Default height of note caption textbox.
const long SC_NOTECAPTION_CELLDIST          =   600;    /// Default distance of note captions to border of anchor cell.
const long SC_NOTECAPTION_OFFSET_Y          = -1500;    /// Default Y offset of note captions to top border of anchor cell.
const long SC_NOTECAPTION_OFFSET_X          =  1500;    /// Default X offset of note captions to left border of anchor cell.
const long SC_NOTECAPTION_BORDERDIST_TEMP   =   100;    /// Distance of temporary note captions to visible sheet area.

// ============================================================================

/** Static helper functions for caption objects. */
class ScCaptionUtil
{
public:
    /** Moves the caption object to the correct layer according to passed visibility. */
    static void         SetCaptionLayer( SdrCaptionObj& rCaption, bool bShown );
    /** Sets basic caption settings required for note caption objects. */
    static void         SetBasicCaptionSettings( SdrCaptionObj& rCaption, bool bShown );
    /** Stores the cell position of the note in the user data area of the caption. */
    static void         SetCaptionUserData( SdrCaptionObj& rCaption, const ScAddress& rPos );
    /** Sets all default formatting attributes to the caption object. */
    static void         SetDefaultItems( SdrCaptionObj& rCaption, ScDocument& rDoc );
    /** Updates caption item set according to the passed item set while removing shadow items. */
    static void         SetCaptionItems( SdrCaptionObj& rCaption, const SfxItemSet& rItemSet );
};

// ----------------------------------------------------------------------------

void ScCaptionUtil::SetCaptionLayer( SdrCaptionObj& rCaption, bool bShown )
{
    SdrLayerID nLayer = bShown ? SC_LAYER_INTERN : SC_LAYER_HIDDEN;
    if( nLayer != rCaption.GetLayer() )
        rCaption.SetLayer( nLayer );
}

void ScCaptionUtil::SetBasicCaptionSettings( SdrCaptionObj& rCaption, bool bShown )
{
    ScDrawLayer::SetAnchor( &rCaption, SCA_PAGE );
    SetCaptionLayer( rCaption, bShown );
    rCaption.SetFixedTail();
    rCaption.SetSpecialTextBoxShadow();
}

void ScCaptionUtil::SetCaptionUserData( SdrCaptionObj& rCaption, const ScAddress& rPos )
{
    // pass true to ScDrawLayer::GetObjData() to create the object data entry
    ScDrawObjData* pObjData = ScDrawLayer::GetObjData( rCaption, true );
    OSL_ENSURE( pObjData, "ScCaptionUtil::SetCaptionUserData - missing drawing object user data" );
    pObjData->maStart = rPos;
    pObjData->mbNote = true;
}

void ScCaptionUtil::SetDefaultItems( SdrCaptionObj& rCaption, ScDocument& rDoc )
{
    SfxItemSet aItemSet = rCaption.GetMergedItemSet();

    // caption tail arrow
    ::basegfx::B2DPolygon aTriangle;
    aTriangle.append( ::basegfx::B2DPoint( 10.0,  0.0 ) );
    aTriangle.append( ::basegfx::B2DPoint(  0.0, 30.0 ) );
    aTriangle.append( ::basegfx::B2DPoint( 20.0, 30.0 ) );
    aTriangle.setClosed( true );
    /*  #99319# Line ends are now created with an empty name. The
        checkForUniqueItem() method then finds a unique name for the item's
        value. */
    aItemSet.Put( XLineStartItem( String::EmptyString(), ::basegfx::B2DPolyPolygon( aTriangle ) ) );
    aItemSet.Put( XLineStartWidthItem( 200 ) );
    aItemSet.Put( XLineStartCenterItem( sal_False ) );
    aItemSet.Put( XFillStyleItem( XFILL_SOLID ) );
    aItemSet.Put( XFillColorItem( String::EmptyString(), ScDetectiveFunc::GetCommentColor() ) );
    aItemSet.Put( SdrCaptionEscDirItem( SDRCAPT_ESCBESTFIT ) );

    // shadow
    /*  SdrShadowItem has sal_False, instead the shadow is set for the
        rectangle only with SetSpecialTextBoxShadow() when the object is
        created (item must be set to adjust objects from older files). */
    aItemSet.Put( SdrOnOffItem(SDRATTR_SHADOW, sal_False ) );
    aItemSet.Put( SdrMetricItem(SDRATTR_SHADOWXDIST, 100 ) );
    aItemSet.Put( SdrMetricItem(SDRATTR_SHADOWYDIST, 100 ) );

    // text attributes
    aItemSet.Put( SdrMetricItem(SDRATTR_TEXT_LEFTDIST, 100 ) );
    aItemSet.Put( SdrMetricItem(SDRATTR_TEXT_RIGHTDIST, 100 ) );
    aItemSet.Put( SdrMetricItem(SDRATTR_TEXT_UPPERDIST, 100 ) );
    aItemSet.Put( SdrMetricItem(SDRATTR_TEXT_LOWERDIST, 100 ) );
    aItemSet.Put( SdrOnOffItem(SDRATTR_TEXT_AUTOGROWWIDTH, sal_False ) );
    aItemSet.Put( SdrOnOffItem(SDRATTR_TEXT_AUTOGROWHEIGHT, sal_True ) );
    // #78943# use the default cell style to be able to modify the caption font
    const ScPatternAttr& rDefPattern = static_cast< const ScPatternAttr& >( rDoc.GetPool()->GetDefaultItem( ATTR_PATTERN ) );
    rDefPattern.FillEditItemSet( &aItemSet );

    rCaption.SetMergedItemSet( aItemSet );
}

void ScCaptionUtil::SetCaptionItems( SdrCaptionObj& rCaption, const SfxItemSet& rItemSet )
{
    // copy all items
    rCaption.SetMergedItemSet( rItemSet );
    // reset shadow items
    rCaption.SetMergedItem( SdrOnOffItem(SDRATTR_SHADOW, sal_False ) );
    rCaption.SetMergedItem( SdrMetricItem(SDRATTR_SHADOWXDIST, 100 ) );
    rCaption.SetMergedItem( SdrMetricItem(SDRATTR_SHADOWYDIST, 100 ) );
    rCaption.SetSpecialTextBoxShadow();
}

// ============================================================================

/** Helper for creation and manipulation of caption drawing objects independent
    from cell annotations. */
class ScCaptionCreator
{
public:
    /** Create a new caption. The caption will not be inserted into the document. */
    explicit            ScCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, bool bShown, bool bTailFront );
    /** Manipulate an existing caption. */
    explicit            ScCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, SdrCaptionObj& rCaption );

    /** Returns the drawing layer page of the sheet contained in maPos. */
    SdrPage*            GetDrawPage();
    /** Returns the caption drawing obejct. */
    inline SdrCaptionObj* GetCaption() { return mpCaption; }

    /** Moves the caption inside the passed rectangle. Uses page area if 0 is passed. */
    void                FitCaptionToRect( const basegfx::B2DRange* pVisRange = 0 );
    /** Places the caption inside the passed rectangle, tries to keep the cell rectangle uncovered. Uses page area if 0 is passed. */
    void                AutoPlaceCaption( const basegfx::B2DRange* pVisRange = 0 );
    /** Updates caption tail and textbox according to current cell position. Uses page area if 0 is passed. */
    void                UpdateCaptionPos( const basegfx::B2DRange* pVisRange = 0 );

protected:
    /** Helper constructor for derived classes. */
    explicit            ScCaptionCreator( ScDocument& rDoc, const ScAddress& rPos );

    /** Calculates the caption tail position according to current cell position. */
    basegfx::B2DPoint   CalcTailPos( bool bTailFront );
    /** Implements creation of the caption object. The caption will not be inserted into the document. */
    void                CreateCaption( bool bShown, bool bTailFront );

private:
    /** Initializes all members. */
    void                Initialize();
    /** Returns the passed rectangle if existing, page rectangle otherwise. */
    inline const basegfx::B2DRange& GetVisRange(const basegfx::B2DRange* pVisRange) const { return pVisRange ? *pVisRange : maPageRange; }

private:
    ScDocument&         mrDoc;
    ScAddress           maPos;
    SdrCaptionObj*      mpCaption;
    basegfx::B2DRange	maPageRange;
    basegfx::B2DRange	maCellRange;
    bool                mbNegPage;
};

// ----------------------------------------------------------------------------

ScCaptionCreator::ScCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, bool bShown, bool bTailFront ) :
    mrDoc( rDoc ),
    maPos( rPos ),
    mpCaption( 0 )
{
    Initialize();
    CreateCaption( bShown, bTailFront );
}

ScCaptionCreator::ScCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, SdrCaptionObj& rCaption ) :
    mrDoc( rDoc ),
    maPos( rPos ),
    mpCaption( &rCaption )
{
    Initialize();
}

ScCaptionCreator::ScCaptionCreator( ScDocument& rDoc, const ScAddress& rPos ) :
    mrDoc( rDoc ),
    maPos( rPos ),
    mpCaption( 0 )
{
    Initialize();
}

SdrPage* ScCaptionCreator::GetDrawPage()
{
    ScDrawLayer* pDrawLayer = mrDoc.GetDrawLayer();
    return pDrawLayer ? pDrawLayer->GetPage( static_cast< sal_uInt16 >( maPos.Tab() ) ) : 0;
}

void ScCaptionCreator::FitCaptionToRect( const basegfx::B2DRange* pVisRange )
{
    const basegfx::B2DRange& rVisRange = GetVisRange( pVisRange );

    // tail position
	mpCaption->SetTailPos(rVisRange.clamp(mpCaption->GetTailPos()));

    // caption rectangle
    basegfx::B2DRange aCaptRange(sdr::legacy::GetLogicRange(*mpCaption));
    basegfx::B2DPoint aCaptPos(aCaptRange.getMinimum());
    // move textbox inside right border of visible area
    aCaptPos.setX(::std::min(aCaptPos.getX(), rVisRange.getMaxX() - aCaptRange.getWidth()));
    // move textbox inside left border of visible area (this may move it outside on right side again)
    aCaptPos.setX(::std::max(aCaptPos.getX(), rVisRange.getMinX()));
    // move textbox inside bottom border of visible area
    aCaptPos.setY(::std::min(aCaptPos.getY(), rVisRange.getMaxY() - aCaptRange.getHeight()));
    // move textbox inside top border of visible area (this may move it outside on bottom side again)
    aCaptPos.setY(::std::max(aCaptPos.getY(), rVisRange.getMinY()));
    // update caption
    aCaptRange.transform(basegfx::tools::createTranslateB2DHomMatrix(aCaptPos - aCaptRange.getMinimum()));
    sdr::legacy::SetLogicRange(*mpCaption, aCaptRange);
}

void ScCaptionCreator::AutoPlaceCaption( const basegfx::B2DRange* pVisRange )
{
    const basegfx::B2DRange& rVisRange = GetVisRange( pVisRange );

    // caption rectangle
    basegfx::B2DRange aCaptRange(sdr::legacy::GetLogicRange(*mpCaption));

    // n***Space contains available space between border of visible area and cell
    const double fLeftSpace(maCellRange.getMinX() - rVisRange.getMinX());
    const double fRightSpace(rVisRange.getMaxX() - maCellRange.getMaxX());
    const double fTopSpace(maCellRange.getMinY() - rVisRange.getMinY());
    const double fBottomSpace(rVisRange.getMaxY() - maCellRange.getMaxY());

    // nNeeded*** contains textbox dimensions plus needed distances to cell or border of visible area
    const double fNeededSpaceX(aCaptRange.getWidth() + SC_NOTECAPTION_CELLDIST);
    const double fNeededSpaceY(aCaptRange.getHeight() + SC_NOTECAPTION_CELLDIST);

    // bFitsWidth*** == true means width of textbox fits into horizontal free space of visible area
    const bool bFitsWidthLeft(fNeededSpaceX <= fLeftSpace);      // text box width fits into the width left of cell
    const bool bFitsWidthRight(fNeededSpaceX <= fRightSpace);    // text box width fits into the width right of cell
    const bool bFitsWidth(aCaptRange.getWidth() <= rVisRange.getWidth());        // text box width fits into width of visible area

    // bFitsHeight*** == true means height of textbox fits into vertical free space of visible area
    const bool bFitsHeightTop(fNeededSpaceY <= fTopSpace);       // text box height fits into the height above cell
    const bool bFitsHeightBottom(fNeededSpaceY <= fBottomSpace); // text box height fits into the height below cell
    const bool bFitsHeight(aCaptRange.getHeight() <= rVisRange.getHeight());     // text box height fits into height of visible area

    // bFits*** == true means the textbox fits completely into free space of visible area
    const bool bFitsLeft(bFitsWidthLeft && bFitsHeight);
    const bool bFitsRight(bFitsWidthRight && bFitsHeight);
    const bool bFitsTop(bFitsWidth && bFitsHeightTop);
    const bool bFitsBottom(bFitsWidth && bFitsHeightBottom);

    basegfx::B2DPoint aCaptPos(0.0, 0.0);

    // use left/right placement if possible, or if top/bottom placement not possible
    if( bFitsLeft || bFitsRight || (!bFitsTop && !bFitsBottom) )
    {
        // prefer left in RTL sheet and right in LTR sheets
        const bool bPreferLeft(bFitsLeft && (mbNegPage || !bFitsRight));
        const bool bPreferRight(bFitsRight && (!mbNegPage || !bFitsLeft));
        
        // move to left, if left is preferred, or if neither left nor right fit and there is more space to the left
        if( bPreferLeft || (!bPreferRight && (fLeftSpace > fRightSpace)) )
            aCaptPos.setX(maCellRange.getMinX() - SC_NOTECAPTION_CELLDIST - aCaptRange.getWidth());
        else // to right
            aCaptPos.setX(maCellRange.getMaxX() + SC_NOTECAPTION_CELLDIST);
        // Y position according to top cell border
        aCaptPos.setY(maCellRange.getMinY() + SC_NOTECAPTION_OFFSET_Y);
    }
    else    // top or bottom placement
    {
        // X position
        aCaptPos.setX(maCellRange.getMinX() + SC_NOTECAPTION_OFFSET_X);
        // top placement, if possible
        if( bFitsTop )
            aCaptPos.setY(maCellRange.getMinY() - SC_NOTECAPTION_CELLDIST - aCaptRange.getHeight());
        else    // bottom placement
            aCaptPos.setY(maCellRange.getMaxY() + SC_NOTECAPTION_CELLDIST);
    }

    // update textbox position in note caption object
    aCaptRange.transform(basegfx::tools::createTranslateB2DHomMatrix(aCaptPos - aCaptRange.getMinimum()));
    sdr::legacy::SetLogicRange(*mpCaption, aCaptRange);
    FitCaptionToRect(pVisRange);
}

void ScCaptionCreator::UpdateCaptionPos( const basegfx::B2DRange* pVisRange )
{
    ScDrawLayer* pDrawLayer = mrDoc.GetDrawLayer();

    // update caption position
    const basegfx::B2DPoint& rOldTailPos = mpCaption->GetTailPos();
    const basegfx::B2DPoint aTailPos(CalcTailPos(false));

    if(!rOldTailPos.equal(aTailPos))
    {
        // create drawing undo action
        if( pDrawLayer && pDrawLayer->IsRecording() )
            pDrawLayer->AddCalcUndo( pDrawLayer->GetSdrUndoFactory().CreateUndoGeoObject( *mpCaption ) );
        // calculate new caption rectangle (#i98141# handle LTR<->RTL switch correctly)
        basegfx::B2DRange aCaptRange(sdr::legacy::GetLogicRange(*mpCaption));
        double fDiffX((rOldTailPos.getX() >= 0.0) ? (aCaptRange.getMinX() - rOldTailPos.getX()) : (rOldTailPos.getX() - aCaptRange.getMaxX()));
        if( mbNegPage ) fDiffX = -fDiffX - aCaptRange.getWidth();
        const double fDiffY(aCaptRange.getMinY() - rOldTailPos.getY());
	    aCaptRange.transform(basegfx::tools::createTranslateB2DHomMatrix(fDiffX, fDiffY));
        // set new tail position and caption rectangle
        mpCaption->SetTailPos( aTailPos );
        sdr::legacy::SetLogicRange(*mpCaption, aCaptRange);
        // fit caption into draw page
        FitCaptionToRect(pVisRange);
    }

    // update cell position in caption user data
    ScDrawObjData* pCaptData = ScDrawLayer::GetNoteCaptionData( *mpCaption, maPos.Tab() );
    if( pCaptData && (maPos != pCaptData->maStart) )
    {
        // create drawing undo action
        if( pDrawLayer && pDrawLayer->IsRecording() )
            pDrawLayer->AddCalcUndo( new ScUndoObjData( mpCaption, pCaptData->maStart, pCaptData->maEnd, maPos, pCaptData->maEnd ) );
        // set new position
        pCaptData->maStart = maPos;
    }
}

basegfx::B2DPoint ScCaptionCreator::CalcTailPos( bool bTailFront )
{
    // tail position
    const bool bTailLeft(bTailFront != mbNegPage);
    basegfx::B2DPoint aTailPos(bTailLeft ? maCellRange.getMinX() : maCellRange.getMaxX(), maCellRange.getMinY());
    // move caption point 1/10 mm inside cell
    if( bTailLeft ) aTailPos.setX(aTailPos.getX() + 10.0); else aTailPos.setX(aTailPos.getX() - 10.0);
    aTailPos.setY(aTailPos.getY() + 10.0);
    return aTailPos;
}

void ScCaptionCreator::CreateCaption( bool bShown, bool bTailFront )
{
    // create the caption drawing object
    const basegfx::B2DPoint aTailPos(CalcTailPos(bTailFront));

	if(!mrDoc.GetDrawLayer())
	{
        mrDoc.InitDrawLayer();
	}
    
	mpCaption = new SdrCaptionObj( 
		*mrDoc.GetDrawLayer(),
		basegfx::tools::createScaleB2DHomMatrix(
			SC_NOTECAPTION_WIDTH, SC_NOTECAPTION_HEIGHT),
		&aTailPos);

    // basic caption settings
    ScCaptionUtil::SetBasicCaptionSettings( *mpCaption, bShown );
}

void ScCaptionCreator::Initialize()
{
    maCellRange = ScDrawLayer::GetCellRange( mrDoc, maPos, true );
    mbNegPage = mrDoc.IsNegativePage( maPos.Tab() );
    if( SdrPage* pDrawPage = GetDrawPage() )
    {
        maPageRange = basegfx::B2DRange(0.0, 0.0, pDrawPage->GetPageScale().getX(), pDrawPage->GetPageScale().getY());
    }
}

// ============================================================================

/** Helper for creation of permanent caption drawing objects for cell notes. */
class ScNoteCaptionCreator : public ScCaptionCreator
{
public:
    /** Create a new caption object and inserts it into the document. */
    explicit            ScNoteCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, ScNoteData& rNoteData );
    /** Manipulate an existing caption. */
    explicit            ScNoteCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, SdrCaptionObj& rCaption, bool bShown );
};

// ----------------------------------------------------------------------------

ScNoteCaptionCreator::ScNoteCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, ScNoteData& rNoteData ) :
    ScCaptionCreator( rDoc, rPos )  // use helper c'tor that does not create the caption yet
{
    SdrPage* pDrawPage = GetDrawPage();
    OSL_ENSURE( pDrawPage, "ScNoteCaptionCreator::ScNoteCaptionCreator - no drawing page" );
    if( pDrawPage )
    {
        // create the caption drawing object
        CreateCaption( rNoteData.mbShown, false );
        rNoteData.mpCaption = GetCaption();
        OSL_ENSURE( rNoteData.mpCaption, "ScNoteCaptionCreator::ScNoteCaptionCreator - missing caption object" );
        if( rNoteData.mpCaption )
        {
            // store note position in user data of caption object
            ScCaptionUtil::SetCaptionUserData( *rNoteData.mpCaption, rPos );
            // insert object into draw page
            pDrawPage->InsertObjectToSdrObjList(*rNoteData.mpCaption);
        }
    }
}

ScNoteCaptionCreator::ScNoteCaptionCreator( ScDocument& rDoc, const ScAddress& rPos, SdrCaptionObj& rCaption, bool bShown ) :
    ScCaptionCreator( rDoc, rPos, rCaption )
{
    SdrPage* pDrawPage = GetDrawPage();
    OSL_ENSURE( pDrawPage, "ScNoteCaptionCreator::ScNoteCaptionCreator - no drawing page" );
    OSL_ENSURE( rCaption.getSdrPageFromSdrObject() == pDrawPage, "ScNoteCaptionCreator::ScNoteCaptionCreator - wrong drawing page in caption" );
    if( pDrawPage && (rCaption.getSdrPageFromSdrObject() == pDrawPage) )
    {
        // store note position in user data of caption object
        ScCaptionUtil::SetCaptionUserData( rCaption, rPos );
        // basic caption settings
        ScCaptionUtil::SetBasicCaptionSettings( rCaption, bShown );
        // set correct tail position
        rCaption.SetTailPos( CalcTailPos( false ) );
    }
}

} // namespace

// ============================================================================

struct ScCaptionInitData
{
    typedef ::std::auto_ptr< SfxItemSet >           SfxItemSetPtr;
    typedef ::std::auto_ptr< OutlinerParaObject >   OutlinerParaObjPtr;

    SfxItemSetPtr       mxItemSet;          /// Caption object formatting.
    OutlinerParaObjPtr  mxOutlinerObj;      /// Text object with all text portion formatting.
    ::rtl::OUString     maSimpleText;       /// Simple text without formatting.
    basegfx::B2DPoint   maCaptionOffset;    /// Caption position relative to cell corner.
    basegfx::B2DVector	maCaptionScale;     /// Size of the caption object.
    bool                mbDefaultPosSize;   /// True = use default position and size for caption.

    explicit            ScCaptionInitData();
};

// ----------------------------------------------------------------------------

ScCaptionInitData::ScCaptionInitData() :
    mbDefaultPosSize( true )
{
}

// ============================================================================

ScNoteData::ScNoteData( bool bShown ) :
    mpCaption( 0 ),
    mbShown( bShown )
{
}

ScNoteData::~ScNoteData()
{
}

// ============================================================================

ScPostIt::ScPostIt( ScDocument& rDoc, const ScAddress& rPos, bool bShown ) :
    mrDoc( rDoc ),
    maNoteData( bShown )
{
    AutoStamp();
    CreateCaption( rPos );
}

ScPostIt::ScPostIt( ScDocument& rDoc, const ScAddress& rPos, const ScPostIt& rNote ) :
    mrDoc( rDoc ),
    maNoteData( rNote.maNoteData )
{
    maNoteData.mpCaption = 0;
    CreateCaption( rPos, rNote.maNoteData.mpCaption );
}

ScPostIt::ScPostIt( ScDocument& rDoc, const ScAddress& rPos, const ScNoteData& rNoteData, bool bAlwaysCreateCaption ) :
    mrDoc( rDoc ),
    maNoteData( rNoteData )
{
    if( bAlwaysCreateCaption || maNoteData.mbShown )
        CreateCaptionFromInitData( rPos );
}

ScPostIt::~ScPostIt()
{
    RemoveCaption();
}

ScPostIt* ScPostIt::Clone( const ScAddress& rOwnPos, ScDocument& rDestDoc, const ScAddress& rDestPos, bool bCloneCaption ) const
{
    CreateCaptionFromInitData( rOwnPos );
    return bCloneCaption ? new ScPostIt( rDestDoc, rDestPos, *this ) : new ScPostIt( rDestDoc, rDestPos, maNoteData, false );
}

void ScPostIt::AutoStamp()
{
    maNoteData.maDate = ScGlobal::pLocaleData->getDate( Date() );
    maNoteData.maAuthor = SvtUserOptions().GetID();
}

const OutlinerParaObject* ScPostIt::GetOutlinerObject() const
{
    if( maNoteData.mpCaption )
        return maNoteData.mpCaption->GetOutlinerParaObject();
    if( maNoteData.mxInitData.get() )
        return maNoteData.mxInitData->mxOutlinerObj.get();
    return 0;
}

const EditTextObject* ScPostIt::GetEditTextObject() const
{
    const OutlinerParaObject* pOPO = GetOutlinerObject();
    return pOPO ? &pOPO->GetTextObject() : 0;
}

OUString ScPostIt::GetText() const
{
    if( const EditTextObject* pEditObj = GetEditTextObject() )
    {
        OUStringBuffer aBuffer;
        for( sal_uInt16 nPara = 0, nParaCount = pEditObj->GetParagraphCount(); nPara < nParaCount; ++nPara )
        {
            if( nPara > 0 )
                aBuffer.append( sal_Unicode( '\n' ) );
            aBuffer.append( pEditObj->GetText( nPara ) );
        }
        return aBuffer.makeStringAndClear();
    }
    if( maNoteData.mxInitData.get() )
        return maNoteData.mxInitData->maSimpleText;
    return OUString();
}

bool ScPostIt::HasMultiLineText() const
{
    if( const EditTextObject* pEditObj = GetEditTextObject() )
        return pEditObj->GetParagraphCount() > 1;
    if( maNoteData.mxInitData.get() )
        return maNoteData.mxInitData->maSimpleText.indexOf( '\n' ) >= 0;
    return false;
}

void ScPostIt::SetText( const ScAddress& rPos, const OUString& rText )
{
    CreateCaptionFromInitData( rPos );
    if( maNoteData.mpCaption )
        maNoteData.mpCaption->SetText( rText );
}

SdrCaptionObj* ScPostIt::GetOrCreateCaption( const ScAddress& rPos ) const
{
    CreateCaptionFromInitData( rPos );
    return maNoteData.mpCaption;
}

void ScPostIt::ForgetCaption()
{
    /*  This function is used in undo actions to give up the responsibility for
        the caption object which is handled by separate drawing undo actions. */
    maNoteData.mpCaption = 0;
    maNoteData.mxInitData.reset();
}

void ScPostIt::ShowCaption( const ScAddress& rPos, bool bShow )
{
    CreateCaptionFromInitData( rPos );
    // no separate drawing undo needed, handled completely inside ScUndoShowHideNote
    maNoteData.mbShown = bShow;
    if( maNoteData.mpCaption )
        ScCaptionUtil::SetCaptionLayer( *maNoteData.mpCaption, bShow );
}

void ScPostIt::ShowCaptionTemp( const ScAddress& rPos, bool bShow )
{
    CreateCaptionFromInitData( rPos );
    if( maNoteData.mpCaption )
        ScCaptionUtil::SetCaptionLayer( *maNoteData.mpCaption, maNoteData.mbShown || bShow );
}

void ScPostIt::UpdateCaptionPos( const ScAddress& rPos )
{
    CreateCaptionFromInitData( rPos );
    if( maNoteData.mpCaption )
    {
        ScCaptionCreator aCreator( mrDoc, rPos, *maNoteData.mpCaption );
        aCreator.UpdateCaptionPos();
    }
}

// private --------------------------------------------------------------------

void ScPostIt::CreateCaptionFromInitData( const ScAddress& rPos ) const
{
    OSL_ENSURE( maNoteData.mpCaption || maNoteData.mxInitData.get(), "ScPostIt::CreateCaptionFromInitData - need caption object or initial caption data" );
    if( maNoteData.mxInitData.get() )
    {
        /*  This function is called from ScPostIt::Clone() when copying cells
            to the clipboard/undo document, and when copying cells from the
            clipboard/undo document. The former should always be called first,
            so if called in an clipboard/undo document, the caption should have
            been created already. */
        OSL_ENSURE( !mrDoc.IsUndo() && !mrDoc.IsClipboard(), "ScPostIt::CreateCaptionFromInitData - note caption should not be created in undo/clip documents" );

        /*  #i104915# Never try to create notes in Undo document, leads to
            crash due to missing document members (e.g. row height array). */
        if( !maNoteData.mpCaption && !mrDoc.IsUndo() )
        {
            // ScNoteCaptionCreator c'tor creates the caption and inserts it into the document and maNoteData
            ScNoteCaptionCreator aCreator( mrDoc, rPos, maNoteData );
            if( maNoteData.mpCaption )
            {
                ScCaptionInitData& rInitData = *maNoteData.mxInitData;

                // transfer ownership of outliner object to caption, or set simple text
                OSL_ENSURE( rInitData.mxOutlinerObj.get() || (rInitData.maSimpleText.getLength() > 0),
                    "ScPostIt::CreateCaptionFromInitData - need either outliner para object or simple text" );
                if( rInitData.mxOutlinerObj.get() )
                    maNoteData.mpCaption->SetOutlinerParaObject( rInitData.mxOutlinerObj.release() );
                else
                    maNoteData.mpCaption->SetText( rInitData.maSimpleText );

                // copy all items or set default items; reset shadow items
                ScCaptionUtil::SetDefaultItems( *maNoteData.mpCaption, mrDoc );
                if( rInitData.mxItemSet.get() )
                    ScCaptionUtil::SetCaptionItems( *maNoteData.mpCaption, *rInitData.mxItemSet );

                // set position and size of the caption object
                if( rInitData.mbDefaultPosSize )
                {
                    // set other items and fit caption size to text
                    maNoteData.mpCaption->SetMergedItem( SdrMetricItem(SDRATTR_TEXT_MINFRAMEWIDTH, SC_NOTECAPTION_WIDTH ) );
                    maNoteData.mpCaption->SetMergedItem( SdrMetricItem(SDRATTR_TEXT_MAXFRAMEWIDTH, SC_NOTECAPTION_MAXWIDTH_TEMP ) );
                    maNoteData.mpCaption->AdjustTextFrameWidthAndHeight();
                    aCreator.AutoPlaceCaption();
                }
                else
                {
                    const basegfx::B2DRange aCellRange(ScDrawLayer::GetCellRange(mrDoc, rPos, true));
                    const bool bNegPage(mrDoc.IsNegativePage(rPos.Tab()));
					const basegfx::B2DPoint aTopLeft(
						bNegPage ? (aCellRange.getMinX() - rInitData.maCaptionOffset.getX()) : (aCellRange.getMaxX() + rInitData.maCaptionOffset.getX()),
						aCellRange.getMinY() + rInitData.maCaptionOffset.getY());
                    const basegfx::B2DRange aCaptRange(aTopLeft, aTopLeft + rInitData.maCaptionScale);
                    sdr::legacy::SetLogicRange(*maNoteData.mpCaption, aCaptRange);
                    aCreator.FitCaptionToRect();
                }
            }
        }
        // forget the initial caption data struct
        maNoteData.mxInitData.reset();
    }
}

void ScPostIt::CreateCaption( const ScAddress& rPos, const SdrCaptionObj* pCaption )
{
    OSL_ENSURE( !maNoteData.mpCaption, "ScPostIt::CreateCaption - unexpected caption object found" );
    maNoteData.mpCaption = 0;

    /*  #i104915# Never try to create notes in Undo document, leads to
        crash due to missing document members (e.g. row height array). */
    OSL_ENSURE( !mrDoc.IsUndo(), "ScPostIt::CreateCaption - note caption should not be created in undo documents" );
    if( mrDoc.IsUndo() )
        return;

    // drawing layer may be missing, if a note is copied into a clipboard document
    if( mrDoc.IsClipboard() )
        mrDoc.InitDrawLayer();

    // ScNoteCaptionCreator c'tor creates the caption and inserts it into the document and maNoteData
    ScNoteCaptionCreator aCreator( mrDoc, rPos, maNoteData );
    if( maNoteData.mpCaption )
    {
        // clone settings of passed caption
        if( pCaption )
        {
            // copy edit text object (object must be inserted into page already)
            if( OutlinerParaObject* pOPO = pCaption->GetOutlinerParaObject() )
                maNoteData.mpCaption->SetOutlinerParaObject( new OutlinerParaObject( *pOPO ) );
            // copy formatting items (after text has been copied to apply font formatting)
            maNoteData.mpCaption->SetMergedItemSetAndBroadcast( pCaption->GetMergedItemSet() );
            // move textbox position relative to new cell, copy textbox size
            basegfx::B2DRange aCaptRange(sdr::legacy::GetLogicRange(*pCaption));
			aCaptRange.transform(basegfx::tools::createTranslateB2DHomMatrix(maNoteData.mpCaption->GetTailPos() - pCaption->GetTailPos()));
            sdr::legacy::SetLogicRange(*maNoteData.mpCaption, aCaptRange);
            aCreator.FitCaptionToRect();
        }
        else
        {
            // set default formatting and default position
            ScCaptionUtil::SetDefaultItems( *maNoteData.mpCaption, mrDoc );
            aCreator.AutoPlaceCaption();
        }

        // create undo action
        if( ScDrawLayer* pDrawLayer = mrDoc.GetDrawLayer() )
            if( pDrawLayer->IsRecording() )
                pDrawLayer->AddCalcUndo( pDrawLayer->GetSdrUndoFactory().CreateUndoNewObject( *maNoteData.mpCaption ) );
    }
}

void ScPostIt::RemoveCaption()
{

    /*  Remove caption object only, if this note is its owner (e.g. notes in
        undo documents refer to captions in original document, do not remove
        them from drawing layer here). */
    ScDrawLayer* pDrawLayer = mrDoc.GetDrawLayer();
    
	if( maNoteData.mpCaption && (pDrawLayer == &maNoteData.mpCaption->getSdrModelFromSdrObject()) )
    {
        OSL_ENSURE( pDrawLayer, "ScPostIt::RemoveCaption - object without drawing layer" );
        SdrPage* pDrawPage = maNoteData.mpCaption->getSdrPageFromSdrObject();
        OSL_ENSURE( pDrawPage, "ScPostIt::RemoveCaption - object without drawing page" );
        if( pDrawPage )
        {
            // create drawing undo action (before removing the object to have valid draw page in undo action)
            bool bRecording = ( pDrawLayer && pDrawLayer->IsRecording() );
            if( bRecording )
                pDrawLayer->AddCalcUndo( pDrawLayer->GetSdrUndoFactory().CreateUndoDeleteObject( *maNoteData.mpCaption ) );
            // remove the object from the drawing page, delete if undo is disabled
            SdrObject* pObj = pDrawPage->RemoveObjectFromSdrObjList( maNoteData.mpCaption->GetNavigationPosition() );
            if( !bRecording )
                deleteSdrObjectSafeAndClearPointer( pObj );
        }
    }
    maNoteData.mpCaption = 0;
}

// ============================================================================

void ScNoteUtil::UpdateCaptionPositions( ScDocument& rDoc, const ScRange& rRange )
{
    // do not use ScCellIterator, it skips filtered and subtotal cells
    for( ScAddress aPos( rRange.aStart ); aPos.Tab() <= rRange.aEnd.Tab(); aPos.IncTab() )
        for( aPos.SetCol( rRange.aStart.Col() ); aPos.Col() <= rRange.aEnd.Col(); aPos.IncCol() )
            for( aPos.SetRow( rRange.aStart.Row() ); aPos.Row() <= rRange.aEnd.Row(); aPos.IncRow() )
                if( ScPostIt* pNote = rDoc.GetNote( aPos ) )
                    pNote->UpdateCaptionPos( aPos );
}

SdrCaptionObj* ScNoteUtil::CreateTempCaption(
        ScDocument& rDoc, const ScAddress& rPos, SdrPage& rDrawPage,
        const OUString& rUserText, const Rectangle& rVisRect, bool bTailFront )
{
    OUStringBuffer aBuffer( rUserText );
    // add plain text of invisible (!) cell note (no formatting etc.)
    SdrCaptionObj* pNoteCaption = 0;
    const ScPostIt* pNote = rDoc.GetNote( rPos );
    if( pNote && !pNote->IsCaptionShown() )
    {
        if( aBuffer.getLength() > 0 )
            aBuffer.appendAscii( RTL_CONSTASCII_STRINGPARAM( "\n--------\n" ) ).append( pNote->GetText() );
        pNoteCaption = pNote->GetOrCreateCaption( rPos );
    }

    // create a caption if any text exists
    if( !pNoteCaption && (aBuffer.getLength() == 0) )
        return 0;

    // prepare visible rectangle (add default distance to all borders)
    Rectangle aVisRect(
        rVisRect.Left() + SC_NOTECAPTION_BORDERDIST_TEMP,
        rVisRect.Top() + SC_NOTECAPTION_BORDERDIST_TEMP,
        rVisRect.Right() - SC_NOTECAPTION_BORDERDIST_TEMP,
        rVisRect.Bottom() - SC_NOTECAPTION_BORDERDIST_TEMP );

    // create the caption object
    ScCaptionCreator aCreator( rDoc, rPos, true, bTailFront );
    SdrCaptionObj* pCaption = aCreator.GetCaption();

    // insert caption into page (needed to set caption text)
    rDrawPage.InsertObjectToSdrObjList(*pCaption);

    // clone the edit text object, unless user text is present, then set this text
    if( pNoteCaption && (rUserText.getLength() == 0) )
    {
        if( OutlinerParaObject* pOPO = pNoteCaption->GetOutlinerParaObject() )
            pCaption->SetOutlinerParaObject( new OutlinerParaObject( *pOPO ) );
        // set formatting (must be done after setting text) and resize the box to fit the text
        pCaption->SetMergedItemSetAndBroadcast( pNoteCaption->GetMergedItemSet() );
        const Rectangle aCaptRect(sdr::legacy::GetLogicRect(*pCaption).TopLeft(), sdr::legacy::GetLogicRect(*pNoteCaption).GetSize() );
        sdr::legacy::SetLogicRect(*pCaption, aCaptRect );
    }
    else
    {
        // if pNoteCaption is null, then aBuffer contains some text
        pCaption->SetText( aBuffer.makeStringAndClear() );
        ScCaptionUtil::SetDefaultItems( *pCaption, rDoc );
        // adjust caption size to text size
        long nMaxWidth = ::std::min< long >( aVisRect.GetWidth() * 2 / 3, SC_NOTECAPTION_MAXWIDTH_TEMP );
        pCaption->SetMergedItem( SdrOnOffItem(SDRATTR_TEXT_AUTOGROWWIDTH, sal_True ) );
        pCaption->SetMergedItem( SdrMetricItem(SDRATTR_TEXT_MINFRAMEWIDTH, SC_NOTECAPTION_WIDTH ) );
        pCaption->SetMergedItem( SdrMetricItem(SDRATTR_TEXT_MAXFRAMEWIDTH, nMaxWidth ) );
        pCaption->SetMergedItem( SdrOnOffItem(SDRATTR_TEXT_AUTOGROWHEIGHT, sal_True ) );
        pCaption->AdjustTextFrameWidthAndHeight();
    }

    // move caption into visible area
	const basegfx::B2DRange aVisRange(aVisRect.Left(), aVisRect.Top(), aVisRect.Right(), aVisRect.Bottom());
    aCreator.AutoPlaceCaption( &aVisRange );
    return pCaption;
}

ScPostIt* ScNoteUtil::CreateNoteFromCaption(
        ScDocument& rDoc, const ScAddress& rPos, SdrCaptionObj& rCaption, bool bShown )
{
    ScNoteData aNoteData( bShown );
    aNoteData.mpCaption = &rCaption;
    ScPostIt* pNote = new ScPostIt( rDoc, rPos, aNoteData, false );
    pNote->AutoStamp();
    rDoc.TakeNote( rPos, pNote );
    // if pNote still points to the note after TakeNote(), insertion was successful
    if( pNote )
    {
        // ScNoteCaptionCreator c'tor updates the caption object to be part of a note
        ScNoteCaptionCreator aCreator( rDoc, rPos, rCaption, bShown );
    }
    return pNote;
}

ScPostIt* ScNoteUtil::CreateNoteFromObjectData(
	ScDocument& rDoc, 
	const ScAddress& rPos, 
	SfxItemSet* pItemSet,
	OutlinerParaObject* pOutlinerObj, 
	const basegfx::B2DRange& rCaptionRange,
	bool bShown, 
	bool bAlwaysCreateCaption )
{
    OSL_ENSURE( pItemSet && pOutlinerObj, "ScNoteUtil::CreateNoteFromObjectData - item set and outliner object expected" );
    ScNoteData aNoteData( bShown );
    aNoteData.mxInitData.reset( new ScCaptionInitData );
    ScCaptionInitData& rInitData = *aNoteData.mxInitData;
    rInitData.mxItemSet.reset( pItemSet );
    rInitData.mxOutlinerObj.reset( pOutlinerObj );

    // convert absolute caption position to relative position
    rInitData.mbDefaultPosSize = rCaptionRange.isEmpty();
    if( !rInitData.mbDefaultPosSize )
    {
        const basegfx::B2DRange aCellRange(ScDrawLayer::GetCellRange(rDoc, rPos, true));
        const bool bNegPage(rDoc.IsNegativePage(rPos.Tab()));
		
		rInitData.maCaptionOffset = basegfx::B2DPoint(
			bNegPage ? (aCellRange.getMinX() - rCaptionRange.getMaxX()) : (rCaptionRange.getMinX() - aCellRange.getMaxX()),
			rCaptionRange.getMinY() - aCellRange.getMinY());
		rInitData.maCaptionScale = rCaptionRange.getRange();
    }

    /*  Create the note and insert it into the document. If the note is
        visible, the caption object will be created automatically. */
    ScPostIt* pNote = new ScPostIt( rDoc, rPos, aNoteData, bAlwaysCreateCaption );
    pNote->AutoStamp();
    rDoc.TakeNote( rPos, pNote );
    // if pNote still points to the note after TakeNote(), insertion was successful
    return pNote;
}

ScPostIt* ScNoteUtil::CreateNoteFromString(
        ScDocument& rDoc, const ScAddress& rPos, const OUString& rNoteText,
        bool bShown, bool bAlwaysCreateCaption )
{
    ScPostIt* pNote = 0;
    if( rNoteText.getLength() > 0 )
    {
        ScNoteData aNoteData( bShown );
        aNoteData.mxInitData.reset( new ScCaptionInitData );
        ScCaptionInitData& rInitData = *aNoteData.mxInitData;
        rInitData.maSimpleText = rNoteText;
        rInitData.mbDefaultPosSize = true;

        /*  Create the note and insert it into the document. If the note is
            visible, the caption object will be created automatically. */
        pNote = new ScPostIt( rDoc, rPos, aNoteData, bAlwaysCreateCaption );
        pNote->AutoStamp();
        rDoc.TakeNote( rPos, pNote );
        // if pNote still points to the note after TakeNote(), insertion was successful
    }
    return pNote;
}

// ============================================================================
