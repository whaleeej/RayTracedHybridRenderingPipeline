#include "WrappedD3D11DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D11DeviceChildBase::WrappedD3D11DeviceChildBase(
	ID3D11DeviceChild* pReal, WrappedD3D11Device* pDevice) :
	m_pReal(pReal), m_pWrappedDevice(pDevice)
{
	m_pWrappedDevice->AddRef();

	m_pRealDevice = m_pWrappedDevice->GetReal();
	m_pReal->AddRef();
}

WrappedD3D11DeviceChildBase::~WrappedD3D11DeviceChildBase()
{
	m_pWrappedDevice->OnDeviceChildReleased(m_pReal);
	m_pReal->Release();
	m_pWrappedDevice->Release();
}

RDCBOOST_NAMESPACE_END

