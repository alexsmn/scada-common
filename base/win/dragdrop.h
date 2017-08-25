#pragma once

#include <oleidl.h>

//class DragDrop : public IDataObject, public IDropSource, public IDropTarget
//{
//public:
//	// IUnknown
//	STDMETHOD(QueryInterface)(/*[in]*/ REFIID riid, void** ppvObject)
//	{
//		if (riid == IID_IDropSource) {
//			*ppvObject = (IDropSource*)this;
//			return S_OK;
//		} else {
//			*ppvObject = NULL;
//			return E_NOINTERFACE;
//		}
//	}
//
//	STDMETHOD_(ULONG, AddRef)() { return -1; }
//	STDMETHOD_(ULONG, Release)() { return -1; }
//
//	// IDataObject
//	STDMETHOD(GetData)(/* [unique][in] */ FORMATETC *pformatetcIn, /* [out] */ STGMEDIUM *pmedium)
//	{
//		return E_NOTIMPL;
//	}
//
//	STDMETHOD(GetDataHere)(/* [unique][in] */ FORMATETC *pformatetc, /* [out][in] */ STGMEDIUM *pmedium)
//	{
//		return E_NOTIMPL;
//	}
//
//	STDMETHOD(QueryGetData)(/* [unique][in] */ FORMATETC *pformatetc)
//	{
//		return E_NOTIMPL;
//	}
//
//	STDMETHOD(GetCanonicalFormatEtc)(/* [unique][in] */ FORMATETC *pformatectIn,
//		/* [out] */ FORMATETC *pformatetcOut)
//	{
//		return E_NOTIMPL;
//	}
//
//	STDMETHOD(SetData)(/* [unique][in] */ FORMATETC *pformatetc, /* [unique][in] */ STGMEDIUM *pmedium,
//		/* [in] */ BOOL fRelease)
//	{
//		return E_NOTIMPL;
//	}
//
//	STDMETHOD(EnumFormatEtc)(/* [in] */ DWORD dwDirection, /* [out] */ IEnumFORMATETC **ppenumFormatEtc)
//	{
//		return E_NOTIMPL;
//	}
//	
//	STDMETHOD(DAdvise)(/* [in] */ FORMATETC *pformatetc, /* [in] */ DWORD advf,
//		/* [unique][in] */ IAdviseSink *pAdvSink, /* [out] */ DWORD *pdwConnection)
//	{
//		return E_NOTIMPL;
//	}
//	
//	STDMETHOD(DUnadvise)(/* [in] */ DWORD dwConnection)
//	{
//		return E_NOTIMPL;
//	}
//
//	STDMETHOD(EnumDAdvise)(/* [out] */ IEnumSTATDATA **ppenumAdvise)
//	{
//		return E_NOTIMPL;
//	}
//
//	// IDropSource
//	STDMETHOD(QueryContinueDrag)(/*[in]*/ BOOL fEscapePressed, /*[in]*/ DWORD grfKeyState)
//	{
//		// DRAGDROP_S_CANCEL if the ESC key or right button is pressed,
//		// or left button is raised before dragging starts.
//		// DRAGDROP_S_DROP if a drop operation should occur. Otherwise S_OK.
//		if (fEscapePressed)
//			return DRAGDROP_S_CANCEL;
//		return S_OK;
//	}
//
//	STDMETHOD(GiveFeedback)(/*[in]*/ DWORD dwEffect)
//	{
//		// dwEffect:
//		// DROPEFFECT_NONE   A drop would not be allowed.
//		// DROPEFFECT_COPY   A copy operation would be performed.
//		// DROPEFFECT_MOVE   A move operation would be performed.
//		// DROPEFFECT_LINK   A link from the dropped data to the original data would be established.
//		// DROPEFFECT_SCROLL   A drag scroll operation is about to occur or is occurring in the target.
//
//		// result:
//		// S_OK
//		// 	The method completed its task successfully, using the cursor set by the source application. 
//		// DRAGDROP_S_USEDEFAULTCURSORS
//		// 	Indicates successful completion of the method, and requests OLE to update the cursor using the OLE-provided default cursors. 
//
//		return DRAGDROP_S_USEDEFAULTCURSORS;
//	}
//
//	// IDropTarget
//	virtual HRESULT STDMETHODCALLTYPE DragEnter(/* [unique][in] */ IDataObject *pDataObj,
//		/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt, /* [out][in] */ DWORD *pdwEffect)
//	{
//		*pdwEffect = DROPEFFECT_NONE;
//		return S_OK;
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE DragOver(/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt,
//		/* [out][in] */ DWORD *pdwEffect)
//	{
//		*pdwEffect = DROPEFFECT_NONE;
//		return S_OK;
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE DragLeave(void)
//	{
//		return S_OK;
//	}
//
//	virtual HRESULT STDMETHODCALLTYPE Drop(/* [unique][in] */ IDataObject *pDataObj,
//		/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt, /* [out][in] */ DWORD *pdwEffect)
//	{
//		return S_OK;
//	}
//};

class DropSource : public IDropSource
{
public:
	// IDropSource
	STDMETHOD(QueryContinueDrag)(/*[in]*/ BOOL fEscapePressed, /*[in]*/ DWORD grfKeyState)
	{
		// DRAGDROP_S_CANCEL if the ESC key or right button is pressed,
		// or left button is raised before dragging starts.
		// DRAGDROP_S_DROP if a drop operation should occur. Otherwise S_OK.
		if (fEscapePressed || (grfKeyState&MK_RBUTTON))
			return DRAGDROP_S_CANCEL;
		if (!(grfKeyState&MK_LBUTTON))
			return DRAGDROP_S_DROP;
		return S_OK;
	}

	STDMETHOD(GiveFeedback)(/*[in]*/ DWORD dwEffect)
	{
		// dwEffect:
		// DROPEFFECT_NONE   A drop would not be allowed.
		// DROPEFFECT_COPY   A copy operation would be performed.
		// DROPEFFECT_MOVE   A move operation would be performed.
		// DROPEFFECT_LINK   A link from the dropped data to the original data would be established.
		// DROPEFFECT_SCROLL   A drag scroll operation is about to occur or is occurring in the target.

		// result:
		// S_OK
		// 	The method completed its task successfully, using the cursor set by the source application. 
		// DRAGDROP_S_USEDEFAULTCURSORS
		// 	Indicates successful completion of the method, and requests OLE to update the cursor using the OLE-provided default cursors. 

		return DRAGDROP_S_USEDEFAULTCURSORS;
	}

	// IUnknown
	STDMETHOD(QueryInterface)(/*[in]*/ REFIID riid, void** ppvObject)
	{
		if (riid == IID_IUnknown) {
			*ppvObject = (IUnknown*)this;
			return S_OK;
		} else if (riid == IID_IDropSource) {
			*ppvObject = (IDropSource*)this;
			return S_OK;
		} else {
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	STDMETHOD_(ULONG, AddRef)() { return static_cast<ULONG>(-1); }
	STDMETHOD_(ULONG, Release)() { return static_cast<ULONG>(-1); }
};

class DataObject : public IDataObject
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(/*[in]*/ REFIID riid, void** ppvObject)
	{
		if (riid == IID_IUnknown) {
			*ppvObject = (IUnknown*)this;
			return S_OK;
		} else if (riid == IID_IDataObject) {
			*ppvObject = (IDataObject*)this;
			return S_OK;
		} else {
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	STDMETHOD_(ULONG, AddRef)() { return static_cast<ULONG>(-1); }
	STDMETHOD_(ULONG, Release)() { return static_cast<ULONG>(-1); }

	// IDataObject
	STDMETHOD(GetData)(/* [unique][in] */ FORMATETC *pformatetcIn, /* [out] */ STGMEDIUM *pmedium)
	{
		return DATA_E_FORMATETC;
	}

	STDMETHOD(GetDataHere)(/* [unique][in] */ FORMATETC *pformatetc, /* [out][in] */ STGMEDIUM *pmedium)
	{
		return DATA_E_FORMATETC;
	}

	STDMETHOD(QueryGetData)(/* [unique][in] */ FORMATETC *pformatetc)
	{
		return DATA_E_FORMATETC;
	}

	STDMETHOD(GetCanonicalFormatEtc)(/* [unique][in] */ FORMATETC *pformatectIn,
		/* [out] */ FORMATETC *pformatetcOut)
	{
		return DATA_E_FORMATETC;
	}

	STDMETHOD(SetData)(/* [unique][in] */ FORMATETC *pformatetc, /* [unique][in] */ STGMEDIUM *pmedium,
		/* [in] */ BOOL fRelease)
	{
		return DATA_E_FORMATETC;
	}

	STDMETHOD(EnumFormatEtc)(/* [in] */ DWORD dwDirection, /* [out] */ IEnumFORMATETC **ppenumFormatEtc)
	{
		return E_NOTIMPL;
	}
	
	STDMETHOD(DAdvise)(/* [in] */ FORMATETC *pformatetc, /* [in] */ DWORD advf,
		/* [unique][in] */ IAdviseSink *pAdvSink, /* [out] */ DWORD *pdwConnection)
	{
		return E_NOTIMPL;
	}
	
	STDMETHOD(DUnadvise)(/* [in] */ DWORD dwConnection)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(EnumDAdvise)(/* [out] */ IEnumSTATDATA **ppenumAdvise)
	{
		return E_NOTIMPL;
	}
};

class DropTarget : public IDropTarget
{
public:
	// IUnknown
	STDMETHOD(QueryInterface)(/*[in]*/ REFIID riid, void** ppvObject)
	{
		if (riid == IID_IUnknown) {
			*ppvObject = (IUnknown*)this;
			return S_OK;
		} else if (riid == IID_IDropTarget) {
			*ppvObject = (IDropTarget*)this;
			return S_OK;
		} else {
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
	}

	STDMETHOD_(ULONG, AddRef)() { return static_cast<ULONG>(-1); }
	STDMETHOD_(ULONG, Release)() { return static_cast<ULONG>(-1); }

	// IDropTarget
	STDMETHOD(DragEnter)(/* [unique][in] */ IDataObject *pDataObj, /* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt, /* [out][in] */ DWORD *pdwEffect)
	{
		POINT point = { pt.x, pt.y };
		*pdwEffect = OnDragEnter(pDataObj, grfKeyState, point);
		return S_OK;
	}

	STDMETHOD(DragOver)(/* [in] */ DWORD grfKeyState, /* [in] */ POINTL pt,
		/* [out][in] */ DWORD *pdwEffect)
	{
		POINT point = { pt.x, pt.y };
		*pdwEffect = OnDragOver(grfKeyState, point);
		return S_OK;
	}

	STDMETHOD(DragLeave)(void)
	{
		OnDragLeave();
		return S_OK;
	}

	STDMETHOD(Drop)(/* [unique][in] */ IDataObject *pDataObj, /* [in] */ DWORD grfKeyState,
		/* [in] */ POINTL pt, /* [out][in] */ DWORD *pdwEffect)
	{
		POINT point = { pt.x, pt.y };
		*pdwEffect = OnDrop(pDataObj, *pdwEffect, point);
		return S_OK;
	}

	virtual DWORD OnDragEnter(IDataObject* obj, DWORD keys, POINT point)
	{
		return DROPEFFECT_NONE;
	}

	virtual DWORD OnDragOver(DWORD keys, POINT point)
	{
		return DROPEFFECT_NONE;
	}

	virtual void OnDragLeave()
	{
	}

	virtual DWORD OnDrop(IDataObject* obj, DWORD effect, POINT point)
	{
		return DROPEFFECT_NONE;
	}
};
