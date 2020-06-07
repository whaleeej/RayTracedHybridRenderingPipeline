#include "..\inc\TexturePool.h"

static TexturePool* gIns_TexturePool = nullptr;

TexturePool& TexturePool::Get()
{
	if (!gIns_TexturePool)
		gIns_TexturePool = new TexturePool;
	return *gIns_TexturePool;
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
	assert(pair.second == true)
}

void TexturePool::loadPBRSeriesTexture(TextureIndex pbr_header, std::string albedo_directory, CommandList& commandList)
{
	auto albedoStrToDefined = [&](std::string albedoName, std::string replace_from, std::string replace_to) {
		size_t start = albedoName.find_last_of(replace_from);
		size_t length = replace_from.size();
		start -= length;
		std::string ret = albedoName.substr(0, start + 1) + replace_to + albedoName.substr(start + length + 1, albedoName.length());
		return ret;
	};

	texturePool.emplace(EXTEND_ALBEDO_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_ALBEDO_HEADER(pbr_header)],string_2_wstring(albedo_directory), TextureUsage::Albedo);
	texturePool.emplace(EXTEND_METALLIC_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_METALLIC_HEADER(pbr_header)], string_2_wstring(albedoStrToDefined(albedo_directory, "albedo", "metallic")), TextureUsage::MetallicMap);
	texturePool.emplace(EXTEND_NORMAL_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_NORMAL_HEADER(pbr_header)], string_2_wstring(albedoStrToDefined(albedo_directory, "albedo", "normal")), TextureUsage::Normalmap);
	texturePool.emplace(EXTEND_ROUGHNESS_HEADER(pbr_header), Texture());
	commandList.LoadTextureFromFile(texturePool[EXTEND_ROUGHNESS_HEADER(pbr_header)], string_2_wstring(albedoStrToDefined(albedo_directory, "albedo", "roughness")), TextureUsage::RoughnessMap);
}

