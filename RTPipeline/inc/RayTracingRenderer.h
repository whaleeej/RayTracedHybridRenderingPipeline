#pragma once

#include <random>
#include <RTHelper.h>

#include "Renderer.h"


class RayTracingRenderer : public Renderer {
public:
	RayTracingRenderer(int w, int h);
	virtual ~RayTracingRenderer();

public://override
	virtual void LoadResource(std::shared_ptr<Scene> scene, RenderResourceMap& resources);
	virtual void LoadPipeline();
	virtual void Update(UpdateEventArgs& e, std::shared_ptr<Scene> scene);
	virtual void Render(RenderEventArgs& e, std::shared_ptr<Scene> scene, std::shared_ptr<CommandList> commandList);

private:
	//Camera
	struct CameraRTCB
	{
		XMVECTOR PositionWS;
		XMMATRIX InverseViewMatrix;
		float fov;
		float padding[3];
	};
	// Frame
	struct FrameIndexRTCB
	{
		uint32_t FrameIndex;
		uint32_t seed;
		float padding[2];
	};
	// ObjectIndex
	struct ObjectIndexRTCB {
		float index;
		float padding[3];
	};

private:
	std::shared_ptr<Scene> m_Scene;
	Texture gPosition; //srv
	Texture gAlbedoMetallic; //srv
	Texture gNormalRoughness; //srv
	Texture gExtra; //srv

private:
	/////////////////////////////////////////////// RT Object 
	std::mt19937 m_generatorURNG;

	void createAccelerationStructures();
	Microsoft::WRL::ComPtr < ID3D12Resource > mpTopLevelAS;
	std::vector<Microsoft::WRL::ComPtr < ID3D12Resource>> mpBottomLevelASes;
	uint64_t mTlasSize = 0;

	void createRtPipelineState();
	RootSignatureDesc createLocalRootDesc(int uav_num, int srv_num, int sampler_num,
		const D3D12_STATIC_SAMPLER_DESC* pStaticSamplers, int cbv_num, int c32_num, int space = 0);
	Microsoft::WRL::ComPtr < ID3D12StateObject > mpPipelineState;
	Microsoft::WRL::ComPtr < ID3D12RootSignature > mpEmptyRootSig;

	void createShaderResources();
	void createSrvUavHeap();
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap > mpSrvUavHeap;
	Texture mRtShadowOutputTexture;
	Texture mRtReflectOutputTexture;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTPointLightCB;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTCameraCB;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTFrameIndexCB;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mpRTMaterialCBList;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mpRTPBRMaterialCBList;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mpRTGameObjectIndexCBList;

	void createShaderTable();
	Microsoft::WRL::ComPtr < ID3D12Resource> mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;

	/*|-rayGen-|-shadowMiss-|-secondaryMiss-|-......-|-shadowChs-|-secondaryChs-|-......-|*/
	const WCHAR* kRayGenShader = L"rayGen";

	const WCHAR* kShadowMissShader = L"shadowMiss";
	const WCHAR* kShadwoClosestHitShader = L"shadowChs";
	const WCHAR* kShadowHitGroup = L"ShadowHitGroup";

	const WCHAR* kSecondaryMissShader = L"secondaryMiss";
	const WCHAR* kSecondaryClosestHitShader = L"secondaryChs";
	const WCHAR* kSecondaryHitGroup = L"SecondaryHitGroup";
};