#include "WrappedD3D12PipelineState.h"
#include "WrappedD3D12RootSignature.h"
RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12PipelineState::WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
	D3D12_COMPUTE_PIPELINE_STATE_DESC& compute_desc, WrappedD3D12RootSignature* pWrappedRootSignature)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_typeDesc(PipelineStateType_Compute, compute_desc),
	m_pWrappedRootSignature(pWrappedRootSignature)
{
	m_pWrappedRootSignature->AddRef();
}

WrappedD3D12PipelineState::WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& graphics_desc, WrappedD3D12RootSignature* pWrappedRootSignature)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_typeDesc(PipelineStateType_Graphics, graphics_desc),
	m_pWrappedRootSignature(pWrappedRootSignature) 
{
	m_pWrappedRootSignature->AddRef();
}

WrappedD3D12PipelineState::~WrappedD3D12PipelineState() {
	m_pWrappedRootSignature->Release();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12PipelineState::GetCachedBlob(
	_COM_Outptr_  ID3DBlob **ppBlob) {
	return GetReal()->GetCachedBlob(ppBlob);
}

ID3D12DeviceChild* WrappedD3D12PipelineState::CopyToDevice(ID3D12Device* pNewDevice) {
	ID3D12PipelineState * pvPipelineState;
	if (m_typeDesc.type ==PipelineStateType_Graphics) {
		auto& desc = m_typeDesc.uPipelineStateDesc.graphicsDesc;
		desc.pRootSignature = m_pWrappedRootSignature->GetReal();
		HRESULT ret = pNewDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pvPipelineState));
		if (FAILED(ret)) {
			LogError("create new D3D12RootSignature failed");
			return NULL;
		}
	}
	else {
		auto& desc = m_typeDesc.uPipelineStateDesc.computeDesc;
		desc.pRootSignature = m_pWrappedRootSignature->GetReal();
		HRESULT ret = pNewDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pvPipelineState));
		if (FAILED(ret)) {
			LogError("create new D3D12RootSignature failed");
			return NULL;
		}
	}
	
	return pvPipelineState;
}

RDCBOOST_NAMESPACE_END