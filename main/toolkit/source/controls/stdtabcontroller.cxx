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
#include "precompiled_toolkit.hxx"
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/awt/XVclContainerPeer.hpp>

#include <toolkit/controls/stdtabcontroller.hxx>
#include <toolkit/controls/stdtabcontrollermodel.hxx>
#include <toolkit/awt/vclxwindow.hxx>
#include <toolkit/helper/macros.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <rtl/memory.h>
#include <rtl/uuid.h>

#include <tools/debug.hxx>
#include <vcl/window.hxx>
#include <comphelper/sequence.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;

//	----------------------------------------------------
//	class StdTabController
//	----------------------------------------------------
StdTabController::StdTabController()
{
}

StdTabController::~StdTabController()
{
}

sal_Bool StdTabController::ImplCreateComponentSequence(
		Sequence< Reference< XControl > >&				rControls,
		const Sequence< Reference< XControlModel > >&	rModels,
		Sequence< Reference< XWindow > >&				rComponents,
		Sequence< Any>*									pTabStops,
		sal_Bool bPeerComponent ) const
{
	sal_Bool bOK = sal_True;			

	// nur die wirklich geforderten Controls
	sal_Int32 nModels = rModels.getLength();
	if (nModels != rControls.getLength())
	{
		Sequence< Reference< XControl > > aSeq( nModels );
		const Reference< XControlModel >* pModels = rModels.getConstArray();
		Reference< XControl >  xCurrentControl;

		sal_Int32 nRealControls = 0;
		for (sal_Int32 n = 0; n < nModels; ++n, ++pModels)
		{
			xCurrentControl = FindControl(rControls, *pModels);
			if (xCurrentControl.is())
				aSeq.getArray()[nRealControls++] = xCurrentControl;
		}
		aSeq.realloc(nRealControls);
		rControls = aSeq;
	}
#ifdef DBG_UTIL
	DBG_ASSERT( rControls.getLength() <= rModels.getLength(), "StdTabController:ImplCreateComponentSequence: inconsistence!" );
		// there may be less controls than models, but never more controls than models
#endif


	const Reference< XControl > * pControls = rControls.getConstArray();
	sal_uInt32 nCtrls = rControls.getLength(); 
	rComponents.realloc( nCtrls );
	Reference< XWindow > * pComps = rComponents.getArray();
	Any* pTabs = NULL;


	if ( pTabStops )
	{
		*pTabStops = Sequence< Any>( nCtrls );
		pTabs = pTabStops->getArray();
	}

	for ( sal_uInt32 n = 0; bOK && ( n < nCtrls ); n++ )
	{			
		// Zum Model passendes Control suchen
		Reference< XControl >  xCtrl(pControls[n]);
		if ( xCtrl.is() )
		{
			if (bPeerComponent)
				pComps[n] = Reference< XWindow > (xCtrl->getPeer(), UNO_QUERY);							
			else
				pComps[n] = Reference< XWindow > (xCtrl, UNO_QUERY);

			// TabStop-Property
			if ( pTabs )
			{					
				// opt: String fuer TabStop als Konstante
				static const ::rtl::OUString aTabStopName( ::rtl::OUString::createFromAscii( "Tabstop" ) );
				
				Reference< XPropertySet >  xPSet( xCtrl->getModel(), UNO_QUERY );
				Reference< XPropertySetInfo >  xInfo = xPSet->getPropertySetInfo();
				if( xInfo->hasPropertyByName( aTabStopName ) )
					*pTabs++ = xPSet->getPropertyValue( aTabStopName );
                else
                    ++pTabs;
			}
		}
		else
		{
			DBG_TRACE( "ImplCreateComponentSequence: Control not found" );
			bOK = sal_False;
		}
	}
	return bOK;
}

void StdTabController::ImplActivateControl( sal_Bool bFirst ) const
{
	// HACK wegen #53688#, muss auf ein Interface abgebildet werden, wenn Controls Remote liegen koennen.
	Reference< XTabController >  xTabController(const_cast< ::cppu::OWeakObject* >(static_cast< const ::cppu::OWeakObject* >(this)), UNO_QUERY);
	Sequence< Reference< XControl > > aCtrls = xTabController->getControls();
	const Reference< XControl > * pControls = aCtrls.getConstArray();
	sal_uInt32 nCount = aCtrls.getLength();
	
	for ( sal_uInt32 n = bFirst ? 0 : nCount; bFirst ? ( n < nCount ) : n; )
	{			
		sal_uInt32 nCtrl = bFirst ? n++ : --n;
		DBG_ASSERT( pControls[nCtrl].is(), "Control nicht im Container!" );
		if ( pControls[nCtrl].is() )
		{
			Reference< XWindowPeer >  xCP = pControls[nCtrl]->getPeer();
			if ( xCP.is() )
			{
				VCLXWindow* pC = VCLXWindow::GetImplementation( xCP );
				if ( pC && pC->GetWindow() && ( pC->GetWindow()->GetStyle() & WB_TABSTOP ) )
				{
					pC->GetWindow()->GrabFocus();
					break;
				}
			}
		}
	}
}

// XInterface
Any StdTabController::queryAggregation( const Type & rType ) throw(RuntimeException)
{
	Any aRet = ::cppu::queryInterface( rType,
										SAL_STATIC_CAST( XTabController*, this ),
										SAL_STATIC_CAST( XServiceInfo*, this ),
										SAL_STATIC_CAST( XTypeProvider*, this ) );
	return (aRet.hasValue() ? aRet : OWeakAggObject::queryAggregation( rType ));
}

// XTypeProvider
IMPL_XTYPEPROVIDER_START( StdTabController )
	getCppuType( ( Reference< XTabController>* ) NULL ),
	getCppuType( ( Reference< XServiceInfo>* ) NULL )
IMPL_XTYPEPROVIDER_END

void StdTabController::setModel( const Reference< XTabControllerModel >& Model ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	mxModel = Model;
}

Reference< XTabControllerModel > StdTabController::getModel(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	return mxModel;
}

void StdTabController::setContainer( const Reference< XControlContainer >& Container ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	mxControlContainer = Container;
}

Reference< XControlContainer > StdTabController::getContainer(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	return mxControlContainer;
}

Sequence< Reference< XControl > > StdTabController::getControls(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	Sequence< Reference< XControl > > aSeq;

	if ( mxControlContainer.is() )
	{
		Sequence< Reference< XControlModel > > aModels = mxModel->getControlModels();
		const Reference< XControlModel > * pModels = aModels.getConstArray();

		Sequence< Reference< XControl > > xCtrls = mxControlContainer->getControls();

		sal_uInt32 nCtrls = aModels.getLength();
		aSeq = Sequence< Reference< XControl > >( nCtrls );
		for ( sal_uInt32 n = 0; n < nCtrls; n++ )
		{
			Reference< XControlModel >  xCtrlModel = pModels[n];
			// Zum Model passendes Control suchen
			Reference< XControl >  xCtrl = FindControl( xCtrls, xCtrlModel );
			aSeq.getArray()[n] = xCtrl;
		}
	}
	return aSeq;
}

void StdTabController::autoTabOrder(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	DBG_ASSERT( mxControlContainer.is(), "autoTabOrder: No ControlContainer!" );
	if ( !mxControlContainer.is() )
		return;

	Sequence< Reference< XControlModel > > aSeq = mxModel->getControlModels();
	Sequence< Reference< XWindow > > aCompSeq;

	// vieleicht erhalte ich hier einen TabController,
	// der schneller die Liste meiner Controls ermittelt
	Reference< XTabController >  xTabController(static_cast< ::cppu::OWeakObject* >(this), UNO_QUERY);	
	Sequence< Reference< XControl > > aControls = xTabController->getControls();

	// #58317# Es sind ggf. noch nicht alle Controls fuer die Models im Container,
	// dann kommt spaeter nochmal ein autoTabOrder...
	if( !ImplCreateComponentSequence( aControls, aSeq, aCompSeq, NULL, sal_False ) )
		return;

	sal_uInt32 nCtrls = aCompSeq.getLength();
	Reference< XWindow > * pComponents = aCompSeq.getArray();

	ComponentEntryList aCtrls;
	sal_uInt32 n;
	for ( n = 0; n < nCtrls; n++ )
	{
		XWindow* pC = (XWindow*)pComponents[n].get();
		ComponentEntry* pE = new ComponentEntry;
		pE->pComponent = pC;
		awt::Rectangle aPosSize = pC->getPosSize();
		pE->aPos.X() = aPosSize.X;
		pE->aPos.Y() = aPosSize.Y;

		sal_uInt16 nPos;
		for ( nPos = 0; nPos < aCtrls.Count(); nPos++ )
		{
			ComponentEntry* pEntry = aCtrls.GetObject( nPos );
			if ( pEntry->aPos.Y() >= pE->aPos.Y() )
			{
				while ( pEntry && ( pEntry->aPos.Y() == pE->aPos.Y() )
								&& ( pEntry->aPos.X() < pE->aPos.X() ) )
				{
					pEntry = aCtrls.GetObject( ++nPos );
				}
				break;
			}
		}
		aCtrls.Insert( pE, nPos );
	}

	Sequence< Reference< XControlModel > > aNewSeq( nCtrls );
	for ( n = 0; n < nCtrls; n++ )
	{
		ComponentEntry* pE = aCtrls.GetObject( n );
		Reference< XControl >  xUC( pE->pComponent, UNO_QUERY );
		aNewSeq.getArray()[n] = xUC->getModel();
		delete pE;
	}
	aCtrls.Clear();

	mxModel->setControlModels( aNewSeq );
}

void StdTabController::activateTabOrder(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	// Am Container die Tab-Reihenfolge aktivieren...

	Reference< XControl >  xC( mxControlContainer, UNO_QUERY );
	Reference< XVclContainerPeer >  xVclContainerPeer;
    if ( xC.is() )
        xVclContainerPeer = xVclContainerPeer.query( xC->getPeer() );
 	if ( !xC.is() || !xVclContainerPeer.is() )
		return;

	// vieleicht erhalte ich hier einen TabController,
	// der schneller die Liste meiner Controls ermittelt
	Reference< XTabController >  xTabController(static_cast< ::cppu::OWeakObject* >(this), UNO_QUERY);	

	// Flache Liste besorgen...
	Sequence< Reference< XControlModel > > aModels = mxModel->getControlModels();
	Sequence< Reference< XWindow > > aCompSeq;
	Sequence< Any> aTabSeq;

	// DG: Aus Optimierungsgruenden werden die Controls mittels getControls() geholt,
	// dieses hoert sich zwar wiedersinning an, fuehrt aber im konkreten Fall (Forms) zu sichtbaren
	// Geschwindigkeitsvorteilen	
	Sequence< Reference< XControl > > aControls = xTabController->getControls();

	// #58317# Es sind ggf. noch nicht alle Controls fuer die Models im Container,
	// dann kommt spaeter nochmal ein activateTabOrder...
	if( !ImplCreateComponentSequence( aControls, aModels, aCompSeq, &aTabSeq, sal_True ) )
		return;

	xVclContainerPeer->setTabOrder( aCompSeq, aTabSeq, mxModel->getGroupControl() );

	::rtl::OUString aName;
	Sequence< Reference< XControlModel > >	aThisGroupModels;
	Sequence< Reference< XWindow > >		aControlComponents;

	sal_uInt32 nGroups = mxModel->getGroupCount();
	for ( sal_uInt32 nG = 0; nG < nGroups; nG++ )
	{
		mxModel->getGroup( nG, aThisGroupModels, aName );

		aControls = xTabController->getControls();
			// ImplCreateComponentSequence has a really strange semantics regarding it's first parameter:
			// upon method entry, it expects a super set of the controls which it returns
			// this means we need to completely fill this sequence with all available controls before
			// calling into ImplCreateComponentSequence

		aControlComponents.realloc( 0 );

		ImplCreateComponentSequence( aControls, aThisGroupModels, aControlComponents, NULL, sal_True );
		xVclContainerPeer->setGroup( aControlComponents );
	}
}

void StdTabController::activateFirst(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	ImplActivateControl( sal_True );
}

void StdTabController::activateLast(  ) throw(RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );
	
	ImplActivateControl( sal_False );
}


Reference< XControl >  StdTabController::FindControl( Sequence< Reference< XControl > >& rCtrls,
 const Reference< XControlModel > & rxCtrlModel )
{

/*
 	// MT: Funktioniert nicht mehr, weil ich nicht mehr bei mir angemeldet bin,
	// weil DG das abfaengt.

	// #54677# Beim Laden eines HTML-Dokuments wird nach jedem Control ein
	// activateTabOrder gerufen und jede Menge Zeit in dieser Methode verbraten.
	// Die Anzahl dieser Schleifendurchlaufe steigt quadratisch, also versuchen
	// das Control direkt vom Model zu erhalten.
	// => Wenn genau ein Control als PropertyChangeListener angemeldet ist,
	// dann muss das auch das richtige sein.

	UnoControlModel* pUnoCtrlModel = UnoControlModel::GetImplementation( rxCtrlModel );


	if ( pUnoCtrlModel )
	{
		ListenerIterator aIt( pUnoCtrlModel->maPropertiesListeners );
		while( aIt.hasMoreElements() )
		{
			XEventListener* pL = aIt.next();
			Reference< XControl >  xC( pL, UNO_QUERY );
			if ( xC.is() )
			{
				if( xC->getContext() == mxControlContainer )
				{
					xCtrl = xC;
					break;
				}
			}
		}
	}
	if ( !xCtrl.is() && rxCtrlModel.is())
*/
	DBG_ASSERT( rxCtrlModel.is(), "ImplFindControl - welches ?!" );
	
	const Reference< XControl > * pCtrls = rCtrls.getConstArray();
	sal_Int32 nCtrls = rCtrls.getLength();
	for ( sal_Int32 n = 0; n < nCtrls; n++ )
	{
		Reference< XControlModel >  xModel(pCtrls[n].is() ? pCtrls[n]->getModel() : Reference< XControlModel > ());
		if ( (XControlModel*)xModel.get() == (XControlModel*)rxCtrlModel.get() )
		{
			Reference< XControl >  xCtrl( pCtrls[n] );
			::comphelper::removeElementAt( rCtrls, n );
			return xCtrl;
		}
	}
	return Reference< XControl > ();
}
