#pragma once

#include <DX12LibPCH.h>
#include <Events.h>
#include <RootSignature.h>
#include <Application.h>
#include "RenderResourceDefinition.h"
#include "RenderTarget.h"
#include "Scene.h"

using RenderResourceIndex = std::string;
using RenderResourceMap = std::map<RenderResourceIndex, std::shared_ptr<Texture>>;
#define RESERVED_TEXTURE_TO_PRESENT  "ReservedTextureToPresent"

class Renderer {
public:
	Renderer(int w, int h);
	virtual ~Renderer();

	virtual void Resize(int w, int h);

	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources)=0;
	virtual void LoadPipeline()=0;
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene) = 0;
	virtual void PreRender(RenderResourceMap& resources);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList) = 0;
	virtual RenderResourceMap* PostRender();

public:
	static Texture createTex2D_RenderTarget(std::wstring name, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT) {
		DXGI_FORMAT HDRFormat = format;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = desc.Format;
		colorClearValue.Color[0] = 0.0f;
		colorClearValue.Color[1] = 0.0f;
		colorClearValue.Color[2] = 0.0f;
		colorClearValue.Color[3] = 1.0f;
		return Texture(desc, &colorClearValue,
			TextureUsage::RenderTarget, name);
	};
	static Texture createTex2D_DepthStencil(std::wstring name, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT) {
		DXGI_FORMAT depthBufferFormat = format;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil = { 1.0f, 0 };
		return Texture(depthDesc, &depthClearValue,
			TextureUsage::Depth, name);
	};
	static Texture createTex2D_ReadOnly(std::wstring name, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT) {
		DXGI_FORMAT HDRFormat = format;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_NONE);
		return Texture(desc, 0, TextureUsage::IntermediateBuffer, name);
	};
	static Texture createTex2D_ReadWrite(std::wstring name, int w, int h, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT) {
		DXGI_FORMAT HDRFormat = format;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return Texture(desc, 0, TextureUsage::IntermediateBuffer, name);
	};
	static Texture createTex3D_ReadWrite(std::wstring name, int w, int h, int d, DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT) {
		DXGI_FORMAT HDRFormat = format;
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex3D(HDRFormat,
			w, h, d, 0,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return Texture(desc, 0, TextureUsage::IntermediateBuffer, name);
	};

protected:
	int m_Width;
	int m_Height;
	RenderResourceMap* m_Resources;
};