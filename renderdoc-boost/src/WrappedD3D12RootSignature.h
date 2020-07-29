#pragma once
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12RootSignature : public WrappedD3D12DeviceChild<ID3D12RootSignature> {
public:
	WrappedD3D12RootSignature(ID3D12RootSignature* pReal, WrappedD3D12Device* pDevice,
		UINT nodeMask,
		const void *pBlobWithRootSignature,
		SIZE_T blobLengthInBytes);
	~WrappedD3D12RootSignature();

public://override

public://function

public://framewokr
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);

protected:
	UINT m_NodeMask;
	void *m_pBlobWithRootSignature;
	SIZE_T m_BlobLengthInBytes;
};

RDCBOOST_NAMESPACE_END