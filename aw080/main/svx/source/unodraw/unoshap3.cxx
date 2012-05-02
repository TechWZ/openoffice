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
#include "precompiled_svx.hxx"

#define _SVX_USE_UNOGLOBALS_
#include <com/sun/star/drawing/HomogenMatrix.hpp>
#include <com/sun/star/drawing/Position3D.hpp>
#include <com/sun/star/drawing/Direction3D.hpp>
#include <com/sun/star/drawing/DoubleSequence.hpp>
#include <com/sun/star/drawing/CameraGeometry.hpp>
#include <vcl/svapp.hxx>
#include <vos/mutex.hxx>
#include <comphelper/serviceinfohelper.hxx>
#include <rtl/uuid.h>
#include <rtl/memory.h>
#include <svx/svdpool.hxx>
#include <svx/unoshape.hxx>
#include <svx/unopage.hxx>
#include <editeng/unoprnms.hxx>
#include <svx/globl3d.hxx>
#include <svx/cube3d.hxx>
#include <svx/sphere3d.hxx>
#include <svx/lathe3d.hxx>
#include <svx/extrud3d.hxx>
#include <svx/polygn3d.hxx>
#include "svx/unoshprp.hxx"
#include "svx/svdmodel.hxx"
#include <basegfx/polygon/b3dpolygon.hxx>
#include <basegfx/polygon/b3dpolygontools.hxx>
#include <com/sun/star/drawing/PolyPolygonShape3D.hpp>
#include <basegfx/polygon/b2dpolypolygontools.hxx>
#include <basegfx/polygon/b3dpolypolygontools.hxx>
#include <basegfx/matrix/b3dhommatrixtools.hxx>
#include <svx/svdlegacy.hxx>
#include <svx/svditer.hxx>
#include <svx/scene3d.hxx>
#include <svx/globaldrawitempool.hxx>

using ::rtl::OUString;
using namespace ::vos;
using namespace ::cppu;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::container;

#define INTERFACE_TYPE( xint ) \
	::getCppuType((const Reference< xint >*)0)

#define QUERYINT( xint ) \
	if( rType == ::getCppuType((const Reference< xint >*)0) ) \
		aAny <<= Reference< xint >(this)

/***********************************************************************
* class Svx3DSceneObject                                               *
***********************************************************************/

//----------------------------------------------------------------------
Svx3DSceneObject::Svx3DSceneObject( SdrObject* pObj, SvxDrawPage* pDrawPage ) throw()
:	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_3DSCENEOBJECT), aSvxMapProvider.GetPropertySet(SVXMAP_3DSCENEOBJECT, GetGlobalDrawObjectItemPool()) )
,	mxPage( pDrawPage )
{
}

//----------------------------------------------------------------------
Svx3DSceneObject::~Svx3DSceneObject() throw()
{
}

//----------------------------------------------------------------------
void Svx3DSceneObject::Create( SdrObject* pNewObj, SvxDrawPage* pNewPage )
{
	SvxShape::Create( pNewObj, pNewPage );
	mxPage = pNewPage;
}

//----------------------------------------------------------------------
uno::Any SAL_CALL Svx3DSceneObject::queryAggregation( const uno::Type & rType ) throw(uno::RuntimeException)
{
	uno::Any aAny;

	QUERYINT( drawing::XShapes );
	else QUERYINT( container::XIndexAccess );
	else QUERYINT( container::XElementAccess );
	else
		return SvxShape::queryAggregation( rType );

	return aAny;
}

uno::Any SAL_CALL Svx3DSceneObject::queryInterface( const uno::Type & rType ) throw( uno::RuntimeException )
{
	return SvxShape::queryInterface( rType );
}

void SAL_CALL Svx3DSceneObject::acquire() throw ( )
{
	SvxShape::acquire();
}

void SAL_CALL Svx3DSceneObject::release() throw ( )
{
	SvxShape::release();
}

// XTypeProvider

uno::Sequence< uno::Type > SAL_CALL Svx3DSceneObject::getTypes()
	throw (uno::RuntimeException)
{

	return SvxShape::getTypes();
}

uno::Sequence< sal_Int8 > SAL_CALL Svx3DSceneObject::getImplementationId()
	throw (uno::RuntimeException)
{
	static uno::Sequence< sal_Int8 > aId;
	if( aId.getLength() == 0 )
	{
		aId.realloc( 16 );
		rtl_createUuid( (sal_uInt8 *)aId.getArray(), 0, sal_True );
	}
	return aId;
}

//----------------------------------------------------------------------
void SAL_CALL Svx3DSceneObject::add( const Reference< drawing::XShape >& xShape )
	throw( uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxShape* pShape = SvxShape::getImplementation( xShape );

	if(!mpObj.is() || !mxPage.is() || pShape == NULL || NULL != pShape->GetSdrObject() )
		throw uno::RuntimeException();

	SdrObject* pSdrObject = dynamic_cast< SdrObject* >(mxPage->_CreateSdrObject( xShape ));
	E3dScene* pSdrShape = dynamic_cast< E3dScene* >(pSdrObject);

	if( pSdrShape && pSdrShape->getChildrenOfSdrObject() )
	{
		mpObj->getChildrenOfSdrObject()->InsertObjectToSdrObjList(*pSdrShape);

		if(pShape)
			pShape->Create( pSdrShape, mxPage.get()  );
	}
	else
	{
		deleteSdrObjectSafeAndClearPointer( pSdrObject );
		throw uno::RuntimeException();
	}

	if( mpModel )
		mpModel->SetChanged();
}

//----------------------------------------------------------------------
void SAL_CALL Svx3DSceneObject::remove( const Reference< drawing::XShape >& xShape )
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	SvxShape* pShape = SvxShape::getImplementation( xShape );

	if(!mpObj.is() || pShape == NULL)
		throw uno::RuntimeException();

	SdrObject* pSdrShape = pShape->GetSdrObject();
	if(pSdrShape == NULL || pSdrShape->getParentOfSdrObject()->getSdrObjectFromSdrObjList() != mpObj.get())
	{
		throw uno::RuntimeException();
	}
	else
	{
		SdrObjList& rList = *pSdrShape->getParentOfSdrObject();

		const sal_uInt32 nObjCount = rList.GetObjCount();
		sal_uInt32 nObjNum = 0;
		while( nObjNum < nObjCount )
		{
			if(rList.GetObj( nObjNum ) == pSdrShape )
				break;
			nObjNum++;
		}

		if( nObjNum < nObjCount )
		{
			SdrObject* pObject = rList.RemoveObjectFromSdrObjList( nObjNum );
            deleteSdrObjectSafeAndClearPointer( pObject );
		}
		else
		{
			DBG_ASSERT( 0, "Fatality! SdrObject is not belonging to its SdrObjList! [CL]" );
		}
	}
}

//----------------------------------------------------------------------
sal_Int32 SAL_CALL Svx3DSceneObject::getCount()
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	sal_Int32 nRetval = 0;
	E3dScene* pE3dScene = dynamic_cast< E3dScene* >(mpObj.get());

	if(pE3dScene && pE3dScene->getChildrenOfSdrObject())
	{
		nRetval = pE3dScene->getChildrenOfSdrObject()->GetObjCount();
	}

	return nRetval;
}

//----------------------------------------------------------------------

uno::Any SAL_CALL Svx3DSceneObject::getByIndex( sal_Int32 Index )
	throw( lang::IndexOutOfBoundsException, lang::WrappedTargetException, uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	if( !mpObj.is() || mpObj->getChildrenOfSdrObject() == NULL )
		throw uno::RuntimeException();

	if( mpObj->getChildrenOfSdrObject()->GetObjCount() <= (sal_uInt32)Index )
		throw lang::IndexOutOfBoundsException();

	SdrObject* pDestObj = mpObj->getChildrenOfSdrObject()->GetObj( Index );
	if(pDestObj == NULL)
		throw lang::IndexOutOfBoundsException();

	Reference< drawing::XShape > xShape( pDestObj->getUnoShape(), uno::UNO_QUERY );
	uno::Any aAny;
	aAny <<= xShape;
	return aAny;
}

//----------------------------------------------------------------------
// ::com::sun::star::container::XElementAccess

uno::Type SAL_CALL Svx3DSceneObject::getElementType()
	throw( uno::RuntimeException )
{
	return ::getCppuType(( const Reference< drawing::XShape>*)0);
}

//----------------------------------------------------------------------
sal_Bool SAL_CALL Svx3DSceneObject::hasElements()
	throw( uno::RuntimeException )
{
	OGuard aGuard( Application::GetSolarMutex() );

	return mpObj.is() && mpObj->getChildrenOfSdrObject() && (mpObj->getChildrenOfSdrObject()->GetObjCount() > 0);
}

//----------------------------------------------------------------------

static bool ConvertHomogenMatrixToObject( E3dObject* pObject, const Any& rValue )
{
	drawing::HomogenMatrix aMat;
	if( rValue >>= aMat )
	{
		pObject->SetB3DTransform(basegfx::tools::UnoHomogenMatrixToB3DHomMatrix(aMat));
		return true;
	}
	return false;
}

static void ConvertObjectToHomogenMatric( E3dObject* pObject, Any& rValue )
{
	drawing::HomogenMatrix aHomMat;
	const basegfx::B3DHomMatrix& rMat = pObject->GetB3DTransform();
	basegfx::tools::B3DHomMatrixToUnoHomogenMatrix(rMat, aHomMat);
	rValue <<= aHomMat;
}

//----------------------------------------------------------------------

struct ImpRememberTransAndRect
{
	basegfx::B3DHomMatrix					maMat;
	Rectangle					maRect;
};

bool Svx3DSceneObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformationsmatrix in das Objekt packen
		    if( ConvertHomogenMatrixToObject( static_cast< E3dObject* >( mpObj.get() ), rValue ) )
			    return true;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_CAMERA_GEOMETRY:
	    {
		    // set CameraGeometry at scene
		    E3dScene* pScene = static_cast< E3dScene* >( mpObj.get() );
		    drawing::CameraGeometry aCamGeo;

		    if(rValue >>= aCamGeo)
		    {
			    basegfx::B3DPoint aVRP(aCamGeo.vrp.PositionX, aCamGeo.vrp.PositionY, aCamGeo.vrp.PositionZ);
			    basegfx::B3DVector aVPN(aCamGeo.vpn.DirectionX, aCamGeo.vpn.DirectionY, aCamGeo.vpn.DirectionZ);
			    basegfx::B3DVector aVUP(aCamGeo.vup.DirectionX, aCamGeo.vup.DirectionY, aCamGeo.vup.DirectionZ);

			    // rescue scene transformation
			    ImpRememberTransAndRect aSceneTAR;
			    aSceneTAR.maMat = pScene->GetB3DTransform();
			    aSceneTAR.maRect = sdr::legacy::GetSnapRect(*pScene);

			    // rescue object transformations
			    SdrObjListIter aIter(*pScene->getChildrenOfSdrObject(), IM_DEEPWITHGROUPS);
			    List aObjTrans;
			    while(aIter.IsMore())
			    {
				    E3dObject* p3DObj = (E3dObject*)aIter.Next();
				    basegfx::B3DHomMatrix* pNew = new basegfx::B3DHomMatrix;
				    *pNew = p3DObj->GetB3DTransform();
				    aObjTrans.Insert(pNew, LIST_APPEND);
			    }

			    // reset object transformations
			    aIter.Reset();
			    while(aIter.IsMore())
			    {
				    E3dObject* p3DObj = (E3dObject*)aIter.Next();
				    p3DObj->SetB3DTransform(basegfx::B3DHomMatrix());
			    }

			    // reset scene transformation and make a complete recalc
			    pScene->SetB3DTransform(basegfx::B3DHomMatrix());

			    // fill old camera from new parameters
			    Camera3D aCam(pScene->GetCamera());
			    const basegfx::B3DRange& rVolume = pScene->GetBoundVolume();
			    double fW = rVolume.getWidth();
			    double fH = rVolume.getHeight();

			    const SfxItemSet& rSceneSet = pScene->GetMergedItemSet();
			    double fCamPosZ = 
				    (double)((const SfxUInt32Item&)rSceneSet.Get(SDRATTR_3DSCENE_DISTANCE)).GetValue();
			    double fCamFocal = 
				    (double)((const SfxUInt32Item&)rSceneSet.Get(SDRATTR_3DSCENE_FOCAL_LENGTH)).GetValue();

			    aCam.SetAutoAdjustProjection(sal_False);
			    aCam.SetViewWindow(- fW / 2, - fH / 2, fW, fH);
			    basegfx::B3DPoint aLookAt;
			    basegfx::B3DPoint aCamPos(0.0, 0.0, fCamPosZ);
			    aCam.SetPosAndLookAt(aCamPos, aLookAt);
			    aCam.SetFocalLength(fCamFocal / 100.0);
			    aCam.SetDefaults(basegfx::B3DPoint(0.0, 0.0, fCamPosZ), aLookAt, fCamFocal / 100.0);
			    aCam.SetDeviceWindow(Rectangle(0, 0, (long)fW, (long)fH));

			    // set at scene
			    pScene->SetCamera(aCam);

			    // #91047# use imported VRP, VPN and VUP (if used)
			    sal_Bool bVRPUsed(!aVRP.equal(basegfx::B3DPoint(0.0, 0.0, 1.0)));
			    sal_Bool bVPNUsed(!aVPN.equal(basegfx::B3DVector(0.0, 0.0, 1.0)));
			    sal_Bool bVUPUsed(!aVUP.equal(basegfx::B3DVector(0.0, 1.0, 0.0)));

			    if(bVRPUsed || bVPNUsed || bVUPUsed)
			    {
				    pScene->GetCameraSet().SetViewportValues(aVRP, aVPN, aVUP);
			    }

			    // set object transformations again at objects
			    aIter.Reset();
			    sal_uInt32 nIndex(0L);
			    while(aIter.IsMore())
			    {
				    E3dObject* p3DObj = (E3dObject*)aIter.Next();
				    basegfx::B3DHomMatrix* pMat = (basegfx::B3DHomMatrix*)aObjTrans.GetObject(nIndex++);
				    p3DObj->SetB3DTransform(*pMat);
				    delete pMat;
			    }

			    // set scene transformation again at scene
			    pScene->SetB3DTransform(aSceneTAR.maMat);
			    sdr::legacy::SetSnapRect(*pScene, aSceneTAR.maRect);

			    return true;
		    }
		    break;
	    }
	    default:
            return SvxShape::setPropertyValueImpl(rName, pProperty, rValue);
	}

	throw IllegalArgumentException();
}

//----------------------------------------------------------------------

bool Svx3DSceneObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Objekt in eine homogene 4x4 Matrix packen
		    ConvertObjectToHomogenMatric( static_cast< E3dObject* >( mpObj.get() ), rValue );
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_CAMERA_GEOMETRY:
	    {
		    // get CameraGeometry from scene
		    E3dScene* pScene = static_cast< E3dScene* >( mpObj.get() );
		    drawing::CameraGeometry aCamGeo;

		    // fill Vectors from scene camera
		    B3dCamera& aCameraSet = pScene->GetCameraSet();
		    basegfx::B3DPoint aVRP(aCameraSet.GetVRP());
		    basegfx::B3DVector aVPN(aCameraSet.GetVPN());
		    basegfx::B3DVector aVUP(aCameraSet.GetVUV());

		    // transfer to structure
		    aCamGeo.vrp.PositionX = aVRP.getX();
		    aCamGeo.vrp.PositionY = aVRP.getY();
		    aCamGeo.vrp.PositionZ = aVRP.getZ();
		    aCamGeo.vpn.DirectionX = aVPN.getX();
		    aCamGeo.vpn.DirectionY = aVPN.getY();
		    aCamGeo.vpn.DirectionZ = aVPN.getZ();
		    aCamGeo.vup.DirectionX = aVUP.getX();
		    aCamGeo.vup.DirectionY = aVUP.getY();
		    aCamGeo.vup.DirectionZ = aVUP.getZ();

		    rValue <<= aCamGeo;
		    break;
	    }
	    default:
            return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL Svx3DSceneObject::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	uno::Sequence< OUString > aSeq( SvxShape::getSupportedServiceNames() );
	comphelper::ServiceInfoHelper::addToSequence( aSeq, 1, "com.sun.star.drawing.Shape3DScene" );
	return aSeq;
}

/***********************************************************************
*                                                                      *
***********************************************************************/

//----------------------------------------------------------------------
Svx3DCubeObject::Svx3DCubeObject( SdrObject* pObj ) throw()
:	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_3DCUBEOBJEKT), aSvxMapProvider.GetPropertySet(SVXMAP_3DCUBEOBJEKT, GetGlobalDrawObjectItemPool()) )
{
}

//----------------------------------------------------------------------
Svx3DCubeObject::~Svx3DCubeObject() throw()
{
}

//----------------------------------------------------------------------
bool Svx3DCubeObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	OGuard aGuard( Application::GetSolarMutex() );

	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformationsmatrix in das Objekt packen
		    if( ConvertHomogenMatrixToObject( static_cast< E3dObject* >( mpObj.get() ), rValue ) )
			    return true;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POSITION: 
	    {
		    // Position in das Objekt packen
		    drawing::Position3D aUnoPos;
		    if( rValue >>= aUnoPos )
		    {
			    basegfx::B3DPoint aPos(aUnoPos.PositionX, aUnoPos.PositionY, aUnoPos.PositionZ);
			    static_cast< E3dCubeObj* >( mpObj.get() )->SetCubePos(aPos);
			    return true;
		    }
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_SIZE:
	    {
		    // Groesse in das Objekt packen
		    drawing::Direction3D aDirection;
		    if( rValue >>= aDirection )
		    {
			    basegfx::B3DVector aSize(aDirection.DirectionX, aDirection.DirectionY, aDirection.DirectionZ);
			    static_cast< E3dCubeObj* >( mpObj.get() )->SetCubeSize(aSize);
			    return true;
		    }
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POS_IS_CENTER:
	    {
		    sal_Bool bNew = sal_False;
		    // sal_Bool bPosIsCenter in das Objekt packen
		    if( rValue >>= bNew )
		    {
			    static_cast< E3dCubeObj* >( mpObj.get() )->SetPosIsCenter(bNew);
			    return true;
		    }
		    break;
	    }
	    default:
            return SvxShape::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw IllegalArgumentException();
}

//----------------------------------------------------------------------

bool Svx3DCubeObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformation in eine homogene Matrix packen
		    ConvertObjectToHomogenMatric( static_cast< E3dObject* >( mpObj.get() ), rValue );
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POSITION:
	    {
		    // Position packen
		    const basegfx::B3DPoint& rPos = ((E3dCubeObj*)mpObj.get())->GetCubePos();
		    drawing::Position3D aPos;

		    aPos.PositionX = rPos.getX();
		    aPos.PositionY = rPos.getY();
		    aPos.PositionZ = rPos.getZ();

		    rValue <<= aPos;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_SIZE:
	    {
		    // Groesse packen
		    const basegfx::B3DVector& rSize = static_cast<E3dCubeObj*>(mpObj.get())->GetCubeSize();
		    drawing::Direction3D aDir;

		    aDir.DirectionX = rSize.getX();
		    aDir.DirectionY = rSize.getY();
		    aDir.DirectionZ = rSize.getZ();

		    rValue <<= aDir;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POS_IS_CENTER:
	    {
		    rValue <<= static_cast<E3dCubeObj*>(mpObj.get())->GetPosIsCenter();
		    break;
	    }
	    default:
            return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL Svx3DCubeObject::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	uno::Sequence< OUString > aSeq( SvxShape::getSupportedServiceNames() );
	comphelper::ServiceInfoHelper::addToSequence( aSeq, 2, "com.sun.star.drawing.Shape3D",
							"com.sun.star.drawing.Shape3DCube");
	return aSeq;
}

/***********************************************************************
*                                                                      *
***********************************************************************/

//----------------------------------------------------------------------
Svx3DSphereObject::Svx3DSphereObject( SdrObject* pObj ) throw()
:	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_3DSPHEREOBJECT), aSvxMapProvider.GetPropertySet(SVXMAP_3DSPHEREOBJECT, GetGlobalDrawObjectItemPool()) )
{
}

//----------------------------------------------------------------------
Svx3DSphereObject::~Svx3DSphereObject() throw()
{
}

//----------------------------------------------------------------------

bool Svx3DSphereObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformationsmatrix in das Objekt packen
		    if( ConvertHomogenMatrixToObject( static_cast< E3dObject* >( mpObj.get() ), rValue ) )
			    return true;
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_POSITION:
	    {
		    // Position in das Objekt packen
		    drawing::Position3D aUnoPos;
		    if( rValue >>= aUnoPos )
		    {
			    basegfx::B3DPoint aPos(aUnoPos.PositionX, aUnoPos.PositionY, aUnoPos.PositionZ);
			    static_cast<E3dSphereObj*>(mpObj.get())->SetCenter(aPos);
			    return true;
		    }
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_SIZE:
	    {
		    // Groesse in das Objekt packen
		    drawing::Direction3D aDir;
		    if( rValue >>= aDir )
		    {
			    basegfx::B3DVector aPos(aDir.DirectionX, aDir.DirectionY, aDir.DirectionZ);
			    static_cast<E3dSphereObj*>(mpObj.get())->SetSize(aPos);
			    return true;
		    }
		    break;
	    }
	    default:
            return SvxShape::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw IllegalArgumentException();
}

//----------------------------------------------------------------------

bool Svx3DSphereObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformation in eine homogene Matrix packen
		    ConvertObjectToHomogenMatric( static_cast< E3dObject* >( mpObj.get() ), rValue );
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POSITION:
	    {
		    // Position packen
		    const basegfx::B3DPoint& rPos = ((E3dSphereObj*)mpObj.get())->Center();
		    drawing::Position3D aPos;

		    aPos.PositionX = rPos.getX();
		    aPos.PositionY = rPos.getY();
		    aPos.PositionZ = rPos.getZ();

		    rValue <<= aPos;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_SIZE:
	    {
		    // Groesse packen
		    const basegfx::B3DVector& rSize = ((E3dSphereObj*)mpObj.get())->Size();
		    drawing::Direction3D aDir;

		    aDir.DirectionX = rSize.getX();
		    aDir.DirectionY = rSize.getY();
		    aDir.DirectionZ = rSize.getZ();

		    rValue <<= aDir;
		    break;
	    }
	    default:
            return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL Svx3DSphereObject::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	uno::Sequence< OUString > aSeq( SvxShape::getSupportedServiceNames() );
	comphelper::ServiceInfoHelper::addToSequence( aSeq, 2, "com.sun.star.drawing.Shape3D",
							"com.sun.star.drawing.Shape3DSphere");
	return aSeq;
}

/***********************************************************************
*                                                                      *
***********************************************************************/

//----------------------------------------------------------------------
Svx3DLatheObject::Svx3DLatheObject( SdrObject* pObj ) throw()
:	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_3DLATHEOBJECT), aSvxMapProvider.GetPropertySet(SVXMAP_3DLATHEOBJECT, GetGlobalDrawObjectItemPool()) )
{
}

//----------------------------------------------------------------------
Svx3DLatheObject::~Svx3DLatheObject() throw()
{
}

bool PolyPolygonShape3D_to_B3dPolyPolygon( 
	const Any& rValue, 
	basegfx::B3DPolyPolygon& rResultPolygon,
	bool bCorrectPolygon)
{
	drawing::PolyPolygonShape3D aSourcePolyPolygon;
	if( !(rValue >>= aSourcePolyPolygon) )
		return false;

	rResultPolygon = basegfx::tools::UnoPolyPolygonShape3DToB3DPolyPolygon(aSourcePolyPolygon, bCorrectPolygon);
	return true;
}

void B3dPolyPolygon_to_PolyPolygonShape3D( const basegfx::B3DPolyPolygon& rSourcePolyPolygon, Any& rValue )
{
	drawing::PolyPolygonShape3D aRetval;
	basegfx::tools::B3DPolyPolygonToUnoPolyPolygonShape3D(rSourcePolyPolygon, aRetval);
	rValue <<= aRetval;
}

//----------------------------------------------------------------------

bool Svx3DLatheObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformationsmatrix in das Objekt packen
		    if( ConvertHomogenMatrixToObject( static_cast< E3dObject* >( mpObj.get() ), rValue ) )
			    return true;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POLYPOLYGON3D:
	    {
		    // Polygondefinition in das Objekt packen
		    basegfx::B3DPolyPolygon aNewB3DPolyPolygon;

		    // #i101520# Probably imported
		    if( PolyPolygonShape3D_to_B3dPolyPolygon( rValue, aNewB3DPolyPolygon, true ) )
		    {
			    // #105127# SetPolyPoly3D sets the 3DVerticalSegments to the number
			    // of points of the polygon. Thus, value gets lost. To avoid this, rescue
			    // item here and re-set after setting the polygon.
			    const sal_uInt32 nPrevVerticalSegs(static_cast<E3dLatheObj*>(mpObj.get())->GetVerticalSegments());

			    // Polygon setzen
			    const basegfx::B3DHomMatrix aIdentity;
			    const basegfx::B2DPolyPolygon aB2DPolyPolygon(basegfx::tools::createB2DPolyPolygonFromB3DPolyPolygon(aNewB3DPolyPolygon, aIdentity));
			    static_cast<E3dLatheObj*>(mpObj.get())->SetPolyPoly2D(aB2DPolyPolygon);
			    const sal_uInt32 nPostVerticalSegs(static_cast<E3dLatheObj*>(mpObj.get())->GetVerticalSegments());

			    if(nPrevVerticalSegs != nPostVerticalSegs)
			    {
				    // restore the vertical segment count
				    static_cast<E3dLatheObj*>(mpObj.get())->SetMergedItem(SfxUInt32Item(SDRATTR_3DOBJ_VERT_SEGS, nPrevVerticalSegs));
			    }
			    return true;
		    }
		    break;
	    }
	    default:
            return SvxShape::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw IllegalArgumentException();
}

//----------------------------------------------------------------------
bool Svx3DLatheObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // pack Transformation to a homogen Matrix
		    drawing::HomogenMatrix aHomMat; 
		    basegfx::B3DHomMatrix aMat = static_cast<E3dObject*>(mpObj.get())->GetB3DTransform(); 
		    basegfx::tools::B3DHomMatrixToUnoHomogenMatrix(aMat, aHomMat);
		    rValue <<= aHomMat;
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_POLYPOLYGON3D:
	    {
		    const basegfx::B2DPolyPolygon& rPolyPoly = static_cast<E3dLatheObj*>(mpObj.get())->GetPolyPoly2D();
		    const basegfx::B3DPolyPolygon aB3DPolyPolygon(basegfx::tools::createB3DPolyPolygonFromB2DPolyPolygon(rPolyPoly));

		    B3dPolyPolygon_to_PolyPolygonShape3D(aB3DPolyPolygon, rValue);
		    break;
	    }
	    default:
            return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL Svx3DLatheObject::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	uno::Sequence< OUString > aSeq( SvxShape::getSupportedServiceNames() );
	comphelper::ServiceInfoHelper::addToSequence( aSeq, 2, "com.sun.star.drawing.Shape3D",
							"com.sun.star.drawing.Shape3DLathe");
	return aSeq;
}

/***********************************************************************
*                                                                      *
***********************************************************************/

Svx3DExtrudeObject::Svx3DExtrudeObject( SdrObject* pObj ) throw()
:	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_3DEXTRUDEOBJECT), aSvxMapProvider.GetPropertySet(SVXMAP_3DEXTRUDEOBJECT, GetGlobalDrawObjectItemPool()) )
{
}

//----------------------------------------------------------------------
Svx3DExtrudeObject::~Svx3DExtrudeObject() throw()
{
}

//----------------------------------------------------------------------

bool Svx3DExtrudeObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformationsmatrix in das Objekt packen
		    if( ConvertHomogenMatrixToObject( static_cast< E3dObject* >( mpObj.get() ), rValue ) )
			    return true;
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_POLYPOLYGON3D:
	    {
		    // Polygondefinition in das Objekt packen
		    basegfx::B3DPolyPolygon aNewB3DPolyPolygon;

		    // #i101520# Probably imported
		    if( PolyPolygonShape3D_to_B3dPolyPolygon( rValue, aNewB3DPolyPolygon, true ) )
		    {
			    // Polygon setzen
			    const basegfx::B3DHomMatrix aIdentity;
			    const basegfx::B2DPolyPolygon aB2DPolyPolygon(basegfx::tools::createB2DPolyPolygonFromB3DPolyPolygon(aNewB3DPolyPolygon, aIdentity));
			    static_cast<E3dExtrudeObj*>(mpObj.get())->SetExtrudePolygon(aB2DPolyPolygon);
			    return true;
		    }
		    break;
	    }
	    default:
            return SvxShape::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw IllegalArgumentException();
}

//----------------------------------------------------------------------

bool Svx3DExtrudeObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // pack Transformation to a homogen Matrix
		    drawing::HomogenMatrix aHomMat; 
		    basegfx::B3DHomMatrix aMat = ((E3dObject*)mpObj.get())->GetB3DTransform(); 
		    basegfx::tools::B3DHomMatrixToUnoHomogenMatrix(aMat, aHomMat);
		    rValue <<= aHomMat;
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_POLYPOLYGON3D:
	    {
		    // Polygondefinition packen
		    const basegfx::B2DPolyPolygon& rPolyPoly = static_cast<E3dExtrudeObj*>(mpObj.get())->GetExtrudePolygon();
		    const basegfx::B3DPolyPolygon aB3DPolyPolygon(basegfx::tools::createB3DPolyPolygonFromB2DPolyPolygon(rPolyPoly));

		    B3dPolyPolygon_to_PolyPolygonShape3D(aB3DPolyPolygon, rValue);
		    break;
	    }
	    default:
            return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL Svx3DExtrudeObject::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	uno::Sequence< OUString > aSeq( SvxShape::getSupportedServiceNames() );
	comphelper::ServiceInfoHelper::addToSequence( aSeq, 2, "com.sun.star.drawing.Shape3D",
							"com.sun.star.drawing.Shape3DExtrude");
	return aSeq;
}

/***********************************************************************
*                                                                      *
***********************************************************************/

//----------------------------------------------------------------------
Svx3DPolygonObject::Svx3DPolygonObject( SdrObject* pObj ) throw()
:	SvxShape( pObj, aSvxMapProvider.GetMap(SVXMAP_3DPOLYGONOBJECT), aSvxMapProvider.GetPropertySet(SVXMAP_3DPOLYGONOBJECT, GetGlobalDrawObjectItemPool()) )
{
}

//----------------------------------------------------------------------
Svx3DPolygonObject::~Svx3DPolygonObject() throw()
{
}

//----------------------------------------------------------------------
bool Svx3DPolygonObject::setPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, const ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    // Transformationsmatrix in das Objekt packen
		    if( ConvertHomogenMatrixToObject( static_cast< E3dObject* >( mpObj.get() ), rValue ) )
			    return true;
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_POLYPOLYGON3D:
	    {
		    // Polygondefinition in das Objekt packen
		    basegfx::B3DPolyPolygon aNewB3DPolyPolygon;

		    // #i101520# Direct API data (e.g. from chart)
		    if( PolyPolygonShape3D_to_B3dPolyPolygon( rValue, aNewB3DPolyPolygon, false ) )
		    {
			    // Polygon setzen
			    static_cast<E3dPolygonObj*>(mpObj.get())->SetPolyPolygon3D(aNewB3DPolyPolygon);
			    return true;
		    }
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_NORMALSPOLYGON3D:
	    {
		    // Normalendefinition in das Objekt packen
		    basegfx::B3DPolyPolygon aNewB3DPolyPolygon;

		    // #i101520# Direct API data (e.g. from chart)
		    if( PolyPolygonShape3D_to_B3dPolyPolygon( rValue, aNewB3DPolyPolygon, false ) )
		    {
			    // Polygon setzen
			    static_cast<E3dPolygonObj*>(mpObj.get())->SetPolyNormals3D(aNewB3DPolyPolygon);
			    return true;
		    }
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_TEXTUREPOLYGON3D:
	    {
		    // Texturdefinition in das Objekt packen
		    basegfx::B3DPolyPolygon aNewB3DPolyPolygon;

		    // #i101520# Direct API data (e.g. from chart)
		    if( PolyPolygonShape3D_to_B3dPolyPolygon( rValue, aNewB3DPolyPolygon, false ) )
		    {
			    // Polygon setzen
			    const basegfx::B3DHomMatrix aIdentity;
			    const basegfx::B2DPolyPolygon aB2DPolyPolygon(basegfx::tools::createB2DPolyPolygonFromB3DPolyPolygon(aNewB3DPolyPolygon, aIdentity));
			    static_cast<E3dPolygonObj*>(mpObj.get())->SetPolyTexture2D(aB2DPolyPolygon);
			    return true;
		    }
		    break;
	    }
	    case OWN_ATTR_3D_VALUE_LINEONLY:
	    {
		    sal_Bool bNew = sal_False;
		    if( rValue >>= bNew )
		    {
			    static_cast<E3dPolygonObj*>(mpObj.get())->SetLineOnly(bNew);
			    return true;
		    }
		    break;
	    }
	    default:
            return SvxShape::setPropertyValueImpl( rName, pProperty, rValue );
	}

	throw IllegalArgumentException();
}

//----------------------------------------------------------------------
bool Svx3DPolygonObject::getPropertyValueImpl( const ::rtl::OUString& rName, const SfxItemPropertySimpleEntry* pProperty, ::com::sun::star::uno::Any& rValue ) throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException)
{
	switch( pProperty->nWID )
	{
	    case OWN_ATTR_3D_VALUE_TRANSFORM_MATRIX:
	    {
		    ConvertObjectToHomogenMatric( static_cast< E3dObject* >( mpObj.get() ), rValue );
		    break;
	    }
	
	    case OWN_ATTR_3D_VALUE_POLYPOLYGON3D:
	    {
		    B3dPolyPolygon_to_PolyPolygonShape3D(static_cast<E3dPolygonObj*>(mpObj.get())->GetPolyPolygon3D(),rValue);
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_NORMALSPOLYGON3D:
	    {
		    B3dPolyPolygon_to_PolyPolygonShape3D(static_cast<E3dPolygonObj*>(mpObj.get())->GetPolyNormals3D(),rValue);
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_TEXTUREPOLYGON3D:
	    {
		    // Texturdefinition packen
		    const basegfx::B2DPolyPolygon& rPolyPoly = static_cast<E3dPolygonObj*>(mpObj.get())->GetPolyTexture2D();
		    const basegfx::B3DPolyPolygon aB3DPolyPolygon(basegfx::tools::createB3DPolyPolygonFromB2DPolyPolygon(rPolyPoly));

		    B3dPolyPolygon_to_PolyPolygonShape3D(aB3DPolyPolygon,rValue);
		    break;
	    }

	    case OWN_ATTR_3D_VALUE_LINEONLY:
	    {
		    rValue <<= (sal_Bool)static_cast<E3dPolygonObj*>(mpObj.get())->GetLineOnly();
		    break;
	    }

	    default:
            return SvxShape::getPropertyValueImpl( rName, pProperty, rValue );
	}

	return true;
}

// ::com::sun::star::lang::XServiceInfo
uno::Sequence< OUString > SAL_CALL Svx3DPolygonObject::getSupportedServiceNames()
	throw(uno::RuntimeException)
{
	Sequence< OUString > aSeq( SvxShape::getSupportedServiceNames() );
	comphelper::ServiceInfoHelper::addToSequence( aSeq, 2, "com.sun.star.drawing.Shape3D",
							"com.sun.star.drawing.Shape3DPolygon");
	return aSeq;
}

// eof
