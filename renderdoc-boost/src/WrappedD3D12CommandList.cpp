#include "WrappedD3D12CommandList.h"

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12CommandAllocator::WrappedD3D12CommandAllocator(ID3D12CommandAllocator* pReal, WrappedD3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_type(type)
{

}

WrappedD3D12CommandAllocator::~WrappedD3D12CommandAllocator() {

}

COMPtr<ID3D12DeviceChild> WrappedD3D12CommandAllocator::CopyToDevice(ID3D12Device* pNewDevice) {
	//丢弃不拷贝
	ID3D12CommandAllocator* pvNewCommandAllocator;
	HRESULT ret = pNewDevice->CreateCommandAllocator(m_type, IID_PPV_ARGS(&pvNewCommandAllocator));
	if (FAILED(ret)) {
		LogError("create new CommandAllocator failed");
		return NULL;
	}
	return pvNewCommandAllocator;
}

WrappedD3D12CommandList::WrappedD3D12CommandList(
	ID3D12CommandList* pReal, WrappedD3D12Device* pDevice, 
	WrappedD3D12CommandAllocator* pWrappedAllocator,
	UINT nodeMask)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_pWrappedCommandAllocator(pWrappedAllocator),
	m_NodeMask(nodeMask)
{
	if(m_pWrappedCommandAllocator)
		m_pWrappedCommandAllocator->AddRef();
}

WrappedD3D12CommandList::~WrappedD3D12CommandList() {
	if (m_pWrappedCommandAllocator)
		m_pWrappedCommandAllocator->Release();
}
ID3D12DeviceChild* WrappedD3D12CommandList::CopyToDevice(ID3D12Device* pNewDevice) {
	//丢弃不拷贝 Initial pipelinestate直接设置为null
	ID3D12CommandList* pvNewCommandList;
	HRESULT ret = pNewDevice->CreateCommandList(m_NodeMask, GetReal()->GetType(), m_pWrappedCommandAllocator->GetReal(), 
		NULL, IID_PPV_ARGS(&pvNewCommandList));
	if (FAILED(ret)) {
		LogError("create new CommandList failed");
		return NULL;
	}
	return pvNewCommandList;
}

WrappedD3D12GraphicsCommansList::WrappedD3D12GraphicsCommansList(
	ID3D12GraphicsCommandList* pReal, WrappedD3D12Device* pDevice,
	WrappedD3D12CommandAllocator* pWrappedAllocator,
	UINT nodeMask)
	: WrappedD3D12DeviceChild(pReal, pDevice),
	m_pWrappedCommandAllocator(pWrappedAllocator),
	m_NodeMask(nodeMask) 
{
	if (m_pWrappedCommandAllocator)
		m_pWrappedCommandAllocator->AddRef();
}

WrappedD3D12GraphicsCommansList::~WrappedD3D12GraphicsCommansList() {
	if (m_pWrappedCommandAllocator)
		m_pWrappedCommandAllocator->Release();
}

ID3D12DeviceChild* WrappedD3D12GraphicsCommansList::CopyToDevice(ID3D12Device* pNewDevice) {
	ID3D12GraphicsCommandList* pvNewCommandList;
	HRESULT ret = pNewDevice->CreateCommandList(m_NodeMask, GetReal()->GetType(), m_pWrappedCommandAllocator->GetReal(),
		NULL, IID_PPV_ARGS(&pvNewCommandList));
	if (FAILED(ret)) {
		LogError("create new CommandList failed");
		return NULL;
	}
	return pvNewCommandList;
}

/************************************************************************/
/*                         override                                                                     */
/************************************************************************/
HRESULT STDMETHODCALLTYPE WrappedD3D12CommandAllocator::Reset(void) {
	return GetReal()->Reset();
}

D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE WrappedD3D12CommandList::GetType(void) {
	return GetReal()->GetType();
}

D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::GetType(void) {
	return GetReal()->GetType();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::Close(void) {
	return GetReal()->Close();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::Reset(
	_In_  ID3D12CommandAllocator *pAllocator,
	_In_opt_  ID3D12PipelineState *pInitialState) {
	return GetReal()->Reset(pAllocator, pInitialState);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ClearState(
	_In_opt_  ID3D12PipelineState *pPipelineState) {
	return GetReal()->ClearState(pPipelineState);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::DrawInstanced(
	_In_  UINT VertexCountPerInstance,
	_In_  UINT InstanceCount,
	_In_  UINT StartVertexLocation,
	_In_  UINT StartInstanceLocation) {
	return GetReal()->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::DrawIndexedInstanced(
	_In_  UINT IndexCountPerInstance,
	_In_  UINT InstanceCount,
	_In_  UINT StartIndexLocation,
	_In_  INT BaseVertexLocation,
	_In_  UINT StartInstanceLocation) {
	return GetReal()->DrawIndexedInstanced(
		IndexCountPerInstance,
		InstanceCount,
		StartIndexLocation,
		BaseVertexLocation,
		StartInstanceLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::Dispatch(
	_In_  UINT ThreadGroupCountX,
	_In_  UINT ThreadGroupCountY,
	_In_  UINT ThreadGroupCountZ) {
	return GetReal()->Dispatch(
		ThreadGroupCountX,
		ThreadGroupCountY,
		ThreadGroupCountZ);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::CopyBufferRegion(
	_In_  ID3D12Resource *pDstBuffer,
	UINT64 DstOffset,
	_In_  ID3D12Resource *pSrcBuffer,
	UINT64 SrcOffset,
	UINT64 NumBytes) {
	return GetReal()->CopyBufferRegion(
		pDstBuffer,
		DstOffset,
		pSrcBuffer,
		SrcOffset,
		NumBytes);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::CopyTextureRegion(
	_In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
	UINT DstX,
	UINT DstY,
	UINT DstZ,
	_In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
	_In_opt_  const D3D12_BOX *pSrcBox) {
	return GetReal()->CopyTextureRegion(
		pDst,
		DstX,
		DstY,
		DstZ,
		pSrc,
		pSrcBox);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::CopyResource(
	_In_  ID3D12Resource *pDstResource,
	_In_  ID3D12Resource *pSrcResource) {
	return GetReal()->CopyResource(
	pDstResource,
	pSrcResource);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::CopyTiles(
	_In_  ID3D12Resource *pTiledResource,
	_In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
	_In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
	_In_  ID3D12Resource *pBuffer,
	UINT64 BufferStartOffsetInBytes,
	D3D12_TILE_COPY_FLAGS Flags) {
	return GetReal()->CopyTiles(
		pTiledResource,
		pTileRegionStartCoordinate,
		pTileRegionSize,
		pBuffer,
		BufferStartOffsetInBytes,
		Flags);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ResolveSubresource(
	_In_  ID3D12Resource *pDstResource,
	_In_  UINT DstSubresource,
	_In_  ID3D12Resource *pSrcResource,
	_In_  UINT SrcSubresource,
	_In_  DXGI_FORMAT Format) {
	return GetReal()->ResolveSubresource(
		pDstResource,
		DstSubresource,
		pSrcResource,
		SrcSubresource,
		Format);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::IASetPrimitiveTopology(
	_In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) {
	return GetReal()->IASetPrimitiveTopology(
		PrimitiveTopology);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::RSSetViewports(
	_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
	_In_reads_(NumViewports)  const D3D12_VIEWPORT *pViewports) {
	return GetReal()->RSSetViewports(
		NumViewports,
		pViewports);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::RSSetScissorRects(
	_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal()->RSSetScissorRects(
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::OMSetBlendFactor(
	_In_reads_opt_(4)  const FLOAT BlendFactor[4]) {
	return GetReal()->OMSetBlendFactor(
		BlendFactor);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::OMSetStencilRef(
	_In_  UINT StencilRef) {
	return GetReal()->OMSetStencilRef(
		StencilRef);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetPipelineState(
	_In_  ID3D12PipelineState *pPipelineState) {
	return GetReal()->SetPipelineState(
		pPipelineState);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ResourceBarrier(
	_In_  UINT NumBarriers,
	_In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers) {
	return GetReal()->ResourceBarrier(
		NumBarriers,
		pBarriers);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ExecuteBundle(
	_In_  ID3D12GraphicsCommandList *pCommandList) {
	return GetReal()->ExecuteBundle(
		pCommandList);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetDescriptorHeaps(
	_In_  UINT NumDescriptorHeaps,
	_In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps) {
	return GetReal()->SetDescriptorHeaps(
		NumDescriptorHeaps,
		ppDescriptorHeaps);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRootSignature(
	_In_opt_  ID3D12RootSignature *pRootSignature) {
	return GetReal()->SetComputeRootSignature(
		pRootSignature);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRootSignature(
	_In_opt_  ID3D12RootSignature *pRootSignature) {
	return GetReal()->SetGraphicsRootSignature(
		pRootSignature);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRootDescriptorTable(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
	return GetReal()->SetComputeRootDescriptorTable(
		RootParameterIndex,
		BaseDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRootDescriptorTable(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
	return GetReal()->SetGraphicsRootDescriptorTable(
		RootParameterIndex,
		BaseDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRoot32BitConstant(
	_In_  UINT RootParameterIndex,
	_In_  UINT SrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal()->SetComputeRoot32BitConstant(
		RootParameterIndex,
		SrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRoot32BitConstant(
	_In_  UINT RootParameterIndex,
	_In_  UINT SrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal()->SetGraphicsRoot32BitConstant(
		RootParameterIndex,
		SrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRoot32BitConstants(
	_In_  UINT RootParameterIndex,
	_In_  UINT Num32BitValuesToSet,
	_In_reads_(Num32BitValuesToSet * sizeof(UINT))  const void *pSrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal()->SetComputeRoot32BitConstants(
		RootParameterIndex,
		Num32BitValuesToSet,
		pSrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRoot32BitConstants(
	_In_  UINT RootParameterIndex,
	_In_  UINT Num32BitValuesToSet,
	_In_reads_(Num32BitValuesToSet * sizeof(UINT))  const void *pSrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal()->SetGraphicsRoot32BitConstants(
		RootParameterIndex,
		Num32BitValuesToSet,
		pSrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRootConstantBufferView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	return GetReal()->SetComputeRootConstantBufferView(
		RootParameterIndex,
		BufferLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRootConstantBufferView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	return GetReal()->SetGraphicsRootConstantBufferView(
		RootParameterIndex,
		BufferLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRootShaderResourceView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	return GetReal()->SetComputeRootShaderResourceView(
		RootParameterIndex,
		BufferLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRootShaderResourceView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	return GetReal()->SetGraphicsRootShaderResourceView(
		RootParameterIndex,
		BufferLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetComputeRootUnorderedAccessView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	return GetReal()->SetComputeRootUnorderedAccessView(
		RootParameterIndex,
		BufferLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetGraphicsRootUnorderedAccessView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	return GetReal()->SetGraphicsRootUnorderedAccessView(
		RootParameterIndex,
		BufferLocation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::IASetIndexBuffer(
	_In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView) {
	return GetReal()->IASetIndexBuffer(
		pView);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::IASetVertexBuffers(
	_In_  UINT StartSlot,
	_In_  UINT NumViews,
	_In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews) {
	return GetReal()->IASetVertexBuffers(
		StartSlot,
		NumViews,
		pViews);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SOSetTargets(
	_In_  UINT StartSlot,
	_In_  UINT NumViews,
	_In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews) {
	return GetReal()->SOSetTargets(
		StartSlot,
		NumViews,
		pViews);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::OMSetRenderTargets(
	_In_  UINT NumRenderTargetDescriptors,
	_In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
	_In_  BOOL RTsSingleHandleToDescriptorRange,
	_In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor) {
	return GetReal()->OMSetRenderTargets(
		NumRenderTargetDescriptors,
		pRenderTargetDescriptors,
		RTsSingleHandleToDescriptorRange,
		pDepthStencilDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ClearDepthStencilView(
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
	_In_  D3D12_CLEAR_FLAGS ClearFlags,
	_In_  FLOAT Depth,
	_In_  UINT8 Stencil,
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal()->ClearDepthStencilView(
		DepthStencilView,
		ClearFlags,
		Depth,
		Stencil,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ClearRenderTargetView(
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
	_In_  const FLOAT ColorRGBA[4],
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal()->ClearRenderTargetView(
		RenderTargetView,
		ColorRGBA,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ClearUnorderedAccessViewUint(
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
	_In_  ID3D12Resource *pResource,
	_In_  const UINT Values[4],
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal()->ClearUnorderedAccessViewUint(
		ViewGPUHandleInCurrentHeap,
		ViewCPUHandle,
		pResource,
		Values,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ClearUnorderedAccessViewFloat(
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
	_In_  ID3D12Resource *pResource,
	_In_  const FLOAT Values[4],
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal()->ClearUnorderedAccessViewFloat(
		ViewGPUHandleInCurrentHeap,
		ViewCPUHandle,
		pResource,
		Values,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::DiscardResource(
	_In_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_DISCARD_REGION *pRegion) {
	return GetReal()->DiscardResource(
		pResource,
		pRegion);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::BeginQuery(
	_In_  ID3D12QueryHeap *pQueryHeap,
	_In_  D3D12_QUERY_TYPE Type,
	_In_  UINT Index) {
	return GetReal()->BeginQuery(
		pQueryHeap,
		Type,
		Index);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::EndQuery(
	_In_  ID3D12QueryHeap *pQueryHeap,
	_In_  D3D12_QUERY_TYPE Type,
	_In_  UINT Index) {
	return GetReal()->EndQuery(
		pQueryHeap,
		Type,
		Index);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ResolveQueryData(
	_In_  ID3D12QueryHeap *pQueryHeap,
	_In_  D3D12_QUERY_TYPE Type,
	_In_  UINT StartIndex,
	_In_  UINT NumQueries,
	_In_  ID3D12Resource *pDestinationBuffer,
	_In_  UINT64 AlignedDestinationBufferOffset) {
	return GetReal()->ResolveQueryData(
		pQueryHeap,
		Type,
		StartIndex,
		NumQueries,
		pDestinationBuffer,
		AlignedDestinationBufferOffset);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetPredication(
	_In_opt_  ID3D12Resource *pBuffer,
	_In_  UINT64 AlignedBufferOffset,
	_In_  D3D12_PREDICATION_OP Operation) {
	return GetReal()->SetPredication(
		pBuffer,
		AlignedBufferOffset,
		Operation);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::SetMarker(
	UINT Metadata,
	_In_reads_bytes_opt_(Size)  const void *pData,
	UINT Size) {
	return GetReal()->SetMarker(
		Metadata,
		pData,
		Size);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::BeginEvent(
	UINT Metadata,
	_In_reads_bytes_opt_(Size)  const void *pData,
	UINT Size) {
	return GetReal()->BeginEvent(
		Metadata,
		pData,
		Size);
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::EndEvent(void) {
	return GetReal()->EndEvent();
}

void STDMETHODCALLTYPE WrappedD3D12GraphicsCommansList::ExecuteIndirect(
	_In_  ID3D12CommandSignature *pCommandSignature,
	_In_  UINT MaxCommandCount,
	_In_  ID3D12Resource *pArgumentBuffer,
	_In_  UINT64 ArgumentBufferOffset,
	_In_opt_  ID3D12Resource *pCountBuffer,
	_In_  UINT64 CountBufferOffset) {
	return GetReal()->ExecuteIndirect(
		pCommandSignature,
		MaxCommandCount,
		pArgumentBuffer,
		ArgumentBufferOffset,
		pCountBuffer,
		CountBufferOffset);
}

RDCBOOST_NAMESPACE_END