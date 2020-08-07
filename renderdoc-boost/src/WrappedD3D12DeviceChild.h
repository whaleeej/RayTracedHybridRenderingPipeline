#pragma once
#include "WrappedD3D12Device.h"

RDCBOOST_NAMESPACE_BEGIN

template<typename NestedTypeBase, typename NestedTypeHighest>
class WrappedD3D12DeviceChild : public WrappedD3D12Object<NestedTypeBase, NestedTypeHighest> {
public:
	WrappedD3D12DeviceChild(NestedTypeBase* pReal, WrappedD3D12Device* pDevice, int highest = 0)
		:WrappedD3D12Object(pReal, highest), m_pWrappedDevice(pDevice)
	{
		m_pRealDevice = m_pWrappedDevice->GetReal().Get();
	};
	virtual ~WrappedD3D12DeviceChild()
	{
		m_pWrappedDevice->OnDeviceChildReleased(static_cast<ID3D12DeviceChild*>(m_pReal.Get()));
	}

public: //override for ID3D12Object
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) {
		//if (riid == __uuidof(ID3D12Pageable))
		//{
		//	*ppvObject = static_cast<ID3D12Pageable*>(this);
		//	AddRef();
		//	return S_OK;
		//}
		return WrappedD3D12Object::QueryInterface(riid, ppvObject);
	}

public: //override for ID3D12DeviceChild
	virtual HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void **ppvDevice) {
		if (riid == __uuidof(ID3D12Device)) {
			*ppvDevice = static_cast<ID3D12Device*>(m_pWrappedDevice.Get());
			m_pWrappedDevice->AddRef();
			return S_OK;
		}
		return S_FALSE;
	}

public: //function
	COMPtr<WrappedD3D12Device> GetWrappedDevice() { return m_pWrappedDevice; }

public: //framework
	virtual void SwitchToDeviceRdc(ID3D12Device* pNewDevice)
	{
		if (m_pRealDevice == pNewDevice)
			return;

		COMPtr<ID3D12DeviceChild> pCopied = CopyToDevice(pNewDevice);
		Assert(pCopied.Get() != NULL);
		m_PrivateData.CopyPrivateData(pCopied.Get());
		m_pReal = pCopied;
		pCopied->SetName(m_ObjectName.c_str());
		m_pRealDevice = pNewDevice;//根据框架, 这里WrappedDevice没有变，只是内部的pReal变了

	}

	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice) = 0;

protected:
	COMPtr<WrappedD3D12Device> const m_pWrappedDevice;
	ID3D12Device* m_pRealDevice;
};

RDCBOOST_NAMESPACE_END