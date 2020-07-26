#include <WrappedD3D12Device.h>

RDCBOOST_NAMESPACE_BEGIN

WrappedD3D12Device::WrappedD3D12Device(ID3D12Device * pRealDevice, const SDeviceCreateParams& param)
	: WrappedD3D12Object(pRealDevice)
	, m_DeviceCreateParams(param)
	, m_bRenderDocDevice(false)
{
}

WrappedD3D12Device::~WrappedD3D12Device() {
	//release handled in WrappedD3D12ObjectBase
}

void WrappedD3D12Device::OnDeviceChildReleased(ID3D12DeviceChild* pReal) {
	if (m_BackRefs.erase(pReal) !=0) return;

	//TODO not in backRefs, then it will be in backbuffer for swapchain, try erase in it

	//TODO unknown wrapped devicechild released error
	LogError("Unknown device child released.");
	Assert(false);
}

void WrappedD3D12Device::SwitchToDevice(ID3D12Device* pNewDevice) {
	//0. create new device(done)
	Assert(pNewDevice != NULL);

	//1. copy private data of device
	m_PrivateData.CopyPrivateData(pNewDevice);

	//2. create new swapchain

	//3. swapchain buffer switchToDevice

	//4. backRefs switchToDevice
	if (!m_BackRefs.empty()) {
		printf("Transferring DeviceChild resources to new device without modifying the content of WrappedDeviceChild\n");
		printf("--------------------------------------------------\n");
		std::map<ID3D12DeviceChild*, WrappedD3D12ObjectBase*> newBackRefs;
		int progress = 0; int idx = 0;
		for (auto it = m_BackRefs.begin(); it != m_BackRefs.end(); it++) {
			it->second->SwitchToDevice(pNewDevice);//key of the framework
			newBackRefs[static_cast<ID3D12DeviceChild*>(it->second->GetRealObject())] = it->second;
			// sequencing
			++idx;
			while (progress < (int)(idx * 50 / m_BackRefs.size()))
			{
				printf(">");
				++progress;
			}
		}
		printf("\n");
		m_BackRefs.swap(newBackRefs);
	}

	//5.pReal substitute
	ULONG refs = m_pReal->Release();
	if (refs != 0)
		LogError("Previous real device ref count: %d", static_cast<int>(refs));
	m_pReal = pNewDevice;
	m_pReal->AddRef();

}

RDCBOOST_NAMESPACE_END

