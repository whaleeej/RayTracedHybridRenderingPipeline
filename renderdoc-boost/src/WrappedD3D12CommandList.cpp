#include "WrappedD3D12CommandList.h"
#include "WrappedD3D12PipelineState.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12Heap.h"
#include "WrappedD3D12RootSignature.h"
#include "WrappedD3D12CommandSignature.h"
#include "WrappedD3D12QueryHeap.h"
RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12CommandAllocator::WrappedD3D12CommandAllocator(ID3D12CommandAllocator* pReal, WrappedD3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type)
	:WrappedD3D12DeviceChild(pReal, pDevice),
	m_type(type)
{

}

WrappedD3D12CommandAllocator::~WrappedD3D12CommandAllocator() {

}

COMPtr<ID3D12DeviceChild> WrappedD3D12CommandAllocator::CopyToDevice(ID3D12Device* pNewDevice) {
	COMPtr<ID3D12CommandAllocator> pvNewCommandAllocator;
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
	:WrappedD3D12DeviceChild(pReal, pDevice, -1),
	m_pWrappedCommandAllocator(pWrappedAllocator),
	m_NodeMask(nodeMask)
{
}

WrappedD3D12CommandList::WrappedD3D12CommandList(
	ID3D12GraphicsCommandList* pReal, WrappedD3D12Device* pDevice,
	WrappedD3D12CommandAllocator* pWrappedAllocator,
	UINT nodeMask)
	: WrappedD3D12DeviceChild(pReal, pDevice, 0),
	m_pWrappedCommandAllocator(pWrappedAllocator),
	m_NodeMask(nodeMask) 
{
}

WrappedD3D12CommandList::~WrappedD3D12CommandList() {
}

COMPtr<ID3D12DeviceChild> WrappedD3D12CommandList::CopyToDevice(ID3D12Device* pNewDevice) {
	//丢弃不拷贝 Initial pipelinestate直接设置为null
	if (checkVersionSupport(0)) {
		COMPtr<ID3D12GraphicsCommandList> pvNewCommandList;
		HRESULT ret = pNewDevice->CreateCommandList(
			m_NodeMask, GetReal()->GetType(),
			m_pWrappedCommandAllocator->GetReal().Get(),
			NULL, IID_PPV_ARGS(&pvNewCommandList));
		if (FAILED(ret)) {
			LogError("create new CommandList failed");
			return NULL;
		}
		return pvNewCommandList;
	}
	else {
		COMPtr<ID3D12CommandList> pvNewCommandList;
		HRESULT ret = pNewDevice->CreateCommandList(
			m_NodeMask, GetReal()->GetType(),
			m_pWrappedCommandAllocator->GetReal().Get(),
			NULL, IID_PPV_ARGS(&pvNewCommandList));
		if (FAILED(ret)) {
			LogError("create new CommandList failed");
			return NULL;
		}
		return pvNewCommandList;
	}
}

void WrappedD3D12CommandList::FlushPendingResourceStates() {
	for (auto it = m_PendingResourceStates.begin(); it != m_PendingResourceStates.end(); it++) {
		it->first->changeToState(it->second);//这里认为持有的资源都有生命周期 // 否则d3d也会出错的
	}
}

/************************************************************************/
/*                         override                                                                     */
/************************************************************************/
HRESULT STDMETHODCALLTYPE WrappedD3D12CommandList::QueryInterface(REFIID riid, void** ppvObject) {
	if (riid == __uuidof(ID3D12GraphicsCommandList)) {
		if (checkVersionSupport(0)) {
			*ppvObject = static_cast<ID3D12GraphicsCommandList*>(this);
			AddRef();
			return S_OK;
		}
		else
			return E_FAIL;
	}
	return WrappedD3D12DeviceChild::QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12CommandAllocator::Reset(void) {
	return GetReal()->Reset();
}

D3D12_COMMAND_LIST_TYPE STDMETHODCALLTYPE WrappedD3D12CommandList::GetType(void) {
	return GetReal()->GetType();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12CommandList::Close(void) {
	return GetReal0()->Close();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12CommandList::Reset(
	_In_  ID3D12CommandAllocator *pAllocator,
	_In_opt_  ID3D12PipelineState *pInitialState) {
	m_PendingResourceStates.clear();
	return GetReal0()->Reset(static_cast<WrappedD3D12CommandAllocator *>(pAllocator)->GetReal().Get(), 
		pInitialState?static_cast<WrappedD3D12PipelineState *>(pInitialState)->GetReal().Get():NULL);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ClearState(
	_In_opt_  ID3D12PipelineState *pPipelineState) {
	return GetReal0()->ClearState(static_cast<WrappedD3D12PipelineState *>(pPipelineState)->GetReal().Get());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::DrawInstanced(
	_In_  UINT VertexCountPerInstance,
	_In_  UINT InstanceCount,
	_In_  UINT StartVertexLocation,
	_In_  UINT StartInstanceLocation) {
	return GetReal0()->DrawInstanced(
		VertexCountPerInstance, 
		InstanceCount, 
		StartVertexLocation, 
		StartInstanceLocation);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::DrawIndexedInstanced(
	_In_  UINT IndexCountPerInstance,
	_In_  UINT InstanceCount,
	_In_  UINT StartIndexLocation,
	_In_  INT BaseVertexLocation,
	_In_  UINT StartInstanceLocation) {
	return GetReal0()->DrawIndexedInstanced(
		IndexCountPerInstance,
		InstanceCount,
		StartIndexLocation,
		BaseVertexLocation,
		StartInstanceLocation);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::Dispatch(
	_In_  UINT ThreadGroupCountX,
	_In_  UINT ThreadGroupCountY,
	_In_  UINT ThreadGroupCountZ) {
	return GetReal0()->Dispatch(
		ThreadGroupCountX,
		ThreadGroupCountY,
		ThreadGroupCountZ);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::CopyBufferRegion(
	_In_  ID3D12Resource *pDstBuffer,
	UINT64 DstOffset,
	_In_  ID3D12Resource *pSrcBuffer,
	UINT64 SrcOffset,
	UINT64 NumBytes) {
	return GetReal0()->CopyBufferRegion(
		static_cast<WrappedD3D12Resource *>(pDstBuffer)->GetReal().Get(),
		DstOffset,
		static_cast<WrappedD3D12Resource *>(pSrcBuffer)->GetReal().Get(),
		SrcOffset,
		NumBytes);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::CopyTextureRegion(
	_In_  const D3D12_TEXTURE_COPY_LOCATION *pDst,
	UINT DstX,
	UINT DstY,
	UINT DstZ,
	_In_  const D3D12_TEXTURE_COPY_LOCATION *pSrc,
	_In_opt_  const D3D12_BOX *pSrcBox) {
	D3D12_TEXTURE_COPY_LOCATION _Dst = *pDst;
	D3D12_TEXTURE_COPY_LOCATION _Src = *pSrc;
	_Dst.pResource = static_cast<WrappedD3D12Resource *>(_Dst.pResource)->GetReal().Get();
	_Src.pResource = static_cast<WrappedD3D12Resource *>(_Src.pResource)->GetReal().Get();

	return GetReal0()->CopyTextureRegion(
		&_Dst,
		DstX,
		DstY,
		DstZ,
		&_Src,
		pSrcBox);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::CopyResource(
	_In_  ID3D12Resource *pDstResource,
	_In_  ID3D12Resource *pSrcResource) {
	return GetReal0()->CopyResource(
		static_cast<WrappedD3D12Resource *>(pDstResource)->GetReal().Get(),
		static_cast<WrappedD3D12Resource *>(pSrcResource)->GetReal().Get());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::CopyTiles(
	_In_  ID3D12Resource *pTiledResource,
	_In_  const D3D12_TILED_RESOURCE_COORDINATE *pTileRegionStartCoordinate,
	_In_  const D3D12_TILE_REGION_SIZE *pTileRegionSize,
	_In_  ID3D12Resource *pBuffer,
	UINT64 BufferStartOffsetInBytes,
	D3D12_TILE_COPY_FLAGS Flags) {
	return GetReal0()->CopyTiles(
		static_cast<WrappedD3D12Resource *>(pTiledResource)->GetReal().Get(),
		pTileRegionStartCoordinate,
		pTileRegionSize,
		static_cast<WrappedD3D12Resource *>(pBuffer)->GetReal().Get(),
		BufferStartOffsetInBytes,
		Flags);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ResolveSubresource(
	_In_  ID3D12Resource *pDstResource,
	_In_  UINT DstSubresource,
	_In_  ID3D12Resource *pSrcResource,
	_In_  UINT SrcSubresource,
	_In_  DXGI_FORMAT Format) {
	return GetReal0()->ResolveSubresource(
		static_cast<WrappedD3D12Resource *>(pDstResource)->GetReal().Get(),
		DstSubresource,
		static_cast<WrappedD3D12Resource *>(pSrcResource)->GetReal().Get(),
		SrcSubresource,
		Format);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::IASetPrimitiveTopology(
	_In_  D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology) {
	return GetReal0()->IASetPrimitiveTopology(
		PrimitiveTopology);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::RSSetViewports(
	_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumViewports,
	_In_reads_(NumViewports)  const D3D12_VIEWPORT *pViewports) {
	return GetReal0()->RSSetViewports(
		NumViewports,
		pViewports);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::RSSetScissorRects(
	_In_range_(0, D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal0()->RSSetScissorRects(
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::OMSetBlendFactor(
	_In_reads_opt_(4)  const FLOAT BlendFactor[4]) {
	return GetReal0()->OMSetBlendFactor(
		BlendFactor);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::OMSetStencilRef(
	_In_  UINT StencilRef) {
	return GetReal0()->OMSetStencilRef(
		StencilRef);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetPipelineState(
	_In_  ID3D12PipelineState *pPipelineState) {
	return GetReal0()->SetPipelineState(
		static_cast<WrappedD3D12PipelineState *>(pPipelineState)->GetReal().Get());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ResourceBarrier(
	_In_  UINT NumBarriers,
	_In_reads_(NumBarriers)  const D3D12_RESOURCE_BARRIER *pBarriers) {
	std::vector<D3D12_RESOURCE_BARRIER > barriers(NumBarriers);
	for (size_t i = 0; i < NumBarriers; i++) {
		barriers[i] = *(pBarriers + i);
		D3D12_RESOURCE_BARRIER& _Barriers = barriers[i];
		switch (_Barriers.Type) {
		case D3D12_RESOURCE_BARRIER_TYPE_TRANSITION:
			m_PendingResourceStates[static_cast<WrappedD3D12Resource *>(_Barriers.Transition.pResource)] = _Barriers.Transition.StateAfter;
			_Barriers.Transition.pResource =
				_Barriers.Transition.pResource ? static_cast<WrappedD3D12Resource *>(_Barriers.Transition.pResource)->GetReal().Get() : NULL;
			break;
		case D3D12_RESOURCE_BARRIER_TYPE_ALIASING:
			_Barriers.Aliasing.pResourceAfter = 
				_Barriers.Aliasing.pResourceAfter ?static_cast<WrappedD3D12Resource *>(_Barriers.Aliasing.pResourceAfter)->GetReal().Get():NULL;
			_Barriers.Aliasing.pResourceBefore = 
				_Barriers.Aliasing.pResourceBefore ?static_cast<WrappedD3D12Resource *>(_Barriers.Aliasing.pResourceBefore)->GetReal().Get():NULL;
			break;
		case D3D12_RESOURCE_BARRIER_TYPE_UAV:
			_Barriers.UAV.pResource = _Barriers.UAV.pResource ?static_cast<WrappedD3D12Resource *>(_Barriers.UAV.pResource)->GetReal().Get():NULL;
			break;
		default:
			LogError("Unknown barrier type");
		}
	}
	return GetReal0()->ResourceBarrier(
		NumBarriers,
		barriers.data());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ExecuteBundle(
	_In_  ID3D12GraphicsCommandList *pCommandList) {
	return GetReal0()->ExecuteBundle(
		static_cast<WrappedD3D12CommandList *>(pCommandList)->GetReal0().Get());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetDescriptorHeaps(
	_In_  UINT NumDescriptorHeaps,
	_In_reads_(NumDescriptorHeaps)  ID3D12DescriptorHeap *const *ppDescriptorHeaps) {
	std::vector<ID3D12DescriptorHeap*> descHeaps(NumDescriptorHeaps);
	for (UINT i = 0; i < NumDescriptorHeaps; i++) {
		descHeaps[i] = static_cast<WrappedD3D12DescriptorHeap *>(ppDescriptorHeaps[i])->GetReal().Get();
	}
	return GetReal0()->SetDescriptorHeaps(
		NumDescriptorHeaps,
		descHeaps.data());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRootSignature(
	_In_opt_  ID3D12RootSignature *pRootSignature) {
	return GetReal0()->SetComputeRootSignature(
		static_cast<WrappedD3D12RootSignature*>(pRootSignature)->GetReal().Get()
		);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRootSignature(
	_In_opt_  ID3D12RootSignature *pRootSignature) {
	return GetReal0()->SetGraphicsRootSignature(
		static_cast<WrappedD3D12RootSignature *>(pRootSignature)->GetReal().Get()
		);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRootDescriptorTable(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
	return GetReal0()->SetComputeRootDescriptorTable(
		RootParameterIndex,
		ANALYZE_WRAPPED_GPU_HANDLE(BaseDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRootDescriptorTable(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE BaseDescriptor) {
	return GetReal0()->SetGraphicsRootDescriptorTable(
		RootParameterIndex,
		ANALYZE_WRAPPED_GPU_HANDLE(BaseDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRoot32BitConstant(
	_In_  UINT RootParameterIndex,
	_In_  UINT SrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal0()->SetComputeRoot32BitConstant(
		RootParameterIndex,
		SrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRoot32BitConstant(
	_In_  UINT RootParameterIndex,
	_In_  UINT SrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal0()->SetGraphicsRoot32BitConstant(
		RootParameterIndex,
		SrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRoot32BitConstants(
	_In_  UINT RootParameterIndex,
	_In_  UINT Num32BitValuesToSet,
	_In_reads_(Num32BitValuesToSet * sizeof(UINT))  const void *pSrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal0()->SetComputeRoot32BitConstants(
		RootParameterIndex,
		Num32BitValuesToSet,
		pSrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRoot32BitConstants(
	_In_  UINT RootParameterIndex,
	_In_  UINT Num32BitValuesToSet,
	_In_reads_(Num32BitValuesToSet * sizeof(UINT))  const void *pSrcData,
	_In_  UINT DestOffsetIn32BitValues) {
	return GetReal0()->SetGraphicsRoot32BitConstants(
		RootParameterIndex,
		Num32BitValuesToSet,
		pSrcData,
		DestOffsetIn32BitValues);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRootConstantBufferView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(BufferLocation);
	return GetReal0()->SetComputeRootConstantBufferView(
		RootParameterIndex,
		realVAddr);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRootConstantBufferView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(BufferLocation);
	return GetReal0()->SetGraphicsRootConstantBufferView(
		RootParameterIndex,
		realVAddr);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRootShaderResourceView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(BufferLocation);
	return GetReal0()->SetComputeRootShaderResourceView(
		RootParameterIndex,
		realVAddr);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRootShaderResourceView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(BufferLocation);
	return GetReal0()->SetGraphicsRootShaderResourceView(
		RootParameterIndex,
		realVAddr);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetComputeRootUnorderedAccessView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(BufferLocation);
	return GetReal0()->SetComputeRootUnorderedAccessView(
		RootParameterIndex,
		realVAddr);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetGraphicsRootUnorderedAccessView(
	_In_  UINT RootParameterIndex,
	_In_  D3D12_GPU_VIRTUAL_ADDRESS BufferLocation) {
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(BufferLocation);
	return GetReal0()->SetGraphicsRootUnorderedAccessView(
		RootParameterIndex,
		realVAddr);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::IASetIndexBuffer(
	_In_opt_  const D3D12_INDEX_BUFFER_VIEW *pView) {
	Assert(pView);
	auto view = *pView;
	DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(view.BufferLocation);
	view.BufferLocation = realVAddr;
	return GetReal0()->IASetIndexBuffer(
		&view);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::IASetVertexBuffers(
	_In_  UINT StartSlot,
	_In_  UINT NumViews,
	_In_reads_opt_(NumViews)  const D3D12_VERTEX_BUFFER_VIEW *pViews) {
	Assert(pViews);
	std::vector<D3D12_VERTEX_BUFFER_VIEW> views(NumViews);
	for (UINT i=0; i<NumViews; i++)
	{
		views[i] = pViews[i];
		DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(views[i].BufferLocation);
		views[i].BufferLocation = realVAddr;
	}
	return GetReal0()->IASetVertexBuffers(
		StartSlot,
		NumViews,
		views.data());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SOSetTargets(
	_In_  UINT StartSlot,
	_In_  UINT NumViews,
	_In_reads_opt_(NumViews)  const D3D12_STREAM_OUTPUT_BUFFER_VIEW *pViews) {
	Assert(pViews);
	std::vector<D3D12_STREAM_OUTPUT_BUFFER_VIEW> views(NumViews);
	for (UINT i = 0; i < NumViews; i++)
	{
		views[i] = pViews[i];
		{
			DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(views[i].BufferLocation);
			views[i].BufferLocation = realVAddr;
		}
		{
			DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(views[i].BufferFilledSizeLocation);
			views[i].BufferFilledSizeLocation = realVAddr;
		}
	}
	return GetReal0()->SOSetTargets(
		StartSlot,
		NumViews,
		views.data());
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::OMSetRenderTargets(
	_In_  UINT NumRenderTargetDescriptors,
	_In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
	_In_  BOOL RTsSingleHandleToDescriptorRange,
	_In_opt_  const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor) {
	UINT numHandles = RTsSingleHandleToDescriptorRange ? RDCMIN(1U, NumRenderTargetDescriptors)
		: NumRenderTargetDescriptors;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtHandles(numHandles);
	const D3D12_CPU_DESCRIPTOR_HANDLE* dspHandle = pDepthStencilDescriptor;
	D3D12_CPU_DESCRIPTOR_HANDLE dsRealHandle;
	if (dspHandle) {
		dsRealHandle = ANALYZE_WRAPPED_CPU_HANDLE(*dspHandle);
		dspHandle = &dsRealHandle;
	}
	for (size_t i = 0; i < numHandles; i++) {
		rtHandles[i].ptr = ANALYZE_WRAPPED_CPU_HANDLE(pRenderTargetDescriptors[i]).ptr;
	}

	return GetReal0()->OMSetRenderTargets(
		NumRenderTargetDescriptors,
		rtHandles.data(),
		RTsSingleHandleToDescriptorRange,
		dspHandle);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ClearDepthStencilView(
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView,
	_In_  D3D12_CLEAR_FLAGS ClearFlags,
	_In_  FLOAT Depth,
	_In_  UINT8 Stencil,
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal0()->ClearDepthStencilView(
		ANALYZE_WRAPPED_CPU_HANDLE(DepthStencilView),
		ClearFlags,
		Depth,
		Stencil,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ClearRenderTargetView(
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView,
	_In_  const FLOAT ColorRGBA[4],
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal0()->ClearRenderTargetView(
		ANALYZE_WRAPPED_CPU_HANDLE(RenderTargetView),
		ColorRGBA,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ClearUnorderedAccessViewUint(
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
	_In_  ID3D12Resource *pResource,
	_In_  const UINT Values[4],
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal0()->ClearUnorderedAccessViewUint(
		ANALYZE_WRAPPED_GPU_HANDLE(ViewGPUHandleInCurrentHeap),
		ANALYZE_WRAPPED_CPU_HANDLE(ViewCPUHandle),
		static_cast<WrappedD3D12Resource *>(pResource)->GetReal().Get(),
		Values,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ClearUnorderedAccessViewFloat(
	_In_  D3D12_GPU_DESCRIPTOR_HANDLE ViewGPUHandleInCurrentHeap,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE ViewCPUHandle,
	_In_  ID3D12Resource *pResource,
	_In_  const FLOAT Values[4],
	_In_  UINT NumRects,
	_In_reads_(NumRects)  const D3D12_RECT *pRects) {
	return GetReal0()->ClearUnorderedAccessViewFloat(
		ANALYZE_WRAPPED_GPU_HANDLE(ViewGPUHandleInCurrentHeap),
		ANALYZE_WRAPPED_CPU_HANDLE(ViewCPUHandle),
		static_cast<WrappedD3D12Resource *>(pResource)->GetReal().Get(),
		Values,
		NumRects,
		pRects);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::DiscardResource(
	_In_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_DISCARD_REGION *pRegion) {
	return GetReal0()->DiscardResource(
		static_cast<WrappedD3D12Resource *>(pResource)->GetReal().Get(),
		pRegion);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::BeginQuery(
	_In_  ID3D12QueryHeap *pQueryHeap,
	_In_  D3D12_QUERY_TYPE Type,
	_In_  UINT Index) {
	return GetReal0()->BeginQuery(
		static_cast<WrappedD3D12QueryHeap*>(pQueryHeap)->GetReal().Get(),
		Type,
		Index);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::EndQuery(
	_In_  ID3D12QueryHeap *pQueryHeap,
	_In_  D3D12_QUERY_TYPE Type,
	_In_  UINT Index) {
	return GetReal0()->EndQuery(
		static_cast<WrappedD3D12QueryHeap*>(pQueryHeap)->GetReal().Get(),
		Type,
		Index);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ResolveQueryData(
	_In_  ID3D12QueryHeap *pQueryHeap,
	_In_  D3D12_QUERY_TYPE Type,
	_In_  UINT StartIndex,
	_In_  UINT NumQueries,
	_In_  ID3D12Resource *pDestinationBuffer,
	_In_  UINT64 AlignedDestinationBufferOffset) {
	return GetReal0()->ResolveQueryData(
		static_cast<WrappedD3D12QueryHeap*>(pQueryHeap)->GetReal().Get(),
		Type,
		StartIndex,
		NumQueries,
		static_cast<WrappedD3D12Resource *>(pDestinationBuffer)->GetReal().Get(),
		AlignedDestinationBufferOffset);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetPredication(
	_In_opt_  ID3D12Resource *pBuffer,
	_In_  UINT64 AlignedBufferOffset,
	_In_  D3D12_PREDICATION_OP Operation) {
	return GetReal0()->SetPredication(
		static_cast<WrappedD3D12Resource *>(pBuffer)->GetReal().Get(),
		AlignedBufferOffset,
		Operation);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::SetMarker(
	UINT Metadata,
	_In_reads_bytes_opt_(Size)  const void *pData,
	UINT Size) {
	return GetReal0()->SetMarker(
		Metadata,
		pData,
		Size);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::BeginEvent(
	UINT Metadata,
	_In_reads_bytes_opt_(Size)  const void *pData,
	UINT Size) {
	return GetReal0()->BeginEvent(
		Metadata,
		pData,
		Size);
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::EndEvent(void) {
	return GetReal0()->EndEvent();
}

void STDMETHODCALLTYPE WrappedD3D12CommandList::ExecuteIndirect(
	_In_  ID3D12CommandSignature *pCommandSignature,
	_In_  UINT MaxCommandCount,
	_In_  ID3D12Resource *pArgumentBuffer,
	_In_  UINT64 ArgumentBufferOffset,
	_In_opt_  ID3D12Resource *pCountBuffer,
	_In_  UINT64 CountBufferOffset) {
	return GetReal0()->ExecuteIndirect(
		static_cast<WrappedD3D12CommandSignature*>(pCommandSignature)->GetReal().Get(),
		MaxCommandCount,
		static_cast<WrappedD3D12Resource *>(pArgumentBuffer)->GetReal().Get(),
		ArgumentBufferOffset,
		static_cast<WrappedD3D12Resource *>(pCountBuffer)->GetReal().Get(),
		CountBufferOffset);
}

RDCBOOST_NAMESPACE_END