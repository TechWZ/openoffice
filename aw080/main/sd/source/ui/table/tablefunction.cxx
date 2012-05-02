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
#include "precompiled_sd.hxx"

#include <sal/config.h>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/drawing/XSelectionFunction.hpp>
#include <com/sun/star/awt/KeyModifier.hpp>
#include <com/sun/star/lang/XInitialization.hpp>

#include <cppuhelper/compbase2.hxx>
#include <cppuhelper/basemutex.hxx>

#include <vcl/svapp.hxx>

#include <svx/svdotable.hxx>
#include <svx/sdr/overlay/overlayobjectcell.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>
#include <svx/svxids.hrc>
#include <editeng/outlobj.hxx>
#include <svx/svdoutl.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdetc.hxx>
#include <editeng/editstat.hxx>
#include <editeng/unolingu.hxx>
#include <svx/sdrpagewindow.hxx>
#include <svx/sdr/table/tabledesign.hxx>
#include <svx/svxdlg.hxx>
#include <vcl/msgbox.hxx>
#include <svx/svdlegacy.hxx>

#include <svl/itempool.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/request.hxx>
#include <svl/style.hxx>

#include "framework/FrameworkHelper.hxx"
#include "app.hrc"
#include "glob.hrc"
#include "DrawViewShell.hxx"
#include "drawdoc.hxx"
#include "DrawDocShell.hxx"
#include "Window.hxx"
#include "drawview.hxx"
#include "sdresid.hxx"
#include "undo/undoobjects.hxx"

using ::rtl::OUString;
using namespace ::sd;
using namespace ::sdr::table;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::linguistic2;

namespace css = ::com::sun::star;

namespace sd
{
extern void showTableDesignDialog( ::Window*, ViewShellBase& );

static void apply_table_style( SdrTableObj* pObj, SdrModel* pModel, const OUString& sTableStyle )
{
	if( pModel && pObj )
	{
		Reference< XNameAccess > xPool( dynamic_cast< XNameAccess* >( pModel->GetStyleSheetPool() ) );
		if( xPool.is() ) try
		{
			const OUString sFamilyName( RTL_CONSTASCII_USTRINGPARAM( "table" ) );
			Reference< XNameContainer > xTableFamily( xPool->getByName( sFamilyName ), UNO_QUERY_THROW );
			OUString aStdName( RTL_CONSTASCII_USTRINGPARAM("default") );
			if( sTableStyle.getLength() )
				aStdName = sTableStyle;
			Reference< XIndexAccess > xStyle( xTableFamily->getByName( aStdName ), UNO_QUERY_THROW );
			pObj->setTableStyle( xStyle );
		}
		catch( Exception& )
		{
			DBG_ERROR("sd::apply_default_table_style(), exception caught!");
		}
	}
}

void DrawViewShell::FuTable(SfxRequest& rReq)
{
	switch( rReq.GetSlot() )
	{
	case SID_INSERT_TABLE:
	{
		sal_Int32 nColumns = 0;
		sal_Int32 nRows = 0;
		OUString sTableStyle;

		SFX_REQUEST_ARG( rReq, pCols, SfxUInt16Item, SID_ATTR_TABLE_COLUMN );
		SFX_REQUEST_ARG( rReq, pRows, SfxUInt16Item, SID_ATTR_TABLE_ROW );
		SFX_REQUEST_ARG( rReq, pStyle, SfxStringItem, SID_TABLE_STYLE );

		if( pCols )
			nColumns = pCols->GetValue();

		if( pRows )
			nRows = pRows->GetValue();

		if( pStyle )
			sTableStyle = pStyle->GetValue();

		if( (nColumns == 0) || (nRows == 0) )
		{
			SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
			::std::auto_ptr<SvxAbstractNewTableDialog> pDlg( pFact ? pFact->CreateSvxNewTableDialog( NULL ) : 0);

			if( !pDlg.get() || (pDlg->Execute() != RET_OK) )
				break;

			nColumns = pDlg->getColumns();
			nRows = pDlg->getRows();
		}

		basegfx::B2DHomMatrix aObjTrans;

		SdrObject* pPickObj = mpView->GetEmptyPresentationObject( PRESOBJ_TABLE );
		if( pPickObj )
		{
			aObjTrans = pPickObj->getSdrObjectTransformation();
			const basegfx::B2DPoint aTopLeft(aObjTrans * basegfx::B2DPoint(0.0, 0.0));
			const basegfx::B2DPoint aBottomLeft(aObjTrans * basegfx::B2DPoint(0.0, 1.0));
			const double fLength(basegfx::B2DVector(aBottomLeft - aTopLeft).getLength());
			const double fScaleFactor(200.0 / (basegfx::fTools::equalZero(fLength) ? 1.0 : fLength));

			aObjTrans = basegfx::tools::createScaleB2DHomMatrix(1.0, fScaleFactor) * aObjTrans;
		}
		else
		{
            const basegfx::B2DPoint aCenter(GetActiveWindow()->GetLogicRange().getCenter());
            const basegfx::B2DVector aScale(14100.0, 200.0);

            aObjTrans = basegfx::tools::createScaleTranslateB2DHomMatrix(
                aScale,
                aCenter - (aScale * 0.5));
		}

		::sdr::table::SdrTableObj* pObj = new ::sdr::table::SdrTableObj(
			*GetDoc(), 
			aObjTrans, 
			nColumns, 
			nRows);

		pObj->SetStyleSheet( GetDoc()->GetDefaultStyleSheet(), sal_True );
		apply_table_style( pObj, GetDoc(), sTableStyle );

		// if we have a pick obj we need to make this new ole a pres obj replacing the current pick obj
		if( pPickObj )
		{
			SdPage* pPage = static_cast< SdPage* >(pPickObj->getSdrPageFromSdrObject());
			if(pPage && pPage->IsPresObj(pPickObj))
			{
				// replace formally used 'pObj->SetUserCall(pPickObj->GetUserCall())' by
                // new notify/listener mechanism
                SdPage* pCurrentlyConnectedSdPage = findConnectionToSdrObject(pPickObj);
                establishConnectionToSdrObject(pObj, pCurrentlyConnectedSdPage);

                pPage->InsertPresObj( pObj, PRESOBJ_TABLE );
			}
		}

		if( pPickObj )
		{
			mpView->ReplaceObjectAtView(*pPickObj, *pObj, true);
		}
		else
		{
			mpView->InsertObjectAtView(*pObj, SDRINSERT_SETDEFLAYER);
		}

		Invalidate(SID_DRAWTBX_INSERT);
		rReq.Ignore();
		break;
	}
	case SID_TABLEDESIGN:
	{
		if( GetDoc() && (GetDoc()->GetDocumentType() == DOCUMENT_TYPE_DRAW) )
		{
			// in draw open a modal dialog since we have no tool pane yet
			showTableDesignDialog( GetActiveWindow(), GetViewShellBase() );
		}
		else
		{
			// Make the slide transition panel visible (expand it) in the
	        // tool pane.
		    framework::FrameworkHelper::Instance(GetViewShellBase())->RequestTaskPanel(
			    framework::FrameworkHelper::msTableDesignPanelURL);
		}

		Cancel();
		rReq.Done ();
	}
	default:
		break;
	}
}

// --------------------------------------------------------------------

void DrawViewShell::GetTableMenuState( SfxItemSet &rSet )
{
	bool bIsUIActive = GetDocSh()->IsUIActive();
	if( bIsUIActive )
	{
		rSet.DisableItem( SID_INSERT_TABLE );
	}
	else
	{
		String aActiveLayer = mpDrawView->GetActiveLayer();
		SdrPageView* pPV = mpDrawView->GetSdrPageView();

		if( bIsUIActive ||
			( aActiveLayer.Len() != 0 && pPV && ( pPV->IsLayerLocked(aActiveLayer) ||
			!pPV->IsLayerVisible(aActiveLayer) ) ) || 
			SD_MOD()->GetWaterCan() )
		{
			rSet.DisableItem( SID_INSERT_TABLE );
		}
	}
}

// --------------------------------------------------------------------

void CreateTableFromRTF( SvStream& rStream, SdDrawDocument& rModel )
{
	rStream.Seek( 0 );
	SdrPage* pPage = rModel.GetPage(0);

	if( pPage )
	{
		::sdr::table::SdrTableObj* pObj = new ::sdr::table::SdrTableObj( 
			rModel, 
			basegfx::tools::createScaleB2DHomMatrix(
				200.0, 200.0));
		pObj->SetStyleSheet( rModel.GetDefaultStyleSheet(), sal_True );
		OUString sTableStyle;
		apply_table_style( pObj, &rModel, sTableStyle );

		pPage->InsertObjectToSdrObjList(*pObj);

		sdr::table::SdrTableObj::ImportAsRTF( rStream, *pObj );
	}
}

}
