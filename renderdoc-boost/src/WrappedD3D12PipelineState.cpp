#include "WrappedD3D12PipelineState.h"
#include "WrappedD3D12RootSignature.h"
RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12PipelineState::WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
	D3D12_COMPUTE_PIPELINE_STATE_DESC& compute_desc, WrappedD3D12RootSignature* pWrappedRootSignature)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_typeDesc(PipelineStateType_Compute, compute_desc),
	m_pWrappedRootSignature(pWrappedRootSignature)
{
	CS = compute_desc.CS;
	if (CS.pShaderBytecode) {
		void * pbc = new byte[CS.BytecodeLength];
		CS.pShaderBytecode = pbc;
		memcpy(pbc, compute_desc.CS.pShaderBytecode, CS.BytecodeLength);
	}
}

WrappedD3D12PipelineState::WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& graphics_desc, WrappedD3D12RootSignature* pWrappedRootSignature)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_typeDesc(PipelineStateType_Graphics, graphics_desc),
	m_pWrappedRootSignature(pWrappedRootSignature) 
{
	VS = graphics_desc.VS;
	if (VS.pShaderBytecode) {
		void * pbc = new byte[VS.BytecodeLength];
		VS.pShaderBytecode = pbc;
		memcpy(pbc, graphics_desc.VS.pShaderBytecode, VS.BytecodeLength);
	}
	PS = graphics_desc.PS;
	if (PS.pShaderBytecode) {
		void * pbc = new byte[PS.BytecodeLength];
		PS.pShaderBytecode = pbc;
		memcpy(pbc, graphics_desc.PS.pShaderBytecode, PS.BytecodeLength);
	}
	DS = graphics_desc.DS;
	if (DS.pShaderBytecode) {
		void * pbc = new byte[DS.BytecodeLength];
		DS.pShaderBytecode = pbc;
		memcpy(pbc, graphics_desc.DS.pShaderBytecode, DS.BytecodeLength);
	}
	HS = graphics_desc.HS;
	if (HS.pShaderBytecode) {
		void * pbc = new byte[HS.BytecodeLength];
		HS.pShaderBytecode = pbc;
		memcpy(pbc, graphics_desc.HS.pShaderBytecode, HS.BytecodeLength);
	}
	GS = graphics_desc.GS;
	if (GS.pShaderBytecode) {
		void * pbc = new byte[GS.BytecodeLength];
		GS.pShaderBytecode = pbc;
		memcpy(pbc, graphics_desc.GS.pShaderBytecode, GS.BytecodeLength);
	}


}

WrappedD3D12PipelineState::~WrappedD3D12PipelineState() {
	if (CS.pShaderBytecode) {
		delete[CS.BytecodeLength](byte*)CS.pShaderBytecode;
	}
	if (VS.pShaderBytecode) {
		delete[VS.BytecodeLength](byte*)VS.pShaderBytecode;
	}
	if (PS.pShaderBytecode) {
		delete[PS.BytecodeLength](byte*)PS.pShaderBytecode;
	}
	if (DS.pShaderBytecode) {
		delete[DS.BytecodeLength](byte*)DS.pShaderBytecode;
	}
	if (HS.pShaderBytecode) {
		delete[HS.BytecodeLength](byte*)HS.pShaderBytecode;
	}
	if (GS.pShaderBytecode) {
		delete[GS.BytecodeLength](byte*)GS.pShaderBytecode;
	}


}

COMPtr<ID3D12DeviceChild> WrappedD3D12PipelineState::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12PipelineState> pvPipelineState;
	if (m_typeDesc.type ==PipelineStateType_Graphics) {
		auto& desc = m_typeDesc.uPipelineStateDesc.graphicsDesc;
		desc.pRootSignature = m_pWrappedRootSignature->GetReal().Get();
		desc.VS = VS;
		desc.PS = PS;
		desc.DS = DS;
		desc.HS = HS;
		desc.GS = GS;
		HRESULT ret = pNewDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pvPipelineState));
		if (FAILED(ret)) {
			LogError("create new D3D12RootSignature failed");
			return NULL;
		}
	}
	else {
		auto& desc = m_typeDesc.uPipelineStateDesc.computeDesc;
		desc.pRootSignature = m_pWrappedRootSignature->GetReal().Get();
		desc.CS = CS;
		HRESULT ret = pNewDevice->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pvPipelineState));
		if (FAILED(ret)) {
			LogError("create new D3D12RootSignature failed");
			return NULL;
		}
	}
	
	return pvPipelineState;
}

/************************************************************************/
/* override                                                                                             */
/************************************************************************/
HRESULT STDMETHODCALLTYPE WrappedD3D12PipelineState::GetCachedBlob(
	_COM_Outptr_  ID3DBlob **ppBlob) {
	return GetReal()->GetCachedBlob(ppBlob);
}

RDCBOOST_NAMESPACE_END