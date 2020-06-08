#pragma once

#include <DirectXMath.h>

enum class Space
{
    Local,
    World,
};

class Camera
{
public:

    Camera();
    virtual ~Camera();

    void XM_CALLCONV set_LookAt( DirectX::FXMVECTOR eye, DirectX::FXMVECTOR target, DirectX::FXMVECTOR up );
    DirectX::XMMATRIX get_ViewMatrix() const;
    DirectX::XMMATRIX get_InverseViewMatrix() const;

    void set_Projection( float fovy, float aspect, float zNear, float zFar );
    DirectX::XMMATRIX get_ProjectionMatrix() const;
    DirectX::XMMATRIX get_InverseProjectionMatrix() const;

    void set_FoV(float fovy);

    float get_FoV() const;

    void XM_CALLCONV set_Translation( DirectX::FXMVECTOR translation );
    DirectX::XMVECTOR get_Translation() const;
    void XM_CALLCONV set_Rotation( DirectX::FXMVECTOR rotation );
    DirectX::XMVECTOR get_Rotation() const;

    void XM_CALLCONV Translate( DirectX::FXMVECTOR translation, Space space = Space::Local );
    void Rotate( DirectX::FXMVECTOR quaternion );

protected:
    virtual void UpdateViewMatrix() const;
    virtual void UpdateInverseViewMatrix() const;
    virtual void UpdateProjectionMatrix() const;
    virtual void UpdateInverseProjectionMatrix() const;

    // This data must be aligned otherwise the SSE intrinsics fail
    // and throw exceptions.
    __declspec(align(16)) struct AlignedData
    {
        // World-space position of the camera.
        DirectX::XMVECTOR m_Translation;
        // World-space rotation of the camera.
        // THIS IS A QUATERNION!!!!
        DirectX::XMVECTOR m_Rotation;

        DirectX::XMMATRIX m_ViewMatrix, m_InverseViewMatrix;
        DirectX::XMMATRIX m_ProjectionMatrix, m_InverseProjectionMatrix;
    };
    AlignedData* pData;

    // projection parameters
    float m_vFoV;   // Vertical field of view.
    float m_AspectRatio; // Aspect ratio
    float m_zNear;      // Near clip distance
    float m_zFar;       // Far clip distance.

    // True if the view matrix needs to be updated.
    mutable bool m_ViewDirty, m_InverseViewDirty;
    // True if the projection matrix needs to be updated.
    mutable bool m_ProjectionDirty, m_InverseProjectionDirty;

private:

};