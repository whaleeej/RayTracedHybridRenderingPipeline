#pragma once
#include <d3d11.h>
#include <d3d12.h>
#include <d3dx12.h>

#define RDCBOOST_NAMESPACE_BEGIN namespace rdcboost\
{

#define RDCBOOST_NAMESPACE_END }

#include <wrl.h>
#define COMPtr Microsoft::WRL::ComPtr

// update fist类的结构
// cons des
// override
// func
// framework

//update 7/25 switchToDevice D3D12框架完成

//update 7/27 完成 device和swapchain1的override

//update 7/29 完成descriptor heap的缓存和拷贝， 完成最小化的所有wrapper

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

//update7/31 支持api传入参数为null的情况，主要支持了CreateView那部分

// update  TODO
// 0. descriptor heap中的resource可能已经被释放了 -》 catch(...)
// 1. 支持CreateView中的参数情况
// 2. 支持其他API的参数情况
// 3. ID3D12Device1-6
// 4. ID3D12GraphicsCommandList1-5
// 5. 支持ID3D12CommandSignature / ID3D12QueryHeap / ID3D12StateObject 
