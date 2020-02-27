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

    /**
     *  Load content required for the demo.
     */
    virtual bool LoadContent() override;

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    virtual void UnloadContent() override;
protected:
    /**
     *  Update the game logic.
     */
    virtual void OnUpdate(UpdateEventArgs& e) override;

    /**
     *  Render stuff.
     */
    virtual void OnRender(RenderEventArgs& e) override;

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    virtual void OnKeyPressed(KeyEventArgs& e) override;

    /**
     * Invoked when a key on the keyboard is released.
     */
    virtual void OnKeyReleased(KeyEventArgs& e);

    /**
     * Invoked when the mouse is moved over the registered window.
     */
    virtual void OnMouseMoved(MouseMotionEventArgs& e);

    /**
     * Invoked when the mouse wheel is scrolled while the registered window has focus.
     */
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
	Camera m_Camera;
	CameraData* m_pAlignedCameraData;

    // Camera controller
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


	////////////////////////////////////////////////////////////////////// Raster  Object 
	// Deferred Render target
	RenderTarget m_DeferredRenderTarget;

	// Root signatures
	RootSignature m_DeferredRootSignature;
	RootSignature m_PostProcessingRootSignature;

	// Pipeline state object.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_DeferredPipelineState;
	// HDR -> PostProcessing tone mapping PSO.
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PostProcessingPipelineState;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	int m_Width;
	int m_Height;

	////////////////////////////////////////////////////////////////////// RT Object 
	void createAccelerationStructures();
	Microsoft::WRL::ComPtr < ID3D12Resource > mpTopLevelAS;
	std::vector<Microsoft::WRL::ComPtr < ID3D12Resource >> mpBottomLevelASes;
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