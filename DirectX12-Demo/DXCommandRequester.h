#pragma once
#include <stdint.h>
#include <queue>
#include <wrl.h> //Win RUNTIME Libraray
#ifndef MS_ComPtr
#define MS_ComPtr(TYPE) Microsoft::WRL::ComPtr<TYPE>
#endif

// DX12 spec
#include <d3d12.h>


class DXCommandRequester {
public:
	DXCommandRequester(MS_ComPtr(ID3D12Device2) device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~DXCommandRequester();

public:
	MS_ComPtr(ID3D12GraphicsCommandList) getCommandList();
	MS_ComPtr(ID3D12CommandQueue) getCommandQueue();

public:
	/////Loop Main Logic/////
	// submit commandlist
	// signal -> setFenceValue
	// Present (outsider resposibility)
	// if(isFenceComplete)
	//		waitForFenceValue
	uint64_t  executeCommandList(MS_ComPtr(ID3D12GraphicsCommandList) commandList);
	uint64_t signal();
	bool isFenceComplete(uint64_t fenceValue);
	void waitForFenceValue(uint64_t fenceValue);
	void flush();

protected:
	MS_ComPtr(ID3D12CommandAllocator) CreateCommandAllocator();
	MS_ComPtr(ID3D12GraphicsCommandList) CreateCommandList(MS_ComPtr(ID3D12CommandAllocator) allocator);

private:
	/////////////////////////////////////IN-CLASS DEFINITION/////////////////////////////////////
	struct CommandAllocatorEntry {
		uint64_t fenceValue;
		MS_ComPtr(ID3D12CommandAllocator) commandAllocator;
	};
	// CommandAllocator associated with a fence value
	using CommandAllocatorEntryQueue = std::queue<CommandAllocatorEntry>;
	using CommandListQueue = std::queue<MS_ComPtr(ID3D12GraphicsCommandList)>;
	/////////////////////////////////////////////TERM/////////////////////////////////////////////

	MS_ComPtr(ID3D12Device2) m_Device;

	D3D12_COMMAND_LIST_TYPE m_CommandListType;
	CommandListQueue m_CommandListQueue;
	CommandAllocatorEntryQueue m_CommandAllocatorQueue;
	MS_ComPtr(ID3D12CommandQueue) m_CommandQueue;

	MS_ComPtr(ID3D12Fence) m_Fence;
	uint64_t m_FenceValue;
	HANDLE m_FenceEvent;

};
