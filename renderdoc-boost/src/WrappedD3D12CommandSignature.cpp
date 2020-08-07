#include "WrappedD3D12CommandSignature.h"
#include "WrappedD3D12RootSignature.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12CommandSignature::WrappedD3D12CommandSignature(ID3D12CommandSignature * pReal, WrappedD3D12Device * pDevice, const D3D12_COMMAND_SIGNATURE_DESC * pDesc, WrappedD3D12RootSignature * pWrappedRootSignature):
	WrappedD3D12DeviceChild(pReal, pDevice, 0), m_pWrappedRootSignature(pWrappedRootSignature)
{
	m_Desc = *pDesc;
	m_Desc.pArgumentDescs = new D3D12_INDIRECT_ARGUMENT_DESC[m_Desc.NumArgumentDescs];
	memcpy((void*)m_Desc.pArgumentDescs, 
		(void*)pDesc->pArgumentDescs, 
		sizeof(D3D12_INDIRECT_ARGUMENT_DESC)*pDesc->NumArgumentDescs);
}

WrappedD3D12CommandSignature::~WrappedD3D12CommandSignature(){
	delete[]m_Desc.pArgumentDescs;
}

COMPtr<ID3D12DeviceChild> WrappedD3D12CommandSignature::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12CommandSignature> pvRootSignature;
	HRESULT ret = pNewDevice->CreateCommandSignature(
		&m_Desc, m_pWrappedRootSignature->GetReal().Get(), 
		IID_PPV_ARGS(&pvRootSignature));
	if (FAILED(ret)) {
		LogError("create new D3D12CommandSignature failed");
		return NULL;
	}
	return pvRootSignature;
}


RDCBOOST_NAMESPACE_END