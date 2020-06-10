#include "GameObject.h"

GameObject::GameObject() :
	gid(0.0f),
	mesh(""),
	transform(XMMatrixTranslation(0, 0, 0), XMMatrixIdentity(), XMMatrixScaling(1.0f, 1.0f, 1.0f)),
	material("default") {}


GameObject::GameObject(float id, MeshIndex index, PBRMaterial material) :
	gid(id),
	mesh(index),
	transform(XMMatrixTranslation(0, 0, 0), XMMatrixIdentity(), XMMatrixScaling(1.0f, 1.0f, 1.0f)),
	material(material) {}

void GameObject::Translate(XMMATRIX translationMatrix) {
	transform.translationMatrix = translationMatrix;
}

void GameObject::Rotate(XMMATRIX rotationMatrix) {
	transform.rotationMatrix = rotationMatrix;
}

void GameObject::Scale(XMMATRIX scaleMatrix) {
	transform.scaleMatrix = scaleMatrix;
}

void GameObject::Draw(CommandList& commandList, Camera& camera)
{
	commandList.SetGraphicsDynamicConstantBuffer(0, transform.ComputeMatCB(camera.get_ViewMatrix(), camera.get_ProjectionMatrix()));
	commandList.SetGraphicsDynamicConstantBuffer(1, material.computeMaterialCB());
	commandList.SetGraphicsDynamicConstantBuffer(2, material.computePBRMaterialCB());
	commandList.SetGraphics32BitConstants(3, gid);
	commandList.SetShaderResourceView(4, 0, *material.getAlbedoTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(4, 1, *material.getMetallicTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(4, 2, *material.getNormalTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(4, 3, *material.getRoughnessTexture(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto& meshPool = MeshPool::Get().getPool();
	if (meshPool.find(mesh) != meshPool.end()) {
		meshPool[mesh]->Draw(commandList);
	}
}