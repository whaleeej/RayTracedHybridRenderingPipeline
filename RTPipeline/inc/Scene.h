#pragma once

#include <DX12LibPCH.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "TexturePool.h"
#include "GameObject.h"
#include "Light.h"
#include "Camera.h"

using GameObjectIndex = std::string;

class Scene {
public:
	Scene();
	~Scene();

public://light
	PointLight m_PointLight;
	GameObjectIndex lightObjectIndex;

public://gamObject
	std::map<GameObjectIndex, std::shared_ptr<GameObject>> gameObjectPool;
	std::multimap<GameObjectIndex, GameObjectIndex> gameObjectAssembling;
	int gNum = 0;

	void copyGameObjectAssembling(GameObjectIndex from, GameObjectIndex to);

	void addSingleGameObject(MeshIndex mesh_name, PBRMaterial material, GameObjectIndex name);

	void addSingleLight(MeshIndex mesh_name, PBRMaterial material, GameObjectIndex name);

	void transformSingleAssembling(std::string name, XMMATRIX translation, XMMATRIX rotation, XMMATRIX scaling);

public://skybox
	std::unique_ptr<Mesh> m_SkyboxMesh;
	TextureIndex cubemap_Index;

	void setSkybox(TextureIndex name, std::unique_ptr<Mesh> skyboxMesh);

public: //model loader
	void Scene::processMesh(aiMesh* mesh, const aiScene* scene, 
		std::string rootPath, std::string albedo, std::string metallic, std::string normal, std::string roughness, 
		std::shared_ptr<CommandList> commandList);

	void processNode(aiNode* node, const aiScene* scene,
		std::string rootPath, std::string albedo, std::string metallic, std::string normal, std::string roughness,
		std::shared_ptr<CommandList> commandList);

	std::string Scene::importModel(std::shared_ptr<CommandList> commandList, std::string path,
		std::string albedo = "albedo", std::string metallic = "metallic", std::string normal = "normal", std::string roughness = "roughness"
		);

public://camera
	struct alignas(16) CameraData
	{
		DirectX::XMVECTOR m_InitialCamPos;
		DirectX::XMVECTOR m_InitialCamRot;
	};
	Camera m_Camera;
	CameraData* m_pAlignedCameraData;
};