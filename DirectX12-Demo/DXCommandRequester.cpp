#include <chrono>
#include <cassert>

#include "DXCommandRequester.h"
#include "DXFactory.h"
#include "Helpers.h"

#if defined(max)
#undef max
#endif
#if defined(min)
#undef min
#endif

DXCommandRequester::DXCommandRequester(MS_ComPtr(ID3D12Device2) device, D3D12_COMMAND_LIST_TYPE type):
	m_FenceValue(0)
	, m_CommandListType(type)
	, m_Device(device)
{
	m_CommandQueue = DXFactory::CreateCommandQueue(m_Device, type);
	m_Fence = DXFactory::CreateFence(m_Device);
	m_FenceEvent =DXFactory::CreateEventHandle();
}

DXCommandRequester::~DXCommandRequester()
{
	::CloseHandle(m_FenceEvent);
}

MS_ComPtr(ID3D12GraphicsCommandList) DXCommandRequester::getCommandList()
{
	MS_ComPtr(ID3D12CommandAllocator) commandAllocator;
	MS_ComPtr(ID3D12GraphicsCommandList) commandList;

	if (!m_CommandAllocatorQueue.empty() && isFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
		m_CommandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	if (!m_CommandListQueue.empty())
	{
		commandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be
   // retrieved when the command list is executed.
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

MS_ComPtr(ID3D12CommandQueue) DXCommandRequester::getCommandQueue()
{
	return m_CommandQueue;
}

uint64_t DXCommandRequester::executeCommandList(MS_ComPtr(ID3D12GraphicsCommandList) commandList)
{
	commandList->Close();

	 //retrieve the associated command allocator
	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(
		commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator)
	);
	ID3D12CommandList* const ppCommandList[] = {
		commandList.Get()
	};

	m_CommandQueue->ExecuteCommandLists(1, ppCommandList);
	uint64_t fenceValue = signal();

	m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{fenceValue,commandAllocator});
	m_CommandListQueue.push(commandList);

	commandAllocator->Release();

	return fenceValue;
}

uint64_t DXCommandRequester::signal()
{
	uint64_t fenceValueForSignal = ++m_FenceValue;
	ThrowIfFailed(
		m_CommandQueue->Signal(m_Fence.Get(), fenceValueForSignal)
	);
	return fenceValueForSignal;
}

bool DXCommandRequester::isFenceComplete(uint64_t fenceValue)
{
	return m_Fence->GetCompletedValue() >= fenceValue;
}

void DXCommandRequester::waitForFenceValue(uint64_t fenceValue)
{
	if (!isFenceComplete(fenceValue))
	{
		ThrowIfFailed(m_Fence->SetEventOnCompletion(fenceValue, m_FenceEvent));
		::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
	}
}

void DXCommandRequester::flush()
{
	uint64_t fenceValueForSignal = signal();
	waitForFenceValue(fenceValueForSignal);
}

MS_ComPtr(ID3D12CommandAllocator) DXCommandRequester::CreateCommandAllocator()
{
	return DXFactory::CreateCommandAllocator(m_Device, m_CommandListType);
}

MS_ComPtr(ID3D12GraphicsCommandList) DXCommandRequester::CreateCommandList(MS_ComPtr(ID3D12CommandAllocator) allocator)
{
	return DXFactory::CreateCommandList(m_Device, allocator, m_CommandListType);
}
