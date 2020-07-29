#include "WrappedD3D12Fence.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12Fence::WrappedD3D12Fence(ID3D12Fence* pReal, WrappedD3D12Device* pDevice, UINT64 InitialValue, D3D12_FENCE_FLAGS Flags)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_InitialValue(InitialValue), m_Flags(Flags)
{
	m_AppFenceValue = InitialValue;
	m_MaxFenceValueToWaitFor = InitialValue;
}

WrappedD3D12Fence::~WrappedD3D12Fence() {

}

ID3D12DeviceChild* WrappedD3D12Fence::CopyToDevice(ID3D12Device* pNewDevice) {
	ID3D12Fence * pvFence;
	HRESULT ret = pNewDevice->CreateFence(m_InitialValue, m_Flags, IID_PPV_ARGS(&pvFence));
	if (FAILED(ret)) {
		LogError("create new D3D12Fence failed");
		return NULL;
	}
	return pvFence;
}

/************************************************************************/
/*                        override                                                                     */
/************************************************************************/
UINT64 STDMETHODCALLTYPE WrappedD3D12Fence::GetCompletedValue(void) {
	return GetReal()->GetCompletedValue();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Fence::SetEventOnCompletion(
	UINT64 Value,
	HANDLE hEvent) {
	if (Value > m_MaxFenceValueToWaitFor) {
		m_MaxFenceValueToWaitFor = Value;
	}
	return GetReal()->SetEventOnCompletion(Value, hEvent);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Fence::Signal(
	UINT64 Value) {
	if (Value > m_AppFenceValue) {
		m_AppFenceValue = Value;
	}
	return GetReal()->Signal(Value);
}




RDCBOOST_NAMESPACE_END