#pragma once

#include "RdcBoostPCH.h"
#include "PrivateDataMap.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12ObjectBase { //interface for m_pReal:ID3D12Object* and switchToDevice
public:
	WrappedD3D12ObjectBase(ID3D12Object* pReal) : m_pReal(pReal) { m_pReal->AddRef(); }
	virtual ~WrappedD3D12ObjectBase() { m_pReal->Release(); }

public: //virtual
	virtual void SwitchToDevice(ID3D12Device* pNewDevice) = 0;
	
public: //func
	ID3D12Object* GetRealObject() { return m_pReal; }

protected:
	ID3D12Object* m_pReal;
};

template<typename NestedType>
class WrappedD3D12Object: public WrappedD3D12ObjectBase, public NestedType{ // account for wrapped layer for object, also the base of all d3d12Class
public:
	WrappedD3D12Object(NestedType* pReal) : WrappedD3D12ObjectBase(pReal), m_Ref(1) {};

public: // override for IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		if (riid == __uuidof(NestedType))
		{
			*ppvObject = static_cast<NestedType*>(this);
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
		return m_pReal->QueryInterface(riid, ppvObject);
		//*ppvObject = NULL;
		//return E_POINTER;
	}

	virtual ULONG STDMETHODCALLTYPE AddRef(void)
	{
		return InterlockedIncrement(&m_Ref);
	}

	virtual ULONG STDMETHODCALLTYPE Release(void)
	{
		unsigned int ret = InterlockedDecrement(&m_Ref);
		if (ret == 0)
			delete this; // also call the destructor of WrappedD3D12ObjectBase, unref m_pReal
		return ret;
	}

public: // func
	NestedType* GetReal() { return static_cast<NestedType*>(m_pReal); }

public: // override for ID3D12Object
	virtual HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
	{
		return GetReal()->GetPrivateData(guid, pDataSize, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
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

	virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
	{
		LogWarn("SetPrivateDataInterface may be risky");
		m_PrivateData.SetPrivateDataInterface(guid, pData);
		return GetReal()->SetPrivateDataInterface(guid, pData);
	}

	virtual HRESULT STDMETHODCALLTYPE SetName(LPCWSTR Name) {
		m_ObjectName = Name;
		return GetReal()->SetName(m_ObjectName.c_str());
	}

protected:
	unsigned int m_Ref;
	std::wstring m_ObjectName;
	PrivateDataMap m_PrivateData;
};

class WrappedD3D12Device; // only need for ptr, avoid cicular include

template<typename NestedType>
class WrappedD3D12DeviceChild : public WrappedD3D12Object<NestedType> { //wrapped for ID3D12DeviceChild, m_pDevice and m_pWrappedDevice cache
public:
	WrappedD3D12DeviceChild(NestedType* pReal, WrappedD3D12Device* pDevice)
		:WrappedD3D12Object(pReal), m_pWrappedDevice(pDevice)
	{
		m_pWrappedDevice->AddRef();
		m_pRealDevice = m_pWrappedDevice->GetReal();
	};
	virtual ~WrappedD3D12DeviceChild()
	{
		m_pWrappedDevice->OnDeviceChildReleased(static_cast<ID3D12DeviceChild*>m_pReal);
	}

public: //function
	ID3D12Device* GetRealDevice() { return m_pRealDevice; }

public: //override for ID3D12DeviceChild
	virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppvDevice) {
		if (riid == __uuidof(ID3D12Device)) {
			*ppvDevice = static_cast<ID3D12Device*>m_pWrappedDevice;
			m_pWrappedDevice->AddRef();
			return S_OK;
		}
		//TODO: ID3D12Device1/2/3...
		return S_FALSE;
	}

public: //framework
	virtual void SwitchToDevice(ID3D12Device* pNewDevice)
	{
		if (m_pRealDevice == pNewDevice)
			return;

		ID3D12DeviceChild* pCopied = CopyToDevice(pNewDevice);
		Assert(pCopied != NULL);
		m_PrivateData.CopyPrivateData(pCopied);
		m_pReal->Release();
		m_pReal = pCopied;
		m_pRealDevice = pNewDevice;
	}

	//used for create the same devicechild with new Device, then the framework will set/substitute the newly created devicechild into the original wrapper and pack its private data
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice) = 0;

protected:
	WrappedD3D12Device* const m_pWrappedDevice;
	ID3D12Device* m_pRealDevice;
};

RDCBOOST_NAMESPACE_END