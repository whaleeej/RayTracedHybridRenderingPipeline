#pragma once


#include <CommandList.h>
#include <VertexBuffer.h>
#include <IndexBuffer.h>

#include <DirectXMath.h>
#include <d3d12.h>

#include <wrl.h>

#include <memory> // For std::unique_ptr
#include <vector>

 // Vertex struct holding position, normal vector, and texture mapping information.
struct VertexPositionNormalTexture
{
    VertexPositionNormalTexture()
    { }

    VertexPositionNormalTexture(const DirectX::XMFLOAT3& position, const DirectX::XMFLOAT3& normal, const DirectX::XMFLOAT2& textureCoordinate)
        : position(position),
        normal(normal),
        textureCoordinate(textureCoordinate)
    { }

    VertexPositionNormalTexture(DirectX::FXMVECTOR position, DirectX::FXMVECTOR normal, DirectX::FXMVECTOR textureCoordinate)
    {
        XMStoreFloat3(&this->position, position);
        XMStoreFloat3(&this->normal, normal);
        XMStoreFloat2(&this->textureCoordinate, textureCoordinate);
    }

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 textureCoordinate;

    static const int InputElementCount = 3;
    static const D3D12_INPUT_ELEMENT_DESC InputElements[InputElementCount];
};

using VertexCollection = std::vector<VertexPositionNormalTexture>;
using IndexCollection = std::vector<uint32_t>;

class Mesh
{
public:
    Mesh();
    virtual ~Mesh();
    Mesh(const Mesh& copy) = delete;

    void Draw(CommandList& commandList);

    enum Buildin{
        CUBE = 0,
        SPHERE,
        CONE,
        TORUS,
        PLANE,
        BUILDINEND
    };

    static std::unique_ptr<Mesh> CreateCube(CommandList& commandList, float size = 1, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreateSphere(CommandList& commandList, float diameter = 1, size_t tessellation = 16, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreateCone(CommandList& commandList, float diameter = 1, float height = 1, size_t tessellation = 32, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreateTorus(CommandList& commandList, float diameter = 1, float thickness = 0.333f, size_t tessellation = 32, bool rhcoords = false);
    static std::unique_ptr<Mesh> CreatePlane(CommandList& commandList, float width = 1, float height = 1, bool rhcoords = false);

	VertexBuffer getVertexBuffer() { return m_VertexBuffer; }
	IndexBuffer getIndexBuffer() { return m_IndexBuffer; }
	UINT getVertexCount() { return m_VertexCount; }
	UINT getIndexCount() { return m_IndexCount; }

    void Initialize(CommandList& commandList, VertexCollection& vertices, IndexCollection& indices, bool rhcoords);

protected:

private:
    friend struct std::default_delete<Mesh>;

    VertexBuffer m_VertexBuffer;
    IndexBuffer m_IndexBuffer;

	UINT m_VertexCount;
    UINT m_IndexCount;
};