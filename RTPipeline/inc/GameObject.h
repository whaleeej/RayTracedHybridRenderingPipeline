#pragma once
#include <DX12LibPCH.h>
#include "MeshPool.h"
#include "Transform.h"
#include "PBRMaterial.h"
#include "Camera.h"

class GameObject
{
public:
	float gid;
	MeshIndex mesh;
	Transform transform;
	PBRMaterial material;

	GameObject();
	GameObject(float id, MeshIndex index, PBRMaterial material);

	void Translate(XMMATRIX translationMatrix);
	void Rotate(XMMATRIX rotationMatrix);
	void Scale(XMMATRIX scaleMatrix);
	void Draw(CommandList& commandList, Camera& camera);
};