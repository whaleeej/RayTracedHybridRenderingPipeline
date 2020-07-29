#pragma once
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12CommandAllocator : public WrappedD3D12DeviceChild<ID3D12CommandAllocator> {
public:
	WrappedD3D12CommandAllocator(ID3D12CommandAllocator* pReal, WrappedD3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type);
	~WrappedD3D12CommandAllocator();

public://override
	virtual HRESULT STDMETHODCALLTYPE Reset(void);
public://function

public://framewokr
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);

protected:
	D3D12_COMMAND_LIST_TYPE m_type;
};

class WrappedD3D12CommandList : public WrappedD3D12DeviceChild<ID3D12CommandList> {
public:
	WrappedD3D12CommandList(
		ID3D12CommandList* pReal, WrappedD3D12Device* pDevice, 
		WrappedD3D12CommandAllocator* pWrappedAllocator,
		UINT nodeMask);
	~WrappedD3D12CommandList();

public://override
	virtual D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE GetType(void);

public://framework
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);
	
protected:
	WrappedD3D12CommandAllocator* m_pWrappedCommandAllocator;
	UINT m_NodeMask;
};

class WrappedD3D12GraphicsCommansList : public WrappedD3D12DeviceChild <ID3D12GraphicsCommandList>{
public:
	WrappedD3D12GraphicsCommansList(
		ID3D12GraphicsCommandList* pReal, WrappedD3D12Device* pDevice,
		WrappedD3D12CommandAllocator* pWrappedAllocator,
		UINT nodeMask);
	~WrappedD3D12GraphicsCommansList();

public://override
	virtual D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE GetType(void);
	virtual HRESULT STDMETHODCALLTYPE Close(void);

	virtual HRESULT STDMETHODCALLTYPE Reset(
		_In_  ID3D12CommandAllocator *pAllocator,
		_In_opt_  ID3D12PipelineState *pInitialState);

	virtual void STDMETHODCALLTYPE ClearState(
		_In_opt_  ID3D12PipelineState *pPipelineState);

	virtual void STDMETHODCALLTYPE DrawInstanced(
		_In_  UINT VertexCountPerInstance,
		_In_  UINT InstanceCount,
		_In_  UINT StartVertexLocation,
		_In_  UINT StartInstanceLocation);

	virtual void STDMETHODCALLTYPE DrawIndexedInstanced(
		_In_  UINT IndexCountPerInstance,
		_In_  UINT InstanceCount,
		_In_  UINT StartIndexLocation,
		_In_  INT BaseVertexLocation,
		_In_  UINT StartInstanceLocation);

	virtual void STDMETHODCALLTYPE Dispatch(
		_In_  UINT ThreadGroupCountX,
		_In_  UINT ThreadGroupCountY,
		_In_  UINT ThreadGroupCountZ);

	virtual void STDMETHODCALLTYPE CopyBufferRegion(
		_In_  ID3D12Resource *pDstBuffer,
		UINT64 DstOffset,
		_In_  ID3D12Resource *pSrcBuffer,
		UINT64 SrcOffset,
		UINT64 NumBytes);

	virtual void STDMETHODCALLTYPE CopyTextureRegion(
		_In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
		UINT DstX,
		UINT DstY,
		UINT DstZ,
		_In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
		_In_opt_  const D3D12_BOX *pSrcBox);

	virtual void STDMETHODCALLTYPE CopyResource(
		_In_  ID3D12Resource *pDstResource,
		_In_  ID3D12Resource *pSrcResource);

	virtual void STDMETHODCALLTYPE CopyTiles(
		_In_  ID3D12Resource *pTiledResource,
		_In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
		_In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
		_In_  ID3D12Resource *pBuffer,
		UINT64 BufferStartOffsetInBytes,
		D3D12_TILE_COPY_FLAGS Flags);

	virtual void STDMETHODCALLTYPE ResolveSubresource(
		_In_  ID3D12Resource *pDstResource,
		_In_  UINT DstSubresource,
		_In_  ID3D12Resource *pSrcResource,
		_In_  UINT SrcSubresource,
		_In_  DXGI_FORMAT Format);

	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(
		_In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology);

	virtual void STDMETHODCALLTYPE RSSetViewports(
		_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
		_In_reads_(NumViewports)  const D3D12_VIEWPORT *pViewports);

	virtual void STDMETHODCALLTYPE RSSetScissorRects(
		_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
		_In_reads_(NumRects)  const D3D12_RECT *pRects);

	virtual void STDMETHODCALLTYPE OMSetBlendFactor(
		_In_reads_opt_(4)  const FLOAT BlendFactor[4]);

	virtual void STDMETHODCALLTYPE OMSetStencilRef(
		_In_  UINT StencilRef);

	virtual void STDMETHODCALLTYPE SetPipelineState(
		_In_  ID3D12PipelineState *pPipelineState);

	virtual void STDMETHODCALLTYPE ResourceBarrier(
		_In_  UINT NumBarriers,
		_In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers);

	virtual void STDMETHODCALLTYPE ExecuteBundle(
		_In_  ID3D12GraphicsCommandList *pCommandList);

	virtual void STDMETHODCALLTYPE SetDescriptorHeaps(
		_In_  UINT NumDescriptorHeaps,
		_In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps);

	virtual void STDMETHODCALLTYPE SetComputeRootSignature(
		_In_opt_  ID3D12RootSignature *pRootSignature);

	virtual void STDMETHODCALLTYPE SetGraphicsRootSignature(
		_In_opt_  ID3D12RootSignature *pRootSignature);

	virtual void STDMETHODCALLTYPE SetComputeRootDescriptorTable(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

	virtual void STDMETHODCALLTYPE SetGraphicsRootDescriptorTable(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor);

	virtual void STDMETHODCALLTYPE SetComputeRoot32BitConstant(
		_In_  UINT RootParameterIndex,
		_In_  UINT SrcData,
		_In_  UINT DestOffsetIn32BitValues);

	virtual void STDMETHODCALLTYPE SetGraphicsRoot32BitConstant(
		_In_  UINT RootParameterIndex,
		_In_  UINT SrcData,
		_In_  UINT DestOffsetIn32BitValues);

	virtual void STDMETHODCALLTYPE SetComputeRoot32BitConstants(
		_In_  UINT RootParameterIndex,
		_In_  UINT Num32BitValuesToSet,
		_In_reads_(Num32BitValuesToSet * sizeof(UINT))  const void *pSrcData,
		_In_  UINT DestOffsetIn32BitValues);

	virtual void STDMETHODCALLTYPE SetGraphicsRoot32BitConstants(
		_In_  UINT RootParameterIndex,
		_In_  UINT Num32BitValuesToSet,
		_In_reads_(Num32BitValuesToSet * sizeof(UINT))  const void *pSrcData,
		_In_  UINT DestOffsetIn32BitValues);

	virtual void STDMETHODCALLTYPE SetComputeRootConstantBufferView(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	virtual void STDMETHODCALLTYPE SetGraphicsRootConstantBufferView(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	virtual void STDMETHODCALLTYPE SetComputeRootShaderResourceView(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	virtual void STDMETHODCALLTYPE SetGraphicsRootShaderResourceView(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	virtual void STDMETHODCALLTYPE SetComputeRootUnorderedAccessView(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	virtual void STDMETHODCALLTYPE SetGraphicsRootUnorderedAccessView(
		_In_  UINT RootParameterIndex,
		_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation);

	virtual void STDMETHODCALLTYPE IASetIndexBuffer(
		_In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView);

	virtual void STDMETHODCALLTYPE IASetVertexBuffers(
		_In_  UINT StartSlot,
		_In_  UINT NumViews,
		_In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews);

	virtual void STDMETHODCALLTYPE SOSetTargets(
		_In_  UINT StartSlot,
		_In_  UINT NumViews,
		_In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews);

	virtual void STDMETHODCALLTYPE OMSetRenderTargets(
		_In_  UINT NumRenderTargetDescriptors,
		_In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
		_In_  BOOL RTsSingleHandleToDescriptorRange,
		_In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);

	virtual void STDMETHODCALLTYPE ClearDepthStencilView(
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
		_In_  D3D12_CLEAR_FLAGS ClearFlags,
		_In_  FLOAT Depth,
		_In_  UINT8 Stencil,
		_In_  UINT NumRects,
		_In_reads_(NumRects)  const D3D12_RECT *pRects);

	virtual void STDMETHODCALLTYPE ClearRenderTargetView(
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
		_In_  const FLOAT ColorRGBA[4],
		_In_  UINT NumRects,
		_In_reads_(NumRects)  const D3D12_RECT *pRects);

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(
		_In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
		_In_  ID3D12Resource *pResource,
		_In_  const UINT Values[4],
		_In_  UINT NumRects,
		_In_reads_(NumRects)  const D3D12_RECT *pRects);

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(
		_In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
		_In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
		_In_  ID3D12Resource *pResource,
		_In_  const FLOAT Values[4],
		_In_  UINT NumRects,
		_In_reads_(NumRects)  const D3D12_RECT *pRects);

	virtual void STDMETHODCALLTYPE DiscardResource(
		_In_  ID3D12Resource *pResource,
		_In_opt_  const D3D12_DISCARD_REGION *pRegion);

	virtual void STDMETHODCALLTYPE BeginQuery(
		_In_  ID3D12QueryHeap *pQueryHeap,
		_In_  D3D12_QUERY_TYPE Type,
		_In_  UINT Index);

	virtual void STDMETHODCALLTYPE EndQuery(
		_In_  ID3D12QueryHeap *pQueryHeap,
		_In_  D3D12_QUERY_TYPE Type,
		_In_  UINT Index);

	virtual void STDMETHODCALLTYPE ResolveQueryData(
		_In_  ID3D12QueryHeap *pQueryHeap,
		_In_  D3D12_QUERY_TYPE Type,
		_In_  UINT StartIndex,
		_In_  UINT NumQueries,
		_In_  ID3D12Resource *pDestinationBuffer,
		_In_  UINT64 AlignedDestinationBufferOffset);

	virtual void STDMETHODCALLTYPE SetPredication(
		_In_opt_  ID3D12Resource *pBuffer,
		_In_  UINT64 AlignedBufferOffset,
		_In_  D3D12_PREDICATION_OP Operation);

	virtual void STDMETHODCALLTYPE SetMarker(
		UINT Metadata,
		_In_reads_bytes_opt_(Size)  const void *pData,
		UINT Size);

	virtual void STDMETHODCALLTYPE BeginEvent(
		UINT Metadata,
		_In_reads_bytes_opt_(Size)  const void *pData,
		UINT Size);

	virtual void STDMETHODCALLTYPE EndEvent(void);

	virtual void STDMETHODCALLTYPE ExecuteIndirect(
		_In_  ID3D12CommandSignature *pCommandSignature,
		_In_  UINT MaxCommandCount,
		_In_  ID3D12Resource *pArgumentBuffer,
		_In_  UINT64 ArgumentBufferOffset,
		_In_opt_  ID3D12Resource *pCountBuffer,
		_In_  UINT64 CountBufferOffset);

public://framework
	virtual ID3D12DeviceChild* CopyToDevice(ID3D12Device* pNewDevice);

protected:
	WrappedD3D12CommandAllocator* m_pWrappedCommandAllocator;
	UINT m_NodeMask;
};

RDCBOOST_NAMESPACE_END