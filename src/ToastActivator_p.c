

/* this ALWAYS GENERATED file contains the proxy stub code */


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

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "ToastActivator.h"

#define TYPE_FORMAT_STRING_SIZE   53                                
#define PROC_FORMAT_STRING_SIZE   57                                
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   0            

typedef struct _ToastActivator_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } ToastActivator_MIDL_TYPE_FORMAT_STRING;

typedef struct _ToastActivator_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } ToastActivator_MIDL_PROC_FORMAT_STRING;

typedef struct _ToastActivator_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } ToastActivator_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const ToastActivator_MIDL_TYPE_FORMAT_STRING ToastActivator__MIDL_TypeFormatString;
extern const ToastActivator_MIDL_PROC_FORMAT_STRING ToastActivator__MIDL_ProcFormatString;
extern const ToastActivator_MIDL_EXPR_FORMAT_STRING ToastActivator__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO INotificationActivationCallback_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO INotificationActivationCallback_ProxyInfo;



#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const ToastActivator_MIDL_PROC_FORMAT_STRING ToastActivator__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure Activate */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 16 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x1 ),	/* 1 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter appUserModelId */

/* 26 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 28 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 30 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter invokedArgs */

/* 32 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 34 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 36 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter data */

/* 38 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 40 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 42 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Parameter dataCount */

/* 44 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 46 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 48 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 50 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 52 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 54 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const ToastActivator_MIDL_TYPE_FORMAT_STRING ToastActivator__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  4 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x12, 0x0,	/* FC_UP */
/*  8 */	NdrFcShort( 0x16 ),	/* Offset= 22 (30) */
/* 10 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 12 */	NdrFcShort( 0x10 ),	/* 16 */
/* 14 */	NdrFcShort( 0x0 ),	/* 0 */
/* 16 */	NdrFcShort( 0x6 ),	/* Offset= 6 (22) */
/* 18 */	0x36,		/* FC_POINTER */
			0x36,		/* FC_POINTER */
/* 20 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 22 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 24 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 26 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 28 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 30 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 32 */	NdrFcShort( 0x0 ),	/* 0 */
/* 34 */	0x29,		/* Corr desc:  parameter, FC_ULONG */
			0x0,		/*  */
/* 36 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 38 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 40 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 44 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 46 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 48 */	NdrFcShort( 0xffda ),	/* Offset= -38 (10) */
/* 50 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */

			0x0
        }
    };


/* Standard interface: __MIDL_itf_ToastActivator_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: INotificationActivationCallback, ver. 0.0,
   GUID={0x62337340,0xCB78,0x4AE9,{0xA5,0x24,0x68,0x54,0x24,0xC5,0x2D,0xC7}} */

#pragma code_seg(".orpc")
static const unsigned short INotificationActivationCallback_FormatStringOffsetTable[] =
    {
    0
    };

static const MIDL_STUBLESS_PROXY_INFO INotificationActivationCallback_ProxyInfo =
    {
    &Object_StubDesc,
    ToastActivator__MIDL_ProcFormatString.Format,
    &INotificationActivationCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO INotificationActivationCallback_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    ToastActivator__MIDL_ProcFormatString.Format,
    &INotificationActivationCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _INotificationActivationCallbackProxyVtbl = 
{
    &INotificationActivationCallback_ProxyInfo,
    &IID_INotificationActivationCallback,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* INotificationActivationCallback::Activate */
};

const CInterfaceStubVtbl _INotificationActivationCallbackStubVtbl =
{
    &IID_INotificationActivationCallback,
    &INotificationActivationCallback_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    ToastActivator__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x801026e, /* MIDL Version 8.1.622 */
    0,
    0,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _ToastActivator_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_INotificationActivationCallbackProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _ToastActivator_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_INotificationActivationCallbackStubVtbl,
    0
};

PCInterfaceName const _ToastActivator_InterfaceNamesList[] = 
{
    "INotificationActivationCallback",
    0
};


#define _ToastActivator_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _ToastActivator, pIID, n)

int __stdcall _ToastActivator_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_ToastActivator_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

const ExtendedProxyFileInfo ToastActivator_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _ToastActivator_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _ToastActivator_StubVtblList,
    (const PCInterfaceName * ) & _ToastActivator_InterfaceNamesList,
    0, /* no delegation */
    & _ToastActivator_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

