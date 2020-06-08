#include "..\inc\MeshPool.h"

static MeshPool* gIns_MeshPool;

MeshPool& MeshPool::Get()
{
	if (!gIns_MeshPool) gIns_MeshPool = new MeshPool();
	return *gIns_MeshPool;
}

void MeshPool::clear()
{
	meshPool.clear();
}

std::shared_ptr<Mesh> MeshPool::getMesh(MeshIndex name)
{
	if(meshPool.find(name)==meshPool.end())
		return std::shared_ptr<Mesh>();
	return meshPool[name];
}

void MeshPool::addMesh(MeshIndex name, std::shared_ptr<Mesh> mesh)
{
	auto pair = meshPool.emplace(name, mesh);
	assert(pair.second == true);
}

int MeshPool::size()
{
	return meshPool.size();
}
