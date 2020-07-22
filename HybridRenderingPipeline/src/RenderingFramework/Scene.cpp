#include <io.h>
#include <random>
#include <cmath>
#include <DirectXColors.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>

#include "Scene.h"


#define SCENE1 1

Scene::Scene():
	m_Forward(0)
	, m_Backward(0)
	, m_Left(0)
	, m_Right(0)
	, m_Up(0)
	, m_Down(0)
	, m_Pitch(0)
	, m_Yaw(0)
	, m_AnimateCamera(false)
	, m_AnimateLights(false)
{
	XMVECTOR cameraPos = XMVectorSet(0, 10, -35, 1);
	XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
	XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

	m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);

	m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);
	if (m_pAlignedCameraData) {
		m_pAlignedCameraData->m_InitialCamPos = m_Camera.get_Translation();
		m_pAlignedCameraData->m_InitialCamRot = m_Camera.get_Rotation();
	}
}

Scene::~Scene()
{
	_aligned_free(m_pAlignedCameraData);
}

void Scene::LoadContent()
{
	loadResource();
	loadGameObject();
	transformGameObject();
}

void Scene::UnLoadContent()
{
	lightObjectIndex = "";
	gameObjectAssembling.clear();
	gameObjectPool.clear();
	cubemap_Index = "";
}

void Scene::OnUpdate(UpdateEventArgs& e)
{
	// camera
	{
		if (m_AnimateCamera)
		{
			cameraAnimTime += static_cast<float>(e.ElapsedTime) * 0.5f * XM_PI / 10.0f;
			// Setup the lights.
			const float radius = 35.0f;
			XMVECTOR cameraPos = XMVectorSet(static_cast<float>(std::sin(cameraAnimTime))* radius,
				10,
				static_cast<float>(std::cos(3.141562653 + cameraAnimTime))* radius,
				1.0);
			XMVECTOR cameraTarget = XMVectorSet(0, 10, 0, 1);
			XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

			m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);
		}
		else {
			float speedMultipler = 4.0f;

			XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
			XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
			m_Camera.Translate(cameraTranslate, Space::Local);
			m_Camera.Translate(cameraPan, Space::Local);

			XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
			m_Camera.set_Rotation(cameraRotation);

			XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
		}
	}

	// light
	{
		static float lightAnimTime = 1.0f * XM_PI;
		if (m_AnimateLights)
		{
			lightAnimTime += static_cast<float>(e.ElapsedTime) * 0.5f * XM_PI;
		}

		// Setup the lights.
		const float radius = 6.0f;
		PointLight& l = m_PointLight;
		l.PositionWS = {
			static_cast<float>(std::sin(lightAnimTime))* radius,
			9,
			static_cast<float>(std::cos(lightAnimTime))* radius,
			1.0f
		};
		l.Color = XMFLOAT4(Colors::White);
		l.Intensity = 2.0f;
		l.Attenuation = 0.01f;
		l.Radius = 2.0f;

		// Update the pointlight gameobject
		{
			gameObjectPool[lightObjectIndex]->Translate(XMMatrixTranslation(l.PositionWS.x, l.PositionWS.y, l.PositionWS.z));
			gameObjectPool[lightObjectIndex]->Scale(XMMatrixScaling(l.Radius, l.Radius, l.Radius));
		}
	}
}

void Scene::OnKeyPressed(KeyEventArgs& e)
{
	switch (e.Key)
	{
	case KeyCode::D1:
		// Reset camera transform
		m_Camera.set_Translation(m_pAlignedCameraData->m_InitialCamPos);
		m_Camera.set_Rotation(m_pAlignedCameraData->m_InitialCamRot);
		m_Pitch = 0.0f;
		m_Yaw = 0.0f;
		break;
	case KeyCode::D2:
		m_AnimateCamera = !m_AnimateCamera;
		cameraAnimTime = 0.0f;
		if (!m_AnimateCamera) {
			m_Camera.set_Translation(m_pAlignedCameraData->m_InitialCamPos);
			m_Camera.set_Rotation(m_pAlignedCameraData->m_InitialCamRot);
			m_Pitch = 0.0f;
			m_Yaw = 0.0f;
		}
		break;
	case KeyCode::D3:
		m_AnimateLights = !m_AnimateLights;
		break;
	case KeyCode::Up:
	case KeyCode::W:
		m_Forward = 2.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_Left = 2.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_Backward = 2.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_Right = 2.0f;
		break;
	case KeyCode::Q:
		m_Down = 2.0f;
		break;
	case KeyCode::E:
		m_Up = 2.0f;
		break;
	}
}

void Scene::OnKeyReleased(KeyEventArgs& e)
{
	switch (e.Key)
	{
	case KeyCode::Up:
	case KeyCode::W:
		m_Forward = 0.0f;
		break;
	case KeyCode::Left:
	case KeyCode::A:
		m_Left = 0.0f;
		break;
	case KeyCode::Down:
	case KeyCode::S:
		m_Backward = 0.0f;
		break;
	case KeyCode::Right:
	case KeyCode::D:
		m_Right = 0.0f;
		break;
	case KeyCode::Q:
		m_Down = 0.0f;
		break;
	case KeyCode::E:
		m_Up = 0.0f;
		break;
	}
}

void Scene::OnMouseMoved(MouseMotionEventArgs& e)
{
	const float mouseSpeed = 0.1f;
	if (e.LeftButton)
	{
		m_Pitch -= e.RelY * mouseSpeed;

		m_Pitch = Math::clamp(m_Pitch, -90.0f, 90.0f);

		m_Yaw -= e.RelX * mouseSpeed;
	}
}

void Scene::OnMouseWheel(MouseWheelEventArgs& e)
{
	auto fov = m_Camera.get_FoV();

	fov -= e.WheelDelta;
	fov = Math::clamp(fov, 12.0f, 90.0f);

	m_Camera.set_FoV(fov);

	char buffer[256];
	sprintf_s(buffer, "FoV: %f\n", fov);
	OutputDebugStringA(buffer);
}

void Scene::OnResize(int width, int height)
{
	float aspectRatio = width / (float)height;
	m_Camera.set_Projection(45.0f, aspectRatio, 0.1f, 100.0f);
}
 
void Scene::addSingleLight(MeshIndex mesh_name, PBRMaterial material, GameObjectIndex name) {
	addSingleGameObject(mesh_name, material, name);
	lightObjectIndex = name;
}

void Scene::addSingleGameObject(MeshIndex mesh_name, PBRMaterial material, GameObjectIndex name) {
	gameObjectPool.emplace(name, std::make_shared<GameObject>());
	gameObjectPool[name]->gid = (float)gNum++;
	gameObjectPool[name]->mesh = mesh_name;
	gameObjectPool[name]->material = material;
	gameObjectAssembling.emplace(name, name);
}

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
		//int a = albedoIndex.find_first_of(':');
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

void Scene::copyGameObjectAssembling(GameObjectIndex from, GameObjectIndex to) {
	auto objAssemble = gameObjectAssembling.equal_range(from);
	std::vector<GameObjectIndex> indices;
	for (auto k = objAssemble.first; k != objAssemble.second; k++) {
		std::shared_ptr<GameObject> localGo = std::make_shared<GameObject>();
		localGo->gid = (float)gNum++;
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
		gameObjectPool[meshIndex]->gid = (float)gNum++;
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

void Scene::loadResource() {
	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	auto& meshPool = MeshPool::Get(); 
	auto& texturePool = TexturePool::Get();

	// Create a Cube mesh
	meshPool.addMesh("cube", Mesh::CreateCube(*commandList));
	meshPool.addMesh("sphere", Mesh::CreateSphere(*commandList));
	meshPool.addMesh("cone", Mesh::CreateCone(*commandList));
	meshPool.addMesh("torus", Mesh::CreateTorus(*commandList));
	meshPool.addMesh("plane", Mesh::CreatePlane(*commandList));

	// Create a skybox mesh
	setSkybox("skybox_cubemap", Mesh::CreateCube(*commandList, 1.0f, true));
	// load cubemap
#ifdef SCENE1
	texturePool.loadTexture("skybox_pano", "Assets/HDR/skybox_default.hdr", TextureUsage::Albedo, *commandList);
#elif SCENE2
	texturePool.loadTexture("skybox_pano", "Assets/HDR/Milkyway_BG.hdr", TextureUsage::Albedo, *commandList);
#else
	texturePool.loadTexture("skybox_pano", "Assets/HDR/Ice_Lake_HiRes_TMap_2.hdr", TextureUsage::Albedo, *commandList);
#endif
	texturePool.loadCubemap(1024, "skybox_pano", "skybox_cubemap", *commandList);

	// load PBR textures
	texturePool.loadPBRSeriesTexture(DEFAULT_PBR_HEADER, "Assets/Textures/pbr/default/albedo.bmp", *commandList);
	texturePool.loadPBRSeriesTexture("rusted_iron", "Assets/Textures/pbr/rusted_iron/albedo.png", *commandList);
	texturePool.loadPBRSeriesTexture("grid_metal", "Assets/Textures/pbr/grid_metal/albedo.png", *commandList);
	texturePool.loadPBRSeriesTexture("metal", "Assets/Textures/pbr/metal/albedo.png", *commandList);

	// run commandlist
	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void Scene::loadGameObject() {

	auto device = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();


	addSingleGameObject("plane", { 0.5f, 0.4f, Material::White }, "Floor plane");

#ifdef SCENE1
	addSingleGameObject("plane", { 0.5f, 0.5f, Material::White }, "Ceiling plane");
	addSingleGameObject("plane", { 0.4f, 0.1f, Material::Pearl }, "Back wall");
	addSingleGameObject("plane", { 0.4f, 0.4f, Material::Copper }, "Front wall");
	addSingleGameObject("plane", { 0.4f, 0.3f, Material::Jade }, "Left wall");
	addSingleGameObject("plane", { 0.4f, 0.3f, Material::Ruby }, "Right wall");
	addSingleGameObject("sphere", { "rusted_iron" }, "sphere");
	addSingleGameObject("cube", { "grid_metal" }, "cube");
	addSingleGameObject("torus", { "metal" }, "torus");
#endif
#ifdef SCENE2
	//load external object
	importModel(commandList, "Assets/Cerberus/Cerberus_LP.FBX", "A.tga", "M.tga", "N.tga", "R.tga");
	importModel(commandList, "Assets/Unreal-actor/model.dae");
	importModel(commandList, "Assets/Sci-fi-Biolab/source/SciFiBiolab.fbx", "COLOR", "METALLIC", "NORMAL", "ROUGHNESS");
#endif
#ifdef SCENE3
	importModel(commandList, "Assets/SM_Chair/SM_Chair.FBX");
	importModel(commandList, "Assets/SM_TableRound/SM_TableRound.FBX");
	importModel(commandList, "Assets/SM_Couch/SM_Couch.FBX");
	importModel(commandList, "Assets/SM_Lamp_Ceiling/SM_Lamp_Ceiling.FBX");
	importModel(commandList, "Assets/SM_MatPreviewMesh/SM_MatPreviewMesh.FBX");
	copyGameObjectAssembling("Assets/SM_Chair", "Assets/SM_Chair_copy");
#endif
	// light object
	addSingleLight("sphere", Material::EmissiveWhite, "sphere light");

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void Scene::transformGameObject() {

	auto device = Application::Get().GetDevice();

	{
		transformSingleAssembling("Assets/SM_TableRound",
			XMMatrixTranslation(0, 0.0, 0)
			, XMMatrixRotationX(-(float)std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		transformSingleAssembling("Assets/SM_Chair",
			XMMatrixTranslation(-10, 0.0, -6)
			, XMMatrixRotationZ(-(float)std::_Pi / 4.0f) * XMMatrixRotationX(-(float)std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		transformSingleAssembling("Assets/SM_Chair_copy",
			XMMatrixTranslation(-4, 0.0, 6)
			, XMMatrixRotationZ((float)std::_Pi / 3.0f) * XMMatrixRotationX(-(float)std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		transformSingleAssembling("Assets/SM_Couch",
			XMMatrixTranslation(8, 0.0, 0)
			, XMMatrixRotationZ(-(float)std::_Pi) * XMMatrixRotationX(-(float)std::_Pi / 2.0f)
			, XMMatrixScaling(0.08f, 0.08f, 0.08f));

		transformSingleAssembling("Assets/SM_Lamp_Ceiling",
			XMMatrixTranslation(2, 0.0, 14)
			, XMMatrixRotationX((float)std::_Pi / 2.0f)
			, XMMatrixScaling(0.12f, 0.12f, 0.12f));

		transformSingleAssembling("Assets/SM_MatPreviewMesh",
			XMMatrixTranslation(0, 5.5, 0)
			, XMMatrixRotationX(-(float)std::_Pi / 2.0f)
			, XMMatrixScaling(0.01f, 0.01f, 0.01f));

		transformSingleAssembling("Assets/Cerberus",
			XMMatrixTranslation(-1.2f, 10.0f, -1.6f)
			, XMMatrixRotationX(-(float)std::_Pi / 3.0f * 2.0f)
			, XMMatrixScaling(0.02f, 0.02f, 0.02f));

		transformSingleAssembling("Assets/Unreal-actor",
			XMMatrixTranslation(0.0f, 10.0f, 0.0f)
			, XMMatrixRotationY((float)std::_Pi)
			, XMMatrixScaling(6.f, 6.f, 6.0f));

		transformSingleAssembling("Assets/Sci-fi-Biolab/source",
			XMMatrixTranslation(0.0f, 0.0f, 0.0f)
			, XMMatrixRotationX(-(float)std::_Pi / 2.0)
			, XMMatrixScaling(8.f, 8.f, 8.0f));

		transformSingleAssembling("sphere"
			, XMMatrixTranslation(4.0f, 3.0f, 2.0f)
			, XMMatrixIdentity()
			, XMMatrixScaling(6.0f, 6.0f, 6.0f));

		transformSingleAssembling("cube"
			, XMMatrixTranslation(-4.0f, 3.0f, -2.0f)
			, XMMatrixRotationY(XMConvertToRadians(45.0f))
			, XMMatrixScaling(6.0f, 6.0f, 6.0f));

		transformSingleAssembling("torus"
			, XMMatrixTranslation(4.0f, 0.6f, -6.0f)
			, XMMatrixRotationY(XMConvertToRadians(45.0f))
			, XMMatrixScaling(4.0f, 4.0f, 4.0f));

		float scalePlane = 20.0f;
		float translateOffset = scalePlane / 2.0f;

		transformSingleAssembling("Floor plane"
			, XMMatrixTranslation(0.0f, 0.0f, 0.0f)
			, XMMatrixIdentity()
			, XMMatrixScaling(scalePlane * 10, 1.0f, scalePlane * 10));

		transformSingleAssembling("Back wall"
			, XMMatrixTranslation(0.0f, translateOffset, translateOffset)
			, XMMatrixRotationX(XMConvertToRadians(-90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		transformSingleAssembling("Ceiling plane"
			, XMMatrixTranslation(0.0f, translateOffset * 2.0f, 0)
			, XMMatrixRotationX(XMConvertToRadians(180))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		transformSingleAssembling("Front wall"
			, XMMatrixTranslation(0, translateOffset, -translateOffset)
			, XMMatrixRotationX(XMConvertToRadians(90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		transformSingleAssembling("Left wall"
			, XMMatrixTranslation(-translateOffset, translateOffset, 0)
			, XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(-90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		transformSingleAssembling("Right wall"
			, XMMatrixTranslation(translateOffset, translateOffset, 0)
			, XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(90))
			, XMMatrixScaling(scalePlane, 1.0f, scalePlane));
	}
}