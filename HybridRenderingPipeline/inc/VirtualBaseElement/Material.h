#pragma once
#include <DX12LibPCH.h>

class Material
{
public:
    struct MaterialCB {
        DirectX::XMFLOAT4 Emissive;
        DirectX::XMFLOAT4 Ambient;
        DirectX::XMFLOAT4 Diffuse;
        DirectX::XMFLOAT4 Specular;
        float SpecularPower;
        float padding[3];
    };

public:
    Material(
        DirectX::XMFLOAT4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f },
        DirectX::XMFLOAT4 ambient = { 0.1f, 0.1f, 0.1f, 1.0f },
        DirectX::XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f },
        DirectX::XMFLOAT4 specular = { 1.0f, 1.0f, 1.0f, 1.0f },
        float specularPower = 128.0f
    )
        : Emissive( emissive )
        , Ambient( ambient )
        , Diffuse( diffuse )
        , Specular( specular )
        , SpecularPower( specularPower ) 
    {}

    Material(const Material& mat)
        : Emissive(mat.Emissive)
        , Ambient(mat.Ambient)
        , Diffuse(mat.Diffuse)
        , Specular(mat.Specular)
        , SpecularPower(mat.SpecularPower)
    {}

    MaterialCB computeMaterialCB(){
        return { Emissive , Ambient , Diffuse, Specular , SpecularPower };
    }

    DirectX::XMFLOAT4 Emissive;
    DirectX::XMFLOAT4 Ambient;
    DirectX::XMFLOAT4 Diffuse;
    DirectX::XMFLOAT4 Specular;
    float SpecularPower;
    
    // Define some interesting materials.
    static const Material Red;
    static const Material Green;
    static const Material Blue;
    static const Material Cyan;
    static const Material Magenta;
    static const Material Yellow;
    static const Material White;
    static const Material Black;
    static const Material Emerald;
    static const Material Jade;
    static const Material Obsidian;
    static const Material Pearl;
    static const Material Ruby;
    static const Material Turquoise;
    static const Material Brass;
    static const Material Bronze;
    static const Material Chrome;
    static const Material Copper;
    static const Material Gold;
    static const Material Silver;
    static const Material BlackPlastic;
    static const Material CyanPlastic;
    static const Material GreenPlastic;
    static const Material RedPlastic;
    static const Material WhitePlastic;
    static const Material YellowPlastic;
    static const Material BlackRubber;
    static const Material CyanRubber;
    static const Material GreenRubber;
    static const Material RedRubber;
    static const Material WhiteRubber;
    static const Material YellowRubber;
	static const Material EmissiveWhite;
};