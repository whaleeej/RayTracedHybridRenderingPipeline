#pragma once
#include <d3d11.h>
#include <d3d12.h>
#include <d3dx12.h>

#define RDCBOOST_NAMESPACE_BEGIN namespace rdcboost\
{

#define RDCBOOST_NAMESPACE_END }

#include <wrl.h>
#define COMPtr Microsoft::WRL::ComPtr

template <typename T>
T RDCMIN(const T &a, const T &b)
{
	return a < b ? a : b;
}
template <typename T>
T RDCMAX(const T &a, const T &b)
{
	return a > b ? a : b;
}
template <typename T>
inline T AlignUpWithMask(T value, size_t mask)
{
	return (T)(((size_t)value + mask) & ~mask);
}

template <typename T>
inline T AlignDownWithMask(T value, size_t mask)
{
	return (T)((size_t)value & ~mask);
}

template <typename T>
inline T AlignUp(T value, size_t alignment)
{
	return AlignUpWithMask(value, alignment - 1);
}

template <typename T>
inline T AlignDown(T value, size_t alignment)
{
	return AlignDownWithMask(value, alignment - 1);
}

template <typename T>
inline bool IsAligned(T value, size_t alignment)
{
	return 0 == ((size_t)value & (alignment - 1));
}

#define ANALYZE_WRAPPED_CPU_HANDLE(wrapped) \
static_cast<WrappedD3D12DescriptorHeap::DescriptorHeapSlot*>((void*)((wrapped).ptr))->getRealCPUHandle()

#define ANALYZE_WRAPPED_GPU_HANDLE(wrapped) \
static_cast<WrappedD3D12DescriptorHeap::DescriptorHeapSlot*>((void*)((wrapped).ptr))->getRealGPUHandle()

#define ANALYZE_WRAPPED_SLOT(pName, wrapped) auto pName = \
static_cast<WrappedD3D12DescriptorHeap::DescriptorHeapSlot*>((void*)((wrapped).ptr))

#define DEFINE_AND_ASSERT_WRAPPED_GPU_VADDR(VADDR) \
auto pWrappedResource_Ano = WrappedD3D12GPUVAddrMgr::Get().GetWrappedResourceByAddr(VADDR);\
Assert(pWrappedResource_Ano);\
auto realVAddr =  pWrappedResource_Ano->GetReal()->GetGPUVirtualAddress() + (VADDR - pWrappedResource_Ano->GetOffset())

// update fist类的结构
// cons des
// override
// func
// framework

//update 7/25 switchToDevice D3D12框架完成

//update 7/27 完成 device和swapchain1的override

//update 7/29 完成descriptor heap的缓存和拷贝， 完成最小化的所有wrapper																										*****Perf Impact xxx fixed *****

//update 7/30 实现等帧逻辑，一帧最后present然后waitForFirstBackBuffer value后调用D3D12CallAtEndOfFrame，然后wrappedDevice中去做switch操作，首先要做的是等待所有fence的value跑完

//update 7/30 初步实现d3d12 object的拷贝框架

// update 7/31 对comptr的理解
// comptr走指针init的话会把对象ref++
// comptr走operator=对象会--原来的，把新的++
// 但是用d3d的create，传入的无论是ptr还是comptr对象内存的ptr，都会生成一个ref=1的对象存在ptr里, 因为传入的是ppv，赋值不会给ref多＋1
// 所以如果在去new一个Wrapper的话要记得new出来的是ref=1的内容

//update7/31 对生命周期的理解
//自己的本体m_pReal需要ref，Wrapper对象即拥有一个real的生命周期                                         ------>   ComPtr
//记录的other_pReal绝对不能ref，仅仅是存一个指针用来在switch的时候比较内容                         ------>   RawPtr
//记录的other_pWrapper需要ref, 在本体释放的时候跟着release                                                    ------>   ComPtr
//在device中new的Wrapper的对象，创建后自动生命周期为1， 赋值给*ppV                                  ------>   RawPtr       
//在backRefs中记录指针，不持有生命周期，在Wrapper进行delete的时候自动从map中erase掉     ------>   RawPtr  
//copyToDevice中创建的对象，改成ComPtr直接走operator=                                                       ------>   ComPtr
//释放wrapped资源的时候需要从device中找到并且删除																																			 *****Perf Impact *****

//update7/31 支持api传入参数为null的情况，主要支持了CreateView那部分

//update8/01 在一个类中同时支持SwapChain1-4， 使用queryInterface的方法，这是一个可以通用的设计

//update8/02 使用Dummy Info Queue来Hook掉Debug Layer的拦截，可以保证renderdoc中创建的renderdoc wrapped device不会进入死循环

//update8/02 缓存pipeline state的时候要拷贝byte code，释放要正确地delete，例如CS，VS，PS，SO等																*****Perf Impact *****

//update8/03 完善资源拷贝的逻辑，1. 不拷贝的资源D3D12_HEAP_TYPE_UPLOAD/D3D12_HEAP_TYPE_READBACK/..											*****Perf Impact ***** //TODO 拷贝upload资源
//          2.因为renderdoc wrapped new res和raw res不能用同一个queue/list/allocator跑，要写回cpu，cpu拷贝，再upload-copy
//          3. readback/upload heap只能是buffer，所以使用d3dx12中的UpdateSubresource那一套机制解决
//          4. desc heap中新建view还是有点问题，先用catch exception来避免为释放的资源新建view

//update8/04 descriptor heap中的内容如果是cpu visible的话是有明确的cpu handle的，如果是gpu visible的话是没有的
// 因此只需要缓存cpu descriptor heap中的params，重建也只要恢复cpu descriptor中的内容就好了
// 因为交出去的是handle，这里的handle需要在wrappedDescriptorHeap中hook一块新的内存地址出来，然后拿回来的时候再解析成真实的d3d api开的descriptorheap的内存地址
// 然而这么做是不行的，因为cpu handle现在看来是会用重叠的，包括cpu和gpu，可能会把gpu的解析成cpu的内容来cache导致恢复的时候报错
// 参考renderdoc的做法是把descriptor wrap掉，传出去的handle.ptr是hook的descriptor的地址

//update8/05
//Descriptor Heap&Handle的wrap机制下的bugfix，优化了cache和isResourceExist的框架
//主要wrapped资源释放后slot中存的任然是裸指针，没法看到其是否释放。
//所以如果这段内存新建一个wrapped资源就会导致资源指向错误，也就是会错的view desc创建了新资源的view，desc对不上就会让d3d device remove掉
//而且资源在重建的时候会导致wrapper内的real变掉。解决方法是每个slot中都记下real，然后在资源重建前，device和swap chain中记下具有生命周期的wrapped->old raw res的指针。在descHeap中的view重建的时候检查是否存在。即整套修改掉了isResourceExist的机制

// update  TODO
// 0. descriptor heap中的resource可能已经被释放了 -》 catch(...)
// 1. 支持CreateView中的参数情况
// 2. 支持其他API的参数情况
// 3. ID3D12Device1-6
// 4. ID3D12GraphicsCommandList1-5
// 5. 支持ID3D12CommandSignature / ID3D12QueryHeap / ID3D12StateObject 
