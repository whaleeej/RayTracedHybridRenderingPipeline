#pragma once
#include <string>
#include "WrappedD3D12Device.h"

RDCBOOST_NAMESPACE_BEGIN

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
		m_pWrappedDevice->OnDeviceChildReleased(static_cast<ID3D12DeviceChild*>(m_pReal));
		m_pWrappedDevice->Release();
	}

public: //override for ID3D12DeviceChild
	virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppvDevice) {
		if (riid == __uuidof(ID3D12Device)) {
			*ppvDevice = static_cast<ID3D12Device*>(m_pWrappedDevice);
			m_pWrappedDevice->AddRef();
			return S_OK;
		}
		//TODO: ID3D12Device1/2/3...
		//else
		//if (riid == __uuidof(ID3D12Device1)) {
		//	*ppvDevice = static_cast<ID3D12Device1*>(m_pWrappedDevice);
		//	m_pWrappedDevice->AddRef();
		//	return S_OK;
		//}
		return S_FALSE;
	}

public: //function
	ID3D12Device* GetRealDevice() { return m_pRealDevice; }

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
		//TODO: need add 1 ref?
		m_pReal->AddRef();
		m_pRealDevice = pNewDevice;
	}

	//used for create the same devicechild with new Device, then the framework will set/substitute the newly created devicechild into the original wrapper and pack its private data
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice) = 0;

protected:
	WrappedD3D12Device* const m_pWrappedDevice; //reffed
	ID3D12Device* m_pRealDevice; // not reffed
};

RDCBOOST_NAMESPACE_END