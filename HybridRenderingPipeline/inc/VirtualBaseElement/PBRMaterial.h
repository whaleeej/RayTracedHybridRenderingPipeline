#pragma once
#include <DX12LibPCH.h>
#include "Material.h"
#include "TexturePool.h"

class PBRMaterial : public Material
{
public:
	struct PBRMaterialCB {
		float metallic;
		float roughness;
		float padding[2];
	};

public:
	PBRMaterial(float m, float r, Material mat): Material(mat) {
		setPBRBase(m, r);
		setPBRTexHeader();
	}

	PBRMaterial(TextureIndex name):Material(Material::White) {
		setPBRBase(1.0f, 1.0f);
		setPBRTexHeader(name);
	}

	PBRMaterial(Material mat) :Material(mat) { // emissive
		setPBRBase(1.0f, 1.0f);
		setPBRTexHeader();
	}

	void setPBRBase(float m, float r) {
		metallic = Math::clamp(m, 0.0001f, 1.0f);
		roughness = Math::clamp(r, 0.0001f, 1.0f);
	}
	void setPBRTexHeader(TextureIndex pbr_header = DEFAULT_PBR_HEADER) {
		AlbedoTexture = EXTEND_ALBEDO_HEADER(pbr_header);
		MetallicTexture = EXTEND_METALLIC_HEADER(pbr_header);
		NormalTexture = EXTEND_NORMAL_HEADER(pbr_header);
		RoughnessTexture = EXTEND_ROUGHNESS_HEADER(pbr_header);
	}

	void setPBRTex(TextureIndex a, TextureIndex m, TextureIndex n, TextureIndex r) {
		AlbedoTexture = a;
		MetallicTexture = m;
		NormalTexture = n;
		RoughnessTexture = r;
	}

	float getMetallic() { return metallic; }
	float getRoughness() { return roughness;  }
	PBRMaterialCB computePBRMaterialCB(){
		return { metallic, roughness };
	}
	std::shared_ptr<Texture> getAlbedoTexture() { return TexturePool::Get().getTexture(AlbedoTexture); }
	std::shared_ptr<Texture> getMetallicTexture() { return TexturePool::Get().getTexture(MetallicTexture); }
	std::shared_ptr<Texture> getNormalTexture() { return TexturePool::Get().getTexture(NormalTexture); }
	std::shared_ptr<Texture> getRoughnessTexture() { return TexturePool::Get().getTexture(RoughnessTexture); }

private: 
	float metallic;
	float roughness;
	TextureIndex AlbedoTexture;
	TextureIndex MetallicTexture;
	TextureIndex NormalTexture;
	TextureIndex RoughnessTexture;
};