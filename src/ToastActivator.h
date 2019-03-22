

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.01.0622 */
/* at Tue Jan 19 04:14:07 2038
 */
/* Compiler settings for ToastActivator.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0622
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __ToastActivator_h__
#define __ToastActivator_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */

#ifndef __INotificationActivationCallback_FWD_DEFINED__
#define __INotificationActivationCallback_FWD_DEFINED__
typedef interface INotificationActivationCallback INotificationActivationCallback;

#endif 	/* __INotificationActivationCallback_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif


/* interface __MIDL_itf_ToastActivator_0000_0000 */
/* [local] */

typedef struct _NOTIFICATION_USER_INPUT_DATA
    {
    LPCWSTR Key;
    LPCWSTR Value;
    } 	NOTIFICATION_USER_INPUT_DATA;



extern RPC_IF_HANDLE __MIDL_itf_ToastActivator_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_ToastActivator_0000_0000_v0_0_s_ifspec;

#ifndef __INotificationActivationCallback_INTERFACE_DEFINED__
#define __INotificationActivationCallback_INTERFACE_DEFINED__

/* interface INotificationActivationCallback */
/* [ref][uuid][object] */


EXTERN_C const IID IID_INotificationActivationCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("62337340-CB78-4AE9-A524-685424C52DC7")
    INotificationActivationCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate(
            /* [string][in] */ LPCWSTR appUserModelId,
            /* [string][in] */ LPCWSTR invokedArgs,
            /* [unique][size_is][in] */ const NOTIFICATION_USER_INPUT_DATA *data,
            /* [in] */ ULONG dataCount) = 0;

    };


#else 	/* C style interface */

    typedef struct INotificationActivationCallbackVtbl
    {
        BEGIN_INTERFACE

        HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
            INotificationActivationCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */
            _COM_Outptr_  void **ppvObject);

        ULONG ( STDMETHODCALLTYPE *AddRef )(
            INotificationActivationCallback * This);

        ULONG ( STDMETHODCALLTYPE *Release )(
            INotificationActivationCallback * This);

        HRESULT ( STDMETHODCALLTYPE *Activate )(
            INotificationActivationCallback * This,
            /* [string][in] */ LPCWSTR appUserModelId,
            /* [string][in] */ LPCWSTR invokedArgs,
            /* [unique][size_is][in] */ const NOTIFICATION_USER_INPUT_DATA *data,
            /* [in] */ ULONG dataCount);

        END_INTERFACE
    } INotificationActivationCallbackVtbl;

    interface INotificationActivationCallback
    {
        CONST_VTBL struct INotificationActivationCallbackVtbl *lpVtbl;
    };



#ifdef COBJMACROS


#define INotificationActivationCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) )

#define INotificationActivationCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) )

#define INotificationActivationCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) )


#define INotificationActivationCallback_Activate(This,appUserModelId,invokedArgs,data,dataCount)	\
    ( (This)->lpVtbl -> Activate(This,appUserModelId,invokedArgs,data,dataCount) )

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __INotificationActivationCallback_INTERFACE_DEFINED__ */


/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


