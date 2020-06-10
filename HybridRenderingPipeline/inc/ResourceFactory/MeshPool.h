#pragma once

#include <DX12LibPCH.h>
#include "Mesh.h"

using MeshIndex = std::string;

class MeshPool {
public:
	static MeshPool& Get();
	void clear();
	std::shared_ptr<Mesh> getMesh(MeshIndex name);
	void addMesh(MeshIndex name, std::shared_ptr<Mesh> mesh);
	int size();
	std::map<MeshIndex, std::shared_ptr<Mesh>>& getPool() { return meshPool; }

private:
	std::map<MeshIndex, std::shared_ptr<Mesh>> meshPool;
};