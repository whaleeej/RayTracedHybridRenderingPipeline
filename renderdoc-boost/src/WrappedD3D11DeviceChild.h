#pragma once
#include "WrappedD3D11Device.h"
#include "PrivateDataMap.h"

RDCBOOST_NAMESPACE_BEGIN
class WrappedD3D11Device;

class WrappedD3D11DeviceChildBase
{
public:
	WrappedD3D11DeviceChildBase(ID3D11DeviceChild* pReal, WrappedD3D11Device* pDevice);

	virtual ~WrappedD3D11DeviceChildBase();

	virtual void SwitchToDevice(ID3D11Device* pNewDevice) = 0;
		
	ID3D11Device* GetRealDevice() { return m_pRealDevice; }

	ID3D11DeviceChild* GetRealDeviceChild()
	{
		return m_pReal;
	}

protected:
	WrappedD3D11Device* const m_pWrappedDevice;
	ID3D11DeviceChild* m_pReal;

private:
	template <typename T>
	friend class WrappedD3D11DeviceChild;

	ID3D11Device* m_pRealDevice;
};

template <typename NestedType>
class WrappedD3D11DeviceChild : public WrappedD3D11DeviceChildBase, public NestedType
{
public:
	WrappedD3D11DeviceChild(NestedType* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11DeviceChildBase(pReal, pDevice), m_Ref(1)
	{
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject)
	{
		if (riid == __uuidof(NestedType))
		{
			*ppvObject = static_cast<NestedType*>(this);
			AddRef();
			return S_OK;
		}
		else if (riid == __uuidof(ID3D11DeviceChild))
		{
			*ppvObject = static_cast<ID3D11DeviceChild*>(this);
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
			delete this;
		return ret;
	}

	virtual void STDMETHODCALLTYPE GetDevice(ID3D11Device **ppDevice)
	{
		if (ppDevice)
		{
			*ppDevice = m_pWrappedDevice;
			m_pWrappedDevice->AddRef();
		}
	}

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

	NestedType* GetReal() { return static_cast<NestedType*>(m_pReal); }

	NestedType* GetRealOrRDCWrapped(bool rdcWrapped);

	void SwitchToDeviceForSwapChainBuffer(ID3D11Device* pNewDevice, NestedType* pNewBuffer)
	{
		if (m_pRealDevice == pNewDevice)
			return;

		CopyToDeviceForSwapChainBuffer(pNewDevice, pNewBuffer);
		m_PrivateData.CopyPrivateData(pNewBuffer);
		m_pReal->Release();
		m_pReal = pNewBuffer;
		m_pRealDevice = pNewDevice;
	}

	virtual void SwitchToDevice(ID3D11Device* pNewDevice)
	{
		if (m_pRealDevice == pNewDevice)
			return;

		ID3D11DeviceChild* pCopied = CopyToDevice(pNewDevice);
		Assert(pCopied != NULL);
		m_PrivateData.CopyPrivateData(pCopied);
		m_pReal->Release();
		m_pReal = pCopied;
		m_pRealDevice = pNewDevice;
	}

protected:
	virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice) = 0;
	virtual void CopyToDeviceForSwapChainBuffer(ID3D11Device* pNewDevice, NestedType* pNewBuffer) {}

protected:
	unsigned int m_Ref;

private:
	PrivateDataMap m_PrivateData;
};

inline WrappedD3D11DeviceChildBase* ConvertToWrappedBase(ID3D11DeviceChild* pWrapped)
{
	return static_cast<WrappedD3D11DeviceChild<ID3D11DeviceChild>*>(pWrapped);
}

template <typename UnwrapType>
UnwrapType* UnwrapDeviceChild(ID3D11DeviceChild* pWrapped)
{
	if (pWrapped == NULL)
		return NULL;

	WrappedD3D11DeviceChildBase* base = ConvertToWrappedBase(pWrapped);
	return static_cast<UnwrapType*>(base->GetRealDeviceChild());
}

template <typename UnwrapType>
UnwrapType* UnwrapSelf(UnwrapType* pWrapped)
{
	return UnwrapDeviceChild<UnwrapType>(pWrapped);
}
RDCBOOST_NAMESPACE_END

