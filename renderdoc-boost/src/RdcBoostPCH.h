#pragma once
#include <d3d11.h>
#include <d3d12.h>
#define RDCBOOST_NAMESPACE_BEGIN namespace rdcboost\
{

#define RDCBOOST_NAMESPACE_END }

#include <wrl.h>
#define COMPtr Microsoft::WRL::ComPtr

//cons des
// override
// func
// framework

//
////自己的本体要ref
////引用的ID3D12Object不要ref
////引用的WrappedD3D12Object要ref
////自己创建的对象（只能是Wrapped），因为使用的是new，所以自动会ref(1)

//对comptr的理解
// comptr走指针init的话会把对象ref++
// comptr走operator=对象会--原来的，把新的++
// 但是用d3d的create，传入的无论是ptr还是comptr对象内存的ptr，都会生成一个ref=1的对象存在ptr里
// 所以如果在去new一个Wrapper的话要记得把原来的ptr ref--

//update 对生命周期的理解
//自己的本体m_pReal需要ref，但是只要ref最多一次, 只跟着new出来的Wrapper对象走，Wrapper对象即代表一个real，考虑改成ComPtr
//记录的other_pReal绝对不能ref，仅仅是存一个指针用来在switch的时候比较内容，因此要谨慎使用这个对象，一般用作标杆
//记录的other_pWrapper需要ref, 这里最后考虑用ComPtr来ref，因为ComPtr在用指针初始化的时候会自动ref++，所以考虑WrappedD3D12Object中初始化为ref0
//在device中创建的Wrapper的对象记录指针，不记录生命周期，在物体delete的时候自动从map中erase掉
//copyToDevice中创建的对象，需要替代掉原本的m_pReal, 如果没有改成ComPtr则需要自动释放原来的，ref新的，改成ComPtr的话直接走operator=