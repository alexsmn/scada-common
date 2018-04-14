

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Thu Dec 01 21:39:59 2016
 */
/* Compiler settings for C:\Work\vidicon\vidicon2\src\TeleClient\TeleClient.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
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
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __TeleClient_h__
#define __TeleClient_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IDataPoint_FWD_DEFINED__
#define __IDataPoint_FWD_DEFINED__
typedef interface IDataPoint IDataPoint;
#endif 	/* __IDataPoint_FWD_DEFINED__ */


#ifndef __IDataPointServer_FWD_DEFINED__
#define __IDataPointServer_FWD_DEFINED__
typedef interface IDataPointServer IDataPointServer;
#endif 	/* __IDataPointServer_FWD_DEFINED__ */


#ifndef __IDataPoint3_FWD_DEFINED__
#define __IDataPoint3_FWD_DEFINED__
typedef interface IDataPoint3 IDataPoint3;
#endif 	/* __IDataPoint3_FWD_DEFINED__ */


#ifndef __IClient_FWD_DEFINED__
#define __IClient_FWD_DEFINED__
typedef interface IClient IClient;
#endif 	/* __IClient_FWD_DEFINED__ */


#ifndef ___IClientEvents_FWD_DEFINED__
#define ___IClientEvents_FWD_DEFINED__
typedef interface _IClientEvents _IClientEvents;
#endif 	/* ___IClientEvents_FWD_DEFINED__ */


#ifndef __Client_FWD_DEFINED__
#define __Client_FWD_DEFINED__

#ifdef __cplusplus
typedef class Client Client;
#else
typedef struct Client Client;
#endif /* __cplusplus */

#endif 	/* __Client_FWD_DEFINED__ */


#ifndef ___IDataPointEvents_FWD_DEFINED__
#define ___IDataPointEvents_FWD_DEFINED__
typedef interface _IDataPointEvents _IDataPointEvents;
#endif 	/* ___IDataPointEvents_FWD_DEFINED__ */


#ifndef ___IDataPointEventsEx_FWD_DEFINED__
#define ___IDataPointEventsEx_FWD_DEFINED__
typedef interface _IDataPointEventsEx _IDataPointEventsEx;
#endif 	/* ___IDataPointEventsEx_FWD_DEFINED__ */


#ifndef ___IDataPointEvents3_FWD_DEFINED__
#define ___IDataPointEvents3_FWD_DEFINED__
typedef interface _IDataPointEvents3 _IDataPointEvents3;
#endif 	/* ___IDataPointEvents3_FWD_DEFINED__ */


#ifndef __DataPoint_FWD_DEFINED__
#define __DataPoint_FWD_DEFINED__

#ifdef __cplusplus
typedef class DataPoint DataPoint;
#else
typedef struct DataPoint DataPoint;
#endif /* __cplusplus */

#endif 	/* __DataPoint_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IDataPoint_INTERFACE_DEFINED__
#define __IDataPoint_INTERFACE_DEFINED__

/* interface IDataPoint */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IDataPoint;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("17F796C8-9B9B-4268-AEA3-B3BC5DE84E76")
    IDataPoint : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Value( 
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Value( 
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Time( 
            /* [retval][out] */ DATE *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Quality( 
            /* [retval][out] */ ULONG *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ValueStr( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Attrib( 
            /* [in] */ BSTR Name,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Address( 
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Prop( 
            /* [in] */ ULONG ID,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propput] */ HRESULT STDMETHODCALLTYPE put_Prop( 
            /* [in] */ ULONG ID,
            /* [in] */ VARIANT newVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XmlNode( 
            /* [retval][out] */ IDispatch **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_Connected( 
            /* [retval][out] */ ULONG *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_ErrorStr( 
            /* [in] */ ULONG Error,
            /* [retval][out] */ BSTR *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_AccessRights( 
            /* [retval][out] */ ULONG *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Write( 
            /* [in] */ VARIANT Value) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE WriteAsync( 
            /* [in] */ VARIANT Value,
            /* [retval][out] */ ULONG *TransID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CancelAsync( 
            /* [in] */ ULONG TransID) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Ack( 
            /* [in] */ BSTR Acker,
            /* [in] */ BSTR Comment) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataPointVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDataPoint * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDataPoint * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDataPoint * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDataPoint * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDataPoint * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDataPoint * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDataPoint * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Value )( 
            IDataPoint * This,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Value )( 
            IDataPoint * This,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Time )( 
            IDataPoint * This,
            /* [retval][out] */ DATE *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Quality )( 
            IDataPoint * This,
            /* [retval][out] */ ULONG *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ValueStr )( 
            IDataPoint * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Attrib )( 
            IDataPoint * This,
            /* [in] */ BSTR Name,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Address )( 
            IDataPoint * This,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Prop )( 
            IDataPoint * This,
            /* [in] */ ULONG ID,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Prop )( 
            IDataPoint * This,
            /* [in] */ ULONG ID,
            /* [in] */ VARIANT newVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XmlNode )( 
            IDataPoint * This,
            /* [retval][out] */ IDispatch **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Connected )( 
            IDataPoint * This,
            /* [retval][out] */ ULONG *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_ErrorStr )( 
            IDataPoint * This,
            /* [in] */ ULONG Error,
            /* [retval][out] */ BSTR *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_AccessRights )( 
            IDataPoint * This,
            /* [retval][out] */ ULONG *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Write )( 
            IDataPoint * This,
            /* [in] */ VARIANT Value);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *WriteAsync )( 
            IDataPoint * This,
            /* [in] */ VARIANT Value,
            /* [retval][out] */ ULONG *TransID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *CancelAsync )( 
            IDataPoint * This,
            /* [in] */ ULONG TransID);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Ack )( 
            IDataPoint * This,
            /* [in] */ BSTR Acker,
            /* [in] */ BSTR Comment);
        
        END_INTERFACE
    } IDataPointVtbl;

    interface IDataPoint
    {
        CONST_VTBL struct IDataPointVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataPoint_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDataPoint_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDataPoint_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDataPoint_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IDataPoint_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IDataPoint_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IDataPoint_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IDataPoint_get_Value(This,pVal)	\
    ( (This)->lpVtbl -> get_Value(This,pVal) ) 

#define IDataPoint_put_Value(This,newVal)	\
    ( (This)->lpVtbl -> put_Value(This,newVal) ) 

#define IDataPoint_get_Time(This,pVal)	\
    ( (This)->lpVtbl -> get_Time(This,pVal) ) 

#define IDataPoint_get_Quality(This,pVal)	\
    ( (This)->lpVtbl -> get_Quality(This,pVal) ) 

#define IDataPoint_get_ValueStr(This,pVal)	\
    ( (This)->lpVtbl -> get_ValueStr(This,pVal) ) 

#define IDataPoint_get_Attrib(This,Name,pVal)	\
    ( (This)->lpVtbl -> get_Attrib(This,Name,pVal) ) 

#define IDataPoint_get_Address(This,pVal)	\
    ( (This)->lpVtbl -> get_Address(This,pVal) ) 

#define IDataPoint_get_Prop(This,ID,pVal)	\
    ( (This)->lpVtbl -> get_Prop(This,ID,pVal) ) 

#define IDataPoint_put_Prop(This,ID,newVal)	\
    ( (This)->lpVtbl -> put_Prop(This,ID,newVal) ) 

#define IDataPoint_get_XmlNode(This,pVal)	\
    ( (This)->lpVtbl -> get_XmlNode(This,pVal) ) 

#define IDataPoint_get_Connected(This,pVal)	\
    ( (This)->lpVtbl -> get_Connected(This,pVal) ) 

#define IDataPoint_get_ErrorStr(This,Error,pVal)	\
    ( (This)->lpVtbl -> get_ErrorStr(This,Error,pVal) ) 

#define IDataPoint_get_AccessRights(This,pVal)	\
    ( (This)->lpVtbl -> get_AccessRights(This,pVal) ) 

#define IDataPoint_Write(This,Value)	\
    ( (This)->lpVtbl -> Write(This,Value) ) 

#define IDataPoint_WriteAsync(This,Value,TransID)	\
    ( (This)->lpVtbl -> WriteAsync(This,Value,TransID) ) 

#define IDataPoint_CancelAsync(This,TransID)	\
    ( (This)->lpVtbl -> CancelAsync(This,TransID) ) 

#define IDataPoint_Ack(This,Acker,Comment)	\
    ( (This)->lpVtbl -> Ack(This,Acker,Comment) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDataPoint_INTERFACE_DEFINED__ */


#ifndef __IDataPointServer_INTERFACE_DEFINED__
#define __IDataPointServer_INTERFACE_DEFINED__

/* interface IDataPointServer */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IDataPointServer;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1927D656-95BA-4c9a-AA5D-C0D919CBF6AC")
    IDataPointServer : public IDispatch
    {
    public:
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_OPCServer( 
            /* [retval][out] */ IUnknown **pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataPointServerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDataPointServer * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDataPointServer * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDataPointServer * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDataPointServer * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDataPointServer * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDataPointServer * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDataPointServer * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OPCServer )( 
            IDataPointServer * This,
            /* [retval][out] */ IUnknown **pVal);
        
        END_INTERFACE
    } IDataPointServerVtbl;

    interface IDataPointServer
    {
        CONST_VTBL struct IDataPointServerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataPointServer_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDataPointServer_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDataPointServer_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDataPointServer_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IDataPointServer_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IDataPointServer_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IDataPointServer_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IDataPointServer_get_OPCServer(This,pVal)	\
    ( (This)->lpVtbl -> get_OPCServer(This,pVal) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDataPointServer_INTERFACE_DEFINED__ */


#ifndef __IDataPoint3_INTERFACE_DEFINED__
#define __IDataPoint3_INTERFACE_DEFINED__

/* interface IDataPoint3 */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IDataPoint3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F89D7DF2-568A-4F7E-8245-C693B8841877")
    IDataPoint3 : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Call( 
            /* [in] */ BSTR MethodName,
            /* [in] */ ULONG ArgumentCount,
            /* [size_is][in] */ VARIANT *Arguments,
            /* [retval][out] */ VARIANT *Result) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IDataPoint3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IDataPoint3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IDataPoint3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IDataPoint3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IDataPoint3 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IDataPoint3 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IDataPoint3 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IDataPoint3 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Call )( 
            IDataPoint3 * This,
            /* [in] */ BSTR MethodName,
            /* [in] */ ULONG ArgumentCount,
            /* [size_is][in] */ VARIANT *Arguments,
            /* [retval][out] */ VARIANT *Result);
        
        END_INTERFACE
    } IDataPoint3Vtbl;

    interface IDataPoint3
    {
        CONST_VTBL struct IDataPoint3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IDataPoint3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IDataPoint3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IDataPoint3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IDataPoint3_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IDataPoint3_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IDataPoint3_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IDataPoint3_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IDataPoint3_Call(This,MethodName,ArgumentCount,Arguments,Result)	\
    ( (This)->lpVtbl -> Call(This,MethodName,ArgumentCount,Arguments,Result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IDataPoint3_INTERFACE_DEFINED__ */


#ifndef __IClient_INTERFACE_DEFINED__
#define __IClient_INTERFACE_DEFINED__

/* interface IClient */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IClient;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("550FEB52-C674-4EC3-9585-13565D879C38")
    IClient : public IDispatch
    {
    public:
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE RequestPoint( 
            /* [in] */ BSTR Address,
            /* [retval][out] */ IDataPoint **Point) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_GlobAttrib( 
            /* [in] */ BSTR Name,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XmlConfig( 
            /* [in] */ BSTR Name,
            /* [retval][out] */ IDispatch **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_XmlNode( 
            /* [in] */ BSTR Name,
            /* [in] */ ULONG ID,
            /* [retval][out] */ IDispatch **pVal) = 0;
        
        virtual /* [helpstring][id][propget] */ HRESULT STDMETHODCALLTYPE get_PointProp( 
            /* [in] */ BSTR Name,
            /* [in] */ ULONG ID,
            /* [retval][out] */ VARIANT *pVal) = 0;
        
        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Evalute( 
            /* [in] */ BSTR Text,
            /* [retval][out] */ BSTR *pVal) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IClientVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IClient * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IClient * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IClient * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IClient * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IClient * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IClient * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IClient * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *RequestPoint )( 
            IClient * This,
            /* [in] */ BSTR Address,
            /* [retval][out] */ IDataPoint **Point);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_GlobAttrib )( 
            IClient * This,
            /* [in] */ BSTR Name,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XmlConfig )( 
            IClient * This,
            /* [in] */ BSTR Name,
            /* [retval][out] */ IDispatch **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_XmlNode )( 
            IClient * This,
            /* [in] */ BSTR Name,
            /* [in] */ ULONG ID,
            /* [retval][out] */ IDispatch **pVal);
        
        /* [helpstring][id][propget] */ HRESULT ( STDMETHODCALLTYPE *get_PointProp )( 
            IClient * This,
            /* [in] */ BSTR Name,
            /* [in] */ ULONG ID,
            /* [retval][out] */ VARIANT *pVal);
        
        /* [helpstring][id] */ HRESULT ( STDMETHODCALLTYPE *Evalute )( 
            IClient * This,
            /* [in] */ BSTR Text,
            /* [retval][out] */ BSTR *pVal);
        
        END_INTERFACE
    } IClientVtbl;

    interface IClient
    {
        CONST_VTBL struct IClientVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IClient_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IClient_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IClient_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IClient_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IClient_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IClient_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IClient_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IClient_RequestPoint(This,Address,Point)	\
    ( (This)->lpVtbl -> RequestPoint(This,Address,Point) ) 

#define IClient_get_GlobAttrib(This,Name,pVal)	\
    ( (This)->lpVtbl -> get_GlobAttrib(This,Name,pVal) ) 

#define IClient_get_XmlConfig(This,Name,pVal)	\
    ( (This)->lpVtbl -> get_XmlConfig(This,Name,pVal) ) 

#define IClient_get_XmlNode(This,Name,ID,pVal)	\
    ( (This)->lpVtbl -> get_XmlNode(This,Name,ID,pVal) ) 

#define IClient_get_PointProp(This,Name,ID,pVal)	\
    ( (This)->lpVtbl -> get_PointProp(This,Name,ID,pVal) ) 

#define IClient_Evalute(This,Text,pVal)	\
    ( (This)->lpVtbl -> Evalute(This,Text,pVal) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IClient_INTERFACE_DEFINED__ */



#ifndef __TeleClientLib_LIBRARY_DEFINED__
#define __TeleClientLib_LIBRARY_DEFINED__

/* library TeleClientLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_TeleClientLib;

#ifndef ___IClientEvents_DISPINTERFACE_DEFINED__
#define ___IClientEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IClientEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IClientEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("9A9362AC-790E-43FE-9B6A-E0E30DED9C0C")
    _IClientEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IClientEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IClientEvents * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IClientEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IClientEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IClientEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IClientEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IClientEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IClientEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IClientEventsVtbl;

    interface _IClientEvents
    {
        CONST_VTBL struct _IClientEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IClientEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IClientEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IClientEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IClientEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IClientEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IClientEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IClientEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IClientEvents_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_Client;

#ifdef __cplusplus

class DECLSPEC_UUID("CF94C1E4-54B7-4E76-B2FB-8D47F3CED2F8")
Client;
#endif

#ifndef ___IDataPointEvents_DISPINTERFACE_DEFINED__
#define ___IDataPointEvents_DISPINTERFACE_DEFINED__

/* dispinterface _IDataPointEvents */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IDataPointEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("5BA438EE-1FF8-4532-AFCD-2D9605191BD4")
    _IDataPointEvents : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IDataPointEventsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IDataPointEvents * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IDataPointEvents * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IDataPointEvents * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IDataPointEvents * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IDataPointEvents * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IDataPointEvents * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IDataPointEvents * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IDataPointEventsVtbl;

    interface _IDataPointEvents
    {
        CONST_VTBL struct _IDataPointEventsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IDataPointEvents_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IDataPointEvents_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IDataPointEvents_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IDataPointEvents_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IDataPointEvents_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IDataPointEvents_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IDataPointEvents_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IDataPointEvents_DISPINTERFACE_DEFINED__ */


#ifndef ___IDataPointEventsEx_DISPINTERFACE_DEFINED__
#define ___IDataPointEventsEx_DISPINTERFACE_DEFINED__

/* dispinterface _IDataPointEventsEx */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IDataPointEventsEx;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("04F3B184-510C-416e-915D-BBA6461CC349")
    _IDataPointEventsEx : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IDataPointEventsExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IDataPointEventsEx * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IDataPointEventsEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IDataPointEventsEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IDataPointEventsEx * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IDataPointEventsEx * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IDataPointEventsEx * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IDataPointEventsEx * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IDataPointEventsExVtbl;

    interface _IDataPointEventsEx
    {
        CONST_VTBL struct _IDataPointEventsExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IDataPointEventsEx_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IDataPointEventsEx_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IDataPointEventsEx_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IDataPointEventsEx_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IDataPointEventsEx_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IDataPointEventsEx_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IDataPointEventsEx_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IDataPointEventsEx_DISPINTERFACE_DEFINED__ */


#ifndef ___IDataPointEvents3_DISPINTERFACE_DEFINED__
#define ___IDataPointEvents3_DISPINTERFACE_DEFINED__

/* dispinterface _IDataPointEvents3 */
/* [helpstring][uuid] */ 


EXTERN_C const IID DIID__IDataPointEvents3;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("2E6A69BE-7360-4565-BE8C-ED6C38CDFBB2")
    _IDataPointEvents3 : public IDispatch
    {
    };
    
#else 	/* C style interface */

    typedef struct _IDataPointEvents3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            _IDataPointEvents3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            _IDataPointEvents3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            _IDataPointEvents3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            _IDataPointEvents3 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            _IDataPointEvents3 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            _IDataPointEvents3 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            _IDataPointEvents3 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } _IDataPointEvents3Vtbl;

    interface _IDataPointEvents3
    {
        CONST_VTBL struct _IDataPointEvents3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define _IDataPointEvents3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define _IDataPointEvents3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define _IDataPointEvents3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define _IDataPointEvents3_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define _IDataPointEvents3_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define _IDataPointEvents3_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define _IDataPointEvents3_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */


#endif 	/* ___IDataPointEvents3_DISPINTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_DataPoint;

#ifdef __cplusplus

class DECLSPEC_UUID("BE21D8F7-FAE1-4024-AC00-A42EC4D02F00")
DataPoint;
#endif
#endif /* __TeleClientLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


