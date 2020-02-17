#pragma once

#define _ENABLE_EXTENDED_ALIGNED_STORAGE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h> // For CommandLineToArgvW

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#if defined(CreateWindow)
#undef CreateWindow
#endif

// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// DirectX 12 specific headers.
#include "d3dx12.h"
#include "d3dcompiler.h"
#include "dxcapi.use.h"
#include <dxgi1_6.h>
#include <dxgiformat.h>
#include <DirectXMath.h>
#include <DirectXTex.h>
#include <comdef.h>
using namespace DirectX;

// STL Headers
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <thread>
#include <vector>
#include <array>
#include <fstream>
#include <sstream>
namespace fs = std::experimental::filesystem;

// Helper functions
#include <Helpers.h>
