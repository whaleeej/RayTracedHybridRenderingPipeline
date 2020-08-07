#pragma once
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12RootSignature;

class WrappedD3D12CommandSignature : public WrappedD3D12DeviceChild<ID3D12CommandSignature, ID3D12CommandSignature> {
public:// cons des
	WrappedD3D12CommandSignature(ID3D12CommandSignature* pReal, WrappedD3D12Device* pDevice,
		const D3D12_COMMAND_SIGNATURE_DESC* pDesc, WrappedD3D12RootSignature* pWrappedRootSignature);
	~WrappedD3D12CommandSignature();

public:// override

public:// func

public:// framework
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	COMPtr<WrappedD3D12RootSignature> m_pWrappedRootSignature;
	D3D12_COMMAND_SIGNATURE_DESC m_Desc;
};

RDCBOOST_NAMESPACE_END