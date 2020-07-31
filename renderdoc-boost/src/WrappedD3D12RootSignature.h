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
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	UINT m_NodeMask;
	SIZE_T m_BlobLengthInBytes;
	void *m_pBlobWithRootSignature;
};

RDCBOOST_NAMESPACE_END