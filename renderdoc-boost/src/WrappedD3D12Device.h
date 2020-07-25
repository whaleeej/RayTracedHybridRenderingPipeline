#pragma once
#include <map>
#include "WrappedD3D12DeviceChild.h"

RDCBOOST_NAMESPACE_BEGIN

class WrappedD3D12Device:public WrappedD3D12Object<ID3D12Device> {
public:
	WrappedD3D12Device(ID3D12Device* pRealDevice);
	~WrappedD3D12Device();

public: //func
	bool isRenderDocDevice() { return m_bRenderDocDevice; }
	bool SetAsRenderDocDevice(bool b) { m_bRenderDocDevice = b; }
	void OnDeviceChildReleased(ID3D12DeviceChild* pReal);

public: // framework
	virtual void SwitchToDevice(ID3D12Device* pNewDevice);

private:
	std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> m_BackRefs;
	//WrappedD3D12DXGISwapChain* m_pWrappedSwapChain;
	//std::vector<WrappedD3D12Texture2D*> m_SwapChainBuffers;
	bool m_bRenderDocDevice;
	//DummyID3D12InfoQueue m_DummyInfoQueue;
};

RDCBOOST_NAMESPACE_END