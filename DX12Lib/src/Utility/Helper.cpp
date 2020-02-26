#include "Helpers.h"
#include "Application.h"
#include "Window.h"

std::wstring string_2_wstring(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<WCHAR>> cvt;
	std::wstring ws = cvt.from_bytes(s);
	return ws;
}

std::string wstring_2_string(const std::wstring& ws)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
	std::string s = cvt.to_bytes(ws);
	return s;
}

void msgBox(const std::string& msg)
{
	HWND gWinHandle = Application::Get().GetWindowByName(L"RTPipeline")->GetWindowHandle();
	MessageBoxA(gWinHandle, msg.c_str(), "Error", MB_OK);
}

void d3dTraceHR(const std::string& msg, HRESULT hr)
{
	char hr_msg[512];
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, hr, 0, hr_msg, ARRAYSIZE(hr_msg), nullptr);

	std::string error_msg = msg + ".\nError! " + hr_msg;
	msgBox(error_msg);
}
