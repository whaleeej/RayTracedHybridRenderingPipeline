#pragma once
#include "WrappedD3D12Object.h"
#include "DeviceCreateParams.h"
#include "WrappedD3D12GPUVAddrMgr.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12DescriptorHeap;
class WrappedD3D12Resource;

class WrappedD3D12Device:public WrappedD3D12Object<ID3D12Device, ID3D12Device> {
	struct DummyID3D12InfoQueue : public ID3D12InfoQueue
	{
		WrappedD3D12Device *m_pDevice;

		DummyID3D12InfoQueue() : m_pDevice(NULL) {}
		//////////////////////////////
		// implement IUnknown
		HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) { return E_NOINTERFACE; }
		ULONG STDMETHODCALLTYPE AddRef() { return m_pDevice->AddRef(); }
		ULONG STDMETHODCALLTYPE Release() { return m_pDevice->Release(); }

		//////////////////////////////
		// implement ID3D12InfoQueue
		virtual HRESULT STDMETHODCALLTYPE SetMessageCountLimit(UINT64 MessageCountLimit) { return S_OK; }
		virtual void STDMETHODCALLTYPE ClearStoredMessages() {}
		virtual HRESULT STDMETHODCALLTYPE GetMessage(UINT64 MessageIndex, D3D12_MESSAGE *pMessage,
			SIZE_T *pMessageByteLength)
		{
			return S_OK;
		}
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesAllowedByStorageFilter() { return 0; }
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesDeniedByStorageFilter() { return 0; }
		virtual UINT64 STDMETHODCALLTYPE GetNumStoredMessages() { return 0; }
		virtual UINT64 STDMETHODCALLTYPE GetNumStoredMessagesAllowedByRetrievalFilter() { return 0; }
		virtual UINT64 STDMETHODCALLTYPE GetNumMessagesDiscardedByMessageCountLimit() { return 0; }
		virtual UINT64 STDMETHODCALLTYPE GetMessageCountLimit() { return 0; }
		virtual HRESULT STDMETHODCALLTYPE AddStorageFilterEntries(D3D12_INFO_QUEUE_FILTER *pFilter)
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE GetStorageFilter(D3D12_INFO_QUEUE_FILTER *pFilter,
			SIZE_T *pFilterByteLength)
		{
			return S_OK;
		}
		virtual void STDMETHODCALLTYPE ClearStorageFilter() {}
		virtual HRESULT STDMETHODCALLTYPE PushEmptyStorageFilter() { return S_OK; }
		virtual HRESULT STDMETHODCALLTYPE PushCopyOfStorageFilter() { return S_OK; }
		virtual HRESULT STDMETHODCALLTYPE PushStorageFilter(D3D12_INFO_QUEUE_FILTER *pFilter)
		{
			return S_OK;
		}
		virtual void STDMETHODCALLTYPE PopStorageFilter() {}
		virtual UINT STDMETHODCALLTYPE GetStorageFilterStackSize() { return 0; }
		virtual HRESULT STDMETHODCALLTYPE AddRetrievalFilterEntries(D3D12_INFO_QUEUE_FILTER *pFilter)
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE GetRetrievalFilter(D3D12_INFO_QUEUE_FILTER *pFilter,
			SIZE_T *pFilterByteLength)
		{
			return S_OK;
		}
		virtual void STDMETHODCALLTYPE ClearRetrievalFilter() {}
		virtual HRESULT STDMETHODCALLTYPE PushEmptyRetrievalFilter() { return S_OK; }
		virtual HRESULT STDMETHODCALLTYPE PushCopyOfRetrievalFilter() { return S_OK; }
		virtual HRESULT STDMETHODCALLTYPE PushRetrievalFilter(D3D12_INFO_QUEUE_FILTER *pFilter)
		{
			return S_OK;
		}
		virtual void STDMETHODCALLTYPE PopRetrievalFilter() {}
		virtual UINT STDMETHODCALLTYPE GetRetrievalFilterStackSize() { return 0; }
		virtual HRESULT STDMETHODCALLTYPE AddMessage(D3D12_MESSAGE_CATEGORY Category,
			D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID,
			LPCSTR pDescription)
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE AddApplicationMessage(D3D12_MESSAGE_SEVERITY Severity,
			LPCSTR pDescription)
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnCategory(D3D12_MESSAGE_CATEGORY Category, BOOL bEnable)
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY Severity, BOOL bEnable)
		{
			return S_OK;
		}
		virtual HRESULT STDMETHODCALLTYPE SetBreakOnID(D3D12_MESSAGE_ID ID, BOOL bEnable) { return S_OK; }
		virtual BOOL STDMETHODCALLTYPE GetBreakOnCategory(D3D12_MESSAGE_CATEGORY Category)
		{
			return FALSE;
		}
		virtual BOOL STDMETHODCALLTYPE GetBreakOnSeverity(D3D12_MESSAGE_SEVERITY Severity)
		{
			return FALSE;
		}
		virtual BOOL STDMETHODCALLTYPE GetBreakOnID(D3D12_MESSAGE_ID ID) { return FALSE; }
		virtual void STDMETHODCALLTYPE SetMuteDebugOutput(BOOL bMute) {}
		virtual BOOL STDMETHODCALLTYPE GetMuteDebugOutput() { return TRUE; }
	};

public:
	WrappedD3D12Device(ID3D12Device* pRealDevice, const SDeviceCreateParams& param);
	~WrappedD3D12Device();

public: // override ID3D12Object
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);

public: // override ID3D12Device
	virtual UINT STDMETHODCALLTYPE GetNodeCount(void);

	virtual HRESULT STDMETHODCALLTYPE CreateCommandQueue(
		_In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
		REFIID riid,
		_COM_Outptr_  void **ppCommandQueue);

	virtual HRESULT STDMETHODCALLTYPE CreateCommandAllocator(
		_In_  D3D12_COMMAND_LIST_TYPE type,
		REFIID riid,
		_COM_Outptr_  void **ppCommandAllocator);

	virtual HRESULT STDMETHODCALLTYPE CreateGraphicsPipelineState(
		_In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
		REFIID riid,
		_COM_Outptr_  void **ppPipelineState);

	virtual HRESULT STDMETHODCALLTYPE CreateComputePipelineState(
		_In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
		REFIID riid,
		_COM_Outptr_  void **ppPipelineState);

	virtual HRESULT STDMETHODCALLTYPE CreateCommandList(
		_In_  UINT nodeMask,
		_In_  D3D12_COMMAND_LIST_TYPE type,
		_In_  ID3D12CommandAllocator *pCommandAllocator,
		_In_opt_  ID3D12PipelineState *pInitialState,
		REFIID riid,
		_COM_Outptr_  void **ppCommandList);

	virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
		D3D12_FEATURE Feature,
		_Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
		UINT FeatureSupportDataSize);

	virtual HRESULT STDMETHODCALLTYPE CreateDescriptorHeap(
		_In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
		REFIID riid,
		_COM_Outptr_  void **ppvHeap);

	virtual UINT STDMETHODCALLTYPE GetDescriptorHandleIncrementSize(
		_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType);

	virtual HRESULT STDMETHODCALLTYPE CreateRootSignature(
		_In_  UINT nodeMask,
		_In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
		_In_  SIZE_T blobLengthInBytes,
		REFIID riid,
		_COM_Outptr_  void **ppvRootSignature);

	virtual void STDMETHODCALLTYPE CreateConstantBufferView(
		_In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) ;

	virtual void STDMETHODCALLTYPE CreateShaderResourceView(
		_In_opt_  ID3D12Resource *pResource,
		_In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor);

	virtual void STDMETHODCALLTYPE CreateUnorderedAccessView(
		_In_opt_  ID3D12Resource *pResource,
		_In_opt_  ID3D12Resource *pCounterResource,
		_In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) ;

	virtual void STDMETHODCALLTYPE CreateRenderTargetView(
		_In_opt_  ID3D12Resource *pResource,
		_In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) ;

	virtual void STDMETHODCALLTYPE CreateDepthStencilView(
		_In_opt_  ID3D12Resource *pResource,
		_In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) ;

	virtual void STDMETHODCALLTYPE CreateSampler(
		_In_  const D3D12_SAMPLER_DESC *pDesc,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) ;

	virtual void STDMETHODCALLTYPE CopyDescriptors(
		_In_  UINT NumDestDescriptorRanges,
		_In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
		_In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
		_In_  UINT NumSrcDescriptorRanges,
		_In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
		_In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
		_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) ;

	virtual void STDMETHODCALLTYPE CopyDescriptorsSimple(
		_In_  UINT NumDescriptors,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
		_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) ;

	virtual D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE GetResourceAllocationInfo(
		_In_  UINT visibleMask,
		_In_  UINT numResourceDescs,
		_In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs) ;

	virtual D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE GetCustomHeapProperties(
		_In_  UINT nodeMask,
		D3D12_HEAP_TYPE heapType) ;

	virtual HRESULT STDMETHODCALLTYPE CreateCommittedResource(
		_In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
		D3D12_HEAP_FLAGS HeapFlags,
		_In_  const D3D12_RESOURCE_DESC *pDesc,
		D3D12_RESOURCE_STATES InitialResourceState,
		_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
		REFIID riidResource,
		_COM_Outptr_opt_  void **ppvResource) ;

	virtual HRESULT STDMETHODCALLTYPE CreateHeap(
		_In_  const D3D12_HEAP_DESC *pDesc,
		REFIID riid,
		_COM_Outptr_opt_  void **ppvHeap) ;

	virtual HRESULT STDMETHODCALLTYPE CreatePlacedResource(
		_In_  ID3D12Heap *pHeap,
		UINT64 HeapOffset,
		_In_  const D3D12_RESOURCE_DESC *pDesc,
		D3D12_RESOURCE_STATES InitialState,
		_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
		REFIID riid,
		_COM_Outptr_opt_  void **ppvResource) ;

	virtual HRESULT STDMETHODCALLTYPE CreateReservedResource(
		_In_  const D3D12_RESOURCE_DESC *pDesc,
		D3D12_RESOURCE_STATES InitialState,
		_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
		REFIID riid,
		_COM_Outptr_opt_  void **ppvResource) ;

	virtual HRESULT STDMETHODCALLTYPE CreateSharedHandle(
		_In_  ID3D12DeviceChild *pObject,
		_In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
		DWORD Access,
		_In_opt_  LPCWSTR Name,
		_Out_  HANDLE *pHandle) ;

	virtual HRESULT STDMETHODCALLTYPE OpenSharedHandle(
		_In_  HANDLE NTHandle,
		REFIID riid,
		_COM_Outptr_opt_  void **ppvObj) ;

	virtual HRESULT STDMETHODCALLTYPE OpenSharedHandleByName(
		_In_  LPCWSTR Name,
		DWORD Access,
		/* [annotation][out] */
		_Out_  HANDLE *pNTHandle) ;

	virtual HRESULT STDMETHODCALLTYPE MakeResident(
		UINT NumObjects,
		_In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects) ;

	virtual HRESULT STDMETHODCALLTYPE Evict(
		UINT NumObjects,
		_In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects) ;

	virtual HRESULT STDMETHODCALLTYPE CreateFence(
		UINT64 InitialValue,
		D3D12_FENCE_FLAGS Flags,
		REFIID riid,
		_COM_Outptr_  void **ppFence) ;

	virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void) ;

	virtual void STDMETHODCALLTYPE GetCopyableFootprints(
		_In_  const D3D12_RESOURCE_DESC *pResourceDesc,
		_In_range_(0, D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
		_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource)  UINT NumSubresources,
		UINT64 BaseOffset,
		_Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
		_Out_writes_opt_(NumSubresources)  UINT *pNumRows,
		_Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
		_Out_opt_  UINT64 *pTotalBytes) ;

	virtual HRESULT STDMETHODCALLTYPE CreateQueryHeap(
		_In_  const D3D12_QUERY_HEAP_DESC *pDesc,
		REFIID riid,
		_COM_Outptr_opt_  void **ppvHeap) ;

	virtual HRESULT STDMETHODCALLTYPE SetStablePowerState(
		BOOL Enable) ;

	virtual HRESULT STDMETHODCALLTYPE CreateCommandSignature(
		_In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
		_In_opt_  ID3D12RootSignature *pRootSignature,
		REFIID riid,
		_COM_Outptr_opt_  void **ppvCommandSignature) ;

	virtual void STDMETHODCALLTYPE GetResourceTiling(
		_In_  ID3D12Resource *pTiledResource,
		_Out_opt_  UINT *pNumTilesForEntireResource,
		_Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
		_Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
		_Inout_opt_  UINT *pNumSubresourceTilings,
		_In_  UINT FirstSubresourceTilingToGet,
		_Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips) ;

	virtual LUID STDMETHODCALLTYPE GetAdapterLuid(void) ;

public: //func
	bool isRenderDocDevice() { return m_bRenderDocDevice; }

	void SetAsRenderDocDevice(bool b) { m_bRenderDocDevice = b; }

	const SDeviceCreateParams& GetDeviceCreateParams() const { return m_DeviceCreateParams; }

	bool isResourceExist(WrappedD3D12Resource* pWrappedResource, ID3D12Resource* pRealResource);

	void cacheResourceReflectionToOldReal();

	void clearResourceReflectionToOldReal();


public: // framework
	virtual void SwitchToDeviceRdc(ID3D12Device* pNewDevice);

	void OnDeviceChildReleased(ID3D12DeviceChild* pReal);

private:
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_RootSignature;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_CommandSignature;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_PipelineState;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_Heap;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_QueryHeap;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_Resource;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_DescriptorHeap;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_CommandAllocator;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_CommandList;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_Fence;
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs_CommandQueue;

	std::map<WrappedD3D12ObjectBase*, ID3D12Resource*>		m_BackRefs_Resource_Reflection; //cache old real device when switching
	DummyID3D12InfoQueue m_DummyInfoQueue;
	SDeviceCreateParams m_DeviceCreateParams;
	bool m_bRenderDocDevice;
};

RDCBOOST_NAMESPACE_END