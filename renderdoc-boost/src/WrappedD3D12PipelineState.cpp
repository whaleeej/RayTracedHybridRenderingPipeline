#include "WrappedD3D12PipelineState.h"
#include "WrappedD3D12RootSignature.h"
RDCBOOST_NAMESPACE_BEGIN

void CopyByteCodeImpl(const void* &dest, const void* &src, const SIZE_T & length) {
	if (src&&length>0) {
		void* pbc = new byte[length];
		dest = pbc;
		memcpy(pbc, src, length);
		return;
	}
	dest = NULL;
}

void ReleaseByteCodeImpl(const void* &dest, const SIZE_T& length) {
	if (dest) {
		delete[]dest;
	}
}

template<typename T>
void CopyTImpl(const T*&dest, const T* & src, const UINT & length) {
	if (src&&length > 0) {
		T* pbc = new T[length];
		dest = pbc;
		memcpy(pbc, src, length * sizeof(T));
		return;
	}
	dest = NULL;
}

template<typename T>
void ReleaseTImpl(const T* &dest, const UINT& length) {
	if (dest) {
		delete[]dest;
	}
}

WrappedD3D12PipelineState::PipelineStateTypeDesc::PipelineStateTypeDesc(PipelineStateType _type, D3D12_COMPUTE_PIPELINE_STATE_DESC& _computeDesc)
{
	type = _type;
	uPipelineStateDesc.compute = _computeDesc;
	CopyByteCodeImpl(uPipelineStateDesc.compute.CS.pShaderBytecode, _computeDesc.CS.pShaderBytecode, _computeDesc.CS.BytecodeLength);
	CopyByteCodeImpl(uPipelineStateDesc.compute.CachedPSO.pCachedBlob, _computeDesc.CachedPSO.pCachedBlob, _computeDesc.CachedPSO.CachedBlobSizeInBytes);
}
WrappedD3D12PipelineState::PipelineStateTypeDesc::PipelineStateTypeDesc(PipelineStateType _type, D3D12_GRAPHICS_PIPELINE_STATE_DESC& _graphicsDesc)
{
	type = _type;
	uPipelineStateDesc.graphics = _graphicsDesc;

	CopyByteCodeImpl(uPipelineStateDesc.graphics.VS.pShaderBytecode, _graphicsDesc.VS.pShaderBytecode, _graphicsDesc.VS.BytecodeLength);
	CopyByteCodeImpl(uPipelineStateDesc.graphics.PS.pShaderBytecode, _graphicsDesc.PS.pShaderBytecode, _graphicsDesc.PS.BytecodeLength);
	CopyByteCodeImpl(uPipelineStateDesc.graphics.DS.pShaderBytecode, _graphicsDesc.DS.pShaderBytecode, _graphicsDesc.DS.BytecodeLength);
	CopyByteCodeImpl(uPipelineStateDesc.graphics.HS.pShaderBytecode, _graphicsDesc.HS.pShaderBytecode, _graphicsDesc.HS.BytecodeLength);
	CopyByteCodeImpl(uPipelineStateDesc.graphics.GS.pShaderBytecode, _graphicsDesc.GS.pShaderBytecode, _graphicsDesc.GS.BytecodeLength);
	CopyTImpl(uPipelineStateDesc.graphics.StreamOutput.pSODeclaration, _graphicsDesc.StreamOutput.pSODeclaration, _graphicsDesc.StreamOutput.NumEntries);
	CopyTImpl(uPipelineStateDesc.graphics.StreamOutput.pBufferStrides, _graphicsDesc.StreamOutput.pBufferStrides, _graphicsDesc.StreamOutput.NumStrides);
	CopyTImpl(uPipelineStateDesc.graphics.InputLayout.pInputElementDescs, _graphicsDesc.InputLayout.pInputElementDescs, _graphicsDesc.InputLayout.NumElements);
	CopyByteCodeImpl(uPipelineStateDesc.graphics.CachedPSO.pCachedBlob, _graphicsDesc.CachedPSO.pCachedBlob, _graphicsDesc.CachedPSO.CachedBlobSizeInBytes);
}


WrappedD3D12PipelineState::PipelineStateTypeDesc::~PipelineStateTypeDesc() {
	if (type == PipelineStateType_Compute) {
		ReleaseByteCodeImpl(uPipelineStateDesc.compute.CS.pShaderBytecode, uPipelineStateDesc.compute.CS.BytecodeLength);
		ReleaseByteCodeImpl(uPipelineStateDesc.compute.CachedPSO.pCachedBlob, uPipelineStateDesc.compute.CachedPSO.CachedBlobSizeInBytes);
	}
	else {
		ReleaseByteCodeImpl(uPipelineStateDesc.graphics.VS.pShaderBytecode, uPipelineStateDesc.graphics.VS.BytecodeLength);
		ReleaseByteCodeImpl(uPipelineStateDesc.graphics.PS.pShaderBytecode, uPipelineStateDesc.graphics.PS.BytecodeLength);
		ReleaseByteCodeImpl(uPipelineStateDesc.graphics.DS.pShaderBytecode, uPipelineStateDesc.graphics.DS.BytecodeLength);
		ReleaseByteCodeImpl(uPipelineStateDesc.graphics.HS.pShaderBytecode, uPipelineStateDesc.graphics.HS.BytecodeLength);
		ReleaseByteCodeImpl(uPipelineStateDesc.graphics.GS.pShaderBytecode, uPipelineStateDesc.graphics.GS.BytecodeLength);
		ReleaseTImpl(uPipelineStateDesc.graphics.StreamOutput.pSODeclaration, uPipelineStateDesc.graphics.StreamOutput.NumEntries);
		ReleaseTImpl(uPipelineStateDesc.graphics.StreamOutput.pBufferStrides, uPipelineStateDesc.graphics.StreamOutput.NumStrides);
		ReleaseTImpl(uPipelineStateDesc.graphics.InputLayout.pInputElementDescs, uPipelineStateDesc.graphics.InputLayout.NumElements);
		ReleaseByteCodeImpl(uPipelineStateDesc.graphics.CachedPSO.pCachedBlob, uPipelineStateDesc.graphics.CachedPSO.CachedBlobSizeInBytes);
	}
}


WrappedD3D12PipelineState::WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
	D3D12_COMPUTE_PIPELINE_STATE_DESC& compute_desc, WrappedD3D12RootSignature* pWrappedRootSignature)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_typeDesc(PipelineStateType_Compute, compute_desc),
	m_pWrappedRootSignature(pWrappedRootSignature)
{


}

WrappedD3D12PipelineState::WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
	D3D12_GRAPHICS_PIPELINE_STATE_DESC& graphics_desc, WrappedD3D12RootSignature* pWrappedRootSignature)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_typeDesc(PipelineStateType_Graphics, graphics_desc),
	m_pWrappedRootSignature(pWrappedRootSignature)
{
	

}

WrappedD3D12PipelineState::~WrappedD3D12PipelineState() {

}


COMPtr<ID3D12DeviceChild> WrappedD3D12PipelineState::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12PipelineState> pvPipelineState;
	if (m_typeDesc.type ==PipelineStateType_Graphics) {
		auto& desc = m_typeDesc.uPipelineStateDesc.graphics;
		desc.pRootSignature = m_pWrappedRootSignature->GetReal().Get();
		HRESULT ret = pNewDevice->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pvPipelineState));
		if (FAILED(ret)) {
			LogError("create new D3D12RootSignature failed");
			return NULL;
		}
	}
	else {
		auto& desc = m_typeDesc.uPipelineStateDesc.compute;
		desc.pRootSignature = m_pWrappedRootSignature->GetReal().Get();
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