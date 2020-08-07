#include "Log.h"
#include "WrappedD3D12GPUVAddrMgr.h"
#include "WrappedD3D12Resource.h"
#include "WrappedD3D12Heap.h"
RDCBOOST_NAMESPACE_BEGIN

static WrappedD3D12GPUVAddrMgr* pGpuVAddrMgr;

bool WrappedD3D12GPUVAddrMgr::Allocation::operator<(const Allocation& allocation)const {
	Assert(this->size != 0 || allocation.size != 0);
	if (this->size != 0 && allocation.size != 0) // both a and b are full allocation 
		return this->offset < allocation.offset;
	// std::set depends on stick lessthen; a is full allocation, b is a place in some allocation
	// b allocation's size equal to zero, but has a place in the form of "offset"
	//-----------------b1--------------b2-----------------b3----------
	//---------------------||aaaaaaaaaaaaaaaaaaaaa||----------------
	//---------------------||--this is the full alloc a--||----------------
	// when a compared with b, return aOffset+aSize < bOffset
	// when b compared with a, return bOffset < aOffset
	if (this->size != 0) {//this is a, a compared with b
		return this->offset + this->size  -1< allocation.offset;
	}
	else {//this is b, b compared with a 
		return this->offset < allocation.offset;
	}
}

WrappedD3D12Resource* WrappedD3D12GPUVAddrMgr::GetWrappedResourceByAddr(OffsetType Addr) {
#ifndef ENABLE_RDC_RESOURCE_RECREATE
	return 0;
#endif
	auto it = m_AllocationsInFlight.find(Allocation(Addr));
	if (it != m_AllocationsInFlight.end()) {
		return it->resource;
	}
	return NULL;
}

WrappedD3D12GPUVAddrMgr::WrappedD3D12GPUVAddrMgr() {
	AddNewBlock(0, VAddrSpaceMax);
	m_NumFreeSpace = VAddrSpaceMax;
};

void WrappedD3D12GPUVAddrMgr::AddNewBlock(OffsetType offset, SizeType size) {
	auto offsetIt = m_FreeListByOffset.emplace(offset, size);
	auto sizeIt = m_FreeListBySize.emplace(size, offsetIt.first);
	offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

WrappedD3D12GPUVAddrMgr& WrappedD3D12GPUVAddrMgr::Get() {
	if (!pGpuVAddrMgr) {
		pGpuVAddrMgr = new WrappedD3D12GPUVAddrMgr();
	}
	return *pGpuVAddrMgr;
}

WrappedD3D12GPUVAddrMgr::OffsetType WrappedD3D12GPUVAddrMgr::Allocate(SizeType size, WrappedD3D12Resource* pResource) {
#ifndef ENABLE_RDC_RESOURCE_RECREATE
	return pResource->GetReal()->GetGPUVirtualAddress();
#endif
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	Assert(size <= m_NumFreeSpace);

	auto smallestBlockIt = m_FreeListBySize.lower_bound(size);
	Assert(smallestBlockIt != m_FreeListBySize.end());

	auto blockSize = smallestBlockIt->first;
	auto offsetIt = smallestBlockIt->second;
	auto offset = offsetIt->first;

	m_FreeListBySize.erase(smallestBlockIt);
	m_FreeListByOffset.erase(offsetIt);
	auto newOffset = offset + size;
	auto newSize = blockSize - size;

	if (newSize > 0)
		AddNewBlock(newOffset, newSize);

	m_NumFreeSpace -= size;
	m_AllocationsInFlight.insert(Allocation( offset, size, pResource));

	return offset;
}

void WrappedD3D12GPUVAddrMgr::Free(OffsetType offset) {
#ifndef ENABLE_RDC_RESOURCE_RECREATE
	return;
#endif
	std::lock_guard<std::mutex> lock(m_AllocationMutex);
	auto it = m_AllocationsInFlight.find(Allocation(offset));
	if (it == m_AllocationsInFlight.end())
		return;
	m_AllocationsStaled.push(*it);
	m_AllocationsInFlight.erase(it);
}

void WrappedD3D12GPUVAddrMgr::Cleanup() {
#ifndef ENABLE_RDC_RESOURCE_RECREATE
	return;
#endif
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	while (!m_AllocationsStaled.empty()) {
		auto& staleAllocation = m_AllocationsStaled.front();
		auto offset = staleAllocation.offset;
		auto size = staleAllocation.size;
		FreeBlock(offset, size);
		m_AllocationsStaled.pop();
	}
}

void WrappedD3D12GPUVAddrMgr::FreeBlock(OffsetType offset, SizeType size) {
	// Find the first element whose offset is greater than the specified offset.
	// This is the block that should appear after the block that is being freed.
	auto nextBlockIt = m_FreeListByOffset.upper_bound(offset);

	// Find the block that appears before the block being freed.
	auto prevBlockIt = nextBlockIt;
	// If it's not the first block in the list.
	if (prevBlockIt != m_FreeListByOffset.begin())
	{
		// Go to the previous block in the list.
		--prevBlockIt;
	}
	else
	{
		// Otherwise, just set it to the end of the list to indicate that no
		// block comes before the one being freed.
		prevBlockIt = m_FreeListByOffset.end();
	}

	// Add the number of free handles back to the heap.
	// This needs to be done before merging any blocks since merging
	// blocks modifies the numDescriptors variable.
	m_NumFreeSpace += size;

	if (prevBlockIt != m_FreeListByOffset.end() &&
		offset == prevBlockIt->first + prevBlockIt->second.Size)
	{
		// The previous block is exactly behind the block that is to be freed.
		//
		// PrevBlock.Offset           Offset
		// |                          |
		// |<-----PrevBlock.Size----->|<------Size-------->|
		//

		// Increase the block size by the size of merging with the previous block.
		offset = prevBlockIt->first;
		size += prevBlockIt->second.Size;

		// Remove the previous block from the free list.
		m_FreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(prevBlockIt);
	}

	if (nextBlockIt != m_FreeListByOffset.end() &&
		offset + size == nextBlockIt->first)
	{
		// The next block is exactly in front of the block that is to be freed.
		//
		// Offset               NextBlock.Offset 
		// |                    |
		// |<------Size-------->|<-----NextBlock.Size----->|

		// Increase the block size by the size of merging with the next block.
		size += nextBlockIt->second.Size;

		// Remove the next block from the free list.
		m_FreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(nextBlockIt);
	}

	// Add the freed block to the free list.
	AddNewBlock(offset, size);
}

RDCBOOST_NAMESPACE_END