#pragma once
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Fence : public WrappedD3D12DeviceChild<ID3D12Fence> {
public:
	WrappedD3D12Fence(ID3D12Fence* pReal, WrappedD3D12Device* pDevice, UINT64 InitialValue, D3D12_FENCE_FLAGS Flags);
	~WrappedD3D12Fence();

public://override
	virtual UINT64 STDMETHODCALLTYPE GetCompletedValue(void);

	virtual HRESULT STDMETHODCALLTYPE SetEventOnCompletion(
		UINT64 Value,
		HANDLE hEvent);

	virtual HRESULT STDMETHODCALLTYPE Signal(
		UINT64 Value);

public://function
	void WaitToCompleteApplicationFenceValue() {
		if (GetCompletedValue() < m_AppFenceValue) {
			auto event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
			Assert(event && "Failed to create fence event handle.");

			// Is this function thread safe?
			this->SetEventOnCompletion(m_AppFenceValue, event);
			::WaitForSingleObject(event, DWORD_MAX);

			::CloseHandle(event);
		}
	}

public://framewokr
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	UINT64 m_InitialValue;
	D3D12_FENCE_FLAGS m_Flags;
	UINT64 m_AppFenceValue;
	UINT64 m_MaxFenceValueToWaitFor;
};

RDCBOOST_NAMESPACE_END