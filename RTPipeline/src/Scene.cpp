#include "Scene.h"

void Scene::copyGameObjectAssembling(GameObjectIndex from, GameObjectIndex to) {
	auto objAssemble = gameObjectAssembling.equal_range(from);
	std::vector<GameObjectIndex> indices;
	for (auto k = objAssemble.first; k != objAssemble.second; k++) {
		std::shared_ptr<GameObject> localGo = std::make_shared<GameObject>();
		localGo->gid = gNum++;
		localGo->mesh = gameObjectPool[k->second]->mesh;
		localGo->transform = gameObjectPool[k->second]->transform;
		localGo->material = gameObjectPool[k->second]->material;
		gameObjectPool.emplace(k->second + to, localGo);
		indices.push_back(k->second + to);
	}
	for (size_t i = 0; i < indices.size(); i++)
	{
		gameObjectAssembling.emplace(to, indices[i]);
	}
}

void Scene::addSingleGameObject(MeshIndex mesh_name, PBRMaterial material, GameObjectIndex name) {
	gameObjectPool.emplace(name, std::make_shared<GameObject>());
	gameObjectPool[name]->gid = gNum++;
	gameObjectPool[name]->mesh = mesh_name;
	gameObjectPool[name]->material = material;
	gameObjectAssembling.emplace(name, name);
}

void Scene::addSingleLight(MeshIndex mesh_name, PBRMaterial material, GameObjectIndex name) {
	addSingleGameObject(mesh_name, material, name);
	lightObjectIndex = name;
}

void Scene::transformSingleAssembling(std::string name, XMMATRIX translation, XMMATRIX rotation, XMMATRIX scaling) {
	auto objAssemble = gameObjectAssembling.equal_range(name);
	for (auto k = objAssemble.first; k != objAssemble.second; k++) {
		gameObjectPool[k->second]->Translate(translation);
		gameObjectPool[k->second]->Rotate(rotation);
		gameObjectPool[k->second]->Scale(scaling);
	}
}

void Scene::setSkybox(TextureIndex name, std::unique_ptr<Mesh> skyboxMesh) {
	cubemap_Index = name;
	m_SkyboxMesh = std::move(skyboxMesh);
}