#include "WrappedD3D12RootSignature.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12RootSignature::WrappedD3D12RootSignature(ID3D12RootSignature* pReal, WrappedD3D12Device* pDevice,
	UINT nodeMask,
	const void *pBlobWithRootSignature,
	SIZE_T blobLengthInBytes) 
	: WrappedD3D12DeviceChild(pReal, pDevice),
	m_NodeMask(nodeMask),
	m_pBlobWithRootSignature(0),
	m_BlobLengthInBytes(blobLengthInBytes)
{
	m_pBlobWithRootSignature = (void*)new byte[blobLengthInBytes];
	//ZeroMemory(m_PBlobWithRootSignature, m_BlobLengthInBytes);
	memcpy(m_pBlobWithRootSignature, pBlobWithRootSignature, blobLengthInBytes);
}
WrappedD3D12RootSignature::~WrappedD3D12RootSignature() {
	delete m_pBlobWithRootSignature;
}


ID3D12DeviceChild* WrappedD3D12RootSignature::CopyToDevice(ID3D12Device* pNewDevice) {
	ID3D12RootSignature * pvRootSignature;
	HRESULT ret = pNewDevice->CreateRootSignature(m_NodeMask, m_pBlobWithRootSignature, m_BlobLengthInBytes, IID_PPV_ARGS(&pvRootSignature));
	if (FAILED(ret)) {
		LogError("create new D3D12RootSignature failed");
		return NULL;
	}
	return pvRootSignature;
}
RDCBOOST_NAMESPACE_END