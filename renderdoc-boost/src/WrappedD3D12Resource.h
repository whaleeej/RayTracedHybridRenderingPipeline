#pragma once
#include <dxgi1_6.h>
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Device;
class WrappedD3D12DXGISwapChain;
class WrappedD3D12Heap;

class WrappedD3D12Resource : public WrappedD3D12DeviceChild<ID3D12Resource>
{
public:
	enum WrappedD3D12ResourceType {
		CommittedWrappedD3D12Resource,
		PlacedWrappedD3D12Resource,
		ReservedWrappedD3D12Resource,
		BackBufferWrappedD3D12Resource
	};
	//WrappedD3D12ResourceType m_Type;
	//D3D12_RESOURCE_DESC m_Desc;
	//D3D12_RESOURCE_STATES m_State;
	//D3D12_CLEAR_VALUE* m_ClearValue;


	////committed
	//D3D12_HEAP_PROPERTIES m_HeapProperties;
	//D3D12_HEAP_FLAGS m_HeapFlags;

	////placed
	//COMPtr<WrappedD3D12Heap> m_pWrappedHeap;
	//UINT64 m_heapOffset;
public:
	WrappedD3D12Resource( //backbuffer
		ID3D12Resource* pReal, WrappedD3D12Device* pDevice
	) :WrappedD3D12DeviceChild(pReal, pDevice),
		m_Type(BackBufferWrappedD3D12Resource),
		m_Desc(),
		m_State(D3D12_RESOURCE_STATE_COMMON),
		m_ClearValue(NULL),
		m_HeapProperties(),
		m_HeapFlags(D3D12_HEAP_FLAG_NONE),
		m_pWrappedHeap(NULL),
		m_heapOffset(0)
	{
	}
	WrappedD3D12Resource(//committed
		ID3D12Resource* pReal, WrappedD3D12Device* pDevice,
		const D3D12_HEAP_PROPERTIES *pHeapProperties,
		D3D12_HEAP_FLAGS heapFlags,
		const D3D12_RESOURCE_DESC *pDesc,
		D3D12_RESOURCE_STATES state,
		const D3D12_CLEAR_VALUE *pClearValue
	) :WrappedD3D12DeviceChild(pReal, pDevice),
		m_Type(CommittedWrappedD3D12Resource),
		m_Desc(*pDesc),
		m_State(state),
		m_ClearValue(NULL),
		m_HeapProperties(*pHeapProperties),
		m_HeapFlags(heapFlags),
		m_pWrappedHeap(NULL),
		m_heapOffset(0)
	{
		if (pClearValue) {
			m_ClearValue = new D3D12_CLEAR_VALUE();
			memcpy(m_ClearValue, pClearValue, sizeof(D3D12_CLEAR_VALUE));
		}
	}
	WrappedD3D12Resource(//placed
		ID3D12Resource* pReal, WrappedD3D12Device* pDevice,
		WrappedD3D12Heap* pHeap,
		UINT64 heapOffset,
		const D3D12_RESOURCE_DESC *pDesc,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON,
		const D3D12_CLEAR_VALUE *pClearValue = NULL
	) :WrappedD3D12DeviceChild(pReal, pDevice),
		m_Type(PlacedWrappedD3D12Resource),
		m_Desc(*pDesc),
		m_State(state),
		m_ClearValue(NULL),
		m_HeapProperties(),
		m_HeapFlags(D3D12_HEAP_FLAG_NONE),
		m_pWrappedHeap(pHeap),
		m_heapOffset(heapOffset)
	{
		if (pClearValue) {
			m_ClearValue = new D3D12_CLEAR_VALUE();
			memcpy(m_ClearValue, pClearValue, sizeof(D3D12_CLEAR_VALUE));
		}
	}
	WrappedD3D12Resource(//reserved
		ID3D12Resource* pReal, WrappedD3D12Device* pDevice,
		const D3D12_RESOURCE_DESC *pDesc,
		D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON,
		const D3D12_CLEAR_VALUE *pClearValue = NULL
	) :WrappedD3D12DeviceChild(pReal, pDevice),
		m_Type(ReservedWrappedD3D12Resource),
		m_Desc(*pDesc),
		m_State(state),
		m_ClearValue(NULL),
		m_HeapProperties(),
		m_HeapFlags(D3D12_HEAP_FLAG_NONE),
		m_pWrappedHeap(0),
		m_heapOffset(0)
	{
		if (pClearValue) {
			m_ClearValue = new D3D12_CLEAR_VALUE();
			memcpy(m_ClearValue, pClearValue, sizeof(D3D12_CLEAR_VALUE));
		}
	}


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

public: //func
	void InitSwapChain(IDXGISwapChain1* pRealSwapChain);
	void changeToState(D3D12_RESOURCE_STATES state);
	D3D12_RESOURCE_STATES queryState() { return m_State; }

public://framework
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);
	void SwitchToSwapChain(IDXGISwapChain1* pNewSwapChain, ID3D12Resource* pNewResource);

protected:

	WrappedD3D12ResourceType m_Type;
	D3D12_RESOURCE_DESC m_Desc;
	D3D12_RESOURCE_STATES m_State;
	D3D12_CLEAR_VALUE* m_ClearValue;

	
	//committed
	D3D12_HEAP_PROPERTIES m_HeapProperties;
	D3D12_HEAP_FLAGS m_HeapFlags;

	//placed
	COMPtr<WrappedD3D12Heap> m_pWrappedHeap;
	UINT64 m_heapOffset;

	//reserved
	//...

	// backbuffer
	IDXGISwapChain1* m_pRealSwapChain;
};


RDCBOOST_NAMESPACE_END