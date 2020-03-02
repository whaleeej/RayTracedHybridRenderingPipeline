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
#include <DirectXMath.h>

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