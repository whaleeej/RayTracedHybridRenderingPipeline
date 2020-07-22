#include "WrappedD3D11Shader.h"
#include "WrappedD3D11ClassLinkage.h"
#include "Log.h"

namespace rdcboost
{
	WrappedD3D11VertexShader::WrappedD3D11VertexShader(
		const void* pShaderBytecode, SIZE_T BytecodeLength, 
		WrappedD3D11ClassLinkage* pWrappedClassLinkage,
		ID3D11VertexShader* pReal, WrappedD3D11Device* pDevice) : 
		WrappedD3D11BaseShader(pShaderBytecode, BytecodeLength, pWrappedClassLinkage, pReal, pDevice)
	{
	}

	HRESULT WrappedD3D11VertexShader::CreateSpecificShader(
		ID3D11Device* pNewDevice, const byte* pShaderBytecode, SIZE_T BytecodeLength, 
		ID3D11ClassLinkage* pNewClassLinkage, ID3D11DeviceChild** ppNewShader)
	{
		ID3D11VertexShader* pNewShader = NULL;
		HRESULT res = pNewDevice->CreateVertexShader(pShaderBytecode, BytecodeLength, pNewClassLinkage, 
													 &pNewShader);
		*ppNewShader = pNewShader;
		return res;
	}

	WrappedD3D11HullShader::WrappedD3D11HullShader(
		const void* pShaderBytecode, SIZE_T BytecodeLength, 
		WrappedD3D11ClassLinkage* pWrappedClassLinkage,
		ID3D11HullShader* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11BaseShader(pShaderBytecode, BytecodeLength, pWrappedClassLinkage, pReal, pDevice)
	{
	}

	HRESULT WrappedD3D11HullShader::CreateSpecificShader(
		ID3D11Device* pNewDevice, const byte* pShaderBytecode, SIZE_T BytecodeLength, 
		ID3D11ClassLinkage* pNewClassLinkage, ID3D11DeviceChild** ppNewShader)
	{
		ID3D11HullShader* pNewShader = NULL;
		HRESULT res = pNewDevice->CreateHullShader(pShaderBytecode, BytecodeLength, pNewClassLinkage,
													 &pNewShader);

		*ppNewShader = pNewShader;
		return res;

	}

	WrappedD3D11DomainShader::WrappedD3D11DomainShader(
		const void* pShaderBytecode, SIZE_T BytecodeLength, 
		WrappedD3D11ClassLinkage* pWrappedClassLinkage,
		ID3D11DomainShader* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11BaseShader(pShaderBytecode, BytecodeLength, pWrappedClassLinkage, pReal, pDevice)
	{
	}

	HRESULT WrappedD3D11DomainShader::CreateSpecificShader(
		ID3D11Device* pNewDevice, const byte* pShaderBytecode, SIZE_T BytecodeLength, 
		ID3D11ClassLinkage* pNewClassLinkage, ID3D11DeviceChild** ppNewShader)
	{
		ID3D11DomainShader* pNewShader = NULL;
		HRESULT res = pNewDevice->CreateDomainShader(pShaderBytecode, BytecodeLength, pNewClassLinkage,
													 &pNewShader);

		*ppNewShader = pNewShader;
		return res;
	}

	WrappedD3D11GeometryShader::WrappedD3D11GeometryShader(
		const void* pShaderBytecode, SIZE_T BytecodeLength, 
		const SGeometryShaderSOInfo& soInfo,
		WrappedD3D11ClassLinkage* pWrappedClassLinkage,
		ID3D11GeometryShader* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11BaseShader(pShaderBytecode, BytecodeLength, pWrappedClassLinkage, pReal, pDevice),
		m_SOInfo(soInfo)
	{
	}

	WrappedD3D11GeometryShader::~WrappedD3D11GeometryShader()
	{
		for (auto it = m_SOInfo.SODeclaration.begin(); it != m_SOInfo.SODeclaration.end(); ++it)
			delete[] it->SemanticName;
	}

	HRESULT WrappedD3D11GeometryShader::CreateSpecificShader(
		ID3D11Device* pNewDevice, const byte* pShaderBytecode, SIZE_T BytecodeLength, 
		ID3D11ClassLinkage* pNewClassLinkage, ID3D11DeviceChild** ppNewShader)
	{
		ID3D11GeometryShader* pNewShader = NULL;
		HRESULT res;
		if (!m_SOInfo.withSO)
		{
			res = pNewDevice->CreateGeometryShader(pShaderBytecode, BytecodeLength, pNewClassLinkage,
												   &pNewShader);
		}
		else
		{
			UINT* strides = m_SOInfo.BufferStrides.empty() ? NULL : &m_SOInfo.BufferStrides[0];
			D3D11_SO_DECLARATION_ENTRY* entry = 
				m_SOInfo.SODeclaration.empty() ? NULL : &m_SOInfo.SODeclaration[0];

			res = pNewDevice->CreateGeometryShaderWithStreamOutput(
					pShaderBytecode, BytecodeLength, 
					entry, (UINT) m_SOInfo.SODeclaration.size(), 
					strides, (UINT) m_SOInfo.BufferStrides.size(), 
					m_SOInfo.rasterizedStream, pNewClassLinkage, &pNewShader);
		}
		
		*ppNewShader = pNewShader;
		return res;
	}

	WrappedD3D11PixelShader::WrappedD3D11PixelShader(
		const void* pShaderBytecode, SIZE_T BytecodeLength, 
		WrappedD3D11ClassLinkage* pWrappedClassLinkage,
		ID3D11PixelShader* pReal, WrappedD3D11Device* pDevice) :
		WrappedD3D11BaseShader(pShaderBytecode, BytecodeLength, pWrappedClassLinkage, pReal, pDevice)
	{
	}

	HRESULT WrappedD3D11PixelShader::CreateSpecificShader(ID3D11Device* pNewDevice, const byte* pShaderBytecode, SIZE_T BytecodeLength, ID3D11ClassLinkage* pNewClassLinkage, ID3D11DeviceChild** ppNewShader)
	{
		ID3D11PixelShader* pNewShader = NULL;
		HRESULT res = pNewDevice->CreatePixelShader(pShaderBytecode, BytecodeLength, pNewClassLinkage,
													&pNewShader);

		*ppNewShader = pNewShader;
		return res;
	}

	WrappedD3D11ComputeShader::WrappedD3D11ComputeShader(
		const void* pShaderBytecode, SIZE_T BytecodeLength, 
		WrappedD3D11ClassLinkage* pWrappedClassLinkage,
		ID3D11ComputeShader* pReal, WrappedD3D11Device* pDevice) : 
		WrappedD3D11BaseShader(pShaderBytecode, BytecodeLength, pWrappedClassLinkage, pReal, pDevice)
	{
	}

	HRESULT WrappedD3D11ComputeShader::CreateSpecificShader(
		ID3D11Device* pNewDevice, const byte* pShaderBytecode, SIZE_T BytecodeLength, 
		ID3D11ClassLinkage* pNewClassLinkage, ID3D11DeviceChild** ppNewShader)
	{
		ID3D11ComputeShader* pNewShader = NULL;
		HRESULT res = pNewDevice->CreateComputeShader(pShaderBytecode, BytecodeLength, pNewClassLinkage,
													  &pNewShader);

		*ppNewShader = pNewShader;
		return res;
	}
}
