#pragma once
#include <d3d11.h>
#include <d3d12.h>
#define RDCBOOST_NAMESPACE_BEGIN namespace rdcboost\
{

#define RDCBOOST_NAMESPACE_END }

//cons des
// override
// func
// framework


//自己的本体要ref
//引用的ID3D12Object不要ref
//引用的WrappedD3D12Object要ref
//自己创建的对象（只能是Wrapped），因为使用的是new，所以自动会ref(1)