#pragma once

#include <DX12LibPCH.h>
#include "GameObject.h"
#include "Light.h"

using GameObjectIndex = std::string;

class Scene {

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
};