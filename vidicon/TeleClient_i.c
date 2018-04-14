

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IDataPoint,0x17F796C8,0x9B9B,0x4268,0xAE,0xA3,0xB3,0xBC,0x5D,0xE8,0x4E,0x76);


MIDL_DEFINE_GUID(IID, IID_IDataPointServer,0x1927D656,0x95BA,0x4c9a,0xAA,0x5D,0xC0,0xD9,0x19,0xCB,0xF6,0xAC);


MIDL_DEFINE_GUID(IID, IID_IDataPoint3,0xF89D7DF2,0x568A,0x4F7E,0x82,0x45,0xC6,0x93,0xB8,0x84,0x18,0x77);


MIDL_DEFINE_GUID(IID, IID_IClient,0x550FEB52,0xC674,0x4EC3,0x95,0x85,0x13,0x56,0x5D,0x87,0x9C,0x38);


MIDL_DEFINE_GUID(IID, LIBID_TeleClientLib,0x039F6DEF,0x60F4,0x47EF,0x8C,0x30,0x89,0x3A,0x68,0x30,0xF7,0x3E);


MIDL_DEFINE_GUID(IID, DIID__IClientEvents,0x9A9362AC,0x790E,0x43FE,0x9B,0x6A,0xE0,0xE3,0x0D,0xED,0x9C,0x0C);


MIDL_DEFINE_GUID(CLSID, CLSID_Client,0xCF94C1E4,0x54B7,0x4E76,0xB2,0xFB,0x8D,0x47,0xF3,0xCE,0xD2,0xF8);


MIDL_DEFINE_GUID(IID, DIID__IDataPointEvents,0x5BA438EE,0x1FF8,0x4532,0xAF,0xCD,0x2D,0x96,0x05,0x19,0x1B,0xD4);


MIDL_DEFINE_GUID(IID, DIID__IDataPointEventsEx,0x04F3B184,0x510C,0x416e,0x91,0x5D,0xBB,0xA6,0x46,0x1C,0xC3,0x49);


MIDL_DEFINE_GUID(IID, DIID__IDataPointEvents3,0x2E6A69BE,0x7360,0x4565,0xBE,0x8C,0xED,0x6C,0x38,0xCD,0xFB,0xB2);


MIDL_DEFINE_GUID(CLSID, CLSID_DataPoint,0xBE21D8F7,0xFAE1,0x4024,0xAC,0x00,0xA4,0x2E,0xC4,0xD0,0x2F,0x00);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



