#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <string>

#include "DescriptorAllocation.h"
class CommandQueue;
class DescriptorAllocator;
class Game;
class Window;

class Application
{
public:

    static void															Create(HINSTANCE hInst);
    static void															Destroy();
    static Application&												Get();
    bool																		IsTearingSupported() const;
	bool																		isDXRSupport() const { return m_DXRSupported; }
    DXGI_SAMPLE_DESC											GetMultisampleQualityLevels( DXGI_FORMAT format, UINT numSamples, D3D12_MULTISAMPLE_QUALITY_LEVEL_FLAGS flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE ) const;
    std::shared_ptr<Window>									CreateRenderWindow(const std::wstring& windowName, int clientWidth, int clientHeight, bool vSync = true );
    void																		DestroyWindow(const std::wstring& windowName);
    void																		DestroyWindow(std::shared_ptr<Window> window);
    std::shared_ptr<Window>									GetWindowByName(const std::wstring& windowName);
    int																		Run(std::shared_ptr<Game> pGame);
    void																		Quit(int exitCode = 0);
    Microsoft::WRL::ComPtr<ID3D12Device>		GetDevice() const;
    std::shared_ptr<CommandQueue>					GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
    void																		Flush();
    DescriptorAllocation											AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
    void																		ReleaseStaleDescriptors( uint64_t finishedFrame );
    UINT																	GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
    static uint64_t														GetFrameCount()  {   return ms_FrameCount; }

protected:

    Application(HINSTANCE hInst);
    virtual ~Application();
    void																		Initialize();
    Microsoft::WRL::ComPtr<IDXGIAdapter4>			GetAdapter(bool bUseWarp);
    Microsoft::WRL::ComPtr<ID3D12Device>		CreateDevice(Microsoft::WRL::ComPtr<IDXGIAdapter4> adapter);
    bool																		CheckTearingSupport();
	

private:
    friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    Application(const Application& copy) = delete;
    Application& operator=(const Application& other) = delete;

private:
    HINSTANCE m_hInstance;

    Microsoft::WRL::ComPtr<ID3D12Device> m_d3d12Device;

    std::shared_ptr<CommandQueue> m_DirectCommandQueue;
    std::shared_ptr<CommandQueue> m_ComputeCommandQueue;
    std::shared_ptr<CommandQueue> m_CopyCommandQueue;

    std::unique_ptr<DescriptorAllocator> m_DescriptorAllocators[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    bool m_TearingSupported;

	bool m_DXRSupported;

    static uint64_t ms_FrameCount;

	bool m_loadedFlg;
};