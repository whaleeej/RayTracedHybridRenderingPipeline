#pragma once
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12StateObject : public WrappedD3D12DeviceChild<ID3D12StateObject, ID3D12StateObject> {
public:// cons des
	WrappedD3D12StateObject(ID3D12StateObject* pReal, WrappedD3D12Device* pDevice,
		const D3D12_STATE_OBJECT_DESC *pDesc
	);
	~WrappedD3D12StateObject();

public:// override

public:// func

public:// framework
	virtual COMPtr<ID3D12DeviceChild> CopyToDevice(ID3D12Device* pNewDevice);

protected:
	D3D12_STATE_OBJECT_DESC m_Desc;
};

RDCBOOST_NAMESPACE_END