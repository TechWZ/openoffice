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


#include "precompiled_reportdesign.hxx"
#include "ReportSection.hxx"
#include "ReportWindow.hxx"
#include "DesignView.hxx"
#include "uistrings.hrc"
#include "RptObject.hxx"
#include "RptModel.hxx"
#include "SectionView.hxx"
#include "RptPage.hxx"
#include "ReportController.hxx"
#include "UITools.hxx"
#include "ViewsWindow.hxx"

#include <svx/svdpagv.hxx>
#include <editeng/eeitemid.hxx>
#include <editeng/adjitem.hxx>
#include <svx/sdrpaintwindow.hxx>
#include <svx/unoshape.hxx>
#include <svx/gallery.hxx>
#include <svx/svxids.hrc>
#include <svx/svditer.hxx>
#include <svx/dbaexchange.hxx>
#include <svx/svdlegacy.hxx>

#include <vcl/svapp.hxx>

#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <toolkit/helper/convert.hxx>
#include "RptDef.hxx"
#include "SectionWindow.hxx"
#include "helpids.hrc"
#include "RptResId.hrc"
#include "dlgedclip.hxx"
#include "UndoActions.hxx"
#include "rptui_slotid.hrc"

#include <connectivity/dbtools.hxx>

#include <vcl/lineinfo.hxx>
#include "ColorChanger.hxx"

#include <svl/itempool.hxx>
#include <svtools/extcolorcfg.hxx>
#include <unotools/confignode.hxx>
#include <framework/imageproducer.hxx>

// =============================================================================
namespace rptui
{
// =============================================================================
using namespace ::com::sun::star;
// -----------------------------------------------------------------------------

sal_Int32 lcl_getOverlappedControlColor(/*const uno::Reference <lang::XMultiServiceFactory> _rxFactory*/)
{
	svtools::ExtendedColorConfig aConfig;
    sal_Int32 nColor = aConfig.GetColorValue(CFG_REPORTDESIGNER, DBOVERLAPPEDCONTROL).getColor();
    return nColor;
}
//------------------------------------------------------------------------------
DBG_NAME( rpt_OReportSection )
OReportSection::OReportSection(OSectionWindow* _pParent,const uno::Reference< report::XSection >& _xSection) 
: Window(_pParent,WB_DIALOGCONTROL)
, ::comphelper::OPropertyChangeListener(m_aMutex)
, DropTargetHelper(this)
,m_pPage(NULL)
,m_pView(NULL)
,m_pParent(_pParent)
,m_pFunc(NULL)
,m_pMulti(NULL)
,m_pReportListener(NULL)
,m_xSection(_xSection)
,m_nPaintEntranceCount(0)
,m_eMode(RPTUI_SELECT)
,m_bDialogModelChanged(sal_False)
,m_bInDrag(sal_False)
{
	DBG_CTOR( rpt_OReportSection,NULL);
    //EnableChildTransparentMode();
	SetHelpId(HID_REPORTSECTION);
	SetMapMode( MapMode( MAP_100TH_MM ) );
    SetParentClipMode( PARENTCLIPMODE_CLIP );
    EnableChildTransparentMode( sal_False );
    SetPaintTransparent( sal_False );
   
	try
	{
		fill();
	}
	catch(uno::Exception&)
	{
		OSL_ENSURE(0,"Exception catched!");
	}

    m_pFunc.reset(new DlgEdFuncSelect( this ));
    m_pFunc->setOverlappedControlColor(lcl_getOverlappedControlColor( /* m_pParent->getViewsWindow()->getView()->getReportView()->getController().getORB() */ ) );
}
//------------------------------------------------------------------------------
OReportSection::~OReportSection()
{
	DBG_DTOR( rpt_OReportSection,NULL);
    m_pPage = NULL;
	//m_pModel->GetUndoEnv().RemoveSection(m_xSection.get());
	if ( m_pMulti.is() )
    	m_pMulti->dispose();
    
	if ( m_pReportListener.is() )
		m_pReportListener->dispose();
	m_pFunc = ::std::auto_ptr<DlgEdFunc>();
    
	{
		::std::auto_ptr<OSectionView> aTemp( m_pView);
		if ( m_pView )
			m_pView->EndListening( *m_pModel );
		m_pView = NULL;
	}
    /*m_pModel->DeletePage(m_pPage->GetPageNumber());*/
}
//------------------------------------------------------------------------------
void OReportSection::Paint( const Rectangle& rRect )
{
    Window::Paint(rRect);

	if ( m_pView && m_nPaintEntranceCount == 0)
	{
        ++m_nPaintEntranceCount;
         // repaint, get PageView and prepare Region
        SdrPageView* pPgView = m_pView->GetSdrPageView();
        const Region aPaintRectRegion(rRect);

        // #i74769#
        SdrPaintWindow* pTargetPaintWindow = 0;

        // mark repaint start
        if(pPgView)
        {
            pTargetPaintWindow = pPgView->GetView().BeginDrawLayers(this, aPaintRectRegion);
            OSL_ENSURE(pTargetPaintWindow, "BeginDrawLayers: Got no SdrPaintWindow (!)");
            // draw background self using wallpaper
            OutputDevice& rTargetOutDev = pTargetPaintWindow->GetTargetOutputDevice();
            rTargetOutDev.DrawWallpaper(rRect, Wallpaper(pPgView->GetApplicationDocumentColor()));
        }

        // do paint (unbuffered) and mark repaint end
        if(pPgView)
        {
            pPgView->DrawLayer(0, this);
            pPgView->GetView().EndDrawLayers(*pTargetPaintWindow, true);
        }

		m_pView->CompleteRedraw(this,aPaintRectRegion);
        --m_nPaintEntranceCount;
	}
}
//------------------------------------------------------------------------------
void OReportSection::Resize()
{
	Window::Resize();
}
//------------------------------------------------------------------------------
void OReportSection::fill()
{
	if ( !m_xSection.is() )
		return;

	m_pMulti = new comphelper::OPropertyChangeMultiplexer(this,m_xSection.get());
	m_pMulti->addProperty(PROPERTY_BACKCOLOR);

	m_pReportListener = addStyleListener(m_xSection->getReportDefinition(),this);

    m_pModel = m_pParent->getViewsWindow()->getView()->getReportView()->getController().getSharedSdrModel();
    m_pPage = m_pModel->getPage(m_xSection);

	m_pView = new OSectionView( *m_pModel.get(), this, m_pParent->getViewsWindow()->getView() );

    // #i93597# tell SdrPage that only left and right page border is defined
    // instead of the full rectangle definition
    m_pPage->setPageBorderOnlyLeftRight(true);

    // without the following call, no grid is painted
	m_pView->ShowSdrPage( *m_pPage );

	m_pView->SetMoveSnapOnlyTopLeft(true);
	ODesignView* pDesignView = m_pParent->getViewsWindow()->getView()->getReportView();

    // #i93595# Adapted grid to a more coarse grid and subdivisions for better visualisation. This
    // is only for visualisation and has nothing to do with the actual snap
    const Size aGridSizeCoarse(pDesignView->getGridSizeCoarse());
    const Size aGridSizeFine(pDesignView->getGridSizeFine());
    m_pView->SetGridCoarse(aGridSizeCoarse);
    m_pView->SetGridFine(aGridSizeFine);

    // #i93595# set snap grid width to snap to all existing subdivisions
    m_pView->SetSnapGridWidth(aGridSizeFine.A(), aGridSizeFine.B());

	m_pView->SetGridSnap( pDesignView->isGridSnap() );	
	m_pView->SetGridFront( sal_False );
	m_pView->SetDragStripes( sal_True );
	m_pView->SetPageVisible();
    sal_Int32 nColor = m_xSection->getBackColor();
    if ( nColor == (sal_Int32)COL_TRANSPARENT )
        nColor = getStyleProperty<sal_Int32>(m_xSection->getReportDefinition(),PROPERTY_BACKCOLOR);
	m_pView->SetApplicationDocumentColor(nColor);

    uno::Reference<report::XReportDefinition> xReportDefinition = m_xSection->getReportDefinition();
    const sal_Int32 nLeftMargin = getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_LEFTMARGIN);
	const sal_Int32 nRightMargin = getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_RIGHTMARGIN);
	m_pPage->SetLeftPageBorder(nLeftMargin);
	m_pPage->SetRightPageBorder(nRightMargin);

// LLA: TODO
//  m_pPage->SetTopPageBorder(-10000);

	m_pView->SetDesignMode( true );	

	m_pView->StartListening( *m_pModel  );
	/*Resize();*/
	m_pPage->SetPageScale( 
		basegfx::B2DVector( 
			getStyleProperty<awt::Size>(xReportDefinition,PROPERTY_PAPERSIZE).Width,
			5*m_xSection->getHeight()));
	const basegfx::B2DVector& rPageScale = m_pPage->GetPageScale();
	m_pView->SetWorkArea( 
		basegfx::B2DRange( 
			nLeftMargin,
			0.0,
			rPageScale.getX() - nLeftMargin - nRightMargin,
			rPageScale.getY()));

    //SetBackground( Wallpaper( COL_BLUE ));
}
// -----------------------------------------------------------------------------
void OReportSection::Paste(const uno::Sequence< beans::NamedValue >& _aAllreadyCopiedObjects,bool _bForce)
{
    OSL_ENSURE(m_xSection.is(),"Why is the section here NULL!");
    if ( m_xSection.is() && _aAllreadyCopiedObjects.getLength() )
    {
	    // stop all drawing actions
	    m_pView->BrkAction();

	    // unmark all objects
	    m_pView->UnmarkAll();
        const ::rtl::OUString sSectionName = m_xSection->getName();
        const sal_Int32 nLength = _aAllreadyCopiedObjects.getLength();
        const beans::NamedValue* pIter = _aAllreadyCopiedObjects.getConstArray();
        const beans::NamedValue* pEnd  = pIter + nLength;
        for(;pIter != pEnd;++pIter)
        {
            if ( _bForce || pIter->Name == sSectionName)
            {
                try
                {
                    uno::Sequence< uno::Reference<report::XReportComponent> > aCopies;
                    pIter->Value >>= aCopies;
                    const uno::Reference<report::XReportComponent>* pCopiesIter = aCopies.getConstArray();
                    const uno::Reference<report::XReportComponent>* pCopiesEnd = pCopiesIter + aCopies.getLength();
                    for (;pCopiesIter != pCopiesEnd ; ++pCopiesIter)
                    {
						SvxShape* pShape = SvxShape::getImplementation( *pCopiesIter );
                        SdrObject* pObject = pShape ? pShape->GetSdrObject() : NULL;
						if ( pObject )
						{   
                            SdrObject* pNeuObj = pObject->CloneSdrObject();
			                m_pPage->InsertObjectToSdrObjList(*pNeuObj);

							const awt::Point aWorkPos((*pCopiesIter)->getPosition());
							const awt::Size aWorkSize((*pCopiesIter)->getSize());
							basegfx::B2DRange aWorkRange(aWorkPos.X, aWorkPos.Y, aWorkPos.X + aWorkSize.Width, aWorkPos.Y + aWorkSize.Height);
							bool bOverlapping = true;
							
							while ( bOverlapping )
							{
								bOverlapping = isOver(aWorkRange,*m_pPage,*m_pView,true,pNeuObj) != NULL;
								if ( bOverlapping )
								{
									aWorkRange.transform(
										basegfx::tools::createTranslateB2DHomMatrix(
											0.0, 
											aWorkRange.getHeight()));
									pNeuObj->setSdrObjectTransformation(
										basegfx::tools::createScaleTranslateB2DHomMatrix(
											aWorkRange.getRange(),
											aWorkRange.getMinimum()));
									//(*pCopiesIter)->setPositionY(aRet.Top());
								}
							}
							m_pView->AddUndo( m_pView->getSdrModelFromSdrView().GetSdrUndoFactory().CreateUndoNewObject( *pNeuObj ) );
							m_pView->MarkObj( *pNeuObj );
                            
							if(m_xSection.is())
							{
								const sal_Int32 nBottom(basegfx::fround(aWorkRange.getMaxY()));
								
								if(static_cast<sal_uInt32>(nBottom) > m_xSection->getHeight())
								{
									m_xSection->setHeight(nBottom);
								}
							}
						}
                    }
                }
                catch(uno::Exception&)
                {
                    OSL_ENSURE(0,"Exception caught while pasting a new object!");
                }
                if ( !_bForce )
                    break;
            }
        }
    }
}
//----------------------------------------------------------------------------
void OReportSection::Delete()
{
	if( !m_pView->areSdrObjectsSelected() )
		return;

	m_pView->BrkAction();
	m_pView->DeleteMarked();
}
//----------------------------------------------------------------------------
void OReportSection::SetMode( DlgEdMode eNewMode )
{
	if ( eNewMode != m_eMode )
	{
		if ( eNewMode == RPTUI_INSERT )
		{
			m_pFunc.reset(new DlgEdFuncInsert( this ));
		}
		else
		{
			m_pFunc.reset(new DlgEdFuncSelect( this ));
		}
        m_pFunc->setOverlappedControlColor(lcl_getOverlappedControlColor( ) );
        m_pModel->SetReadOnly(eNewMode == RPTUI_READONLY);
        m_eMode = eNewMode;
	}	
}
// -----------------------------------------------------------------------------
void OReportSection::Copy(uno::Sequence< beans::NamedValue >& _rAllreadyCopiedObjects)
{	
    Copy(_rAllreadyCopiedObjects,false);
}
//----------------------------------------------------------------------------
void OReportSection::Copy(uno::Sequence< beans::NamedValue >& _rAllreadyCopiedObjects,bool _bEraseAnddNoClone)
{	
    OSL_ENSURE(m_xSection.is(),"Why is the section here NULL!");
	if( !m_pView->areSdrObjectsSelected() || !m_xSection.is() )
		return;

	// stop all drawing actions
	//m_pView->BrkAction();

	// insert control models of marked objects into clipboard dialog model
	const SdrObjectVector aSelection(m_pView->getSelectedSdrObjectVectorFromSdrMarkView());
    ::std::vector< uno::Reference<report::XReportComponent> > aCopies;
    aCopies.reserve(aSelection.size());
    SdrUndoFactory& rUndo = m_pView->getSdrModelFromSdrView().GetSdrUndoFactory();

	for( sal_uInt32 i = aSelection.size(); i > 0; )
	{
        --i;
		SdrObject* pSdrObject = aSelection[i];
        OObjectBase* pObj = dynamic_cast<OObjectBase*>(pSdrObject);

		if ( pObj  )
		{
            try
            {
                SdrObject* pNeuObj = pSdrObject->CloneSdrObject();
                aCopies.push_back(uno::Reference<report::XReportComponent>(pNeuObj->getUnoShape(),uno::UNO_QUERY));

                if ( _bEraseAnddNoClone )
                {
                    m_pView->AddUndo( rUndo.CreateUndoDeleteObject( *pSdrObject ) );
                    m_pPage->RemoveObjectFromSdrObjList(pSdrObject->GetNavigationPosition());
                }

            }
            catch(uno::Exception&)
            {
                OSL_ENSURE(0,"Can't copy report elements!");
            }
		}
	} // for( sal_uLong i = 0; i < nMark; i++ )

    if ( !aCopies.empty() )
    {
        ::std::reverse(aCopies.begin(),aCopies.end());
        const sal_Int32 nLength = _rAllreadyCopiedObjects.getLength();
        _rAllreadyCopiedObjects.realloc( nLength + 1);
        beans::NamedValue* pNewValue = _rAllreadyCopiedObjects.getArray() + nLength;
        pNewValue->Name = m_xSection->getName();
        pNewValue->Value <<= uno::Sequence< uno::Reference<report::XReportComponent> >(&(*aCopies.begin()),aCopies.size());
    }
}
//----------------------------------------------------------------------------
void OReportSection::MouseButtonDown( const MouseEvent& rMEvt )
{
    m_pParent->getViewsWindow()->getView()->setMarked(m_pView,sal_True); // mark the section in which is clicked
	m_pFunc->MouseButtonDown( rMEvt );
    Window::MouseButtonDown(rMEvt);
}
//----------------------------------------------------------------------------
void OReportSection::MouseButtonUp( const MouseEvent& rMEvt )
{
	if ( !m_pFunc->MouseButtonUp( rMEvt ) )
        m_pParent->getViewsWindow()->getView()->getReportView()->getController().executeUnChecked(SID_OBJECT_SELECT,uno::Sequence< beans::PropertyValue>());
}

//----------------------------------------------------------------------------

void OReportSection::MouseMove( const MouseEvent& rMEvt )
{
	m_pFunc->MouseMove( rMEvt );

}
//----------------------------------------------------------------------------
void OReportSection::SetGridVisible(sal_Bool _bVisible)
{
	m_pView->SetGridVisible( _bVisible );
}
//------------------------------------------------------------------------------
void OReportSection::SelectAll(const sal_uInt16 _nObjectType)
{
	if ( m_pView )
    {
        if ( _nObjectType == OBJ_NONE )
            m_pView->MarkAllObj();
        else
        {
            m_pView->UnmarkAll();
            SdrObjListIter aIter(*m_pPage,IM_DEEPNOGROUPS);
            SdrObject* pObjIter = NULL;        
            while( (pObjIter = aIter.Next()) != NULL )
            {
                if ( pObjIter->GetObjIdentifier() == _nObjectType )
                    m_pView->MarkObj( *pObjIter );
            }
        }		
    }
}
void lcl_insertMenuItemImages(PopupMenu& rContextMenu,OReportController& rController,const uno::Reference< report::XReportDefinition>& _xReportDefinition,uno::Reference<frame::XFrame>& _rFrame,sal_Bool _bHiContrast)
{
    const sal_uInt16 nCount = rContextMenu.GetItemCount();
	for (sal_uInt16 i = 0; i < nCount; ++i)
	{
		if ( MENUITEM_SEPARATOR != rContextMenu.GetItemType(i))
		{
			const sal_uInt16 nId = rContextMenu.GetItemId(i);
            PopupMenu* pPopupMenu = rContextMenu.GetPopupMenu( nId );
            if ( pPopupMenu )
            {
                lcl_insertMenuItemImages(*pPopupMenu,rController,_xReportDefinition,_rFrame,_bHiContrast);
            }
            else
            {
                const ::rtl::OUString sCommand = rContextMenu.GetItemCommand(nId);
                rContextMenu.SetItemImage(nId,framework::GetImageFromURL(_rFrame,sCommand,sal_False,_bHiContrast));
                if ( nId == SID_PAGEHEADERFOOTER )
                {
                    String sText = String(ModuleRes((_xReportDefinition.is() && _xReportDefinition->getPageHeaderOn()) ? RID_STR_PAGEHEADERFOOTER_DELETE : RID_STR_PAGEHEADERFOOTER_INSERT));
                    rContextMenu.SetItemText(nId,sText);
                }
                else if ( nId == SID_REPORTHEADERFOOTER )
                {
                    String sText = String(ModuleRes((_xReportDefinition.is() && _xReportDefinition->getReportHeaderOn()) ? RID_STR_REPORTHEADERFOOTER_DELETE : RID_STR_REPORTHEADERFOOTER_INSERT));
                    rContextMenu.SetItemText(nId,sText);
                }
            }
			rContextMenu.CheckItem(nId,rController.isCommandChecked(nId));
			rContextMenu.EnableItem(nId,rController.isCommandEnabled(nId));
		}
	} // for (sal_uInt16 i = 0; i < nCount; ++i)
}
//----------------------------------------------------------------------------
void OReportSection::Command( const CommandEvent& _rCEvt )
{
    Window::Command(_rCEvt);
	switch (_rCEvt.GetCommand())
	{
		case COMMAND_CONTEXTMENU:
		{
            const StyleSettings& rSettings = Application::GetSettings().GetStyleSettings();
            sal_Bool bHiContrast = rSettings.GetHighContrastMode();
            OReportController& rController = m_pParent->getViewsWindow()->getView()->getReportView()->getController();
            uno::Reference<frame::XFrame> xFrame = rController.getFrame();
            PopupMenu aContextMenu( ModuleRes( RID_MENU_REPORT ) );			
            uno::Reference< report::XReportDefinition> xReportDefinition = getSection()->getReportDefinition();
			
            lcl_insertMenuItemImages(aContextMenu,rController,xReportDefinition,xFrame,bHiContrast);

			Point aPos = _rCEvt.GetMousePosPixel();
			m_pView->EndAction();
			const sal_uInt16 nId = aContextMenu.Execute(this, aPos);
			if ( nId )
			{
                uno::Sequence< beans::PropertyValue> aArgs;
                if ( nId == SID_ATTR_CHAR_COLOR_BACKGROUND )
                {
                    aArgs.realloc(1);
                    aArgs[0].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Selection"));
                    aArgs[0].Value <<= m_xSection;
                }
				rController.executeChecked(nId,aArgs);
			}
		}
		break;
	}
}
// -----------------------------------------------------------------------------
void OReportSection::_propertyChanged(const beans::PropertyChangeEvent& _rEvent) throw( uno::RuntimeException)
{
	if ( m_xSection.is() )
	{
		if ( _rEvent.Source == m_xSection || PROPERTY_BACKCOLOR == _rEvent.PropertyName )
		{
            sal_Int32 nColor = m_xSection->getBackColor();
            if ( nColor == (sal_Int32)COL_TRANSPARENT )
                nColor = getStyleProperty<sal_Int32>(m_xSection->getReportDefinition(),PROPERTY_BACKCOLOR);
			m_pView->SetApplicationDocumentColor(nColor);
			Invalidate(INVALIDATE_NOCHILDREN|INVALIDATE_NOERASE);
		}
		else
		{
            uno::Reference<report::XReportDefinition> xReportDefinition = m_xSection->getReportDefinition();
            const sal_Int32 nLeftMargin  = getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_LEFTMARGIN);
            const sal_Int32 nRightMargin = getStyleProperty<sal_Int32>(xReportDefinition,PROPERTY_RIGHTMARGIN);
            const sal_Int32 nPaperWidth  = getStyleProperty<awt::Size>(xReportDefinition,PROPERTY_PAPERSIZE).Width;
            
            if ( _rEvent.PropertyName == PROPERTY_LEFTMARGIN )
            {
	            m_pPage->SetLeftPageBorder(nLeftMargin);
            }
            else if ( _rEvent.PropertyName == PROPERTY_RIGHTMARGIN )
            {
	            m_pPage->SetRightPageBorder(nRightMargin);
            }

			const basegfx::B2DVector& rCurrentPageSize = m_pPage->GetPageScale();
            const sal_uInt32 nNewHeight(m_xSection->getHeight() >= 0.0 ? 5 * m_xSection->getHeight() : 0);
        
            if(basegfx::fround(rCurrentPageSize.getY()) != nNewHeight || nPaperWidth != basegfx::fround(rCurrentPageSize.getX()))
			{
		        m_pPage->SetPageScale(basegfx::B2DVector(nPaperWidth, nNewHeight));
				const basegfx::B2DVector& rNewPageSize = m_pPage->GetPageScale();
		        m_pView->SetWorkArea(
					basegfx::B2DRange(
						nLeftMargin, 
						0.0,
						rNewPageSize.getX() - nLeftMargin - nRightMargin,
						rNewPageSize.getY()));
            }

            impl_adjustObjectSizePosition(nPaperWidth,nLeftMargin,nRightMargin);
            m_pParent->Invalidate(INVALIDATE_UPDATE | INVALIDATE_TRANSPARENT);
		}
	}	
}
void OReportSection::impl_adjustObjectSizePosition(sal_Int32 i_nPaperWidth,sal_Int32 i_nLeftMargin,sal_Int32 i_nRightMargin)
{
    try
    {
	    sal_Int32 nRightBorder = i_nPaperWidth - i_nRightMargin;
        const sal_Int32 nCount = m_xSection->getCount();
        for (sal_Int32 i = 0; i < nCount; ++i)
        {
            bool bChanged = false;
            uno::Reference< report::XReportComponent> xReportComponent(m_xSection->getByIndex(i),uno::UNO_QUERY_THROW);
            awt::Point aPos = xReportComponent->getPosition();
            awt::Size aSize = xReportComponent->getSize();
            SvxShape* pShape = SvxShape::getImplementation( xReportComponent );
            SdrObject* pObject = pShape ? pShape->GetSdrObject() : NULL;
            if ( pObject )
            {
                OObjectBase* pBase = dynamic_cast<OObjectBase*>(pObject);
                pBase->EndListening(sal_False);
                if ( aPos.X < i_nLeftMargin )
                {
                    aPos.X  = i_nLeftMargin;
                    bChanged = true;
                }
                if ( (aPos.X + aSize.Width) > nRightBorder )
                {
                    aPos.X = nRightBorder - aSize.Width;
                    if ( aPos.X < i_nLeftMargin )
                    {
                        aSize.Width += aPos.X - i_nLeftMargin;
                        aPos.X = i_nLeftMargin;
                        // add listener around
                        pBase->StartListening();
                        xReportComponent->setSize(aSize);
                        pBase->EndListening(sal_False);
                    }
                    bChanged = true;
                }
                if ( aPos.Y < 0 )
                    aPos.Y = 0;
                if ( bChanged ) 
                {
                    xReportComponent->setPosition(aPos);
                    correctOverlapping(pObject,*this,false);
                    Rectangle aRet(VCLPoint(xReportComponent->getPosition()),VCLSize(xReportComponent->getSize()));
					aRet.setHeight(aRet.getHeight() + 1);
					aRet.setWidth(aRet.getWidth() + 1);
                    if ( m_xSection.is() && (static_cast<sal_uInt32>(aRet.getHeight() + aRet.Top()) > m_xSection->getHeight()) )
			            m_xSection->setHeight(aRet.getHeight() + aRet.Top());

                    pObject->ActionChanged();
                }
                pBase->StartListening();
            }
        } // for (sal_Int32 i = 0; i < nCount; ++i)
    }
    catch(uno::Exception)
    {
        OSL_ENSURE(0,"Exception caught: OReportSection::_propertyChanged(");
    }
}
//------------------------------------------------------------------------------
sal_Bool OReportSection::handleKeyEvent(const KeyEvent& _rEvent)
{
	return m_pFunc.get() ? m_pFunc->handleKeyEvent(_rEvent) : sal_False;
}
// -----------------------------------------------------------------------------
void OReportSection::deactivateOle()
{
    if ( m_pFunc.get() )
		m_pFunc->deactivateOle(true);
}
// -----------------------------------------------------------------------------
void OReportSection::createDefault(const ::rtl::OUString& _sType)
{
    SdrObject* pObj = m_pView->GetCreateObj();//rMarkList.GetMark(0).GetObj();
    if ( !pObj )
        return;
    createDefault(_sType,pObj);
}
// -----------------------------------------------------------------------------
void OReportSection::createDefault(const ::rtl::OUString& _sType,SdrObject* _pObj)
{
    sal_Bool bAttributesAppliedFromGallery = sal_False;

	if ( GalleryExplorer::GetSdrObjCount( GALLERY_THEME_POWERPOINT ) )
	{
		std::vector< rtl::OUString > aObjList;
		if ( GalleryExplorer::FillObjListTitle( GALLERY_THEME_POWERPOINT, aObjList ) )
		{
            std::vector< rtl::OUString >::iterator aIter = aObjList.begin();
            std::vector< rtl::OUString >::iterator aEnd = aObjList.end();
			for (sal_uInt32 i=0 ; aIter != aEnd; ++aIter,++i)
			{
				if ( aIter->equalsIgnoreAsciiCase( _sType ) )
				{
					OReportModel aReportModel(NULL);
					SfxItemPool& rPool = aReportModel.GetItemPool();
					rPool.FreezeIdRanges();
					if ( GalleryExplorer::GetSdrObj( GALLERY_THEME_POWERPOINT, i, &aReportModel ) )
					{
						const SdrObject* pSourceObj = aReportModel.GetPage( 0 )->GetObj( 0 );
						if( pSourceObj )
						{
							const SfxItemSet& rSource = pSourceObj->GetMergedItemSet();
							SfxItemSet aDest( _pObj->GetObjectItemPool(), 				// ranges from SdrAttrObj
							SDRATTR_START, SDRATTR_SHADOW_LAST,
							SDRATTR_MISC_FIRST, SDRATTR_MISC_LAST,
							SDRATTR_TEXTDIRECTION, SDRATTR_TEXTDIRECTION,
							// Graphic Attributes
							SDRATTR_GRAF_FIRST, SDRATTR_GRAF_LAST,
							// 3d Properties
							SDRATTR_3D_FIRST, SDRATTR_3D_LAST,
							// CustomShape properties
							SDRATTR_CUSTOMSHAPE_FIRST, SDRATTR_CUSTOMSHAPE_LAST,
							// range from SdrTextObj
							EE_ITEMS_START, EE_ITEMS_END,
							// end
							0, 0);
							aDest.Set( rSource );
							_pObj->SetMergedItemSet( aDest );

							const long aOldRotation(sdr::legacy::GetRotateAngle(*pSourceObj));
							if ( aOldRotation )
							{
								sdr::legacy::RotateSdrObject(*_pObj, sdr::legacy::GetSnapRect(*_pObj).Center(), aOldRotation);
							}
							bAttributesAppliedFromGallery = sal_True;
						}
					}
					break;
				}
			}
		}
	}
	if ( !bAttributesAppliedFromGallery )
	{
		_pObj->SetMergedItem( SvxAdjustItem( SVX_ADJUST_CENTER ,ITEMID_ADJUST) );
		_pObj->SetMergedItem( SdrTextVertAdjustItem( SDRTEXTVERTADJUST_CENTER ) );
		_pObj->SetMergedItem( SdrTextHorzAdjustItem( SDRTEXTHORZADJUST_BLOCK ) );
		_pObj->SetMergedItem( SdrOnOffItem(SDRATTR_TEXT_AUTOGROWHEIGHT, sal_False ) );
		((SdrObjCustomShape*)_pObj)->MergeDefaultAttributes( &_sType );
	}
}
// -----------------------------------------------------------------------------
uno::Reference< report::XReportComponent > OReportSection::getCurrentControlModel() const
{
	uno::Reference< report::XReportComponent > xModel;
	if ( m_pView )
	{
		OObjectBase* pSelected = dynamic_cast< OObjectBase* >(m_pView->getSelectedIfSingle());

		if ( pSelected )
        {
			xModel = pSelected->getReportComponent().get();
		}
	}

	return xModel;
}
// -----------------------------------------------------------------------------
void OReportSection::fillControlModelSelection(::std::vector< uno::Reference< uno::XInterface > >& _rSelection) const
{
    if ( m_pView )
	{
		const SdrObjectVector aSelection(m_pView->getSelectedSdrObjectVectorFromSdrMarkView());

        for(sal_uInt32 i(0); i < aSelection.size(); ++i)
        {
			const OObjectBase* pObj = dynamic_cast< const OObjectBase* >(aSelection[i]);
			
			if ( pObj )
			{
                _rSelection.push_back(pObj->getReportComponent());
		}
	}
}
}
// -----------------------------------------------------------------------------
sal_Int8 OReportSection::AcceptDrop( const AcceptDropEvent& _rEvt )
{
    OSL_TRACE("AcceptDrop::DropEvent.Action %i\n", _rEvt.mnAction);

    ::Point aDropPos(_rEvt.maPosPixel);
    const MouseEvent aMouseEvt(aDropPos);
    if ( m_pFunc->isOverlapping(aMouseEvt) )
        return DND_ACTION_NONE;

    if ( _rEvt.mnAction == DND_ACTION_COPY ||
         _rEvt.mnAction == DND_ACTION_LINK
         )
    {
		if (!m_pParent) return DND_ACTION_NONE;
		sal_uInt16 nCurrentPosition = 0;
		nCurrentPosition = m_pParent->getViewsWindow()->getPosition(m_pParent);
		if (_rEvt.mnAction == DND_ACTION_COPY )
		{
			// we must assure, we can't drop in the top section
			if (nCurrentPosition < 1)
			{
				return DND_ACTION_NONE;
			}
			return DND_ACTION_LINK;
		}
		if (_rEvt.mnAction == DND_ACTION_LINK)
		{
			// we must assure, we can't drop in the bottom section
			if (m_pParent->getViewsWindow()->getSectionCount() > (nCurrentPosition + 1)  )
			{
				return DND_ACTION_COPY;
			}
			return DND_ACTION_NONE;
		}
    }
    else
    {
        const DataFlavorExVector& rFlavors = GetDataFlavorExVector();
        if (   ::svx::OMultiColumnTransferable::canExtractDescriptor(rFlavors)
            || ::svx::OColumnTransferable::canExtractColumnDescriptor(rFlavors, CTF_FIELD_DESCRIPTOR | CTF_CONTROL_EXCHANGE | CTF_COLUMN_DESCRIPTOR) )
            return _rEvt.mnAction;
        
        const sal_Int8 nDropOption = ( OReportExchange::canExtract(rFlavors) ) ? DND_ACTION_COPYMOVE : DND_ACTION_NONE;

        return nDropOption;
    }
    return DND_ACTION_NONE;
}

// -----------------------------------------------------------------------------
sal_Int8 OReportSection::ExecuteDrop( const ExecuteDropEvent& _rEvt )
{
    OSL_TRACE("ExecuteDrop::DropEvent.Action %i\n", _rEvt.mnAction);
    ::Point aDropPos(PixelToLogic(_rEvt.maPosPixel));
    const MouseEvent aMouseEvt(aDropPos);
    if ( m_pFunc->isOverlapping(aMouseEvt) )
        return DND_ACTION_NONE;

    sal_Int8 nDropOption = DND_ACTION_NONE;
    const TransferableDataHelper aDropped(_rEvt.maDropEvent.Transferable);
    DataFlavorExVector& rFlavors = aDropped.GetDataFlavorExVector();
    bool bMultipleFormat = ::svx::OMultiColumnTransferable::canExtractDescriptor(rFlavors);
    if ( OReportExchange::canExtract(rFlavors) )
    {
        OReportExchange::TSectionElements aCopies = OReportExchange::extractCopies(aDropped);
		Paste(aCopies,true);
        nDropOption = DND_ACTION_COPYMOVE;
        m_pParent->getViewsWindow()->BrkAction();
        m_pParent->getViewsWindow()->unmarkAllObjects(m_pView);
        //m_pParent->getViewsWindow()->getView()->setMarked(m_pView,sal_True);
    } // if ( OReportExchange::canExtract(rFlavors) )
    else if ( bMultipleFormat
        || ::svx::OColumnTransferable::canExtractColumnDescriptor(rFlavors, CTF_FIELD_DESCRIPTOR | CTF_CONTROL_EXCHANGE | CTF_COLUMN_DESCRIPTOR) )
    {
        m_pParent->getViewsWindow()->getView()->setMarked(m_pView,sal_True);
        m_pView->UnmarkAll();

		const basegfx::B2DRange aWorkArea(m_pView->GetWorkArea());
		const basegfx::B2DPoint aClampedDropPos(aWorkArea.clamp(basegfx::B2DPoint(aDropPos.X(), aDropPos.Y())));
		aDropPos = Point(basegfx::fround(aClampedDropPos.getX()), basegfx::fround(aClampedDropPos.getY()));

        uno::Sequence<beans::PropertyValue> aValues;
        if ( !bMultipleFormat )
        {
            ::svx::ODataAccessDescriptor aDescriptor = ::svx::OColumnTransferable::extractColumnDescriptor(aDropped);
		    
            aValues.realloc(1);
            aValues[0].Value <<= aDescriptor.createPropertyValueSequence();
        } // if ( !bMultipleFormat )
        else 
            aValues = ::svx::OMultiColumnTransferable::extractDescriptor(aDropped);
        
        beans::PropertyValue* pIter = aValues.getArray();
        beans::PropertyValue* pEnd  = pIter + aValues.getLength();
        for(;pIter != pEnd; ++pIter)
        {
            uno::Sequence<beans::PropertyValue> aCurrent;
            pIter->Value >>= aCurrent;
            sal_Int32 nLength = aCurrent.getLength();
            if ( nLength )
            {
                aCurrent.realloc(nLength + 3);
                aCurrent[nLength].Name = PROPERTY_POSITION;
                aCurrent[nLength++].Value <<= AWTPoint(aDropPos);
                // give also the DND Action (Shift|Ctrl) Key to really say what we want 
                aCurrent[nLength].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("DNDAction"));
                aCurrent[nLength++].Value <<= _rEvt.mnAction;

                aCurrent[nLength].Name = ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("Section"));
                aCurrent[nLength++].Value <<= getSection();
                pIter->Value <<= aCurrent;
            }
        }

        // we use this way to create undo actions
        OReportController& rController = m_pParent->getViewsWindow()->getView()->getReportView()->getController();
		rController.executeChecked(SID_ADD_CONTROL_PAIR,aValues);
        nDropOption = DND_ACTION_COPY;
    }
	return nDropOption;
}
// -----------------------------------------------------------------------------
void OReportSection::stopScrollTimer()
{
    m_pFunc->stopScrollTimer();
}
// -----------------------------------------------------------------------------
bool OReportSection::isUiActive() const
{
    return m_pFunc->isUiActive();
}
// -----------------------------------------------------------------------------
// =============================================================================
}
// =============================================================================
