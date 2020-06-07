#pragma once
#include <DX12LibPCH.h>
#include "Material.h"
#include "TexturePool.h"

class PBRMaterial : public Material
{
public:
	PBRMaterial(float m, float r){
		setPBRBase(m, r);
		setPBRTex(DEFAULT_PBR_HEADER);
	}

	PBRMaterial(TextureIndex name) {
		setPBRBase(1.0f, 1.0f);
		setPBRTex(name);
	}


	void setPBRBase(float m, float r) {
		metallic = Math::clamp(m, 0.0001f, 1.0f);
		roughness = Math::clamp(r, 0.0001f, 1.0f);
	}
	void setPBRTex(TextureIndex pbr_header) {
		AlbedoTexture = EXTEND_ALBEDO_HEADER(pbr_header);
		MetallicTexture = EXTEND_METALLIC_HEADER(pbr_header);
		NormalTexture = EXTEND_NORMAL_HEADER(pbr_header);
		RoughnessTexture = EXTEND_ROUGHNESS_HEADER(pbr_header);
	}

	float getMetallic() { return metallic; }
	float getRoughness() { return roughness;  }
	Texture getAlbedoTexture() { return TexturePool::Get().getTexture(AlbedoTexture); }
	Texture getMetallicTexture() { return TexturePool::Get().getTexture(MetallicTexture); }
	Texture getNormalTexture() { return TexturePool::Get().getTexture(NormalTexture); }
	Texture getRoughnessTexture() { return TexturePool::Get().getTexture(RoughnessTexture); }

private: 
	float metallic;
	float roughness;
	TextureIndex AlbedoTexture;
	TextureIndex MetallicTexture;
	TextureIndex NormalTexture;
	TextureIndex RoughnessTexture;
};