#pragma once
#include "RdcBoostPCH.h"
#include "PrivateDataMap.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12ObjectBase {
public:
	WrappedD3D12ObjectBase(ID3D12Object* pReal) : m_pReal(pReal) {}
	virtual ~WrappedD3D12ObjectBase() {}

public: //func
	COMPtr<ID3D12Object> GetRealObject() { return m_pReal; }

public: //framework
	virtual void SwitchToDeviceRdc(ID3D12Device* pNewDevice) = 0;

protected:
	COMPtr<ID3D12Object> m_pReal;
};

template<typename NestedTypeBase, typename NestedTypeHighest>
class WrappedD3D12Object : public WrappedD3D12ObjectBase, public NestedTypeHighest {
public:
	WrappedD3D12Object(NestedTypeBase* pReal) : WrappedD3D12ObjectBase(pReal), m_Ref(1) {};

public: // override for IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject)
	{
		if (riid == __uuidof(NestedTypeBase))
		{
			*ppvObject = static_cast<NestedTypeBase*>(this);
			AddRef();
			return S_OK;
		}
		else if (riid == __uuidof(ID3D12Object))
		{
			*ppvObject = static_cast<ID3D12Object*>(this);
			AddRef();
			return S_OK;
		}
		else if (riid == __uuidof(IUnknown))
		{
			*ppvObject = static_cast<IUnknown*>(this);
			AddRef();
			return S_OK;
		}
		LogError("InValid Query for this Interface");
		return E_NOINTERFACE;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return InterlockedIncrement(&m_Ref);
	}

	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		LONG ret = InterlockedDecrement(&m_Ref);
		if (ret == 0)
			delete this;
		return ret;
	}

public: // override for ID3D12Object
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData)
	{
		return GetReal()->GetPrivateData(guid, pDataSize, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData)
	{
		char* pStrData2 = NULL;
		if (guid == WKPDID_D3DDebugObjectName)
		{ // work-around for renderdoc 
			const char* pStrData = (const char*)pData;
			if (pStrData[DataSize - 1] != '\0')
			{
				pStrData2 = new char[DataSize + 1];
				memcpy(pStrData2, pData, DataSize);
				pStrData2[DataSize] = '\0';

				DataSize = DataSize + 1;
				pData = pStrData2;
			}
		}

		m_PrivateData.SetPrivateData(guid, DataSize, pData);
		HRESULT res = GetReal()->SetPrivateData(guid, DataSize, pData);
		delete[] pStrData2;

		return res;
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData)
	{
		LogWarn("SetPrivateDataInterface may be risky");
		m_PrivateData.SetPrivateDataInterface(guid, pData);
		return GetReal()->SetPrivateDataInterface(guid, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetName(LPCWSTR Name) {
		m_ObjectName = Name;
		return GetReal()->SetName(m_ObjectName.c_str());
	}

public: // func
	COMPtr<NestedTypeBase> GetReal() {
		return COMPtr<NestedTypeBase>(static_cast<NestedTypeBase*>(m_pReal.Get()));
	}

	std::wstring getName() { 
		return m_ObjectName; 
	}

protected:
	ULONG m_Ref;
	std::wstring m_ObjectName;
	PrivateDataMap m_PrivateData;
};

RDCBOOST_NAMESPACE_END