#pragma once
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN
class WrappedD3D12RootSignature;

class WrappedD3D12PipelineState : public WrappedD3D12DeviceChild<ID3D12PipelineState> {
public:
	enum PipelineStateType {
		PipelineStateType_Graphics,
		PipelineStateType_Compute
	};

	struct PipelineStateTypeDesc {
		PipelineStateType type;
		union UPipelineStateDesc {
			D3D12_COMPUTE_PIPELINE_STATE_DESC compute;
			D3D12_GRAPHICS_PIPELINE_STATE_DESC graphics;
		} uPipelineStateDesc;
		PipelineStateTypeDesc(PipelineStateType _type, D3D12_COMPUTE_PIPELINE_STATE_DESC& _computeDesc);
		PipelineStateTypeDesc(PipelineStateType _type, D3D12_GRAPHICS_PIPELINE_STATE_DESC& _graphicsDesc);
		~PipelineStateTypeDesc();
	};

public:
	WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
		D3D12_COMPUTE_PIPELINE_STATE_DESC& compute_desc, WrappedD3D12RootSignature* pWrappedRootSignature);
	WrappedD3D12PipelineState(ID3D12PipelineState* pReal, WrappedD3D12Device* pDevice,
		D3D12_GRAPHICS_PIPELINE_STATE_DESC& graphics_desc, WrappedD3D12RootSignature* pWrappedRootSignature);
	~WrappedD3D12PipelineState();

public://override
	virtual HRESULT STDMETHODCALLTYPE GetCachedBlob(
		_COM_Outptr_  ID3DBlob **ppBlob);

public://function

public://framewokr
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	COMPtr<WrappedD3D12RootSignature> m_pWrappedRootSignature;
	PipelineStateTypeDesc m_typeDesc;
};

RDCBOOST_NAMESPACE_END