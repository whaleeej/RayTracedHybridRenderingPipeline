#include "WrappedD3D12QueryHeap.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12QueryHeap::WrappedD3D12QueryHeap(ID3D12QueryHeap* pReal, WrappedD3D12Device* pDevice,
	const D3D12_QUERY_HEAP_DESC *pDesc):
	WrappedD3D12DeviceChild(pReal, pDevice, 0), m_Desc(*pDesc)
{

}

WrappedD3D12QueryHeap::~WrappedD3D12QueryHeap() {

}

COMPtr<ID3D12DeviceChild> WrappedD3D12QueryHeap::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12QueryHeap> pvQueryHeap;
	HRESULT ret = pNewDevice->CreateQueryHeap(
		&m_Desc, IID_PPV_ARGS(&pvQueryHeap));
	if (FAILED(ret)) {
		LogError("create new D3D12QueryHeap failed");
		return NULL;
	}
	return pvQueryHeap;
}

RDCBOOST_NAMESPACE_END