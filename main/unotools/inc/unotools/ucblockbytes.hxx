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


#ifndef _UNTOOLS_UCBLOCKBYTES_HXX
#define _UNTOOLS_UCBLOCKBYTES_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/ucb/XContent.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include "unotools/unotoolsdllapi.h"

#include <vos/thread.hxx>
#include <vos/conditn.hxx>
#include <vos/mutex.hxx>
#include <tools/stream.hxx>
#include <tools/link.hxx>
#include <tools/errcode.hxx>
#include <tools/datetime.hxx>

namespace com
{
	namespace sun
	{
		namespace star
		{
			namespace task
			{
				class XInteractionHandler;
			}
			namespace io
			{
				class XStream;
				class XInputStream;
				class XOutputStream;
				class XSeekable;
			}
			namespace ucb
			{
				class XContent;
			}
			namespace beans
			{
				struct PropertyValue;
			}
		}
	}
}

namespace utl
{
SV_DECL_REF( UcbLockBytes )

class UcbLockBytesHandler : public SvRefBase
{
    sal_Bool        m_bActive;
public:
    enum LoadHandlerItem
    {
        DATA_AVAILABLE,
        DONE,
        CANCEL
    };

                    UcbLockBytesHandler()
                        : m_bActive( sal_True )
                    {}

    virtual void    Handle( LoadHandlerItem nWhich, UcbLockBytesRef xLockBytes ) = 0;
    void            Activate( sal_Bool bActivate = sal_True ) { m_bActive = bActivate; }
    sal_Bool        IsActive() const { return m_bActive; }
};

SV_DECL_IMPL_REF( UcbLockBytesHandler )

#define NS_UNO ::com::sun::star::uno
#define NS_IO ::com::sun::star::io
#define NS_UCB ::com::sun::star::ucb
#define NS_BEANS ::com::sun::star::beans
#define NS_TASK ::com::sun::star::task

class UNOTOOLS_DLLPUBLIC UcbLockBytes : public virtual SvLockBytes
{
	vos::OCondition			m_aInitialized;
	vos::OCondition			m_aTerminated;
	vos::OMutex				m_aMutex;

    String                  m_aContentType;
    String                  m_aRealURL;
    DateTime                m_aExpireDate;

    NS_UNO::Reference < NS_IO::XInputStream >  m_xInputStream;
    NS_UNO::Reference < NS_IO::XOutputStream > m_xOutputStream;
    NS_UNO::Reference < NS_IO::XSeekable >     m_xSeekable;
	void*					m_pCommandThread; // is alive only for compatibility reasons
    UcbLockBytesHandlerRef  m_xHandler;

	sal_uInt32              m_nRead;
	sal_uInt32              m_nSize;
    ErrCode                 m_nError;

    sal_Bool                m_bTerminated : 1;
    sal_Bool                m_bDontClose : 1;
    sal_Bool                m_bStreamValid : 1;

	DECL_LINK(				DataAvailHdl, void * );

                            UcbLockBytes( UcbLockBytesHandler* pHandler=NULL );
protected:
	virtual                 ~UcbLockBytes (void);

public:
							// properties: Referer, PostMimeType
    static UcbLockBytesRef  CreateLockBytes( const NS_UNO::Reference < NS_UCB::XContent >& xContent,
                                            const ::rtl::OUString& rReferer,
                                            const ::rtl::OUString& rMediaType,
                                            const NS_UNO::Reference < NS_IO::XInputStream >& xPostData,
                                            const NS_UNO::Reference < NS_TASK::XInteractionHandler >& xInter,
											UcbLockBytesHandler* pHandler=0 );

    static UcbLockBytesRef  CreateLockBytes( const NS_UNO::Reference < NS_UCB::XContent >& xContent,
											const NS_UNO::Sequence < NS_BEANS::PropertyValue >& rProps,
											StreamMode eMode,
                                            const NS_UNO::Reference < NS_TASK::XInteractionHandler >& xInter,
											UcbLockBytesHandler* pHandler=0 );

    static UcbLockBytesRef  CreateInputLockBytes( const NS_UNO::Reference < NS_IO::XInputStream >& xContent );
    static UcbLockBytesRef  CreateLockBytes( const NS_UNO::Reference < NS_IO::XStream >& xContent );

	// SvLockBytes
	virtual void            SetSynchronMode (sal_Bool bSynchron);
	virtual ErrCode         ReadAt ( sal_uLong nPos, void *pBuffer, sal_uLong nCount, sal_uLong *pRead) const;
	virtual ErrCode         WriteAt ( sal_uLong, const void*, sal_uLong, sal_uLong *pWritten);
	virtual ErrCode         Flush (void) const;
	virtual ErrCode         SetSize (sal_uLong);
	virtual ErrCode         Stat ( SvLockBytesStat *pStat, SvLockBytesStatFlag) const;

    void                    SetError( ErrCode nError )
                            { m_nError = nError; }

    ErrCode                 GetError() const
                            { return m_nError; }

    void                    Cancel(); // is alive only for compatibility reasons

    // the following properties are available when and after the first DataAvailable callback has been executed
    String                  GetContentType() const;
    String                  GetRealURL() const;
    DateTime                GetExpireDate() const;

    // calling this method delegates the responsibility to call closeinput to the caller!
    NS_UNO::Reference < NS_IO::XInputStream > getInputStream();
    NS_UNO::Reference < NS_IO::XStream > getStream();

#if _SOLAR__PRIVATE
    sal_Bool                setInputStream_Impl( const NS_UNO::Reference < NS_IO::XInputStream > &rxInputStream,
												 sal_Bool bSetXSeekable = sal_True );
    sal_Bool                setStream_Impl( const NS_UNO::Reference < NS_IO::XStream > &rxStream );
    void                    terminate_Impl (void);

    NS_UNO::Reference < NS_IO::XInputStream > getInputStream_Impl() const
                            {
                                vos::OGuard aGuard( SAL_CONST_CAST(UcbLockBytes*, this)->m_aMutex );
                                return m_xInputStream;
                            }

    NS_UNO::Reference < NS_IO::XOutputStream > getOutputStream_Impl() const
                            {
                                vos::OGuard aGuard( SAL_CONST_CAST(UcbLockBytes*, this)->m_aMutex );
                                return m_xOutputStream;
                            }

    NS_UNO::Reference < NS_IO::XSeekable > getSeekable_Impl() const
                            {
                                vos::OGuard aGuard( SAL_CONST_CAST(UcbLockBytes*, this)->m_aMutex );
                                return m_xSeekable;
                            }

    sal_Bool                hasInputStream_Impl() const
                            {
                                vos::OGuard aGuard( SAL_CONST_CAST(UcbLockBytes*, this)->m_aMutex );
                                return m_xInputStream.is();
                            }

    void                    setDontClose_Impl()
                            { m_bDontClose = sal_True; }

    void                    SetContentType_Impl( const String& rType ) { m_aContentType = rType; }
    void                    SetRealURL_Impl( const String& rURL )  { m_aRealURL = rURL; }
    void                    SetExpireDate_Impl( const DateTime& rDateTime )  { m_aExpireDate = rDateTime; }
    void                    SetStreamValid_Impl();
#endif
};

SV_IMPL_REF( UcbLockBytes );

}

#endif
