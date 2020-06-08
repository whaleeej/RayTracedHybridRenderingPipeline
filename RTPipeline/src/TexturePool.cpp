#include "..\inc\TexturePool.h"

static TexturePool* gIns_TexturePool = nullptr;

TexturePool& TexturePool::Get()
{
	if (!gIns_TexturePool)
		gIns_TexturePool = new TexturePool;
	return *gIns_TexturePool;
}

void TexturePool::clear()
{
	texturePool.clear();
}

bool TexturePool::exist(TextureIndex name)
{
	return texturePool.find(name) != texturePool.end();
}

Texture TexturePool::getTexture(TextureIndex name)
{
	auto it = texturePool.find(name);
	if (it != texturePool.end()) return it->second;
	else return Texture();
}

void TexturePool::addTexture(TextureIndex name ,Texture& tex)
{
	auto pair = texturePool.emplace(name, tex);
	assert(pair.second == true);
}

void TexturePool::loadTexture(TextureIndex name, std::string path, TextureUsage ussage, CommandList& commandList)
{
	texturePool.emplace(name, Texture());
	commandList.LoadTextureFromFile(texturePool[name], string_2_wstring(path), ussage);
}

std::string TexturePool::albedoStrToDefined (std::string albedoName, std::string replace_from, std::string replace_to) {
	size_t dotend = albedoName.find_last_of(".");
	int dotInFrom = replace_from.find_last_of(".");
	if (dotInFrom >= 0)
		dotend = albedoName.size();
	size_t start = albedoName.find_last_of(replace_from, dotend);
	size_t length = replace_from.size();
	start -= length;
	std::string ret = albedoName.substr(0, start + 1) + replace_to + albedoName.substr(start + length + 1, albedoName.length());
	return ret;
};

void TexturePool::loadPBRSeriesTexture(TextureIndex pbr_header, std::string albedo_directory, CommandList& commandList)
{
	texturePool.emplace(EXTEND_ALBEDO_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_ALBEDO_HEADER(pbr_header)],string_2_wstring(albedo_directory), TextureUsage::Albedo);
	texturePool.emplace(EXTEND_METALLIC_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_METALLIC_HEADER(pbr_header)], string_2_wstring(albedoStrToDefined(albedo_directory, "albedo", "metallic")), TextureUsage::MetallicMap);
	texturePool.emplace(EXTEND_NORMAL_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_NORMAL_HEADER(pbr_header)], string_2_wstring(albedoStrToDefined(albedo_directory, "albedo", "normal")), TextureUsage::Normalmap);
	texturePool.emplace(EXTEND_ROUGHNESS_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_ROUGHNESS_HEADER(pbr_header)], string_2_wstring(albedoStrToDefined(albedo_directory, "albedo", "roughness")), TextureUsage::RoughnessMap);
}

void TexturePool::loadCubemap(UINT size, TextureIndex pano_name, TextureIndex cubemap_name, CommandList& commandList)
{
	auto skyboxCubemapDesc = texturePool[pano_name].GetD3D12ResourceDesc();
	skyboxCubemapDesc.Width = skyboxCubemapDesc.Height = size * 1;
	skyboxCubemapDesc.DepthOrArraySize = 6;
	skyboxCubemapDesc.MipLevels = 1;
	texturePool.emplace(cubemap_name, Texture(skyboxCubemapDesc, nullptr, TextureUsage::Albedo, string_2_wstring(cubemap_name)));
	commandList.PanoToCubemap(texturePool[cubemap_name], texturePool[pano_name]);
}

