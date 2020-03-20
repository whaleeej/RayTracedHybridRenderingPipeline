#pragma once
#include <DX12LibPCH.h>
#include <Camera.h>
#include <Game.h>
#include <IndexBuffer.h>
#include <Light.h>
#include <Window.h>
#include <Mesh.h>
#include <RenderTarget.h>
#include <RootSignature.h>
#include <Texture.h>
#include <VertexBuffer.h>
#include <RTHelper.h>
#include <DirectXMath.h>
#include <Material.h>

// temporal define for workset dim
#define LOCAL_WIDTH 8
#define LOCAL_HEIGHT 8
#define BLOCK_EDGE_LENGTH 32
#define BLOCK_PIXELS BLOCK_EDGE_LENGTH*BLOCK_EDGE_LENGTH
#define BLOCK_EDGE_HALF (BLOCK_EDGE_LENGTH / 2)
#define WORKSET_WIDTH (BLOCK_EDGE_LENGTH * ((m_Width + BLOCK_EDGE_LENGTH - 1) / BLOCK_EDGE_LENGTH))
#define WORKSET_HEIGHT (BLOCK_EDGE_LENGTH *  ((m_Height + BLOCK_EDGE_LENGTH - 1) / BLOCK_EDGE_LENGTH))
#define WORKSET_WITH_MARGINS_WIDTH (WORKSET_WIDTH + BLOCK_EDGE_LENGTH)
#define WORKSET_WITH_MARGINS_HEIGHT (WORKSET_HEIGHT + BLOCK_EDGE_LENGTH)
#define OUTPUT_SIZE (WORKSET_WIDTH * WORKSET_HEIGHT)
#define LOCAL_SIZE 256
#define FITTER_GLOBAL (LOCAL_SIZE * ((WORKSET_WITH_MARGINS_WIDTH / BLOCK_EDGE_LENGTH) * (WORKSET_WITH_MARGINS_HEIGHT / BLOCK_EDGE_LENGTH)))
// feature buffer define
#define BUFFER_COUNT 13
#define FEATURES_NOT_SCALED 4
#define FEATURES_SCALED 6
#define NOT_SCALED_FEATURE_BUFFERS \
"1.f,"\
"normal.x,"\
"normal.y,"\
"normal.z,"
#define SCALED_FEATURE_BUFFERS \
"world_position.x,"\
"world_position.y,"\
"world_position.z,"\
"world_position.x*world_position.x,"\
"world_position.y*world_position.y,"\
"world_position.z*world_position.z"

class HybridPipeline : public Game
{
public:
    using super = Game;

    HybridPipeline(const std::wstring& name, int width, int height, bool vSync = false);
    virtual ~HybridPipeline();
    virtual bool LoadContent() override;
    virtual void UnloadContent() override;

protected:
    virtual void OnUpdate(UpdateEventArgs& e) override;
    virtual void OnRender(RenderEventArgs& e) override;
    virtual void OnKeyPressed(KeyEventArgs& e) override;
    virtual void OnKeyReleased(KeyEventArgs& e);
    virtual void OnMouseMoved(MouseMotionEventArgs& e);
    virtual void OnMouseWheel(MouseWheelEventArgs& e) override;
    virtual void OnResize(ResizeEventArgs& e) override; 

private:
	void loadResource();
	void loadGameObject();
	void transformGameObject();
	void loadDXResource();
	void loadPipeline();
	void updateBuffer();

private:
	// Transform
	struct MatCB
	{
		XMMATRIX ModelMatrix;
		XMMATRIX InverseTransposeModelMatrix;
		XMMATRIX ModelViewProjectionMatrix;
	};
	struct Transform
	{
		Transform(XMMATRIX trans, XMMATRIX rot, XMMATRIX scale) :
			translationMatrix(trans), rotationMatrix(rot), scaleMatrix(scale) {};
		XMMATRIX translationMatrix;
		XMMATRIX rotationMatrix;
		XMMATRIX scaleMatrix;

		XMMATRIX ComputeModel()
		{
			return scaleMatrix * rotationMatrix * translationMatrix;
		}
		MatCB ComputeMatCB(CXMMATRIX view, CXMMATRIX projection)
		{
			MatCB matCB;
			matCB.ModelMatrix = scaleMatrix * rotationMatrix * translationMatrix;
			matCB.InverseTransposeModelMatrix = XMMatrixTranspose(XMMatrixInverse(nullptr, matCB.ModelMatrix));
			matCB.ModelViewProjectionMatrix = matCB.ModelMatrix * view * projection;
			return matCB;
		}
	};
	// Material
	using BaseMaterial = Material;
	using TextureIndex = std::string;
	struct PBRMaterial
	{
		float metallic;
		float roughness;
		uint32_t  Padding[2];
		PBRMaterial(float m, float r) :metallic(m), roughness(r), Padding{ 0 } {};
	};
	struct TextureMaterial
	{
		TextureMaterial(TextureIndex index_leading) {
			AlbedoTexture = index_leading + "_albedo";
			MetallicTexture = index_leading + "_metallic";
			NormalTexture = index_leading + "_normal";
			RoughnessTexture = index_leading + "_roughness";
		}
		TextureIndex AlbedoTexture;
		TextureIndex MetallicTexture;
		TextureIndex NormalTexture;
		TextureIndex RoughnessTexture;
	};
	struct HybridMaterial
	{
		HybridMaterial(TextureIndex index_leading) 
			:base{ Material::White }, pbr{ 1.0f, 1.0f }, tex(index_leading){}
		BaseMaterial base;
		PBRMaterial pbr;
		TextureMaterial tex;
	};
	// Mesh
	using MeshIndex = std::string;
	// GameObject
	using GameObjectIndex = std::string;
	class GameObject 
	{
	public:
		float gid;
		MeshIndex mesh;
		Transform transform;
		HybridMaterial material;

		GameObject() :
			mesh(""),
			transform(XMMatrixTranslation(0, 0, 0), XMMatrixIdentity(), XMMatrixScaling(1.0f, 1.0f, 1.0f)),
			material("default") {}
		void Translate(XMMATRIX translationMatrix) {
			transform.translationMatrix = translationMatrix;
		}
		void Rotate(XMMATRIX rotationMatrix) {
			transform.rotationMatrix = rotationMatrix;
		}
		void Scale(XMMATRIX scaleMatrix) {
			transform.scaleMatrix = scaleMatrix;
		}
		void Draw(CommandList& commandList, Camera& camera, std::map<TextureIndex, Texture>& texturePool, std::map<MeshIndex, std::shared_ptr<Mesh>>& meshPool);
	};
	//Camera
	struct CameraRTCB
	{
		XMVECTOR PositionWS;
		XMMATRIX InverseViewMatrix;
		float fov;
		float padding[3];
	};
	struct alignas(16) CameraData
	{
		DirectX::XMVECTOR m_InitialCamPos;
		DirectX::XMVECTOR m_InitialCamRot;
	};
	// Frame
	struct FrameIndexCB
	{
		uint32_t FrameIndex;
		float padding[3];
	};
	// ObjectIndex
	struct ObjectIndexRTCB {
		float index;
		float padding[3];
	};

	/////////////////////////////////////////////// Container
	std::map<MeshIndex, std::shared_ptr<Mesh>> meshPool;
	std::map<TextureIndex, Texture> texturePool;
	std::map<GameObjectIndex, std::shared_ptr<GameObject>> gameObjectPool;
	PointLight m_PointLight;
	GameObjectIndex lightObjectIndex;

	/////////////////////////////////////////////// Camera controller
	Camera m_Camera;
	CameraData* m_pAlignedCameraData;
    float m_Forward;
    float m_Backward;
    float m_Left;
    float m_Right;
    float m_Up;
    float m_Down;
    float m_Pitch;
    float m_Yaw;
    // Rotate the lights in a circle.
    bool m_AnimateLights;
    // Set to true if the Shift key is pressed.
    bool m_Shift;

	/////////////////////////////////////////////// Raster  Object 
	// Deferred gbuffer gen
	RenderTarget m_GBuffer;
	Texture gPosition; //srv
	Texture gAlbedoMetallic; //srv
	Texture gNormalRoughness; //srv
	Texture gExtra; //srv
	Texture gPosition_prev; //srv
	Texture gAlbedoMetallic_prev; //srv
	Texture gNormalRoughness_prev; //srv
	Texture gExtra_prev; //srv

	// SVGF Post Temporal
	Texture col_acc; //uav
	Texture moment_acc; //uav
	Texture his_length; //uav
	Texture col_acc_prev; //srv
	Texture moment_acc_prev; //srv
	Texture his_length_prev; //srv

	XMMATRIX viewProjectMatrix_prev;

	// SVGF PostATrous
	Texture color_inout[2];
	Texture variance_inout[2];

	uint32_t ATrous_Level_Max = 2;

	// Post SpatialResample
	Texture g_indirectOutput;

	// Post Temporal Resample
	Texture radiance_acc;
	Texture radiance_acc_prev;

	// BMFR_1_TemporalNoisy
	Texture noisy_curr;
	Texture noisy_prev;
	Texture spp_curr;
	Texture spp_prev;
	Texture pixel_reproject;
	Texture pixel_accept;
	Texture A_LQS_matrix;

	// BMFR_2_QRFactorization
	Texture lqs_weights;
	Texture feature_scale_minmax;

	// Root signatures
	RootSignature m_DeferredRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_DeferredPipelineState;

	RootSignature m_PostSVGFTemporalRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostSVGFTemporalPipelineState;

	RootSignature m_PostSVGFATrousRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostSVGFATrousPipelineState;

	RootSignature m_PostSpatialResampleRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostSpatialResampleState;

	RootSignature m_PostTemporalResampleRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostTemporalResampleState;

	RootSignature m_PostLightingRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostLightingPipelineState;

	RootSignature m_PostBMFRTemporalNoisyRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostBMFRTemporalNoisyPipelineState;

	RootSignature m_PostBMFRQRFactorizationRootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostBMFRQRFactorizationPipelineState;


	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	int m_Width;
	int m_Height;
	int local_width= LOCAL_WIDTH;
	int local_height= LOCAL_HEIGHT;

	/////////////////////////////////////////////// RT Object 
	void createAccelerationStructures();
	Microsoft::WRL::ComPtr < ID3D12Resource > mpTopLevelAS;
	std::vector<Microsoft::WRL::ComPtr < ID3D12Resource>> mpBottomLevelASes;
	uint64_t mTlasSize = 0;

	void createRtPipelineState();
	RootSignatureDesc createLocalRootDesc(int uav_num, int srv_num, int sampler_num, const D3D12_STATIC_SAMPLER_DESC* pStaticSamplers, int cbv_num, int c32_num, int space=0);
	Microsoft::WRL::ComPtr < ID3D12StateObject > mpPipelineState;
	Microsoft::WRL::ComPtr < ID3D12RootSignature > mpEmptyRootSig;

	void createShaderResources();
	void createSrvUavHeap();
	void updateSrvUavHeap();
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