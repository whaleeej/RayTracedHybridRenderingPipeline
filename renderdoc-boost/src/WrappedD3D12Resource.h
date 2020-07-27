#pragma once
#include <dxgi1_6.h>
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Device;
class WrappedD3D12DXGISwapChain;

class WrappedD3D12Resource : public WrappedD3D12DeviceChild<ID3D12Resource>
{
public:
	WrappedD3D12Resource(ID3D12Resource* pReal, WrappedD3D12Device* pDevice);
	~WrappedD3D12Resource();

public: //override
	virtual HRESULT STDMETHODCALLTYPE Map(
		UINT Subresource,
		_In_opt_  const D3D12_RANGE *pReadRange,
		_Outptr_opt_result_bytebuffer_(_Inexpressible_("Dependent on resource"))  void **ppData);

	virtual void STDMETHODCALLTYPE Unmap(
		UINT Subresource,
		_In_opt_  const D3D12_RANGE *pWrittenRange);

	virtual D3D12_RESOURCE_DESC STDMETHODCALLTYPE GetDesc(void);

	virtual D3D12_GPU_VIRTUAL_ADDRESS STDMETHODCALLTYPE GetGPUVirtualAddress(void);

	virtual HRESULT STDMETHODCALLTYPE WriteToSubresource(
		UINT DstSubresource,
		_In_opt_  const D3D12_BOX *pDstBox,
		_In_  const void *pSrcData,
		UINT SrcRowPitch,
		UINT SrcDepthPitch);

	virtual HRESULT STDMETHODCALLTYPE ReadFromSubresource(
		_Out_  void *pDstData,
		UINT DstRowPitch,
		UINT DstDepthPitch,
		UINT SrcSubresource,
		_In_opt_  const D3D12_BOX *pSrcBox);

	virtual HRESULT STDMETHODCALLTYPE GetHeapProperties(
		_Out_opt_  D3D12_HEAP_PROPERTIES *pHeapProperties,
		_Out_opt_  D3D12_HEAP_FLAGS *pHeapFlags);

public://framework
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);
	void InitSwapChain(IDXGISwapChain1* pRealSwapChain);
	void SwitchToSwapChain(IDXGISwapChain1* pNewSwapChain, ID3D12Resource* pNewResource);

protected:
	IDXGISwapChain1* m_pRealSwapChain; //not reffed
};


RDCBOOST_NAMESPACE_END