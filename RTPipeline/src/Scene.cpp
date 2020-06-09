#include "Scene.h"

#include <io.h>

Scene::Scene()
{
	XMVECTOR cameraPos = XMVectorSet(0, 10, -35, 1);
	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

	m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);

	m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);
	m_pAlignedCameraData->m_InitialCamPos = m_Camera.get_Translation();
	m_pAlignedCameraData->m_InitialCamRot = m_Camera.get_Rotation();
}

Scene::~Scene()
{
	_aligned_free(m_pAlignedCameraData);
}

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

void Scene::processMesh(aiMesh* mesh, const aiScene* scene, 
	std::string rootPath, std::string albedo, std::string metallic, std::string normal, std::string roughness, 
	std::shared_ptr<CommandList> commandList) {
	auto& texturePool = TexturePool::Get();
	auto& meshPool = MeshPool::Get();
	
	VertexCollection vertices;
	IndexCollection indices;
	for (size_t i = 0; i < mesh->mNumVertices; i++)// 构建vertex
	{
		VertexPositionNormalTexture vnt;
		vnt.position.x = mesh->mVertices[i].x;
		vnt.position.y = mesh->mVertices[i].y;
		vnt.position.z = mesh->mVertices[i].z;
		vnt.normal.x = mesh->mNormals[i].x;
		vnt.normal.y = mesh->mNormals[i].y;
		vnt.normal.z = mesh->mNormals[i].z;
		if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
		{
			vnt.textureCoordinate.x = mesh->mTextureCoords[0][i].x;
			vnt.textureCoordinate.y = 1.0f - mesh->mTextureCoords[0][i].y;
		}
		else {
			vnt.textureCoordinate.x = 0.0f;
			vnt.textureCoordinate.y = 0.0f;
		}
		vertices.push_back(vnt);
	}
	for (size_t i = 0; i < mesh->mNumFaces; i++)// 构建indices
	{
		aiFace face = mesh->mFaces[i];
		for (size_t j = 0; j < face.mNumIndices; j++)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	TextureIndex albedoIndex;
	TextureIndex metallicIndex;
	TextureIndex normalIndex;
	TextureIndex roughnessIndex;
	// 加载贴图
	if (mesh->mMaterialIndex >= 0)
	{
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		//加载模型内的albedo贴图 顺带加载同目录下的Metallic，Normal，Roughness
		unsigned int count = material->GetTextureCount(aiTextureType_DIFFUSE);
		aiString str;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &str);//默认第一个
		albedoIndex = str.data;
		if ((int)albedoIndex.find_first_of(':') < 0) {
			albedoIndex = rootPath + "/" + albedoIndex;
		}
		metallicIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, metallic);
		normalIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, normal);
		roughnessIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, roughness);
	}
	if (!texturePool.exist(albedoIndex)//错误检查
		|| !texturePool.exist(metallicIndex)
		|| !texturePool.exist(normalIndex)
		|| !texturePool.exist(roughnessIndex)
		) {
		albedoIndex = rootPath + "/" + "default_albedo.jpg";
		albedoIndex = TexturePool::albedoStrToDefined(albedoIndex, "albedo", albedo);
		metallicIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, metallic);
		normalIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, normal);
		roughnessIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, roughness);
	}

	////生成mesh //因为index是int32 所以可能要分多批mesh填装
	size_t slot_size = 0xffffffff - 3;
	for (size_t i = 0; i < (int)std::ceil((float)indices.size() / slot_size); i++)
	{
		VertexCollection localVetices;
		IndexCollection localIndices;
		size_t start = i * slot_size;
		size_t end = std::min(start + slot_size, indices.size());
		for (size_t j = 0; j < vertices.size(); j++)
		{
			localVetices.push_back(vertices[j]);
		}
		for (size_t j = start; j < end; j++)
		{
			localIndices.push_back(indices[j]);
		}
		MeshIndex meshIndex;
		meshIndex = meshIndex + "scene_" + std::to_string((uint64_t)scene) + "_mesh_" + std::to_string((uint64_t)mesh) + "_" + std::to_string(i);
		std::shared_ptr<Mesh> dxMesh = std::make_shared<Mesh>();
		dxMesh->Initialize(*commandList, localVetices, localIndices, true);
		meshPool.addMesh(meshIndex, dxMesh);

		//缓存gameobject
		gameObjectPool.emplace(meshIndex, std::make_shared<GameObject>());//6
		gameObjectPool[meshIndex]->gid = gNum++;
		gameObjectPool[meshIndex]->mesh = meshIndex;
		gameObjectPool[meshIndex]->material = {Material::White};
		gameObjectPool[meshIndex]->material.setPBRTex(albedoIndex, metallicIndex, normalIndex, roughnessIndex);
		gameObjectAssembling.emplace(rootPath, meshIndex);
	}
};

void Scene::processNode(aiNode* node, const aiScene* scene,
	std::string rootPath, std::string albedo, std::string metallic, std::string normal, std::string roughness,
	std::shared_ptr<CommandList> commandList) {
	// processNodes
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		processMesh(mesh, scene, rootPath, albedo, metallic, normal, roughness, commandList);
	}
	// then do the same for each of its children
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		processNode(node->mChildren[i], scene, rootPath, albedo, metallic, normal, roughness, commandList);
	}
};

std::string Scene::importModel(std::shared_ptr<CommandList> commandList,
	std::string path, std::string albedo, std::string metallic, std::string normal, std::string roughness) {

	auto& texturePool = TexturePool::Get();

	std::string rootPath = path.substr(0, path.find_last_of('/'));

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals /* | aiProcess_FlipUVs*/);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		ThrowIfFailed(-1);
	}
	//先缓存所有的texture
	bool isLoaded = false;
	for (size_t i = 0; i < scene->mNumMaterials; i++)
	{
		aiMaterial* material = scene->mMaterials[i];
		int count = material->GetTextureCount(aiTextureType_DIFFUSE);
		if (count <= 0) {
			continue;
		}
		aiString str;
		//默认的diffuse贴图
		material->GetTexture(aiTextureType_DIFFUSE, 0, &str); // 这里默认这个mat中这个类型的贴图只有一张
		TextureIndex albedoIndex;
		albedoIndex = str.data;
		int a = albedoIndex.find_first_of(':');
		if ((int)albedoIndex.find_first_of(':') < 0) {
			albedoIndex = rootPath + "/" + albedoIndex;
		}
		TextureIndex metallicIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, metallic);
		TextureIndex normalIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, normal);
		TextureIndex roughnessIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, roughness);
		texturePool.loadTexture(albedoIndex, albedoIndex, TextureUsage::Albedo, *commandList);
		texturePool.loadTexture(metallicIndex, metallicIndex, TextureUsage::MetallicMap, *commandList);
		texturePool.loadTexture(normalIndex, normalIndex, TextureUsage::Normalmap, *commandList);
		texturePool.loadTexture(roughnessIndex, roughnessIndex, TextureUsage::RoughnessMap, *commandList);
		isLoaded = true;
	}
	TextureIndex albedoIndex = rootPath + "/" + "default_albedo.jpg";
	albedoIndex = TexturePool::albedoStrToDefined(albedoIndex, "albedo", albedo);
	TextureIndex metallicIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, metallic);
	TextureIndex normalIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, normal);
	TextureIndex roughnessIndex = TexturePool::albedoStrToDefined(albedoIndex, albedo, roughness);
	if (_access(albedoIndex.c_str(), 0) >= 0) {
		texturePool.loadTexture(albedoIndex, albedoIndex, TextureUsage::Albedo, *commandList);
	}
	if (_access(metallicIndex.c_str(), 0) >= 0) {
		texturePool.loadTexture(metallicIndex, metallicIndex, TextureUsage::MetallicMap, *commandList);
	}
	if (_access(normalIndex.c_str(), 0) >= 0) {
		texturePool.loadTexture(normalIndex, normalIndex, TextureUsage::Normalmap, *commandList);
	}
	if (_access(roughnessIndex.c_str(), 0) >= 0) {
		texturePool.loadTexture(roughnessIndex, roughnessIndex, TextureUsage::RoughnessMap, *commandList);
	}
	//随后load mesh并且绑定贴图加载入
	processNode(scene->mRootNode, scene, rootPath, albedo, metallic, normal, roughness, commandList);
	return rootPath;
}