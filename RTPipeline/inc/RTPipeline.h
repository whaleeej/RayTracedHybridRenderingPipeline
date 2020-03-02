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
	// Light
	struct LightPropertiesCB
	{
		uint32_t NumPointLights;
		uint32_t NumSpotLights;
		float padding[2];
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

	/////////////////////////////////////////////// Container
	std::map<MeshIndex, std::shared_ptr<Mesh>> meshPool;
	std::map<TextureIndex, Texture> texturePool;
	std::map<GameObjectIndex, std::shared_ptr<GameObject>> gameObjectPool;
	int numPointLights = 1;
	int numSpotLights = 1;
	std::vector<PointLight> m_PointLights;
	std::vector<SpotLight> m_SpotLights;

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
	
	// Post Temporal
	RenderTarget m_VarianceBuffer;  // = variance_inout[1];
	Texture col_acc; //uav
	Texture moment_acc; //uav
	Texture his_length; //uav
	Texture gPosition_prev; //srv
	Texture gAlbedoMetallic_prev; //srv
	Texture gNormalRoughness_prev; //srv
	Texture gExtra_prev; //srv
	Texture col_acc_prev; //srv
	Texture moment_acc_prev; //srv
	Texture his_length_prev; //srv

	XMMATRIX viewProjectMatrix_prev;

	// PostATrous
	Texture color_inout[2];
	Texture variance_inout[2];

	uint32_t ATrous_Level_Max = 3;

	// Root signatures
	RootSignature m_DeferredRootSignature;
	RootSignature m_PostTemporalRootSignature;
	RootSignature m_PostATrousRootSignature;

	// Pipeline state object.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_DeferredPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostTemporalPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostATrousPipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;
	int m_Width;
	int m_Height;

	/////////////////////////////////////////////// RT Object 
	void createAccelerationStructures();
	Microsoft::WRL::ComPtr < ID3D12Resource > mpTopLevelAS;
	std::vector<Microsoft::WRL::ComPtr < ID3D12Resource>> mpBottomLevelASes;
	uint64_t mTlasSize = 0;

	void createRtPipelineState();
	RootSignatureDesc createLocalRootDesc(int uav_num, int srv_num, int cbv_num, int c32_num);
	Microsoft::WRL::ComPtr < ID3D12StateObject > mpPipelineState;
	Microsoft::WRL::ComPtr < ID3D12RootSignature > mpEmptyRootSig;

	void createShaderResourcesAndSrvUavheap();
	Microsoft::WRL::ComPtr < ID3D12DescriptorHeap > mpSrvUavHeap;
	std::shared_ptr< Texture > mpOutputTexture;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTPointLightSB;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTSpotLightSB;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTLightPropertiesCB;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpRTCameraCB;
	Microsoft::WRL::ComPtr <ID3D12Resource> mpFrameIndexCB;

	void createShaderTable();
	Microsoft::WRL::ComPtr < ID3D12Resource> mpShaderTable;
	uint32_t mShaderTableEntrySize = 0;

	/*|-RayGen-|-RayMiss-|-......-|-AnyHitShader-|-ChosetHitShader-|-......-|*/
	const WCHAR* kRayGenShader = L"rayGen";
	const WCHAR* kMissShader = L"miss";
	const WCHAR* kAnyHitShader = L"ahs";
	const WCHAR* kClosestHitShader = L"chs";
	const WCHAR* kHitGroup = L"HitGroup";
};