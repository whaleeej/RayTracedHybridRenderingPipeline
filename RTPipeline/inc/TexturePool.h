#pragma once

#include <DX12LibPCH.h>
#include <CommandList.h>
#include <Texture.h>

#define EXTEND_ALBEDO_HEADER(header) header+"_albedo"
#define EXTEND_METALLIC_HEADER(header) header+"_metallic"
#define EXTEND_NORMAL_HEADER(header) header+"_normal"
#define EXTEND_ROUGHNESS_HEADER(header) header+"_roughness"
#define DEFAULT_PBR_HEADER "default"

using TextureIndex = std::string;

class TexturePool {
public:
	static std::string TexturePool::albedoStrToDefined(std::string albedoName, std::string replace_from, std::string replace_to);

public:
	static TexturePool& Get();
	
	void clear();

	bool exist(TextureIndex name);

	Texture getTexture(TextureIndex name);

	void addTexture(TextureIndex name, Texture& tex);

	void loadTexture(TextureIndex name, std::string path, TextureUsage ussage, CommandList& commandList);
	
	void loadPBRSeriesTexture(TextureIndex pbr_header, std::string albedo_directory, CommandList& commandList);

	void loadCubemap(UINT size,TextureIndex pano_name, TextureIndex cubemap_name, CommandList& commandList);


private:
	std::map<TextureIndex, Texture> texturePool;
};