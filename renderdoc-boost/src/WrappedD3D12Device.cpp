#include "WrappedD3D12Device.h"
#include "WrappedD3D12Heap.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12Fence.h"
#include "WrappedD3D12CommandList.h"
#include "WrappedD3D12CommandQueue.h"
#include "WrappedD3D12RootSignature.h"
#include "WrappedD3D12PipelineState.h"

RDCBOOST_NAMESPACE_BEGIN


WrappedD3D12Device::WrappedD3D12Device(ID3D12Device * pRealDevice, const SDeviceCreateParams& param)
	: WrappedD3D12Object(pRealDevice)
	, m_DeviceCreateParams(param)
	, m_bRenderDocDevice(false)
{
	m_DummyInfoQueue.m_pDevice = this;
}

WrappedD3D12Device::~WrappedD3D12Device() {
}

void WrappedD3D12Device::OnDeviceChildReleased(ID3D12DeviceChild* pReal) {
	if (m_BackRefs_RootSignature.erase(pReal) !=0) return;
	if (m_BackRefs_PipelineState.erase(pReal) != 0) return;
	if (m_BackRefs_Heap.erase(pReal) != 0) return;
	if (m_BackRefs_Resource.erase(pReal) != 0) return;
	if (m_BackRefs_DescriptorHeap.erase(pReal) != 0) return;
	if (m_BackRefs_CommandAllocator.erase(pReal) != 0) return;
	if (m_BackRefs_CommandList.erase(pReal) != 0) return;
	if (m_BackRefs_Fence.erase(pReal) != 0) return;
	if (m_BackRefs_CommandQueue.erase(pReal) != 0) return;

	//TODO unknown wrapped devicechild released error
	LogError("Unknown device child released.");
	Assert(false);
	return;
}

void WrappedD3D12Device::SwitchToDeviceRdc(ID3D12Device* pNewDevice) {
	//create new device(done)
	Assert(pNewDevice != NULL);

	//copy private data of device
	m_PrivateData.CopyPrivateData(pNewDevice);

	auto backRefTransferFunc = [=](std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*>& m_BackRefs, std::string objectTypeStr)->void {
		if (!m_BackRefs.empty()) {
			printf("Transferring %s to new device without modifying the content of WrappedDeviceChild\n", objectTypeStr.c_str());
			printf("--------------------------------------------------\n");
			std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> newBackRefs;
			int progress=0; int idx=0;
			for (auto it = m_BackRefs.begin(); it != m_BackRefs.end(); it++) {
				it->second->SwitchToDeviceRdc(pNewDevice);//key of the framework
				newBackRefs[static_cast<ID3D12DeviceChild*>(it->second->GetRealObject().Get())] = it->second;
				// sequencing
				++idx;
				while (progress < (int)(idx * 50 / m_BackRefs.size()))
				{
					printf(">");
					++progress;
				}
			}
			printf("\n");
			m_BackRefs.swap(newBackRefs);
		}
	};
	auto backRefResourcesTransferFunc = [=](std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*>& m_BackRefs, std::string objectTypeStr)->void {
		if (!m_BackRefs.empty()) {
			printf("Transferring %s to new device without modifying the content of WrappedDeviceChild\n", objectTypeStr.c_str());
			printf("--------------------------------------------------\n");
			std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> newBackRefs; //新的资源->Wrapper
			std::map<WrappedD3D12ObjectBase*, COMPtr<ID3D12DeviceChild>> newBackRefsReflection;//Wrapper->旧的资源, 用来保存旧资源的生命周期
			//因为这个生命周期在switchToDevice1框架中被释放了，但是我要用它来进行拷贝到新资源内，所以要用ComPtr保存下来
			
			D3D12_COMMAND_QUEUE_DESC commandQueueDesc={
			/*.Type = */D3D12_COMMAND_LIST_TYPE_DIRECT,
			/*.Priority =*/ D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			/*.Flags =*/ D3D12_COMMAND_QUEUE_FLAG_NONE,
			/*.NodeMask =*/ 0 };
			COMPtr<ID3D12CommandQueue>copyCommandQueue;
			pNewDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&copyCommandQueue));
			COMPtr<ID3D12CommandAllocator> copyCommandAllocator;
			pNewDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&copyCommandAllocator));
			COMPtr<ID3D12GraphicsCommandList> copyCommandList;
			pNewDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&copyCommandList));
			uint64_t copyFenceValue = 0;
			COMPtr<ID3D12Fence> copyFence;
			pNewDevice->CreateFence(copyFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence));

			int progress = 0; int idx = 0;
			for (auto it = m_BackRefs.begin(); it != m_BackRefs.end(); it++) {
				// 缓存wrapper到旧资源的ptr，使用comptr来保存生命
				newBackRefsReflection[it->second] = it->first; 
				
				//key of the framework
				it->second->SwitchToDeviceRdc(pNewDevice);
				newBackRefs[static_cast<ID3D12DeviceChild*>(it->second->GetRealObject().Get())] = it->second;
				
				// sequencing
				++idx;
				while (progress < (int)(idx * 50 / m_BackRefs.size()))
				{
					printf(">");
					++progress;
				}

				if (!static_cast<WrappedD3D12Resource*>(it->second)->needCopy()) continue;
				if (static_cast<WrappedD3D12Resource*>(it->second)->queryState() != D3D12_RESOURCE_STATE_COPY_SOURCE)
				{
					//enque commandlist
					D3D12_RESOURCE_BARRIER srcResourceBarrier;
					ZeroMemory(&srcResourceBarrier, sizeof(srcResourceBarrier));
					srcResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					srcResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					srcResourceBarrier.Transition.pResource = static_cast<ID3D12Resource*>(it->first);
					srcResourceBarrier.Transition.StateBefore = static_cast<WrappedD3D12Resource*>(it->second)->queryState();//根据框架判断resouce有关的操作已经完成，resource实际的状态已经置为query到的state了
					srcResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
					srcResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					copyCommandList->ResourceBarrier(1, &srcResourceBarrier);
				}
				copyCommandList->CopyResource(static_cast<WrappedD3D12Resource*>(it->second)->GetReal().Get(), static_cast<ID3D12Resource*>(it->first));
				if (D3D12_RESOURCE_STATE_COPY_DEST != static_cast<WrappedD3D12Resource*>(it->second)->queryState())
				{
					D3D12_RESOURCE_BARRIER destResourceBarrier;
					ZeroMemory(&destResourceBarrier, sizeof(destResourceBarrier));
					destResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					destResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					destResourceBarrier.Transition.pResource = static_cast<ID3D12Resource*>(it->second->GetRealObject().Get());
					destResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
					destResourceBarrier.Transition.StateAfter = static_cast<WrappedD3D12Resource*>(it->second)->queryState();
					destResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					copyCommandList->ResourceBarrier(1, &destResourceBarrier);
				}			
			}
			std::vector<ID3D12CommandList* > listsToExec(1);
			listsToExec[0] = copyCommandList.Get();
			copyCommandQueue->ExecuteCommandLists(1, listsToExec.data());
			copyCommandQueue->Signal(copyFence.Get(), ++copyFenceValue);
			if ((copyFence->GetCompletedValue() < copyFenceValue)) //等待copy queue执行完毕
			{
				auto event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
				Assert(event && "Failed to create fence event handle.");

				// Is this function thread safe?
				copyFence->SetEventOnCompletion(copyFenceValue, event);
				::WaitForSingleObject(event, DWORD_MAX);

				::CloseHandle(event);
			}


			printf("\n");
			m_BackRefs.swap(newBackRefs);
		}
	};
	//0 切换RootSignature和依赖于Root Signature的Pipeline State
	backRefTransferFunc(m_BackRefs_RootSignature, "RootSignatures");
	backRefTransferFunc(m_BackRefs_PipelineState, "PipelineStates");
	//1 切换保存资源的resource heap
	backRefTransferFunc(m_BackRefs_Heap, "Heaps"); 
	//2 要确保所有的fence进行Signal的Value都已经Complete了
	for (auto it = m_BackRefs_Fence.begin(); it != m_BackRefs_Fence.end(); it++) {
		auto pWrappedFence = static_cast<WrappedD3D12Fence*>(it->second);
		pWrappedFence->WaitToCompleteApplicationFenceValue();
	}
	//3 那么认为所有帧组织的commandlist都提交给commandQueue并且执行完毕了
	// 可以新建resource并且进行copy ->使用新的device的copy queue
	backRefResourcesTransferFunc(m_BackRefs_Resource, "Resources");
	//4 因为资源更新了，所以指向资源的descriptor也要重建
	backRefTransferFunc(m_BackRefs_DescriptorHeap, "DescriptorHeaps");
	//5 新建command相关的object，重建但不恢复
	backRefTransferFunc(m_BackRefs_CommandAllocator, "CommandAllocator");
	backRefTransferFunc(m_BackRefs_CommandList, "CommandList,");
	backRefTransferFunc(m_BackRefs_Fence, "Fence");
	backRefTransferFunc(m_BackRefs_CommandQueue, "CommandQueue");

	//5.pReal substitute
	m_pReal = pNewDevice;
}

WrappedD3D12DescriptorHeap* WrappedD3D12Device::findInBackRefDescriptorHeaps(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
	//TODO 加速结构
	for (auto it = m_BackRefs_DescriptorHeap.begin(); it != m_BackRefs_DescriptorHeap.end(); it++) {
		if (it->second) {
			WrappedD3D12DescriptorHeap* pWrappedHeap = static_cast<WrappedD3D12DescriptorHeap*>(it->second);
			auto start = pWrappedHeap->GetCPUDescriptorHandleForHeapStart();
			auto offset = pWrappedHeap->GetDesc().NumDescriptors;
			auto interval = GetReal()->GetDescriptorHandleIncrementSize(pWrappedHeap->GetDesc().Type);
			if (handle.ptr >= start.ptr && handle.ptr < start.ptr + offset * interval) {
				return pWrappedHeap;
			}
		}
	}
	return NULL;
}

/************************************************************************/
/*                         override                                                                     */
/************************************************************************/
HRESULT STDMETHODCALLTYPE WrappedD3D12Device::QueryInterface(REFIID riid, void** ppvObject){
	if (riid == __uuidof(ID3D12InfoQueue))
	{
		*ppvObject = static_cast<ID3D12InfoQueue*>(&m_DummyInfoQueue);
		m_DummyInfoQueue.AddRef();
		return S_OK;
	}
	return WrappedD3D12Object::QueryInterface(riid, ppvObject);
}


//////////////////////////////////////////////////////////////////////////Create D3D12 Object
HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandQueue(
	_In_  const D3D12_COMMAND_QUEUE_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_  void **ppCommandQueue){
	if (ppCommandQueue == NULL)
		return GetReal()->CreateCommandQueue(pDesc, riid, NULL);

	COMPtr<ID3D12CommandQueue> pCommandQueue = NULL;
	HRESULT ret = GetReal()->CreateCommandQueue(pDesc, IID_PPV_ARGS(&pCommandQueue));
	if (!FAILED(ret)) {
		WrappedD3D12CommandQueue* wrapped = new WrappedD3D12CommandQueue(pCommandQueue.Get(), this);
		*ppCommandQueue = static_cast<ID3D12CommandQueue*>(wrapped);
		m_BackRefs_CommandQueue[pCommandQueue.Get()] = wrapped;
	}
	else {
		*ppCommandQueue = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandAllocator(
	_In_  D3D12_COMMAND_LIST_TYPE type,
	REFIID riid,
	_COM_Outptr_  void **ppCommandAllocator){
	if (ppCommandAllocator == NULL)
		return GetReal()->CreateCommandAllocator(type, riid, NULL);

	COMPtr < ID3D12CommandAllocator> pCommandAllocator = NULL;
	HRESULT ret = GetReal()->CreateCommandAllocator(type, IID_PPV_ARGS(&pCommandAllocator));
	if (!FAILED(ret)) {
		WrappedD3D12CommandAllocator* wrapped = new WrappedD3D12CommandAllocator(pCommandAllocator.Get(), this, type);
		*ppCommandAllocator = static_cast<ID3D12CommandAllocator *>(wrapped);
		m_BackRefs_CommandAllocator[pCommandAllocator.Get()] = wrapped;
	}
	else {
		*ppCommandAllocator = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateGraphicsPipelineState(
	_In_  const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_  void **ppPipelineState){
	if (ppPipelineState == NULL)
		return GetReal()->CreateGraphicsPipelineState(pDesc, riid, NULL);

	COMPtr < ID3D12PipelineState> pPipelineState = NULL;
	auto pWrappedRootSignature = static_cast<WrappedD3D12RootSignature*>(pDesc->pRootSignature);//这里因为Desc里指向的RootSignature是Wrapped的，所以他要做替换
	D3D12_GRAPHICS_PIPELINE_STATE_DESC  tmpDesc = *pDesc;
	tmpDesc.pRootSignature = pWrappedRootSignature->GetReal().Get();
	HRESULT ret = GetReal()->CreateGraphicsPipelineState(&tmpDesc, IID_PPV_ARGS(&pPipelineState));
	if (!FAILED(ret)) {
		WrappedD3D12PipelineState* wrapped = new WrappedD3D12PipelineState(pPipelineState.Get(), this,
			tmpDesc, pWrappedRootSignature
		);
		*ppPipelineState = static_cast<ID3D12PipelineState *>(wrapped);
		m_BackRefs_PipelineState[pPipelineState.Get()] = wrapped;
	}
	else {
		*ppPipelineState = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateComputePipelineState(
	_In_  const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_  void **ppPipelineState){
	if (ppPipelineState == NULL)
		return GetReal()->CreateComputePipelineState(pDesc, riid, NULL);

	COMPtr < ID3D12PipelineState> pPipelineState = NULL;
	auto pWrappedRootSignature = static_cast<WrappedD3D12RootSignature*>(pDesc->pRootSignature);//这里因为Desc里指向的RootSignature是Wrapped的，所以他要做替换
	D3D12_COMPUTE_PIPELINE_STATE_DESC  tmpDesc = *pDesc;
	tmpDesc.pRootSignature = pWrappedRootSignature->GetReal().Get();
	HRESULT ret = GetReal()->CreateComputePipelineState(&tmpDesc, IID_PPV_ARGS(&pPipelineState));
	if (!FAILED(ret)) {
		WrappedD3D12PipelineState* wrapped = new WrappedD3D12PipelineState(
			pPipelineState.Get(), this,
			tmpDesc, pWrappedRootSignature
			);
		*ppPipelineState = static_cast<ID3D12PipelineState *>(wrapped);
		m_BackRefs_PipelineState[pPipelineState.Get()] = wrapped;
	}
	else {
		*ppPipelineState = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandList(
	_In_  UINT nodeMask,
	_In_  D3D12_COMMAND_LIST_TYPE type,
	_In_  ID3D12CommandAllocator *pCommandAllocator,
	_In_opt_  ID3D12PipelineState *pInitialState,
	REFIID riid,
	_COM_Outptr_  void **ppCommandList){
	Assert(riid == __uuidof(ID3D12CommandList) || riid == __uuidof(ID3D12GraphicsCommandList));

	if (ppCommandList == NULL)
		return GetReal()->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid, NULL);

	if (riid == __uuidof(ID3D12CommandList)) {
		COMPtr < ID3D12CommandList> pCommandList = NULL;
		HRESULT ret = GetReal()->CreateCommandList(nodeMask, type, 
			static_cast<WrappedD3D12CommandAllocator *>(pCommandAllocator)->GetReal().Get(),
			static_cast<WrappedD3D12PipelineState *>(pInitialState)->GetReal().Get(),
			IID_PPV_ARGS(&pCommandList));
		if (!FAILED(ret)) {
			WrappedD3D12CommandList* wrapped = new WrappedD3D12CommandList(//默认这里的Allocator是Wrapped的
				pCommandList.Get(), this,
				static_cast<WrappedD3D12CommandAllocator* >(pCommandAllocator), nodeMask
			);
			*ppCommandList = static_cast<ID3D12CommandList*>(wrapped);
			m_BackRefs_CommandList[pCommandList.Get()] = wrapped;
		}
		else {
			*ppCommandList = NULL;
		}
		return ret;
	}
	else {
		COMPtr < ID3D12GraphicsCommandList> pCommandList = NULL;
		HRESULT ret = GetReal()->CreateCommandList(nodeMask, type,
			static_cast<WrappedD3D12CommandAllocator *>(pCommandAllocator)->GetReal().Get(),
			pInitialState?static_cast<WrappedD3D12PipelineState *>(pInitialState)->GetReal().Get():NULL,
			IID_PPV_ARGS(&pCommandList));
		if (!FAILED(ret)) {
			WrappedD3D12GraphicsCommansList* wrapped = new WrappedD3D12GraphicsCommansList(//默认这里的Allocator是Wrapped的
				pCommandList.Get(), this,
				static_cast<WrappedD3D12CommandAllocator*>(pCommandAllocator), nodeMask
			);
			*ppCommandList = static_cast<ID3D12GraphicsCommandList*>(wrapped);
			m_BackRefs_CommandList[pCommandList.Get()] = wrapped;
		}
		else {
			*ppCommandList = NULL;
		}
		return ret;
	}
	
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateDescriptorHeap(
	_In_  const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
	REFIID riid,
	_COM_Outptr_  void **ppvHeap){
	if (ppvHeap == NULL)
		return GetReal()->CreateDescriptorHeap(pDescriptorHeapDesc, riid, NULL);

	COMPtr<ID3D12DescriptorHeap> pvHeap = NULL;
	HRESULT ret = GetReal()->CreateDescriptorHeap(pDescriptorHeapDesc, IID_PPV_ARGS(&pvHeap));
	if (!FAILED(ret)) {
		WrappedD3D12DescriptorHeap* wrapped = new WrappedD3D12DescriptorHeap(pvHeap.Get(), this);
		*ppvHeap = static_cast<ID3D12DescriptorHeap*>(wrapped);
		m_BackRefs_DescriptorHeap[pvHeap.Get()] = wrapped;
	}
	else {
		*ppvHeap = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateRootSignature(
	_In_  UINT nodeMask,
	_In_reads_(blobLengthInBytes)  const void *pBlobWithRootSignature,
	_In_  SIZE_T blobLengthInBytes,
	REFIID riid,
	_COM_Outptr_  void **ppvRootSignature) {
	if (ppvRootSignature == NULL)
		return GetReal()->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid, NULL);

	COMPtr<ID3D12RootSignature> pvRootSignature = NULL;
	HRESULT ret = GetReal()->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, IID_PPV_ARGS(&pvRootSignature));
	if (!FAILED(ret)) {
		WrappedD3D12RootSignature* wrapped = new WrappedD3D12RootSignature(
			pvRootSignature.Get(), this,
			nodeMask, pBlobWithRootSignature, blobLengthInBytes
		);
		*ppvRootSignature = static_cast<ID3D12RootSignature*>(wrapped);
		m_BackRefs_RootSignature[pvRootSignature.Get()] = wrapped;
	}
	else {
		*ppvRootSignature = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommittedResource(
	_In_  const D3D12_HEAP_PROPERTIES *pHeapProperties,
	D3D12_HEAP_FLAGS HeapFlags,
	_In_  const D3D12_RESOURCE_DESC *pDesc,
	D3D12_RESOURCE_STATES InitialResourceState,
	_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
	REFIID riidResource,
	_COM_Outptr_opt_  void **ppvResource) {
	if (ppvResource == NULL)
		return GetReal()->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, riidResource, NULL);

	COMPtr<ID3D12Resource> pvResource = NULL;
	HRESULT ret = GetReal()->CreateCommittedResource(pHeapProperties, HeapFlags, pDesc, InitialResourceState, pOptimizedClearValue, IID_PPV_ARGS(&pvResource));
	if (!FAILED(ret)) {
		WrappedD3D12Resource* wrapped = new WrappedD3D12Resource(
			pvResource.Get(), this, 
			pHeapProperties,
			HeapFlags,
			pDesc,
			InitialResourceState,
			pOptimizedClearValue

		);
		*ppvResource = static_cast<ID3D12Resource*>(wrapped);
		m_BackRefs_Resource[pvResource.Get()] = wrapped;
	}
	else {
		*ppvResource = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateHeap(
	_In_  const D3D12_HEAP_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvHeap) {
	if (ppvHeap == NULL)
		return GetReal()->CreateHeap(pDesc, riid, NULL);

	COMPtr<ID3D12Heap> pvHeap = NULL;
	HRESULT ret = GetReal()->CreateHeap(pDesc, IID_PPV_ARGS(&pvHeap));
	if (!FAILED(ret)) {
		WrappedD3D12Heap* wrapped = new WrappedD3D12Heap(pvHeap.Get(), this);
		*ppvHeap = static_cast<ID3D12Heap*>(wrapped);
		m_BackRefs_Heap[pvHeap.Get()] = wrapped;
	}
	else {
		*ppvHeap = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreatePlacedResource(
	_In_  ID3D12Heap *pHeap,
	UINT64 HeapOffset,
	_In_  const D3D12_RESOURCE_DESC *pDesc,
	D3D12_RESOURCE_STATES InitialState,
	_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvResource) {
	if (ppvResource == NULL)
		return GetReal()->CreatePlacedResource(
			static_cast<WrappedD3D12Heap *>(pHeap)->GetReal().Get(), 
			HeapOffset, pDesc, InitialState, pOptimizedClearValue, riid, NULL);

	COMPtr<ID3D12Resource> pvResource = NULL;
	HRESULT ret = GetReal()->CreatePlacedResource(static_cast<WrappedD3D12Heap *>(pHeap)->GetReal().Get(), HeapOffset, pDesc, InitialState, pOptimizedClearValue, IID_PPV_ARGS(&pvResource));
	if (!FAILED(ret)) {
		WrappedD3D12Resource* wrapped = new WrappedD3D12Resource(
			pvResource.Get(), this, 
			static_cast<WrappedD3D12Heap*>(pHeap),
			HeapOffset,
			pDesc,
			InitialState,
			pOptimizedClearValue
		);
		*ppvResource = static_cast<ID3D12Resource*>(wrapped);
		m_BackRefs_Resource[pvResource.Get()] = wrapped;
	}
	else {
		*ppvResource = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateReservedResource(
	_In_  const D3D12_RESOURCE_DESC *pDesc,
	D3D12_RESOURCE_STATES InitialState,
	_In_opt_  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvResource) {
	if (ppvResource == NULL)
		return GetReal()->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, riid, NULL);

	COMPtr< ID3D12Resource> pvResource = NULL;
	HRESULT ret = GetReal()->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, IID_PPV_ARGS(&pvResource));
	if (!FAILED(ret)) {
		WrappedD3D12Resource* wrapped = new WrappedD3D12Resource(
			pvResource.Get(), this,
			pDesc,
			InitialState,
			pOptimizedClearValue
		);
		*ppvResource = static_cast<ID3D12Resource*>(wrapped);
		m_BackRefs_Resource[pvResource.Get()] = wrapped;
	}
	else {
		*ppvResource = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::OpenSharedHandle(
	_In_  HANDLE NTHandle,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvObj) {
	LogError("OpenSharedHandle not supported yet");
	return GetReal()->OpenSharedHandle(NTHandle, riid, ppvObj);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateFence(
	UINT64 InitialValue,
	D3D12_FENCE_FLAGS Flags,
	REFIID riid,
	_COM_Outptr_  void **ppFence) {
	if (ppFence == NULL)
		return GetReal()->CreateFence(InitialValue, Flags, riid, NULL);

	COMPtr< ID3D12Fence> pFence = NULL;
	HRESULT ret = GetReal()->CreateFence(InitialValue, Flags, IID_PPV_ARGS(&pFence));
	if (!FAILED(ret)) {
		WrappedD3D12Fence* wrapped = new WrappedD3D12Fence(pFence.Get(), this, InitialValue, Flags);
		*ppFence = static_cast<ID3D12Fence*>(wrapped);
		m_BackRefs_Fence[pFence.Get()] = wrapped;
	}
	else {
		*ppFence = NULL;
	}
	return ret;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateQueryHeap(
	_In_  const D3D12_QUERY_HEAP_DESC *pDesc,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvHeap) {
	LogError("Query Heap Not supported yet");
	return 0;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateCommandSignature(
	_In_  const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
	_In_opt_  ID3D12RootSignature *pRootSignature,
	REFIID riid,
	_COM_Outptr_opt_  void **ppvCommandSignature) {
	LogError("Command Signature Not supported yet");
	return 0;
}


//////////////////////////////////////////////////////////////////////////Create Descriptor
void STDMETHODCALLTYPE WrappedD3D12Device::CreateConstantBufferView(
	_In_opt_  const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pDesc);
	auto pWrappedHeap = findInBackRefDescriptorHeaps(DestDescriptor);
	if (!pWrappedHeap)
	{
		LogError("Error when create descriptor");
		return;
	}
	WrappedD3D12DescriptorHeap::DescriptorHeapSlotDesc slotDesc;
	slotDesc.viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_CBV;

	//TODO: sneaky 通过遍历res的GPU VADDR来找WrappedRes
	WrappedD3D12Resource* pWrappedD3D12Res = NULL;
	for (auto it = m_BackRefs_Resource.begin(); it != m_BackRefs_Resource.end(); it++) {
		if (it->second) {
			auto pTmpRes = static_cast<WrappedD3D12Resource*>(it->second);
			if (pTmpRes->GetGPUVirtualAddress() == pDesc->BufferLocation) {
				pWrappedD3D12Res = pTmpRes;
				break;
			}
		}
	}
	if (!pWrappedD3D12Res)
	{
		LogError("Error when create CBV descriptor");
		return; 
	}

	slotDesc.pWrappedD3D12Resource = pWrappedD3D12Res;
	slotDesc.concreteViewDesc.cbv = *pDesc;
	pWrappedHeap->cacheDescriptorCreateParam(slotDesc, DestDescriptor);

	GetReal()->CreateConstantBufferView(pDesc, DestDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateShaderResourceView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	auto pWrappedHeap = findInBackRefDescriptorHeaps(DestDescriptor);
	if (!pWrappedHeap)
	{
		LogError("Error when create descriptor");
		return;
	}
	WrappedD3D12DescriptorHeap::DescriptorHeapSlotDesc slotDesc;
	slotDesc.viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_SRV;
	slotDesc.pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	if (pDesc)
		slotDesc.concreteViewDesc.srv = *pDesc;
	else
		slotDesc.isViewDescNull = true;
	pWrappedHeap->cacheDescriptorCreateParam(slotDesc, DestDescriptor);

	GetReal()->CreateShaderResourceView(
		pResource?static_cast<WrappedD3D12Resource *>(pResource)->GetReal().Get():NULL,
		pDesc, 
		DestDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateUnorderedAccessView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  ID3D12Resource *pCounterResource,
	_In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	auto pWrappedHeap = findInBackRefDescriptorHeaps(DestDescriptor);
	if (!pWrappedHeap)
	{
		LogError("Error when create descriptor");
		return;
	}
	WrappedD3D12DescriptorHeap::DescriptorHeapSlotDesc slotDesc;
	slotDesc.viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_UAV;
	slotDesc.pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	slotDesc.pWrappedD3D12CounterResource = static_cast<WrappedD3D12Resource*>(pCounterResource);
	if (pDesc)
		slotDesc.concreteViewDesc.uav = *pDesc;
	else
		slotDesc.isViewDescNull = true;
	pWrappedHeap->cacheDescriptorCreateParam(slotDesc, DestDescriptor);

	GetReal()->CreateUnorderedAccessView(
		pResource?static_cast<WrappedD3D12Resource*>(pResource)->GetReal().Get():NULL, 
		pCounterResource?static_cast<WrappedD3D12Resource*>(pCounterResource)->GetReal().Get():pCounterResource,
		pDesc, DestDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateRenderTargetView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	auto pWrappedHeap = findInBackRefDescriptorHeaps(DestDescriptor);
	if (!pWrappedHeap)
	{
		LogError("Error when create descriptor");
		return;
	}
	WrappedD3D12DescriptorHeap::DescriptorHeapSlotDesc slotDesc;
	slotDesc.viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_RTV;
	slotDesc.pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	if (pDesc)
		slotDesc.concreteViewDesc.rtv = *pDesc;
	else
		slotDesc.isViewDescNull = true;
	pWrappedHeap->cacheDescriptorCreateParam(slotDesc, DestDescriptor);

	GetReal()->CreateRenderTargetView(
		pResource ? static_cast<WrappedD3D12Resource*>(pResource)->GetReal().Get() : NULL,
		pDesc, DestDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateDepthStencilView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	auto pWrappedHeap = findInBackRefDescriptorHeaps(DestDescriptor);
	if (!pWrappedHeap)
	{
		LogError("Error when create descriptor");
		return;
	}
	WrappedD3D12DescriptorHeap::DescriptorHeapSlotDesc slotDesc;
	slotDesc.viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_DSV;
	slotDesc.pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	if (pDesc)
		slotDesc.concreteViewDesc.dsv = *pDesc;
	else
		slotDesc.isViewDescNull = true;
	pWrappedHeap->cacheDescriptorCreateParam(slotDesc, DestDescriptor);

	GetReal()->CreateDepthStencilView(
		pResource ? static_cast<WrappedD3D12Resource*>(pResource)->GetReal().Get() : NULL,
		pDesc, DestDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateSampler(
	_In_  const D3D12_SAMPLER_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	auto pWrappedHeap = findInBackRefDescriptorHeaps(DestDescriptor);
	if (!pWrappedHeap)
	{
		LogError("Error when create descriptor");
		return;
	}
	WrappedD3D12DescriptorHeap::DescriptorHeapSlotDesc slotDesc;
	slotDesc.viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_SamplerV;
	slotDesc.concreteViewDesc.sampler = *pDesc;
	pWrappedHeap->cacheDescriptorCreateParam(slotDesc, DestDescriptor);

	GetReal()->CreateSampler(pDesc, DestDescriptor);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CopyDescriptors(
	_In_  UINT NumDestDescriptorRanges,
	_In_reads_(NumDestDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
	_In_reads_opt_(NumDestDescriptorRanges)  const UINT *pDestDescriptorRangeSizes,
	_In_  UINT NumSrcDescriptorRanges,
	_In_reads_(NumSrcDescriptorRanges)  const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
	_In_reads_opt_(NumSrcDescriptorRanges)  const UINT *pSrcDescriptorRangeSizes,
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) {

	UINT srcDescriptorNum = 0; UINT destDescriptorNum = 0;
	const SIZE_T interval = GetReal()->GetDescriptorHandleIncrementSize(DescriptorHeapsType);
	// analyze the cache for dest
	std::vector<WrappedD3D12DescriptorHeap*> destHeapsPtr(NumDestDescriptorRanges);
	std::vector<SIZE_T> destHeapsStartIndex(NumDestDescriptorRanges);
	for (UINT destRangeIndex = 0; destRangeIndex < NumDestDescriptorRanges; destRangeIndex++) {
		auto pDescHeap = findInBackRefDescriptorHeaps(pDestDescriptorRangeStarts[destRangeIndex]);
		Assert(pDescHeap&&pDestDescriptorRangeSizes[destRangeIndex] < pDescHeap->GetDesc().NumDescriptors
			&&pDescHeap->GetDesc().Type == DescriptorHeapsType
		);
		destDescriptorNum += pDestDescriptorRangeSizes[destRangeIndex];
		destHeapsPtr[destRangeIndex]=(pDescHeap);
		destHeapsStartIndex[destRangeIndex] = (pDestDescriptorRangeStarts[destRangeIndex].ptr - pDescHeap->GetCPUDescriptorHandleForHeapStart().ptr) / interval;
	}

	// analyze the cache for src//note pSrcDescriptorRangeSizes Can be NULL
	std::vector<WrappedD3D12DescriptorHeap*> srcHeapsPtr(NumSrcDescriptorRanges);
	std::vector<SIZE_T> srcHeapsStartIndex(NumSrcDescriptorRanges);
	std::vector<UINT> tmpSrcDescriptorRangeSizes(NumSrcDescriptorRanges);
	for (UINT srcRangeIndex = 0; srcRangeIndex < NumSrcDescriptorRanges; srcRangeIndex++) {
		auto pDescHeap = findInBackRefDescriptorHeaps(pSrcDescriptorRangeStarts[srcRangeIndex]);
		Assert(pDescHeap&&pDescHeap->GetDesc().Type == DescriptorHeapsType);
		if (pSrcDescriptorRangeSizes) {//pSrcDescriptorRangeSizes can be NULL here 
			Assert(pSrcDescriptorRangeSizes[srcRangeIndex] < pDescHeap->GetDesc().NumDescriptors	);
			srcDescriptorNum += pSrcDescriptorRangeSizes[srcRangeIndex];
		}
		else {
			tmpSrcDescriptorRangeSizes[srcRangeIndex] = pDestDescriptorRangeSizes[srcRangeIndex];
		}
		srcHeapsPtr[srcRangeIndex] = (pDescHeap);
		srcHeapsStartIndex[srcRangeIndex] = (pSrcDescriptorRangeStarts[srcRangeIndex].ptr - pDescHeap->GetCPUDescriptorHandleForHeapStart().ptr) / interval;
	}
	if (srcDescriptorNum == 0)
		srcDescriptorNum = destDescriptorNum;
	Assert(srcDescriptorNum == destDescriptorNum);

	struct RangeIndexImpl { // Implementation of the range index pivot
		UINT rangeNum = 0;
		UINT indexInRange = 0;
		RangeIndexImpl(UINT  _NumRanges, const UINT* _pRangeSizes) :
			NumRanges(_NumRanges), pRangeSizes(_pRangeSizes) {};
		bool rangeNext(){
			if (rangeNum >= NumRanges) return false;
			indexInRange++;
			if (indexInRange >= pRangeSizes[rangeNum]) {
				indexInRange = 0;
				rangeNum++;
			}
			if (rangeNum >= NumRanges) return false;
			return true;
		};
	private:
		UINT NumRanges;
		const UINT* pRangeSizes;
	};
	RangeIndexImpl srcRangeIndex(NumSrcDescriptorRanges, pSrcDescriptorRangeSizes? pSrcDescriptorRangeSizes: tmpSrcDescriptorRangeSizes.data());
	RangeIndexImpl destRangeIndex(NumDestDescriptorRanges, pDestDescriptorRangeSizes);

	do {
		auto pSrcDescriptorHeap = srcHeapsPtr[srcRangeIndex.rangeNum];
		auto srcStart = srcHeapsStartIndex[srcRangeIndex.rangeNum];

		auto pDestDescriptorHeap =destHeapsPtr[destRangeIndex.rangeNum];
		auto destStart = destHeapsStartIndex[destRangeIndex.rangeNum];

		auto& srcDescriptorCreateParamCache = pSrcDescriptorHeap->getDescriptorCreateParamCache(srcStart + srcRangeIndex.indexInRange);
		pDestDescriptorHeap->cacheDescriptorCreateParamByIndex(srcDescriptorCreateParamCache, destStart+ destRangeIndex.indexInRange);
	} while (srcRangeIndex.rangeNext() && destRangeIndex.rangeNext());

	return GetReal()->CopyDescriptors(NumDestDescriptorRanges,
		pDestDescriptorRangeStarts,
		pDestDescriptorRangeSizes,
		NumSrcDescriptorRanges,
		pSrcDescriptorRangeStarts,
		pSrcDescriptorRangeSizes,
		DescriptorHeapsType);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CopyDescriptorsSimple(
	_In_  UINT NumDescriptors,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) {
	auto srcDescriptorHeap = findInBackRefDescriptorHeaps(SrcDescriptorRangeStart);
	auto destDescriptorHeap = findInBackRefDescriptorHeaps(DestDescriptorRangeStart);

	Assert(DescriptorHeapsType == srcDescriptorHeap->GetDesc().Type
		&&DescriptorHeapsType == destDescriptorHeap->GetDesc().Type);
	
	auto interval = GetReal()->GetDescriptorHandleIncrementSize(DescriptorHeapsType);
	auto srcIndex = (SrcDescriptorRangeStart.ptr - srcDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr) / interval;
	auto destIndex = (DestDescriptorRangeStart.ptr - destDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr) / interval;

	for (UINT i = 0; i < NumDescriptors; i++) { //TODO: change with memcopy for the whole slot
		auto& srcDescriptorCreateParamCache = srcDescriptorHeap->getDescriptorCreateParamCache(i + srcIndex);
		destDescriptorHeap->cacheDescriptorCreateParamByIndex(srcDescriptorCreateParamCache, i + destIndex);
	}
	GetReal()->CopyDescriptorsSimple(NumDescriptors, DestDescriptorRangeStart, SrcDescriptorRangeStart, DescriptorHeapsType);
}

//////////////////////////////////////////////////////////////////////////Miscellaneous 
UINT STDMETHODCALLTYPE WrappedD3D12Device::GetNodeCount(void) {
	return GetReal()->GetNodeCount();
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CheckFeatureSupport(
	D3D12_FEATURE Feature,
	_Inout_updates_bytes_(FeatureSupportDataSize)  void *pFeatureSupportData,
	UINT FeatureSupportDataSize){
	return GetReal()->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

UINT STDMETHODCALLTYPE WrappedD3D12Device::GetDescriptorHandleIncrementSize(
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType){
	return GetReal()->GetDescriptorHandleIncrementSize(DescriptorHeapType);
}

D3D12_RESOURCE_ALLOCATION_INFO STDMETHODCALLTYPE WrappedD3D12Device::GetResourceAllocationInfo(
	_In_  UINT visibleMask,
	_In_  UINT numResourceDescs,
	_In_reads_(numResourceDescs)  const D3D12_RESOURCE_DESC *pResourceDescs){
	return  GetReal()->GetResourceAllocationInfo(visibleMask, numResourceDescs, pResourceDescs);
}

D3D12_HEAP_PROPERTIES STDMETHODCALLTYPE WrappedD3D12Device::GetCustomHeapProperties(
	_In_  UINT nodeMask,
	D3D12_HEAP_TYPE heapType){
	return  GetReal()->GetCustomHeapProperties(nodeMask, heapType);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::CreateSharedHandle(
	_In_  ID3D12DeviceChild *pObject,
	_In_opt_  const SECURITY_ATTRIBUTES *pAttributes,
	DWORD Access,
	_In_opt_  LPCWSTR Name,
	_Out_  HANDLE *pHandle){
	LogError("CreateSharedHandle not supported yet");
	return NULL;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::OpenSharedHandleByName(
	_In_  LPCWSTR Name,
	DWORD Access,
	/* [annotation][out] */
	_Out_  HANDLE *pNTHandle) {
	LogError("OpenSharedHandleByName not supported yet");
	//return GetReal()->OpenSharedHandleByName(Name, Access, pNTHandle);
	return NULL;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::MakeResident(
	UINT NumObjects,
	_In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects){
	LogError("MakeResident not supported yet");
	//return GetReal()->MakeResident(NumObjects, ppObjects);
	return NULL;
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::Evict(
	UINT NumObjects,
	_In_reads_(NumObjects)  ID3D12Pageable *const *ppObjects){
	LogError("Evict not supported yet");
	return NULL;
	//return GetReal()->MakeResident(NumObjects, ppObjects);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::GetDeviceRemovedReason(void){
	return GetReal()->GetDeviceRemovedReason();
}

void STDMETHODCALLTYPE WrappedD3D12Device::GetCopyableFootprints(
	_In_  const D3D12_RESOURCE_DESC *pResourceDesc,
	_In_range_(0, D3D12_REQ_SUBRESOURCES)  UINT FirstSubresource,
	_In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource)  UINT NumSubresources,
	UINT64 BaseOffset,
	_Out_writes_opt_(NumSubresources)  D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
	_Out_writes_opt_(NumSubresources)  UINT *pNumRows,
	_Out_writes_opt_(NumSubresources)  UINT64 *pRowSizeInBytes,
	_Out_opt_  UINT64 *pTotalBytes){
	return GetReal()->GetCopyableFootprints(
		pResourceDesc, FirstSubresource, NumSubresources, BaseOffset, pLayouts, pNumRows, pRowSizeInBytes, pTotalBytes);
}

HRESULT STDMETHODCALLTYPE WrappedD3D12Device::SetStablePowerState(
	BOOL Enable){
	return GetReal()->SetStablePowerState(Enable);
}

void STDMETHODCALLTYPE WrappedD3D12Device::GetResourceTiling(
	_In_  ID3D12Resource *pTiledResource,
	_Out_opt_  UINT *pNumTilesForEntireResource,
	_Out_opt_  D3D12_PACKED_MIP_INFO *pPackedMipDesc,
	_Out_opt_  D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
	_Inout_opt_  UINT *pNumSubresourceTilings,
	_In_  UINT FirstSubresourceTilingToGet,
	_Out_writes_(*pNumSubresourceTilings)  D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips){
	return GetReal()->GetResourceTiling(
		static_cast<WrappedD3D12Resource *>(pTiledResource)->GetReal().Get()
		, pNumTilesForEntireResource, pPackedMipDesc,
		pStandardTileShapeForNonPackedMips, pNumSubresourceTilings, 
		FirstSubresourceTilingToGet, pSubresourceTilingsForNonPackedMips
	);
}

LUID STDMETHODCALLTYPE WrappedD3D12Device::GetAdapterLuid(void){
	return  GetReal()->GetAdapterLuid();
}

RDCBOOST_NAMESPACE_END

