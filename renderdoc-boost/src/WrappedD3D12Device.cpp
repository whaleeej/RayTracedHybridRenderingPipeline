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
	auto resPivot = m_BackRefs_Resource.find(pReal);
	if (resPivot != m_BackRefs_Resource.end()) {
		m_BackRefs_Resource_Reflection.erase(resPivot->second);
		m_BackRefs_Resource.erase(pReal);
		return;
	}
		
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

	//for (auto it = m_BackRefs_DescriptorHeap.begin(); it != m_BackRefs_DescriptorHeap.end(); it++) {
	//	auto descriptorHeap = static_cast<WrappedD3D12DescriptorHeap*>(it->second);
	//	auto desc = descriptorHeap->GetDesc();
	//	auto flag = desc.Flags;
	//	auto start = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//	auto interval = GetDescriptorHandleIncrementSize(desc.Type);
	//	auto size = desc.NumDescriptors;
	//	auto end = start.ptr + interval * size;
	//	printf("");
	//}


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
			size_t counter = m_BackRefs.size();
			ID3D12Device* pOldDevice = GetReal().Get();
			// pool
			std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> newBackRefs; //新的资源->Wrapper
			std::vector<COMPtr<ID3D12Resource>> oldBackRefsSrc(counter); // old device
			std::vector<COMPtr<ID3D12Resource>> oldBackRefsReadback(counter);// old device
			std::vector<COMPtr<ID3D12Resource>> newBackRefsUpload(counter); // new device
			std::vector<COMPtr<ID3D12Resource>> newBackRefsDest(counter); // new device
			std::vector<UINT64> requiredRefsSize(counter);
			
			D3D12_COMMAND_QUEUE_DESC commandQueueDesc={
			/*.Type = */D3D12_COMMAND_LIST_TYPE_DIRECT,
			/*.Priority =*/ D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
			/*.Flags =*/ D3D12_COMMAND_QUEUE_FLAG_NONE,
			/*.NodeMask =*/ 0 };
			// command series old
			COMPtr<ID3D12CommandQueue>copyCommandQueue;
			pOldDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&copyCommandQueue));
			COMPtr<ID3D12CommandAllocator> copyCommandAllocator;
			pOldDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&copyCommandAllocator));
			COMPtr<ID3D12GraphicsCommandList> copyCommandList;
			pOldDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, copyCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&copyCommandList));
			uint64_t copyFenceValue = 0;
			COMPtr<ID3D12Fence> copyFence;
			pOldDevice->CreateFence(copyFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence));
			// command series new
			COMPtr<ID3D12CommandQueue>copyCommandQueue1;
			pNewDevice->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&copyCommandQueue1));
			COMPtr<ID3D12CommandAllocator> copyCommandAllocator1;
			pNewDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&copyCommandAllocator1));
			COMPtr<ID3D12GraphicsCommandList> copyCommandList1;
			pNewDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, copyCommandAllocator1.Get(), nullptr, IID_PPV_ARGS(&copyCommandList1));
			uint64_t copyFenceValue1 = 0;
			COMPtr<ID3D12Fence> copyFence1;
			pNewDevice->CreateFence(copyFenceValue1, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&copyFence1));
			// setup and framework
			int progress = 0; int idx = 0; size_t counterReal = 0;
			for (auto it = m_BackRefs.begin(); it != m_BackRefs.end(); it++) {
				COMPtr<ID3D12Resource> tmpOldResource = (static_cast<ID3D12Resource*>(it->first));
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
				printf("\n");

				if (!static_cast<WrappedD3D12Resource*>(it->second)->needCopy()) continue;
				D3D12_RESOURCE_DESC desc = tmpOldResource->GetDesc();

				// 缓存旧资源的ptr，使用comptr来保存生命
				oldBackRefsSrc[counterReal] = tmpOldResource;
				// 查询footprint
				UINT64 NumSubresources = desc.MipLevels;
				UINT64 RequiredSize = 0;
				UINT64 MemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * NumSubresources;
				Assert(MemToAlloc <= SIZE_MAX);
				void* pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
				Assert(pMem);
				D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT*>(pMem);
				UINT64* pRowSizesInBytes = reinterpret_cast<UINT64*>(pLayouts + NumSubresources);
				UINT* pNumRows = reinterpret_cast<UINT*>(pRowSizesInBytes + NumSubresources);
				pOldDevice->GetCopyableFootprints(&desc, 0, NumSubresources, 0, pLayouts, pNumRows, pRowSizesInBytes, &RequiredSize);
				requiredRefsSize[counterReal] = RequiredSize;
				// 生成readback resource buffer
				{
					D3D12_HEAP_PROPERTIES properties = { D3D12_HEAP_TYPE_READBACK, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
					pOldDevice->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
						&CD3DX12_RESOURCE_DESC::Buffer(RequiredSize), D3D12_RESOURCE_STATE_COPY_DEST,
						NULL, IID_PPV_ARGS(&oldBackRefsReadback[counterReal])
					);
				}
				// 生成upload resource buffer
				{
					D3D12_HEAP_PROPERTIES properties = { D3D12_HEAP_TYPE_UPLOAD, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };
					pNewDevice->CreateCommittedResource(&properties, D3D12_HEAP_FLAG_NONE,
						&CD3DX12_RESOURCE_DESC::Buffer(RequiredSize), D3D12_RESOURCE_STATE_GENERIC_READ,
						NULL, IID_PPV_ARGS(&newBackRefsUpload[counterReal])
					);
				}
				//缓存目标资源
				newBackRefsDest[counterReal] = static_cast<ID3D12Resource*>(it->second->GetRealObject().Get());

				if (static_cast<WrappedD3D12Resource*>(it->second)->queryState() != D3D12_RESOURCE_STATE_COPY_SOURCE)
				{
					//enque commandlist
					D3D12_RESOURCE_BARRIER srcResourceBarrier;
					ZeroMemory(&srcResourceBarrier, sizeof(srcResourceBarrier));
					srcResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					srcResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					srcResourceBarrier.Transition.pResource = oldBackRefsSrc[counterReal].Get();
					srcResourceBarrier.Transition.StateBefore = static_cast<WrappedD3D12Resource*>(it->second)->queryState();
					srcResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
					srcResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					copyCommandList->ResourceBarrier(1, &srcResourceBarrier);
				}
				//copyCommandList->CopyResource(oldBackRefsReadback[counterReal].Get(), oldBackRefsSrc[counterReal].Get());
				//copyCommandList1->CopyResource(newBackRefsDest[counterReal].Get(), newBackRefsUpload[counterReal].Get());
				// copy from old ref to readback
				if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
					copyCommandList->CopyBufferRegion(
						oldBackRefsReadback[counterReal].Get(), 0, oldBackRefsSrc[counterReal].Get(), pLayouts[0].Offset, pLayouts[0].Footprint.Width);
				}
				else {
					for (UINT i = 0; i < NumSubresources; ++i)
					{
						CD3DX12_TEXTURE_COPY_LOCATION Dst(oldBackRefsReadback[counterReal].Get(), pLayouts[i]);
						CD3DX12_TEXTURE_COPY_LOCATION Src(oldBackRefsSrc[counterReal].Get(), i+0);
						copyCommandList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
					}
				}
				// copy from upload to new ref
				if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
					copyCommandList1->CopyBufferRegion(
						newBackRefsDest[counterReal].Get(), 0, newBackRefsUpload[counterReal].Get(), pLayouts[0].Offset, pLayouts[0].Footprint.Width);
				}
				else {
					for (UINT i = 0; i < NumSubresources; ++i)
					{
						CD3DX12_TEXTURE_COPY_LOCATION Dst(newBackRefsDest[counterReal].Get(), i + 0);
						CD3DX12_TEXTURE_COPY_LOCATION Src(newBackRefsUpload[counterReal].Get(), pLayouts[i]);
						copyCommandList1->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
					}
				}
				if (D3D12_RESOURCE_STATE_COPY_DEST != static_cast<WrappedD3D12Resource*>(it->second)->queryState())
				{
					D3D12_RESOURCE_BARRIER destResourceBarrier;
					ZeroMemory(&destResourceBarrier, sizeof(destResourceBarrier));
					destResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
					destResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
					destResourceBarrier.Transition.pResource = newBackRefsDest[counterReal].Get();
					destResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
					destResourceBarrier.Transition.StateAfter = static_cast<WrappedD3D12Resource*>(it->second)->queryState();
					destResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
					copyCommandList1->ResourceBarrier(1, &destResourceBarrier);
				}			
				++counterReal;
				HeapFree(GetProcessHeap(), 0, pMem);
			}

			{ // execute commandqueue
				std::vector<ID3D12CommandList* > listsToExec(1);
				listsToExec[0] = copyCommandList.Get();
				copyCommandList->Close();
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
			}
			// CPU copy
			for (size_t i = 0; i < counterReal; i++) {
				D3D12_RESOURCE_DESC& desc = oldBackRefsSrc[i]->GetDesc();
				UINT64 requiredSize = requiredRefsSize[i];
				BYTE* pSrcData=0; BYTE* pDestData=0;
				HRESULT hr = oldBackRefsReadback[i]->Map(0, NULL, reinterpret_cast<void**>(&pSrcData));
				HRESULT hr1 = newBackRefsUpload[i]->Map(0, NULL, reinterpret_cast<void**>(&pDestData));
				Assert(!FAILED(hr) && !FAILED(hr1)&&pSrcData&&pDestData);
				memcpy(pDestData, pSrcData, requiredSize);
				oldBackRefsReadback[i]->Unmap(0, NULL);
				newBackRefsUpload[i]->Unmap(0, NULL);
			}
			{ // execute commandqueue1
				std::vector<ID3D12CommandList* > listsToExec(1);
				listsToExec[0] = copyCommandList1.Get();
				copyCommandList1->Close();
				copyCommandQueue1->ExecuteCommandLists(1, listsToExec.data());
				copyCommandQueue1->Signal(copyFence1.Get(), ++copyFenceValue1);
				if ((copyFence1->GetCompletedValue() < copyFenceValue1)) //等待copy queue执行完毕
				{
					auto event = ::CreateEvent(NULL, FALSE, FALSE, NULL);
					Assert(event && "Failed to create fence event handle.");

					// Is this function thread safe?
					copyFence1->SetEventOnCompletion(copyFenceValue1, event);
					::WaitForSingleObject(event, DWORD_MAX);

					::CloseHandle(event);
				}
			}
			m_BackRefs.swap(newBackRefs);
		}
	};
	//2 要确保所有的fence进行Signal的Value都已经Complete了
	for (auto it = m_BackRefs_Fence.begin(); it != m_BackRefs_Fence.end(); it++) {
		auto pWrappedFence = static_cast<WrappedD3D12Fence*>(it->second);
		pWrappedFence->WaitToCompleteApplicationFenceValue();
	}

	//0 切换RootSignature和依赖于Root Signature的Pipeline State
	backRefTransferFunc(m_BackRefs_RootSignature, "RootSignatures");
	backRefTransferFunc(m_BackRefs_PipelineState, "PipelineStates");
	//1 切换保存资源的resource heap
	backRefTransferFunc(m_BackRefs_Heap, "Heaps"); 
	
	//3 那么认为所有帧组织的commandlist都提交给commandQueue并且执行完毕了
	// 可以新建resource并且进行copy ->使用新的device的copy queue
	backRefResourcesTransferFunc(m_BackRefs_Resource, "Resources");
	//5 新建command相关的object，重建但不恢复
	backRefTransferFunc(m_BackRefs_CommandAllocator, "CommandAllocator");
	backRefTransferFunc(m_BackRefs_CommandList, "CommandList,");
	backRefTransferFunc(m_BackRefs_Fence, "Fence");
	backRefTransferFunc(m_BackRefs_CommandQueue, "CommandQueue");
	//4 因为资源更新了，所以指向资源的descriptor也要重建
	backRefTransferFunc(m_BackRefs_DescriptorHeap, "DescriptorHeaps");

	//5.pReal substitute
	m_pReal = pNewDevice;
}

bool WrappedD3D12Device::isResourceExist(WrappedD3D12Resource* pWrappedResource) {
	if (m_BackRefs_Resource_Reflection.find(pWrappedResource) != m_BackRefs_Resource_Reflection.end())
		return true;
	for (auto it = m_BackRefs_CommandQueue.begin(); it != m_BackRefs_CommandQueue.end(); it++) {
		if (static_cast<WrappedD3D12CommandQueue*>(it->second)->isResourceExist(pWrappedResource)) {
			return true;
		}
	}
	return false;
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
		m_BackRefs_Resource_Reflection.emplace(wrapped);
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
		m_BackRefs_Resource_Reflection.emplace(wrapped);
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
		m_BackRefs_Resource_Reflection.emplace(wrapped);
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
	Assert(pDesc);//TODO 解析GPU VADDR
	ANALYZE_WRAPPED_SLOT(pSlot, DestDescriptor);
	pSlot->viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_CBV;
	pSlot->concreteViewDesc.cbv = *pDesc;
	pSlot->isViewDescNull = false;
	GetReal()->CreateConstantBufferView(pDesc, ANALYZE_WRAPPED_CPU_HANDLE(DestDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateShaderResourceView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	ANALYZE_WRAPPED_SLOT(pSlot, DestDescriptor);
	pSlot->viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_SRV;
	pSlot->pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	if (pDesc) {
		pSlot->concreteViewDesc.srv = *pDesc;
		pSlot->isViewDescNull = false;
	}
	else
		pSlot->isViewDescNull = true;
	GetReal()->CreateShaderResourceView(
		pResource?static_cast<WrappedD3D12Resource *>(pResource)->GetReal().Get():NULL,
		pDesc, 
		ANALYZE_WRAPPED_CPU_HANDLE(DestDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateUnorderedAccessView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  ID3D12Resource *pCounterResource,
	_In_opt_  const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	ANALYZE_WRAPPED_SLOT(pSlot, DestDescriptor);
	pSlot->viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_UAV;
	pSlot->pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	pSlot->pWrappedD3D12CounterResource = static_cast<WrappedD3D12Resource*>(pCounterResource);
	if (pDesc) {
		pSlot->concreteViewDesc.uav = *pDesc;
		pSlot->isViewDescNull = false;
	}
	else
		pSlot->isViewDescNull = true;
	GetReal()->CreateUnorderedAccessView(
		pResource?static_cast<WrappedD3D12Resource*>(pResource)->GetReal().Get():NULL, 
		pCounterResource?static_cast<WrappedD3D12Resource*>(pCounterResource)->GetReal().Get():pCounterResource,
		pDesc, 
		ANALYZE_WRAPPED_CPU_HANDLE(DestDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateRenderTargetView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	ANALYZE_WRAPPED_SLOT(pSlot, DestDescriptor);
	pSlot->viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_RTV;
	pSlot->pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	if (pDesc) {
		pSlot->isViewDescNull = false;
		pSlot->concreteViewDesc.rtv = *pDesc;
	}
	else
		pSlot->isViewDescNull = true;
	GetReal()->CreateRenderTargetView(
		pResource ? static_cast<WrappedD3D12Resource*>(pResource)->GetReal().Get() : NULL,
		pDesc, 
		ANALYZE_WRAPPED_CPU_HANDLE(DestDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateDepthStencilView(
	_In_opt_  ID3D12Resource *pResource,
	_In_opt_  const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	Assert(pResource != NULL || pDesc != NULL);
	ANALYZE_WRAPPED_SLOT(pSlot, DestDescriptor);
	pSlot->viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_DSV;
	pSlot->pWrappedD3D12Resource = static_cast<WrappedD3D12Resource*>(pResource);
	if (pDesc) {
		pSlot->isViewDescNull = false;
		pSlot->concreteViewDesc.dsv = *pDesc;
	}
	else
		pSlot->isViewDescNull = true;
	GetReal()->CreateDepthStencilView(
		pResource ? static_cast<WrappedD3D12Resource*>(pResource)->GetReal().Get() : NULL,
		pDesc, 
		ANALYZE_WRAPPED_CPU_HANDLE(DestDescriptor));
}

void STDMETHODCALLTYPE WrappedD3D12Device::CreateSampler(
	_In_  const D3D12_SAMPLER_DESC *pDesc,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor) {
	ANALYZE_WRAPPED_SLOT(pSlot, DestDescriptor);
	pSlot->viewDescType = WrappedD3D12DescriptorHeap::ViewDesc_SamplerV;
	pSlot->concreteViewDesc.sampler = *pDesc;
	pSlot->isViewDescNull = false;
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
	std::vector<UINT> destNullRangeSizes(NumDestDescriptorRanges, 1);
	std::vector<UINT>  srcNullRangeSizes(NumSrcDescriptorRanges, 1);
	if(pDestDescriptorRangeSizes==NULL){
		pDestDescriptorRangeSizes = destNullRangeSizes.data();
	}
	if (pSrcDescriptorRangeSizes == NULL) {
		pSrcDescriptorRangeSizes = srcNullRangeSizes.data();
	}

	UINT srcDescriptorNum = 0; UINT destDescriptorNum = 0;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> pRealDestHandleStarts(NumDestDescriptorRanges);
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> pRealSrcHandleStarts(NumSrcDescriptorRanges);
	for (UINT destRangeIndex = 0; destRangeIndex < NumDestDescriptorRanges; destRangeIndex++) {
		ANALYZE_WRAPPED_SLOT(pSlot, pDestDescriptorRangeStarts[destRangeIndex]);
		Assert(pDestDescriptorRangeSizes[destRangeIndex] + pSlot->idx < pSlot->m_DescriptorHeap->GetDesc().NumDescriptors
			&&pSlot->m_DescriptorHeap->GetDesc().Type == DescriptorHeapsType);
		destDescriptorNum += pDestDescriptorRangeSizes[destRangeIndex];
		pRealDestHandleStarts[destRangeIndex] = pSlot->getRealCPUHandle();
	}
	for (UINT srcRangeIndex = 0; srcRangeIndex < NumSrcDescriptorRanges; srcRangeIndex++) {
		ANALYZE_WRAPPED_SLOT(pSlot, pSrcDescriptorRangeStarts[srcRangeIndex]);
		Assert(pSrcDescriptorRangeSizes[srcRangeIndex] + pSlot->idx < pSlot->m_DescriptorHeap->GetDesc().NumDescriptors
			&&pSlot->m_DescriptorHeap->GetDesc().Type == DescriptorHeapsType);
		srcDescriptorNum += pSrcDescriptorRangeSizes[srcRangeIndex];
		pRealSrcHandleStarts[srcRangeIndex] = pSlot->getRealCPUHandle();
	}
	Assert(srcDescriptorNum == destDescriptorNum);

	struct RangeIndexImpl { // Implementation of the range index pivot
		UINT rangeNum = 0;
		UINT indexInRange = 0;
		RangeIndexImpl(UINT  _NumRanges, const UINT* _pRangeSizes, const D3D12_CPU_DESCRIPTOR_HANDLE *_pDescriptorRangeStarts) :
			NumRanges(_NumRanges), pRangeSizes(_pRangeSizes),
			pDescriptorRangeStarts(_pDescriptorRangeStarts)
		{};

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

		WrappedD3D12DescriptorHeap::DescriptorHeapSlot& getSlot() {
			ANALYZE_WRAPPED_SLOT(pSlot, pDescriptorRangeStarts[rangeNum]);
			return *(pSlot + indexInRange);
		}

	private:
		UINT NumRanges;
		const UINT* pRangeSizes;
		const D3D12_CPU_DESCRIPTOR_HANDLE *pDescriptorRangeStarts;
	};
	RangeIndexImpl srcRangeIndex(NumSrcDescriptorRanges, pSrcDescriptorRangeSizes, pSrcDescriptorRangeStarts);
	RangeIndexImpl destRangeIndex(NumDestDescriptorRanges, pDestDescriptorRangeSizes, pDestDescriptorRangeStarts);

	do {
		auto& src = srcRangeIndex.getSlot();
		auto& dest = destRangeIndex.getSlot();
		dest = src;
	} while (srcRangeIndex.rangeNext() && destRangeIndex.rangeNext());

	return GetReal()->CopyDescriptors(
		NumDestDescriptorRanges,
		pRealDestHandleStarts.data(),
		pDestDescriptorRangeSizes,
		NumSrcDescriptorRanges,
		pRealSrcHandleStarts.data(),
		pSrcDescriptorRangeSizes,
		DescriptorHeapsType);
}

void STDMETHODCALLTYPE WrappedD3D12Device::CopyDescriptorsSimple(
	_In_  UINT NumDescriptors,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
	_In_  D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
	_In_  D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType) {
	ANALYZE_WRAPPED_SLOT(pSrcSlot, SrcDescriptorRangeStart);
	ANALYZE_WRAPPED_SLOT(pDestSlot,DestDescriptorRangeStart);
	Assert((pSrcSlot->idx + NumDescriptors )< pSrcSlot->m_DescriptorHeap->GetDesc().NumDescriptors&&
		(pDestSlot->idx + NumDescriptors) < pDestSlot->m_DescriptorHeap->GetDesc().NumDescriptors);
	for (UINT i = 0; i < NumDescriptors; i++) {
		auto& src = *(pSrcSlot+i);
		auto& dest = *(pDestSlot+i);
		dest = src;
	}
	GetReal()->CopyDescriptorsSimple(NumDescriptors, pDestSlot->getRealCPUHandle(), pSrcSlot->getRealCPUHandle(), DescriptorHeapsType);
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
	//return GetReal()->GetDescriptorHandleIncrementSize(DescriptorHeapType);//hooked with slot
	return sizeof(WrappedD3D12DescriptorHeap::DescriptorHeapSlot);
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

