#pragma once
#define WIN32_LEAN_AND_MEAN // for Windows.h minimum included
#include <Windows.h>
#include <stdint.h>

class Window {
public:
	void registerWindowClass(HINSTANCE hInst, LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM));
	void createWindow(HINSTANCE hInst);
	void showWindow();
	void setFullscreen(bool fullscreen);
	void resize(uint32_t width, uint32_t height);

public:
	HWND g_hWnd; // Window handle.
	RECT g_WindowRect; // Window rectangle record (used to toggle fullscreen state)

	uint32_t g_ClientWidth = 1280;
	uint32_t g_ClientHeight = 720;
	bool g_Fullscreen = false;

	// Used for registering & creating the window
	const wchar_t* windowClassName = L"DX12WindowClass";
	const wchar_t* windowTitle = L"DirectX12 Demo";
};
