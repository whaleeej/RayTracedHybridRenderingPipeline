#pragma once
#include <list>
#include <set>
#include <map>
#include <vector>
#include <mutex>
#include <queue>

namespace rdcboost{

class WrappedD3D12Resource;

class WrappedD3D12GPUVAddrMgr {
// define
	using OffsetType = UINT64;
	using SizeType = UINT64; //size of byte
	const SizeType VAddrSpaceMax = 0xfffffffffffffffe;

	struct FreeBlockInfo;
	using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;
	using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

	struct FreeBlockInfo{
		FreeBlockInfo(SizeType size)
			: Size(size)
		{}

		SizeType Size;
		FreeListBySize::iterator FreeListBySizeIt;
	};

	struct Allocation {
		Allocation(UINT64 _offset, UINT64 _size,
			WrappedD3D12Resource* _resource) :
			offset(_offset), size(_size), resource(_resource) {}
		Allocation(UINT64 _offset) :
			offset(_offset), size(0), resource(0) {}

		UINT64 offset;
		UINT64 size;
		WrappedD3D12Resource* resource;

		bool operator<(const Allocation& allocation)const;
	};

protected:
	WrappedD3D12GPUVAddrMgr();

	void AddNewBlock(OffsetType offset, SizeType size);

	void FreeBlock(OffsetType offset, SizeType size);

public:
	static WrappedD3D12GPUVAddrMgr& Get();

	WrappedD3D12Resource* GetWrappedResourceByAddr(OffsetType Addr);

	OffsetType Allocate(SizeType size, WrappedD3D12Resource* pResource);

	void Free(OffsetType offset);

	void Cleanup();

	FreeListByOffset m_FreeListByOffset;
	FreeListBySize m_FreeListBySize;
	std::set<Allocation> m_AllocationsInFlight;
	std::queue<Allocation> m_AllocationsStaled;

	SizeType m_NumFreeSpace;

	std::mutex m_AllocationMutex;
};

}