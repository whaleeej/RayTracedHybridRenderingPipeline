#pragma once
#include "WrappedD3D11DeviceChild.h"
#include "WrappedD3D11ClassLinkage.h"

namespace rdcboost
{
	class WrappedD3D11ClassLinkage;
	template <typename NestedType>
	class WrappedD3D11BaseShader : public WrappedD3D11DeviceChild<NestedType>
	{
	public:
		WrappedD3D11BaseShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
							   WrappedD3D11ClassLinkage* pWrappedClassLinkage,
							   NestedType* pReal, WrappedD3D11Device* pDevice) :
			WrappedD3D11DeviceChild(pReal, pDevice), m_BytecodeLength(BytecodeLength),
			m_pWrappedClassLinkage(pWrappedClassLinkage)
		{
			if (m_pWrappedClassLinkage)
				m_pWrappedClassLinkage->AddRef();

			m_pShaderBytecode = new byte[BytecodeLength];
			memcpy(m_pShaderBytecode, pShaderBytecode, BytecodeLength);
		}

		virtual ~WrappedD3D11BaseShader()
		{
			delete[] m_pShaderBytecode;

			if (m_pWrappedClassLinkage)
				m_pWrappedClassLinkage->Release();
		}

		virtual ID3D11DeviceChild* CopyToDevice(ID3D11Device* pNewDevice)
		{
			ID3D11ClassLinkage* pNewClassLinkage = NULL;
			if (m_pWrappedClassLinkage != NULL)
			{
				m_pWrappedClassLinkage->SwitchToDevice(pNewDevice);	// ensure transfer to new device
				pNewClassLinkage = m_pWrappedClassLinkage->GetReal();
			}

			ID3D11DeviceChild* pNewShader = NULL;
			if (FAILED(CreateSpecificShader(pNewDevice, m_pShaderBytecode, m_BytecodeLength,
											pNewClassLinkage, &pNewShader)))
			{
				LogError("CreateShader failed when CopyToDevice");
			}

			if (m_pWrappedClassLinkage != NULL)
			{
				m_pWrappedClassLinkage->CopyInstancesToDevice(pNewDevice);
			}

			return pNewShader;
		}

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice, 
											 const byte* m_pShaderBytecode, SIZE_T m_BytecodeLength, 
											 ID3D11ClassLinkage* pNewClassLinkage, 
											 ID3D11DeviceChild** pNewShader) = 0;

	private:
		byte* m_pShaderBytecode;
		SIZE_T m_BytecodeLength;
		WrappedD3D11ClassLinkage* m_pWrappedClassLinkage;
	};

	class WrappedD3D11VertexShader : public WrappedD3D11BaseShader<ID3D11VertexShader>
	{
	public:
		WrappedD3D11VertexShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
								 WrappedD3D11ClassLinkage* pWrappedClassLinkage,
								 ID3D11VertexShader* pReal, WrappedD3D11Device* pDevice);

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice, 
											 const byte* pShaderBytecode, SIZE_T BytecodeLength,
											 ID3D11ClassLinkage* pNewClassLinkage,
											 ID3D11DeviceChild** ppNewShader);
	};

	class WrappedD3D11HullShader : public WrappedD3D11BaseShader<ID3D11HullShader>
	{
	public:
		WrappedD3D11HullShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
							   WrappedD3D11ClassLinkage* pWrappedClassLinkage,
							   ID3D11HullShader* pReal, WrappedD3D11Device* pDevice);

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice,
											 const byte* pShaderBytecode, SIZE_T BytecodeLength,
											 ID3D11ClassLinkage* pNewClassLinkage,
											 ID3D11DeviceChild** ppNewShader);
	};

	class WrappedD3D11DomainShader : public WrappedD3D11BaseShader<ID3D11DomainShader>
	{
	public:
		WrappedD3D11DomainShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
								 WrappedD3D11ClassLinkage* pWrappedClassLinkage,
								 ID3D11DomainShader* pReal, WrappedD3D11Device* pDevice);

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice,
											 const byte* pShaderBytecode, SIZE_T BytecodeLength,
											 ID3D11ClassLinkage* pNewClassLinkage,
											 ID3D11DeviceChild** ppNewShader);
	};

	struct SGeometryShaderSOInfo
	{
		bool withSO;
		std::vector<D3D11_SO_DECLARATION_ENTRY> SODeclaration;
		std::vector<UINT> BufferStrides;
		UINT rasterizedStream;
	};
	class WrappedD3D11GeometryShader : public WrappedD3D11BaseShader<ID3D11GeometryShader>
	{
	public:
		WrappedD3D11GeometryShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
								   const SGeometryShaderSOInfo& soInfo,
								   WrappedD3D11ClassLinkage* pWrappedClassLinkage,
								   ID3D11GeometryShader* pReal, WrappedD3D11Device* pDevice);

		virtual ~WrappedD3D11GeometryShader();

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice,
											 const byte* pShaderBytecode, SIZE_T BytecodeLength,
											 ID3D11ClassLinkage* pNewClassLinkage,
											 ID3D11DeviceChild** ppNewShader);

	private:
		SGeometryShaderSOInfo m_SOInfo;
	};

	class WrappedD3D11PixelShader : public WrappedD3D11BaseShader<ID3D11PixelShader>
	{
	public:
		WrappedD3D11PixelShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
								WrappedD3D11ClassLinkage* pWrappedClassLinkage,
								ID3D11PixelShader* pReal, WrappedD3D11Device* pDevice);

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice,
											 const byte* pShaderBytecode, SIZE_T BytecodeLength,
											 ID3D11ClassLinkage* pNewClassLinkage,
											 ID3D11DeviceChild** ppNewShader);
	};

	class WrappedD3D11ComputeShader : public WrappedD3D11BaseShader<ID3D11ComputeShader>
	{
	public:
		WrappedD3D11ComputeShader(const void* pShaderBytecode, SIZE_T BytecodeLength,
								  WrappedD3D11ClassLinkage* pWrappedClassLinkage,
								  ID3D11ComputeShader* pReal, WrappedD3D11Device* pDevice);

	protected:
		virtual HRESULT CreateSpecificShader(ID3D11Device* pNewDevice,
											 const byte* pShaderBytecode, SIZE_T BytecodeLength,
											 ID3D11ClassLinkage* pNewClassLinkage,
											 ID3D11DeviceChild** ppNewShader);
	};
}

