#include "RTHelper.h"

static dxc::DxcDllSupport gDxcDllHelper;

ID3DBlobPtr compileLibrary(const WCHAR* filename, const WCHAR* targetString)
{
	// Initialize the helper
	ThrowIfFailed(gDxcDllHelper.Initialize());
	IDxcCompilerPtr pCompiler;
	IDxcLibraryPtr pLibrary;
	ThrowIfFailed(gDxcDllHelper.CreateInstance(CLSID_DxcCompiler, &pCompiler));
	ThrowIfFailed(gDxcDllHelper.CreateInstance(CLSID_DxcLibrary, &pLibrary));

	// Open and read the file
	std::ifstream shaderFile(filename);
	if (shaderFile.good() == false)
	{
		msgBox("Can't open file " + wstring_2_string(std::wstring(filename)));
		ThrowIfFailed(-1);
		return nullptr;
	}
	std::stringstream strStream;
	strStream << shaderFile.rdbuf();
	std::string shader = strStream.str();

	// Create blob from the string
	IDxcBlobEncodingPtr pTextBlob;
	ThrowIfFailed(pLibrary->CreateBlobWithEncodingFromPinned((LPBYTE)shader.c_str(), (uint32_t)shader.size(), 0, &pTextBlob));

	// Compile
	IDxcOperationResultPtr pResult;
	ThrowIfFailed(pCompiler->Compile(pTextBlob, filename, L"", targetString, nullptr, 0, nullptr, 0, nullptr, &pResult));

	// Verify the result
	HRESULT resultCode;
	ThrowIfFailed(pResult->GetStatus(&resultCode));
	if (FAILED(resultCode))
	{
		IDxcBlobEncodingPtr pError;
		ThrowIfFailed(pResult->GetErrorBuffer(&pError));
		std::string log = convertBlobToString(pError.GetInterfacePtr());
		msgBox("Compiler error:\n" + log);
		ThrowIfFailed(-1);
		return nullptr;
	}

	IDxcBlobPtr pBlob;
	ThrowIfFailed(pResult->GetResult(&pBlob));
	return pBlob;
}

Microsoft::WRL::ComPtr<ID3D12RootSignature> createRootSignature(Microsoft::WRL::ComPtr<ID3D12Device1> pDevice, const D3D12_ROOT_SIGNATURE_DESC& desc)
{
	Microsoft::WRL::ComPtr<ID3DBlob> pSigBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob;
	HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &pSigBlob, &pErrorBlob);
	if (FAILED(hr))
	{
		std::string msg = convertBlobToString(pErrorBlob.Get());
		msgBox(msg);
		ThrowIfFailed(-1);
		return nullptr;
	}
	Microsoft::WRL::ComPtr<ID3D12RootSignature> pRootSig;
	ThrowIfFailed(pDevice->CreateRootSignature(0, pSigBlob->GetBufferPointer(), pSigBlob->GetBufferSize(), IID_PPV_ARGS(&pRootSig)));
	return pRootSig;
}

Microsoft::WRL::ComPtr<ID3D12Resource> createBuffer(Microsoft::WRL::ComPtr <ID3D12Device1> pDevice, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps)
{
	D3D12_RESOURCE_DESC bufDesc = {};
	bufDesc.Alignment = 0;
	bufDesc.DepthOrArraySize = 1;
	bufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufDesc.Flags = flags;
	bufDesc.Format = DXGI_FORMAT_UNKNOWN;
	bufDesc.Height = 1;
	bufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	bufDesc.MipLevels = 1;
	bufDesc.SampleDesc.Count = 1;
	bufDesc.SampleDesc.Quality = 0;
	bufDesc.Width = size;

	Microsoft::WRL::ComPtr<ID3D12Resource> pBuffer;
	ThrowIfFailed(pDevice->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer)));
	return pBuffer;
}

