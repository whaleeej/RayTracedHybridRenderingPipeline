#include <RTPipeline.h>

#include <Application.h>
#include <CommandQueue.h>
#include <CommandList.h>
#include <Helpers.h>
#include <Light.h>

#include <Window.h>

#include <wrl.h>
using namespace Microsoft::WRL;

#include <d3dx12.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>
#include <ResourceStateTracker.h>
#include <ASBuffer.h>
#include <Helpers.h>
#include <DirectXMath.h>

using namespace DirectX;

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

static bool g_AllowFullscreenToggle = true;
static uint64_t frameCount = 0;
static uint64_t totalFrameCount = 0;
static double totalTime = 0.0;

HybridPipeline::HybridPipeline(const std::wstring& name, int width, int height, bool vSync)
    : super(name, width, height, vSync)
    , m_ScissorRect(CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX))
    , m_Viewport(CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height)))
    , m_Forward(0)
    , m_Backward(0)
    , m_Left(0)
    , m_Right(0)
    , m_Up(0)
    , m_Down(0)
    , m_Pitch(0)
    , m_Yaw(0)
    , m_AnimateLights(false)
    , m_Shift(false)
	, m_Width(0)
	, m_Height(0)
{

    XMVECTOR cameraPos = XMVectorSet(0, 10, -35 , 1);
    XMVECTOR cameraTarget = XMVectorSet(0, 5, 0, 1);
    XMVECTOR cameraUp = XMVectorSet(0, 1, 0, 0);

    m_Camera.set_LookAt(cameraPos, cameraTarget, cameraUp);

    m_pAlignedCameraData = (CameraData*)_aligned_malloc(sizeof(CameraData), 16);
    m_pAlignedCameraData->m_InitialCamPos = m_Camera.get_Translation();
    m_pAlignedCameraData->m_InitialCamRot = m_Camera.get_Rotation();
}

HybridPipeline::~HybridPipeline()
{
    _aligned_free(m_pAlignedCameraData);
}

bool HybridPipeline::LoadContent()
{
	loadResource();
	loadGameObject();
	transformGameObject();
	loadDXResource();
	loadPipeline();
	{ 
		createAccelerationStructures();
		createRtPipelineState();
		createShaderResources();
		createSrvUavHeap();
		//createSamplerHeap();
		createShaderTable();
	}
	return true;
}

void HybridPipeline::OnResize(ResizeEventArgs& e)
{
    super::OnResize(e);

    if (m_Width != e.Width || m_Height != e.Height)
    {
        m_Width = std::max(1, e.Width);
        m_Height = std::max(1, e.Height);

        float aspectRatio = m_Width / (float)m_Height;
        m_Camera.set_Projection(45.0f, aspectRatio, 0.1f, 100.0f);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
            static_cast<float>(m_Width), static_cast<float>(m_Height));

		m_GBuffer.Resize(m_Width, m_Height);
    }
}

void HybridPipeline::UnloadContent()
{
}

void HybridPipeline::OnUpdate(UpdateEventArgs& e)
{
    super::OnUpdate(e);

	// frametime
	{
		totalTime += e.ElapsedTime;
		frameCount++;
		totalFrameCount++;

		if (totalTime > 1.0)
		{
			double fps = frameCount / totalTime;

			char buffer[512];
			sprintf_s(buffer, "FPS: %f,  FrameCount: %lld\n", fps, totalFrameCount);
			OutputDebugStringA(buffer);

			frameCount = 0;
			totalTime = 0.0;
		}
	}

	// camera
	{
		float speedMultipler = (m_Shift ? 16.0f : 4.0f);

		XMVECTOR cameraTranslate = XMVectorSet(m_Right - m_Left, 0.0f, m_Forward - m_Backward, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
		XMVECTOR cameraPan = XMVectorSet(0.0f, m_Up - m_Down, 0.0f, 1.0f) * speedMultipler * static_cast<float>(e.ElapsedTime);
		m_Camera.Translate(cameraTranslate, Space::Local);
		m_Camera.Translate(cameraPan, Space::Local);

		XMVECTOR cameraRotation = XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_Pitch), XMConvertToRadians(m_Yaw), 0.0f);
		m_Camera.set_Rotation(cameraRotation);

		XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
	}

	// light
	{ 
		XMMATRIX viewMatrix = m_Camera.get_ViewMatrix();
		static float lightAnimTime = 0.0f;
		if (m_AnimateLights)
		{
			lightAnimTime += static_cast<float>(e.ElapsedTime) * 0.5f * XM_PI;
		}

		// Setup the lights.
		const float radius = 6.0f;
		PointLight& l = m_PointLight;
		l.PositionWS = {
			static_cast<float>(std::sin(lightAnimTime )) * radius,
			9,
			static_cast<float>(std::cos(lightAnimTime)) * radius,
			1.0f
		};
		l.Color = XMFLOAT4(Colors::White);
		l.Intensity = 2.0f;
		l.Attenuation = 0.01f;
		l.Radius = 1.0f;

		// Update the pointlight gameobject
		{
			gameObjectPool[lightObjectIndex]->Translate(XMMatrixTranslation(l.PositionWS.x, l.PositionWS.y, l.PositionWS.z));
			gameObjectPool[lightObjectIndex]->Scale(XMMatrixScaling(l.Radius, l.Radius, l.Radius));
		}
	}

	// update buffer for gpu
	updateBuffer();
}

void HybridPipeline::OnRender(RenderEventArgs& e)
{
    super::OnRender(e);

    auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto commandList = commandQueue->GetCommandList();
	
	FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

	CameraRTCB mCameraCB;
	mCameraCB.PositionWS = m_Camera.get_Translation();
	mCameraCB.InverseViewMatrix = m_Camera.get_InverseViewMatrix();
	mCameraCB.fov = m_Camera.get_FoV();

	// Rasterizing Pipe, G-Buffer-Gen
	{
		{
			commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color0), clearColor);
			commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color1), clearColor);
			commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color2), clearColor);
			commandList->ClearTexture(m_GBuffer.GetTexture(AttachmentPoint::Color3), clearColor);
			commandList->ClearDepthStencilTexture(m_GBuffer.GetTexture(AttachmentPoint::DepthStencil), D3D12_CLEAR_FLAG_DEPTH);
		}

		commandList->SetViewport(m_Viewport);
		commandList->SetScissorRect(m_ScissorRect);
		commandList->SetRenderTarget(m_GBuffer);
		commandList->SetPipelineState(m_DeferredPipelineState);
		commandList->SetGraphicsRootSignature(m_DeferredRootSignature);

		for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++) {
			it->second->Draw(*commandList, m_Camera, texturePool, meshPool);
		}
	}

	// Let's raytrace
	{ 
		commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mpSrvUavHeap.Get());
		//commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, mpSamplerHeap.Get());

		commandList->TransitionBarrier(*mpRtShadowOutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->TransitionBarrier(*mpRtReflectOutputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->TransitionBarrier(m_GBuffer.GetTexture(AttachmentPoint::Color0), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_GBuffer.GetTexture(AttachmentPoint::Color1), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_GBuffer.GetTexture(AttachmentPoint::Color2), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->TransitionBarrier(m_GBuffer.GetTexture(AttachmentPoint::Color3), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
		raytraceDesc.Width = m_Viewport.Width;
		raytraceDesc.Height = m_Viewport.Height;
		raytraceDesc.Depth = 1;

		// RayGen is the first entry in the shader-table
		raytraceDesc.RayGenerationShaderRecord.StartAddress = mpShaderTable->GetGPUVirtualAddress() + 0 * mShaderTableEntrySize;
		raytraceDesc.RayGenerationShaderRecord.SizeInBytes = mShaderTableEntrySize;

		// Miss is the second entry in the shader-table
		size_t missOffset = 1 * mShaderTableEntrySize;
		raytraceDesc.MissShaderTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + missOffset;
		raytraceDesc.MissShaderTable.StrideInBytes = mShaderTableEntrySize;
		raytraceDesc.MissShaderTable.SizeInBytes = mShaderTableEntrySize * 2;   // Only two

		int objectToRT = 0;
		for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
		{
			// 特殊处理剔除光源不加入光追
			std::string objIndex = it->first;
			if (objIndex.find("light") != std::string::npos) {
				continue;
			}
			objectToRT++;
			commandList->TransitionBarrier(texturePool[it->second->material.tex.AlbedoTexture], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->TransitionBarrier(texturePool[it->second->material.tex.MetallicTexture], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->TransitionBarrier(texturePool[it->second->material.tex.NormalTexture], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->TransitionBarrier(texturePool[it->second->material.tex.RoughnessTexture], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->TransitionBarrier(meshPool[it->second->mesh]->getVertexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->TransitionBarrier(meshPool[it->second->mesh]->getIndexBuffer(), D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		}

		// Hit is the third entry in the shader-table
		size_t hitOffset = 3 * mShaderTableEntrySize;
		raytraceDesc.HitGroupTable.StartAddress = mpShaderTable->GetGPUVirtualAddress() + hitOffset;
		raytraceDesc.HitGroupTable.StrideInBytes = mShaderTableEntrySize;
		raytraceDesc.HitGroupTable.SizeInBytes = mShaderTableEntrySize * ( 2* objectToRT);

		// Bind the empty root signature
		commandList->SetComputeRootSignature(mpEmptyRootSig);
		// Dispatch
		commandList->SetPipelineState1(mpPipelineState);
		commandList->DispatchRays(&raytraceDesc);
	}
	
	// Perform Temporal Accumulation and Variance Estimation
	{ 
		commandList->SetPipelineState(m_PostSVGFTemporalPipelineState);
		commandList->SetComputeRootSignature(m_PostSVGFTemporalRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, col_acc_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, moment_acc_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, his_length_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, *mpRtShadowOutputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, col_acc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, moment_acc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, his_length, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, variance_inout[1], D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->SetComputeDynamicConstantBuffer(1, viewProjectMatrix_prev);
		commandList->Dispatch(m_Width / 8, m_Height / 8);
	}
	
	// Perform ATrous Iteration
	{
		commandList->SetPipelineState(m_PostSVGFATrousPipelineState);
		commandList->SetComputeRootSignature(m_PostSVGFATrousRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		
		// pingpang interation
		for (uint32_t level = 1; level <= ATrous_Level_Max; level++) {
			assert(ATrous_Level_Max > 1);
			uint32_t pingPangSrvUavOffset = ppSrvUavOffset;
			Texture color_in = (level==1)? col_acc : color_inout[level % 2];
			Texture color_out = (level == ATrous_Level_Max)? col_acc : color_inout[(level + 1) % 2];
			Texture variance_in = variance_inout[level % 2]; // 这里预先设定过temporal pipeline的rt就是variance_inout[1]
			Texture variance_out = variance_inout[(level + 1) % 2];
			commandList->SetShaderResourceView(0, pingPangSrvUavOffset++, color_in, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->SetShaderResourceView(0, pingPangSrvUavOffset++, variance_in, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->SetUnorderedAccessView(0, pingPangSrvUavOffset++, color_out, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			commandList->SetUnorderedAccessView(0, pingPangSrvUavOffset++, variance_out, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			commandList->SetCompute32BitConstants(1, level);
			commandList->Dispatch(m_Width / 8, m_Height / 8);
		}
	}

	//// perform post spatial resample 
	//{
	//	commandList->SetPipelineState(m_PostSpatialResampleState);
	//	commandList->SetComputeRootSignature(m_PostSpatialResampleRootSignature);
	//	uint32_t ppSrvUavOffset = 0;
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, *mpRtShadowOutputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, *mpRtReflectOutputTexture, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, g_indirectOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	commandList->SetComputeDynamicConstantBuffer(1, mCameraCB);
	//	commandList->Dispatch(m_Width / 8, m_Height / 8);
	//}

	//// perform postTemporalResample
	//{
	//	commandList->SetPipelineState(m_PostTemporalResampleState);
	//	commandList->SetComputeRootSignature(m_PostTemporalResampleRootSignature);
	//	uint32_t ppSrvUavOffset = 0;
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, radiance_acc_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, his_length_prev, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetShaderResourceView(0, ppSrvUavOffset++, g_indirectOutput, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	//	commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, radiance_acc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	commandList->SetUnorderedAccessView(0, ppSrvUavOffset++, his_length, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//	commandList->SetComputeDynamicConstantBuffer(1, viewProjectMatrix_prev);
	//	commandList->Dispatch(m_Width/8, m_Height/8);
	//}

	// post lighting
	{
		commandList->SetViewport(m_Viewport);
		commandList->SetScissorRect(m_ScissorRect);
		commandList->SetRenderTarget(m_pWindow->GetRenderTarget());
		commandList->SetPipelineState(m_PostLightingPipelineState);
		commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->SetGraphicsRootSignature(m_PostLightingRootSignature);
		uint32_t ppSrvUavOffset = 0;
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gPosition, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gAlbedoMetallic, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gNormalRoughness, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, gExtra, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, col_acc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetShaderResourceView(0, ppSrvUavOffset++, *mpRtReflectOutputTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandList->SetGraphicsDynamicConstantBuffer(1, m_PointLight);
		commandList->SetGraphicsDynamicConstantBuffer(2, mCameraCB);
		commandList->Draw(3);
	}

	// prev buffer cache
	{
		auto swapCurrPrevTexture = [](Texture& t1, Texture& t2) {
			Texture tmp = t2;
			t2 = t1;
			t1 = tmp;
		};
		viewProjectMatrix_prev = m_Camera.get_ViewMatrix() * m_Camera.get_ProjectionMatrix();
		swapCurrPrevTexture(gPosition_prev, gPosition);
		swapCurrPrevTexture(gAlbedoMetallic_prev, gAlbedoMetallic);
		swapCurrPrevTexture(gNormalRoughness_prev, gNormalRoughness);
		swapCurrPrevTexture(gExtra_prev, gExtra);
		swapCurrPrevTexture(col_acc_prev, col_acc);
		swapCurrPrevTexture(moment_acc_prev, moment_acc);
		swapCurrPrevTexture(his_length_prev, his_length);
		swapCurrPrevTexture(radiance_acc_prev, radiance_acc);
		// G Buffer re-orgnizating
		m_GBuffer.AttachTexture(AttachmentPoint::Color0, gPosition);
		m_GBuffer.AttachTexture(AttachmentPoint::Color1, gAlbedoMetallic);
		m_GBuffer.AttachTexture(AttachmentPoint::Color2, gNormalRoughness);
		m_GBuffer.AttachTexture(AttachmentPoint::Color3, gExtra);
	}

    commandQueue->ExecuteCommandList(commandList);
    // Present
    m_pWindow->Present();
}

void HybridPipeline::OnKeyPressed(KeyEventArgs& e)
{
    super::OnKeyPressed(e);
    switch (e.Key)
    {
    case KeyCode::Escape:
        Application::Get().Quit(0);
        break;
    case KeyCode::Enter:
        if (e.Alt)
        {
    case KeyCode::F11:
        if (g_AllowFullscreenToggle)
        {
            m_pWindow->ToggleFullscreen();
            g_AllowFullscreenToggle = false;
        }
        break;
        }
    case KeyCode::V:
        m_pWindow->ToggleVSync();
        break;
    case KeyCode::R:
        // Reset camera transform
        m_Camera.set_Translation(m_pAlignedCameraData->m_InitialCamPos);
        m_Camera.set_Rotation(m_pAlignedCameraData->m_InitialCamRot);
        m_Pitch = 0.0f;
        m_Yaw = 0.0f;
        break;
    case KeyCode::Up:
    case KeyCode::W:
        m_Forward = 2.0f;
        break;
    case KeyCode::Left:
    case KeyCode::A:
        m_Left = 2.0f;
        break;
    case KeyCode::Down:
    case KeyCode::S:
        m_Backward = 2.0f;
        break;
    case KeyCode::Right:
    case KeyCode::D:
        m_Right = 2.0f;
        break;
    case KeyCode::Q:
        m_Down =2.0f;
        break;
    case KeyCode::E:
        m_Up = 2.0f;
        break;
    case KeyCode::Space:
        m_AnimateLights = !m_AnimateLights;
        break;
    case KeyCode::ShiftKey:
        //m_Shift = true;
		// disable the shift key
        break;
    }
    
}

void HybridPipeline::OnKeyReleased(KeyEventArgs& e)
{
    super::OnKeyReleased(e);

    switch (e.Key)
    {
    case KeyCode::Enter:
        if (e.Alt)
        {
    case KeyCode::F11:
        g_AllowFullscreenToggle = true;
        }
        break;
    case KeyCode::Up:
    case KeyCode::W:
        m_Forward = 0.0f;
        break;
    case KeyCode::Left:
    case KeyCode::A:
        m_Left = 0.0f;
        break;
    case KeyCode::Down:
    case KeyCode::S:
        m_Backward = 0.0f;
        break;
    case KeyCode::Right:
    case KeyCode::D:
        m_Right = 0.0f;
        break;
    case KeyCode::Q:
        m_Down = 0.0f;
        break;
    case KeyCode::E:
        m_Up = 0.0f;
        break;
    case KeyCode::ShiftKey:
        m_Shift = false;
        break;
    }
    
}

void HybridPipeline::OnMouseMoved(MouseMotionEventArgs& e)
{
    super::OnMouseMoved(e);

    const float mouseSpeed = 0.1f;
    if (e.LeftButton)
    {
        m_Pitch -= e.RelY * mouseSpeed;

        m_Pitch = Math::clamp(m_Pitch, -90.0f, 90.0f);

        m_Yaw -= e.RelX * mouseSpeed;
    }
    
}

void HybridPipeline::OnMouseWheel(MouseWheelEventArgs& e)
{
    auto fov = m_Camera.get_FoV();

    fov -= e.WheelDelta;
    fov = Math::clamp(fov, 12.0f, 90.0f);

    m_Camera.set_FoV(fov);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", fov);
    OutputDebugStringA(buffer);
}

////////////////////////////////////////////////////////////////////// internal update
void HybridPipeline::loadResource() {
	auto device = Application::Get().GetDevice();
	/////////////////////////////////// Resource Loading
	auto commandQueue = Application::Get().GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	auto commandList = commandQueue->GetCommandList();

	// Create a Cube mesh
	meshPool.emplace("cube", Mesh::CreateCube(*commandList));
	meshPool.emplace("sphere", Mesh::CreateSphere(*commandList));
	meshPool.emplace("cone", Mesh::CreateCone(*commandList));
	meshPool.emplace("torus", Mesh::CreateTorus(*commandList));
	meshPool.emplace("plane", Mesh::CreatePlane(*commandList));

	// load PBR textures
	texturePool.emplace("default_albedo", Texture());
	commandList->LoadTextureFromFile(texturePool["default_albedo"], L"Assets/Textures/pbr/default/albedo.bmp", TextureUsage::Albedo);
	texturePool.emplace("default_metallic", Texture());
	commandList->LoadTextureFromFile(texturePool["default_metallic"], L"Assets/Textures/pbr/default/metallic.bmp", TextureUsage::MetallicMap);
	texturePool.emplace("default_normal", Texture());
	commandList->LoadTextureFromFile(texturePool["default_normal"], L"Assets/Textures/pbr/default/normal.bmp", TextureUsage::Normalmap);
	texturePool.emplace("default_roughness", Texture());
	commandList->LoadTextureFromFile(texturePool["default_roughness"], L"Assets/Textures/pbr/default/roughness.bmp", TextureUsage::RoughnessMap);
	//// rusted iron
	texturePool.emplace("rusted_iron_albedo", Texture());
	commandList->LoadTextureFromFile(texturePool["rusted_iron_albedo"], L"Assets/Textures/pbr/rusted_iron/albedo.png", TextureUsage::Albedo);
	texturePool.emplace("rusted_iron_metallic", Texture());
	commandList->LoadTextureFromFile(texturePool["rusted_iron_metallic"], L"Assets/Textures/pbr/rusted_iron/metallic.png", TextureUsage::MetallicMap);
	texturePool.emplace("rusted_iron_normal", Texture());
	commandList->LoadTextureFromFile(texturePool["rusted_iron_normal"], L"Assets/Textures/pbr/rusted_iron/normal.png", TextureUsage::Normalmap);
	texturePool.emplace("rusted_iron_roughness", Texture());
	commandList->LoadTextureFromFile(texturePool["rusted_iron_roughness"], L"Assets/Textures/pbr/rusted_iron/roughness.png", TextureUsage::RoughnessMap);
	//// grid metal
	texturePool.emplace("grid_metal_albedo", Texture());
	commandList->LoadTextureFromFile(texturePool["grid_metal_albedo"], L"Assets/Textures/pbr/grid_metal/albedo.png", TextureUsage::Albedo);
	texturePool.emplace("grid_metal_metallic", Texture());
	commandList->LoadTextureFromFile(texturePool["grid_metal_metallic"], L"Assets/Textures/pbr/grid_metal/metallic.png", TextureUsage::MetallicMap);
	texturePool.emplace("grid_metal_normal", Texture());
	commandList->LoadTextureFromFile(texturePool["grid_metal_normal"], L"Assets/Textures/pbr/grid_metal/normal.png", TextureUsage::Normalmap);
	texturePool.emplace("grid_metal_roughness", Texture());
	commandList->LoadTextureFromFile(texturePool["grid_metal_roughness"], L"Assets/Textures/pbr/grid_metal/roughness.png", TextureUsage::RoughnessMap);
	//// metal
	texturePool.emplace("metal_albedo", Texture());
	commandList->LoadTextureFromFile(texturePool["metal_albedo"], L"Assets/Textures/pbr/metal/albedo.png", TextureUsage::Albedo);
	texturePool.emplace("metal_metallic", Texture());
	commandList->LoadTextureFromFile(texturePool["metal_metallic"], L"Assets/Textures/pbr/metal/metallic.png", TextureUsage::MetallicMap);
	texturePool.emplace("metal_normal", Texture());
	commandList->LoadTextureFromFile(texturePool["metal_normal"], L"Assets/Textures/pbr/metal/normal.png", TextureUsage::Normalmap);
	texturePool.emplace("metal_roughness", Texture());
	commandList->LoadTextureFromFile(texturePool["metal_roughness"], L"Assets/Textures/pbr/metal/roughness.png", TextureUsage::RoughnessMap);

	auto fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}
void HybridPipeline::loadGameObject() {
	auto device = Application::Get().GetDevice();
	/////////////////////////////////// GameObject Gen
	float gid = 0.0f;

	gameObjectPool.emplace("sphere", std::make_shared<GameObject>());
	gameObjectPool["sphere"]->gid = gid++;
	gameObjectPool["sphere"]->mesh = "sphere";
	gameObjectPool["sphere"]->material.base = Material::White;
	gameObjectPool["sphere"]->material.pbr = PBRMaterial(1.0f, 1.0f);
	gameObjectPool["sphere"]->material.tex = TextureMaterial("rusted_iron");

	gameObjectPool.emplace("cube", std::make_shared<GameObject>());
	gameObjectPool["cube"]->gid = gid++;
	gameObjectPool["cube"]->mesh = "cube";
	gameObjectPool["cube"]->material.base = Material::White;
	gameObjectPool["cube"]->material.pbr = PBRMaterial(1.0f, 1.0f);
	gameObjectPool["cube"]->material.tex = TextureMaterial("grid_metal");

	gameObjectPool.emplace("torus", std::make_shared<GameObject>());
	gameObjectPool["torus"]->gid = gid++;
	gameObjectPool["torus"]->mesh = "torus";
	gameObjectPool["torus"]->material.base = Material::White;
	gameObjectPool["torus"]->material.pbr = PBRMaterial(1.0f, 1.0f);
	gameObjectPool["torus"]->material.tex = TextureMaterial("metal");

	gameObjectPool.emplace("Floor plane", std::make_shared<GameObject>()); //1
	gameObjectPool["Floor plane"]->gid = gid++;
	gameObjectPool["Floor plane"]->mesh = "plane";
	gameObjectPool["Floor plane"]->material.base = Material::White;
	gameObjectPool["Floor plane"]->material.pbr = PBRMaterial(0.5f, 0.4f);
	gameObjectPool["Floor plane"]->material.tex = TextureMaterial("default");

	gameObjectPool.emplace("Back wall", std::make_shared<GameObject>());//2
	gameObjectPool["Back wall"]->gid = gid++;
	gameObjectPool["Back wall"]->mesh = "plane";
	gameObjectPool["Back wall"]->material.base = Material::Turquoise;
	gameObjectPool["Back wall"]->material.pbr = PBRMaterial(0.5f, 0.1f);
	gameObjectPool["Back wall"]->material.tex = TextureMaterial("default");

	gameObjectPool.emplace("Ceiling plane", std::make_shared<GameObject>());//3
	gameObjectPool["Ceiling plane"]->gid = gid++;
	gameObjectPool["Ceiling plane"]->mesh = "plane";
	gameObjectPool["Ceiling plane"]->material.base = Material::White;
	gameObjectPool["Ceiling plane"]->material.pbr = PBRMaterial(0.5f, 0.5f);
	gameObjectPool["Ceiling plane"]->material.tex = TextureMaterial("default");

	gameObjectPool.emplace("Front wall", std::make_shared<GameObject>());//4
	gameObjectPool["Front wall"]->gid = gid++;
	gameObjectPool["Front wall"]->mesh = "plane";
	gameObjectPool["Front wall"]->material.base = Material::Copper;
	gameObjectPool["Front wall"]->material.pbr = PBRMaterial(0.5f, 0.7f);
	gameObjectPool["Front wall"]->material.tex = TextureMaterial("default");

	gameObjectPool.emplace("Left wall", std::make_shared<GameObject>());//5
	gameObjectPool["Left wall"]->gid = gid++;
	gameObjectPool["Left wall"]->mesh = "plane";
	gameObjectPool["Left wall"]->material.base = Material::Jade;
	gameObjectPool["Left wall"]->material.pbr = PBRMaterial(0.5f, 0.3f);
	gameObjectPool["Left wall"]->material.tex = TextureMaterial("default");

	gameObjectPool.emplace("Right wall", std::make_shared<GameObject>());//6
	gameObjectPool["Right wall"]->gid = gid++;
	gameObjectPool["Right wall"]->mesh = "plane";
	gameObjectPool["Right wall"]->material.base = Material::Ruby;
	gameObjectPool["Right wall"]->material.pbr = PBRMaterial(0.5f, 0.6f);
	gameObjectPool["Right wall"]->material.tex = TextureMaterial("default");

	// light object
	lightObjectIndex = "sphere light";
	gameObjectPool.emplace(lightObjectIndex, std::make_shared<GameObject>());//7
	gameObjectPool[lightObjectIndex]->gid = gid++;
	gameObjectPool[lightObjectIndex]->mesh = "sphere";
	gameObjectPool[lightObjectIndex]->material.base = Material::EmissiveWhite;
	gameObjectPool[lightObjectIndex]->material.pbr = PBRMaterial(1.0f, 1.0f);
	gameObjectPool[lightObjectIndex]->material.tex = TextureMaterial("default");
}
void HybridPipeline::transformGameObject() {
	auto device = Application::Get().GetDevice();

	/////////////////////////////////// Initial the transform
	{
		gameObjectPool["sphere"]->Translate(XMMatrixTranslation(4.0f, 3.0f, 2.0f));
		gameObjectPool["sphere"]->Rotate(XMMatrixIdentity());
		gameObjectPool["sphere"]->Scale(XMMatrixScaling(6.0f, 6.0f, 6.0f));

		gameObjectPool["cube"]->Translate(XMMatrixTranslation(-4.0f, 3.0f, -2.0f));
		gameObjectPool["cube"]->Rotate(XMMatrixRotationY(XMConvertToRadians(45.0f)));
		gameObjectPool["cube"]->Scale(XMMatrixScaling(6.0f, 6.0f, 6.0f));

		gameObjectPool["torus"]->Translate(XMMatrixTranslation(4.0f, 0.6f, -6.0f));
		gameObjectPool["torus"]->Rotate(XMMatrixRotationY(XMConvertToRadians(45.0f)));
		gameObjectPool["torus"]->Scale(XMMatrixScaling(4.0f, 4.0f, 4.0f));

		float scalePlane = 20.0f;
		float translateOffset = scalePlane / 2.0f;

		gameObjectPool["Floor plane"]->Translate(XMMatrixTranslation(0.0f, 0.0f, 0.0f));
		gameObjectPool["Floor plane"]->Rotate(XMMatrixIdentity());
		gameObjectPool["Floor plane"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Back wall"]->Translate(XMMatrixTranslation(0.0f, translateOffset, translateOffset));
		gameObjectPool["Back wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(-90)));
		gameObjectPool["Back wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Ceiling plane"]->Translate(XMMatrixTranslation(0.0f, translateOffset * 2.0f, 0));
		gameObjectPool["Ceiling plane"]->Rotate(XMMatrixRotationX(XMConvertToRadians(180)));
		gameObjectPool["Ceiling plane"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Front wall"]->Translate(XMMatrixTranslation(0, translateOffset, -translateOffset));
		gameObjectPool["Front wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(90)));
		gameObjectPool["Front wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Left wall"]->Translate(XMMatrixTranslation(-translateOffset, translateOffset, 0));
		gameObjectPool["Left wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(-90)));
		gameObjectPool["Left wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));

		gameObjectPool["Right wall"]->Translate(XMMatrixTranslation(translateOffset, translateOffset, 0));
		gameObjectPool["Right wall"]->Rotate(XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixRotationY(XMConvertToRadians(90)));
		gameObjectPool["Right wall"]->Scale(XMMatrixScaling(scalePlane, 1.0f, scalePlane));
	}
}
void HybridPipeline::loadDXResource() {
	auto createTex2D_RenderTarget = [](std::wstring name, int w, int h){
		DXGI_FORMAT HDRFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET| D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		D3D12_CLEAR_VALUE colorClearValue;
		colorClearValue.Format = desc.Format;
		colorClearValue.Color[0] = 0.0f;
		colorClearValue.Color[1] = 0.0f;
		colorClearValue.Color[2] = 0.0f;
		colorClearValue.Color[3] = 1.0f;
		return Texture(desc, &colorClearValue,
			TextureUsage::RenderTarget, name);
	};
	auto createTex2D_DepthStencil = [](std::wstring name, int w, int h) {
		DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D32_FLOAT;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(depthBufferFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
		D3D12_CLEAR_VALUE depthClearValue;
		depthClearValue.Format = depthDesc.Format;
		depthClearValue.DepthStencil = { 1.0f, 0 };
		return Texture(depthDesc, &depthClearValue,
			TextureUsage::Depth, name);
	};
	auto createTex2D_ReadOnly = [](std::wstring name, int w, int h) {
		DXGI_FORMAT HDRFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_NONE);
		return Texture(desc, 0, TextureUsage::IntermediateBuffer, name);
	};
	auto createTex2D_ReadWrite = [](std::wstring name, int w, int h) {
		DXGI_FORMAT HDRFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 };
		// Create an off-screen multi render target(MRT) and a depth buffer.
		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(HDRFormat,
			w, h, 1, 0, sampleDesc.Count, sampleDesc.Quality,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		return Texture(desc, 0, TextureUsage::IntermediateBuffer, name);
	};

	{
		gPosition = createTex2D_RenderTarget(L"gPosition+HitTexture", m_Width, m_Height);
		gAlbedoMetallic = createTex2D_RenderTarget(L"gAlbedoMetallic Texture", m_Width, m_Height);
		gNormalRoughness = createTex2D_RenderTarget(L"gNormalRoughness Texture", m_Width, m_Height);
		gExtra = createTex2D_RenderTarget(L"gExtra: ObjectId Texture", m_Width, m_Height);
		// Attach the HDR texture to the HDR render target.
		m_GBuffer.AttachTexture(AttachmentPoint::Color0, gPosition);
		m_GBuffer.AttachTexture(AttachmentPoint::Color1, gAlbedoMetallic);
		m_GBuffer.AttachTexture(AttachmentPoint::Color2, gNormalRoughness);
		m_GBuffer.AttachTexture(AttachmentPoint::Color3, gExtra);
		m_GBuffer.AttachTexture(AttachmentPoint::DepthStencil, createTex2D_DepthStencil(L"Depth Render Target", m_Width, m_Height));

		gPosition_prev = createTex2D_RenderTarget(L"gPosition_prev", m_Width, m_Height);
		gAlbedoMetallic_prev = createTex2D_RenderTarget(L"gAlbedoMetallic_prev", m_Width, m_Height);
		gNormalRoughness_prev = createTex2D_RenderTarget(L"gNormalRoughness_prev", m_Width, m_Height);
		gExtra_prev = createTex2D_RenderTarget(L"gExtra_prev", m_Width, m_Height);

		col_acc = createTex2D_ReadWrite(L"col_acc", m_Width, m_Height);
		moment_acc = createTex2D_ReadWrite(L"moment_acc", m_Width, m_Height);
		his_length = createTex2D_ReadWrite(L"his_length", m_Width, m_Height);
		col_acc_prev = createTex2D_ReadWrite(L"col_acc_prev", m_Width, m_Height);
		moment_acc_prev = createTex2D_ReadWrite(L"moment_acc_prev", m_Width, m_Height);
		his_length_prev = createTex2D_ReadWrite(L"his_length_prev", m_Width, m_Height);

		color_inout[0] = createTex2D_ReadWrite(L"color_inout[0]", m_Width, m_Height);
		color_inout[1] = createTex2D_ReadWrite(L"color_inout[1]", m_Width, m_Height);
		variance_inout[0] = createTex2D_ReadWrite(L"variance_inout[0]", m_Width, m_Height);
		variance_inout[1] = createTex2D_ReadWrite(L"variance_inout[1]", m_Width, m_Height);

		g_indirectOutput = createTex2D_ReadWrite(L"g_indirectOutput", m_Width, m_Height);

		radiance_acc = createTex2D_ReadWrite(L"radiance_acc", m_Width, m_Height);
		radiance_acc_prev = createTex2D_ReadWrite(L"radiance_acc_prev", m_Width, m_Height);
	}

}
void HybridPipeline::loadPipeline() {
	auto device = Application::Get().GetDevice();
	/////////////////////////////////// Root Signature
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		// Disable the multiple sampling for performance and simplicity
		DXGI_SAMPLE_DESC sampleDesc = { 1, 0 }/*Application::Get().GetMultisampleQualityLevels(HDRFormat, D3D12_MAX_MULTISAMPLE_SAMPLE_COUNT)*/;

		// Create a root signature for the Deferred Shadings pipeline.
		{
			// Load the Deferred shaders.
			ComPtr<ID3DBlob> vs;
			ComPtr<ID3DBlob> ps;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/Deferred_VS.cso", &vs));
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/Deferred_PS.cso", &ps));

			// Allow input layout and deny unnecessary access to certain pipeline stages.
			D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
				D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0) };

			CD3DX12_ROOT_PARAMETER1 rootParameters[5];
			rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
			rootParameters[1].InitAsConstantBufferView(0, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[2].InitAsConstantBufferView(1, 1, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[3].InitAsConstants(1, 2, 1, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[4].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);

			//CD3DX12_STATIC_SAMPLER_DESC linearRepeatSampler(0, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
			CD3DX12_STATIC_SAMPLER_DESC anisotropicSampler(0, D3D12_FILTER_ANISOTROPIC);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters, 1, &anisotropicSampler, rootSignatureFlags);

			m_DeferredRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			// Setup the Deferred pipeline state.
			struct DeferredPipelineStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
				CD3DX12_PIPELINE_STATE_STREAM_SAMPLE_DESC SampleDesc;
			} deferredPipelineStateStream;

			deferredPipelineStateStream.pRootSignature = m_DeferredRootSignature.GetRootSignature().Get();
			deferredPipelineStateStream.InputLayout = { VertexPositionNormalTexture::InputElements, VertexPositionNormalTexture::InputElementCount };
			deferredPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			deferredPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
			deferredPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
			deferredPipelineStateStream.DSVFormat = m_GBuffer.GetDepthStencilFormat();
			deferredPipelineStateStream.RTVFormats = m_GBuffer.GetRenderTargetFormats();
			deferredPipelineStateStream.SampleDesc = sampleDesc;

			D3D12_PIPELINE_STATE_STREAM_DESC deferredPipelineStateStreamDesc = {
				sizeof(DeferredPipelineStateStream), &deferredPipelineStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&deferredPipelineStateStreamDesc, IID_PPV_ARGS(&m_DeferredPipelineState)));
		}

		// Create the PostSVGFTempral_CS Root Signature
		{
			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 12, 0),
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4, 0) };

			CD3DX12_ROOT_PARAMETER1 rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
			rootParameters[1].InitAsConstantBufferView(0, 0);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

			m_PostSVGFTemporalRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			// Create the PostProcessing PSO
			ComPtr<ID3DBlob> cs;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostSVGFTemporal_CS.cso", &cs));

			struct PostProcessingPipelineStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			} postProcessingPipelineStateStream;

			postProcessingPipelineStateStream.pRootSignature = m_PostSVGFTemporalRootSignature.GetRootSignature().Get();
			postProcessingPipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

			D3D12_PIPELINE_STATE_STREAM_DESC postProcessingPipelineStateStreamDesc = {
				sizeof(PostProcessingPipelineStateStream), &postProcessingPipelineStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&postProcessingPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostSVGFTemporalPipelineState)));
		}

		// Create the PostATrous_CS Root Signature
		{
			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0),
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0)
			};

			CD3DX12_ROOT_PARAMETER1 rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
			rootParameters[1].InitAsConstants(4, 0);

			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

			m_PostSVGFATrousRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			ComPtr<ID3DBlob> cs;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostSVGFATrous_CS.cso", &cs));

			struct PostATrousPipelineStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			} postATrousPipelineStateStream;

			postATrousPipelineStateStream.pRootSignature = m_PostSVGFATrousRootSignature.GetRootSignature().Get();
			postATrousPipelineStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

			D3D12_PIPELINE_STATE_STREAM_DESC postAtrousPipelineStateStreamDesc = {
				sizeof(PostATrousPipelineStateStream), &postATrousPipelineStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&postAtrousPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostSVGFATrousPipelineState)));
		}

		// Create the PostSpatial_CS Root Signature
		{
			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0),
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0)
			};

			CD3DX12_ROOT_PARAMETER1 rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
			rootParameters[1].InitAsConstantBufferView(0, 0);


			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

			m_PostSpatialResampleRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			ComPtr<ID3DBlob> cs;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostResampleSpatial_CS.cso", &cs));

			struct PostSpatialResampleStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			} postSpatialResampleStateStream;

			postSpatialResampleStateStream.pRootSignature = m_PostSpatialResampleRootSignature.GetRootSignature().Get();
			postSpatialResampleStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

			D3D12_PIPELINE_STATE_STREAM_DESC postSpatialResampleStateStreamDesc = {
				sizeof(postSpatialResampleStateStream), &postSpatialResampleStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&postSpatialResampleStateStreamDesc, IID_PPV_ARGS(&m_PostSpatialResampleState)));
		}

		// Create the PostTemporalResample_CS Root Signature
		{
			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[2] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 11, 0),
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0)
			};

			CD3DX12_ROOT_PARAMETER1 rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange);
			rootParameters[1].InitAsConstantBufferView(0, 0);


			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

			m_PostTemporalResampleRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			ComPtr<ID3DBlob> cs;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostResampleTemporal_CS.cso", &cs));

			struct PostTemporalResampleStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_CS CS;
			} postTemporalResampleStateStream;

			postTemporalResampleStateStream.pRootSignature = m_PostTemporalResampleRootSignature.GetRootSignature().Get();
			postTemporalResampleStateStream.CS = CD3DX12_SHADER_BYTECODE(cs.Get());

			D3D12_PIPELINE_STATE_STREAM_DESC postTemporalResampleStateStreamDesc = {
				sizeof(postTemporalResampleStateStream), &postTemporalResampleStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&postTemporalResampleStateStreamDesc, IID_PPV_ARGS(&m_PostTemporalResampleState)));
		}

		// Create the PostLighting_PS Root Signature
		{
			CD3DX12_DESCRIPTOR_RANGE1 descriptorRange[1] = {
				CD3DX12_DESCRIPTOR_RANGE1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0)
			};

			CD3DX12_ROOT_PARAMETER1 rootParameters[3];
			rootParameters[0].InitAsDescriptorTable(arraysize(descriptorRange), descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[1].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
			rootParameters[2].InitAsConstantBufferView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);


			CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
			rootSignatureDescription.Init_1_1(arraysize(rootParameters), rootParameters);

			m_PostLightingRootSignature.SetRootSignatureDesc(rootSignatureDescription.Desc_1_1, featureData.HighestVersion);

			ComPtr<ID3DBlob> vs;
			ComPtr<ID3DBlob> ps;
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostProcessing_VS.cso", &vs));
			ThrowIfFailed(D3DReadFileToBlob(L"build_vs2019/data/shaders/RTPipeline/PostLighting_PS.cso", &ps));

			CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
			rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

			struct PostLightingStateStream
			{
				CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
				CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
				CD3DX12_PIPELINE_STATE_STREAM_VS VS;
				CD3DX12_PIPELINE_STATE_STREAM_PS PS;
				CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
				CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
			} postLightingPipelineStateStream;

			postLightingPipelineStateStream.pRootSignature = m_PostLightingRootSignature.GetRootSignature().Get();
			postLightingPipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			postLightingPipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
			postLightingPipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(ps.Get());
			postLightingPipelineStateStream.Rasterizer = rasterizerDesc;
			postLightingPipelineStateStream.RTVFormats = m_pWindow->GetRenderTarget().GetRenderTargetFormats();

			D3D12_PIPELINE_STATE_STREAM_DESC postLightingPipelineStateStreamDesc = {
				sizeof(postLightingPipelineStateStream), &postLightingPipelineStateStream
			};
			ThrowIfFailed(device->CreatePipelineState(&postLightingPipelineStateStreamDesc, IID_PPV_ARGS(&m_PostLightingPipelineState)));
		}
	}
}
void HybridPipeline::updateBuffer()
{
	// update the structure buffer of frameId
	{
		FrameIndexCB fid;
		fid.FrameIndex = static_cast<uint32_t>(totalFrameCount);
		void* pData;
		mpRTFrameIndexCB->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &fid, sizeof(FrameIndexCB));
		mpRTFrameIndexCB->Unmap(0, nullptr);
	}

	// update the camera for rt
	{
		CameraRTCB mCameraCB;
		mCameraCB.PositionWS = m_Camera.get_Translation();
		mCameraCB.InverseViewMatrix = m_Camera.get_InverseViewMatrix();
		mCameraCB.fov = m_Camera.get_FoV();
		void* pData;
		mpRTCameraCB->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &mCameraCB, sizeof(CameraRTCB));
		mpRTCameraCB->Unmap(0, nullptr);
	}

	// update the light for rt
	{
		void* pData;
		mpRTPointLightCB->Map(0, nullptr, (void**)&pData);
		memcpy(pData, &(m_PointLight), sizeof(PointLight));
		mpRTPointLightCB->Unmap(0, nullptr);
	}
}

void HybridPipeline::GameObject::Draw(CommandList& commandList, Camera& camera, std::map<TextureIndex, Texture>& texturePool, std::map<MeshIndex, std::shared_ptr<Mesh>>& meshPool)
{
	commandList.SetGraphicsDynamicConstantBuffer(0, transform.ComputeMatCB(camera.get_ViewMatrix(), camera.get_ProjectionMatrix()));
	commandList.SetGraphicsDynamicConstantBuffer(1, material.base);
	commandList.SetGraphicsDynamicConstantBuffer(2, material.pbr);
	commandList.SetGraphics32BitConstants(3, gid);
	commandList.SetShaderResourceView(4, 0, (texturePool.find(material.tex.AlbedoTexture) != texturePool.end()) ? texturePool[material.tex.AlbedoTexture] : texturePool["default_albedo"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(4, 1, (texturePool.find(material.tex.MetallicTexture) != texturePool.end()) ? texturePool[material.tex.MetallicTexture] : texturePool["default_metallic"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(4, 2, (texturePool.find(material.tex.NormalTexture) != texturePool.end()) ? texturePool[material.tex.NormalTexture] : texturePool["default_normal"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	commandList.SetShaderResourceView(4, 3, (texturePool.find(material.tex.RoughnessTexture) != texturePool.end()) ? texturePool[material.tex.RoughnessTexture] : texturePool["default_roughness"], D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	if (meshPool.find(mesh) != meshPool.end()) {
		meshPool[mesh]->Draw(commandList);
	}
}

////////////////////////////////////////////////////////////////////// RT Object 
void HybridPipeline::createAccelerationStructures()
{
	const D3D12_HEAP_PROPERTIES kUploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	auto pDevice = Application::Get().GetDevice();
	auto commandQueue = Application::Get().GetCommandQueue();
	auto commandList = commandQueue->GetCommandList();
	
	std::vector<AccelerationStructureBuffers> bottomLevelBuffers;
	mpBottomLevelASes.resize(meshPool.size()); 
	bottomLevelBuffers.resize(meshPool.size());//Before the commandQueue finishes its execution of AS creation, these resources' lifecycle have to be held
	std::map<MeshIndex, int> MeshIndex2blasSeqCache; // Record MeshIndex to the sequence of mesh in mpBottomLevelASes

	int blasSeqCount=0;
	for (auto it = meshPool.begin(); it != meshPool.end(); it++) { //mpBottomLevelASes

		MeshIndex2blasSeqCache.emplace(it->first, blasSeqCount);
		auto pMesh = it->second;

		// set the index and vertex buffer to generic read status //Crucial!!!!!!!! // Make Sure the correct status // hey yeah
		commandList->TransitionBarrier(pMesh->getVertexBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);
		commandList->TransitionBarrier(pMesh->getIndexBuffer(), D3D12_RESOURCE_STATE_GENERIC_READ);

		D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
		geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geomDesc.Triangles.VertexBuffer.StartAddress = pMesh->getVertexBuffer().GetD3D12Resource()->GetGPUVirtualAddress();//Vertex Setup
		geomDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(VertexPositionNormalTexture);
		geomDesc.Triangles.VertexFormat = VertexPositionNormalTexture::InputElements[0].Format;
		geomDesc.Triangles.VertexCount = pMesh->getVertexCount();
		geomDesc.Triangles.IndexBuffer = pMesh->getIndexBuffer().GetD3D12Resource()->GetGPUVirtualAddress(); //Index Setup
		geomDesc.Triangles.IndexFormat = pMesh->getIndexBuffer().GetIndexBufferView().Format;
		geomDesc.Triangles.IndexCount = pMesh->getIndexCount();
		
		geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		// Get the size requirements for the scratch and AS buffers
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = 1;
		inputs.pGeometryDescs = &geomDesc;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
		bottomLevelBuffers[blasSeqCount].pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, 
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		bottomLevelBuffers[blasSeqCount].pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, 
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		// 成员变量持有，全局记录持久资源的状态
		ASBuffer asBuffer;
		mpBottomLevelASes[blasSeqCount] = bottomLevelBuffers[blasSeqCount].pResult;
		asBuffer.SetD3D12Resource(mpBottomLevelASes[blasSeqCount]);
		asBuffer.SetName(L"BottomLevelAS"+string_2_wstring(it->first));
		ResourceStateTracker::AddGlobalResourceState(mpBottomLevelASes[blasSeqCount].Get(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		// Create the bottom-level AS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs = inputs;
		asDesc.DestAccelerationStructureData = bottomLevelBuffers[blasSeqCount].pResult->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = bottomLevelBuffers[blasSeqCount].pScratch->GetGPUVirtualAddress();

		commandList->BuildRaytracingAccelerationStructure(&asDesc);

		// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
		commandList->UAVBarrier(asBuffer, true);
		blasSeqCount++;
	}

	AccelerationStructureBuffers topLevelBuffers; //构建GameObject
	{ // topLevelBuffers
		uint32_t nonLightCount = 0;
		for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
		{
			std::string objIndex = it->first;
			if (objIndex.find("light") != std::string::npos) {
				continue;
			}
			nonLightCount++;
		}
		int gameObjectCount = gameObjectPool.size();
		// First, get the size of the TLAS buffers and create them
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
		inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		inputs.NumDescs = nonLightCount;
		inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info;
		pDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

		// Create the buffers
		topLevelBuffers.pScratch = createBuffer(pDevice, info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, kDefaultHeapProps);
		topLevelBuffers.pResult = createBuffer(pDevice, info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, kDefaultHeapProps);
		mTlasSize = info.ResultDataMaxSizeInBytes;
		// 成员变量持有，全局记录持久资源的状态
		ASBuffer asBuffer;
		mpTopLevelAS = topLevelBuffers.pResult;
		asBuffer.SetD3D12Resource(mpTopLevelAS.Get());
		asBuffer.SetName(L"TopLevelAS");
		ResourceStateTracker::AddGlobalResourceState(mpTopLevelAS.Get(), D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

		

		// The instance desc should be inside a buffer, create and map the buffer
		topLevelBuffers.pInstanceDesc = createBuffer(pDevice, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * nonLightCount, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
		topLevelBuffers.pInstanceDesc->Map(0, nullptr, (void**)& pInstanceDesc);
		ZeroMemory(pInstanceDesc, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * nonLightCount);

		int tlasGoCount=0;
		for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
		{
			// 特殊处理剔除光源不加入光追
			std::string objIndex = it->first;
			if (objIndex.find("light")!=std::string::npos) {
				continue;
			}

			// Initialize the instance desc. We only have a single instance
			pInstanceDesc[tlasGoCount].InstanceID = tlasGoCount;                            // This value will be exposed to the shader via InstanceID()
			pInstanceDesc[tlasGoCount].InstanceContributionToHitGroupIndex = 2* tlasGoCount;   // This is the offset inside the shader-table. 
			pInstanceDesc[tlasGoCount].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			XMMATRIX m = XMMatrixTranspose(it->second->transform.ComputeModel());
			memcpy(pInstanceDesc[tlasGoCount].Transform, m.r, sizeof(pInstanceDesc[tlasGoCount].Transform));
			pInstanceDesc[tlasGoCount].AccelerationStructure =/* mpBottomLevelAS->GetGPUVirtualAddress();*/ mpBottomLevelASes[MeshIndex2blasSeqCache[it->second->mesh]]->GetGPUVirtualAddress();
			pInstanceDesc[tlasGoCount].InstanceMask = 0xFF;
			tlasGoCount++;
		}
		
		// Unmap
		topLevelBuffers.pInstanceDesc->Unmap(0, nullptr);

		// Create the TLAS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
		asDesc.Inputs = inputs;
		asDesc.Inputs.InstanceDescs = topLevelBuffers.pInstanceDesc->GetGPUVirtualAddress();
		asDesc.DestAccelerationStructureData = topLevelBuffers.pResult->GetGPUVirtualAddress();
		asDesc.ScratchAccelerationStructureData = topLevelBuffers.pScratch->GetGPUVirtualAddress();

		commandList->BuildRaytracingAccelerationStructure(&asDesc);

		// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
		commandList->UAVBarrier(asBuffer, true);
	}

	// Wait for the execution of commandlist on the direct command queue, then release scratch resources in topLevelBuffers and bottomLevelBuffers
	uint64_t mFenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(mFenceValue);
}

void HybridPipeline::createRtPipelineState()
{
	// Need 10 subobjects:
	//  1 for the DXIL library
	//  1 for hit-group
	//  2 for RayGen root-signature (root-signature and the subobject association)
	//  2 for the root-signature shared between miss and hit shaders (signature and association)
	//  2 for shader config (shared between all programs. 1 for the config, 1 for association)
	//  1 for pipeline config
	//  1 for the global root signature
	auto mpDevice = Application::Get().GetDevice();

	std::array<D3D12_STATE_SUBOBJECT, 13> subobjects;
	uint32_t index = 0;

	// Compile the shader
	auto pDxilLib = compileLibrary(L"RTPipeline/shaders/RayTracing.hlsl", L"lib_6_3");
	const WCHAR* entryPoints[] = { kRayGenShader, kShadowMissShader ,kShadwoClosestHitShader, kSecondaryMissShader , kSecondaryClosestHitShader };
	DxilLibrary dxilLib = DxilLibrary(pDxilLib, entryPoints, arraysize(entryPoints));
	subobjects[index++] = dxilLib.stateSubobject; // 0 Library

	HitProgram shadowHitProgram(0, kShadwoClosestHitShader, kShadowHitGroup);
	subobjects[index++] = shadowHitProgram.subObject; // 1 shadow Hit Group

	HitProgram secondaryHitProgram(0, kSecondaryClosestHitShader, kSecondaryHitGroup);
	subobjects[index++] = secondaryHitProgram.subObject; // 2 secondary Hit Group

	CD3DX12_STATIC_SAMPLER_DESC static_sampler_desc(0);

	// Create the ray-gen root-signature and association
	LocalRootSignature rgsRootSignature(mpDevice, createLocalRootDesc(2,5,1, &static_sampler_desc,3,0,0).desc);
	subobjects[index] = rgsRootSignature.subobject; // 3 RayGen Root Sig

	uint32_t rgsRootIndex = index++; // 3
	ExportAssociation rgsRootAssociation(&kRayGenShader, 1, &(subobjects[rgsRootIndex]));
	subobjects[index++] = rgsRootAssociation.subobject; // 4 Associate Root Sig to RGS

	// Create the secondary root-signature and association

	LocalRootSignature secondaryRootSignature(mpDevice, createLocalRootDesc(0, 7, 1, &static_sampler_desc ,5,0,1).desc);
	subobjects[index] = secondaryRootSignature.subobject;// 5 Secondary chs sig

	uint32_t secondaryRootIndex = index++;//5
	ExportAssociation secondaryRootAssociation(&kSecondaryClosestHitShader, 1, &(subobjects[secondaryRootIndex]));
	subobjects[index++] = secondaryRootAssociation.subobject;//6 Associate secondary sig to secondarychs

	// Create the miss- and hit-programs root-signature and association
	D3D12_ROOT_SIGNATURE_DESC emptyDesc = {};
	emptyDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	LocalRootSignature hitMissRootSignature(mpDevice, emptyDesc);
	subobjects[index] = hitMissRootSignature.subobject; // 7 Root Sig to be shared between Miss and shadowCHS

	uint32_t hitMissRootIndex = index++; // 7
	const WCHAR* missHitExportName[] = { kShadowMissShader, kShadwoClosestHitShader, kSecondaryMissShader };
	ExportAssociation missHitRootAssociation(missHitExportName, arraysize(missHitExportName), &(subobjects[hitMissRootIndex]));
	subobjects[index++] = missHitRootAssociation.subobject; // 8 Associate Root Sig to Miss and CHS

	// Bind the payload size to the programs
	ShaderConfig shaderConfig(sizeof(float) * 2, sizeof(float) * 4);
	subobjects[index] = shaderConfig.subobject; // 9 Shader Config

	uint32_t shaderConfigIndex = index++; // 9 
	const WCHAR* shaderExports[] = {kRayGenShader, kShadowMissShader, kSecondaryMissShader, kShadwoClosestHitShader, kSecondaryClosestHitShader};
	ExportAssociation configAssociation(shaderExports, arraysize(shaderExports), &(subobjects[shaderConfigIndex]));
	subobjects[index++] = configAssociation.subobject; // 10 Associate Shader Config to Miss, shadowCHS, shadowRGS, secondaryCHS, secondaryRGS

	// Create the pipeline config
	PipelineConfig config(4);
	subobjects[index++] = config.subobject; // 11 configuration 

	// Create the global root signature and store the empty signature
	GlobalRootSignature root(mpDevice, {});
	mpEmptyRootSig = root.pRootSig;
	subobjects[index++] = root.subobject; // 12

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = index; // 13
	desc.pSubobjects = subobjects.data();
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ThrowIfFailed(mpDevice->CreateStateObject(&desc, IID_PPV_ARGS(&mpPipelineState)));
}

RootSignatureDesc HybridPipeline::createLocalRootDesc(int uav_num, int srv_num, int sampler_num, const D3D12_STATIC_SAMPLER_DESC* pStaticSamplers, int cbv_num, int c32_num, int space)
{
	// Create the root-signature 
	//******Layout*********/
	// Param[0] = DescriptorTable
	/////////////// |---------------range for UAV---------------| |-------------------range for SRV-------------------|
	// Param[1-n] = 
	RootSignatureDesc desc;
	int range_size = 0;
	if (uav_num > 0) range_size++;
	if (srv_num > 0) range_size++;

	desc.range.resize(range_size);
	int range_counter = 0;
	// UAV
	if (uav_num > 0) {
		desc.range[range_counter].BaseShaderRegister = 0;
		desc.range[range_counter].NumDescriptors = uav_num;
		desc.range[range_counter].RegisterSpace = space;
		desc.range[range_counter].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		desc.range[range_counter].OffsetInDescriptorsFromTableStart = 0;
		range_counter++;
	}
	// SRV
	if (srv_num > 0) {
		desc.range[range_counter].BaseShaderRegister = 0;
		desc.range[range_counter].NumDescriptors = srv_num;
		desc.range[range_counter].RegisterSpace = space;
		desc.range[range_counter].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		desc.range[range_counter].OffsetInDescriptorsFromTableStart = uav_num;
		range_counter++;
	}
	
	int descriptor_table_counter = 0;
	if (range_counter > 0) descriptor_table_counter++;
	//if (sample_range_counter > 0) descriptor_table_counter++;

	int i = 0;
	desc.rootParams.resize(descriptor_table_counter + cbv_num + c32_num );
	{
		
		if (range_counter > 0) {
			desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			desc.rootParams[i].DescriptorTable.NumDescriptorRanges = range_size;
			desc.rootParams[i].DescriptorTable.pDescriptorRanges = desc.range.data();
			i++;
		}
		//if (sample_range_counter > 0) {
		//	desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		//	desc.rootParams[i].DescriptorTable.NumDescriptorRanges = sample_range_size;
		//	desc.rootParams[i].DescriptorTable.pDescriptorRanges = sampleDesc.range.data();
		//	i++;
		//}
	}
	for (; i < cbv_num + descriptor_table_counter; i++) {
		desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		desc.rootParams[i].Descriptor.RegisterSpace = space;
		desc.rootParams[i].Descriptor.ShaderRegister = i- descriptor_table_counter;
	}
	for (; i < cbv_num + c32_num + descriptor_table_counter; i++) {
		desc.rootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		desc.rootParams[i].Constants.Num32BitValues = 1;
		desc.rootParams[i].Constants.RegisterSpace = space;
		desc.rootParams[i].Constants.ShaderRegister = i - descriptor_table_counter;
	}
	// Create the desc
	desc.desc.NumParameters = descriptor_table_counter + cbv_num+ c32_num;
	desc.desc.pParameters = desc.rootParams.data();
	desc.desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
	if (sampler_num > 0) {
		desc.desc.NumStaticSamplers = sampler_num;
		desc.desc.pStaticSamplers = pStaticSamplers;
	}

	return desc;
}

void HybridPipeline::createShaderResources()
{
	const D3D12_HEAP_PROPERTIES kUploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	auto pDevice = Application::Get().GetDevice();
	//****************************UAV Resource
	// gOutput
	mpRtShadowOutputTexture = std::make_shared<Texture>(CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, m_Viewport.Width, m_Viewport.Height,1,0,1,0, 
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN,0
	), nullptr, TextureUsage::IntermediateBuffer, L"mpRtShadowOutputTexture");
	mpRtReflectOutputTexture = std::make_shared<Texture>(CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_R32G32B32A32_FLOAT, m_Viewport.Width, m_Viewport.Height, 1, 0, 1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_TEXTURE_LAYOUT_UNKNOWN, 0
	), nullptr, TextureUsage::IntermediateBuffer, L"mpRtReflectOutputTexture");

	//****************************SRV Resource
	//gRtScene / GPosition / GAlbedo / GMetallic / GNormal / GRoughness
	
	//****************************CBV Resource
	//create the constant buffer resource for cameraCB
	mpRTPointLightCB = createBuffer(pDevice, sizeof(PointLight), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	mpRTCameraCB = createBuffer(pDevice, sizeof(CameraRTCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ,kUploadHeapProps);
	mpRTFrameIndexCB = createBuffer(pDevice, sizeof(FrameIndexCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);

	//****************************CBV for per object 
	int objectToRT = 0;
	for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
	}
	mpRTMaterialCBList.resize(objectToRT);
	mpRTPBRMaterialCBList.resize(objectToRT);
	mpRTGameObjectIndexCBList.resize(objectToRT);
	objectToRT = 0;
	for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}

		void* pData = 0;
		// base material
		mpRTMaterialCBList[objectToRT] = createBuffer(pDevice, sizeof(BaseMaterial), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mpRTMaterialCBList[objectToRT]->Map(0, nullptr, (void**)& pData);
		memcpy(pData, &it->second->material.base, sizeof(BaseMaterial));
		mpRTMaterialCBList[objectToRT]->Unmap(0, nullptr);
		// pbr material
		mpRTPBRMaterialCBList[objectToRT] = createBuffer(pDevice, sizeof(PBRMaterial), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mpRTPBRMaterialCBList[objectToRT]->Map(0, nullptr, (void**)& pData);
		memcpy(pData, &it->second->material.pbr, sizeof(PBRMaterial));
		mpRTPBRMaterialCBList[objectToRT]->Unmap(0, nullptr);
		// rt go index
		mpRTGameObjectIndexCBList[objectToRT] = createBuffer(pDevice, sizeof(ObjectIndexRTCB), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
		mpRTGameObjectIndexCBList[objectToRT]->Map(0, nullptr, (void**)& pData);
		memcpy(pData, &it->second->gid, sizeof(float));
		mpRTGameObjectIndexCBList[objectToRT]->Unmap(0, nullptr);
		
		objectToRT++;
	}
	
}

void HybridPipeline::createSrvUavHeap() {
	auto pDevice = Application::Get().GetDevice();

	int objectToRT = 0;
	for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
	}

	//****************************Descriptor heap

	// Create the uavSrvHeap and its handle
	uint32_t srvuavBias = 7;
	uint32_t srvuavPerHitSize = 7;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = srvuavBias + objectToRT * srvuavPerHitSize;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mpSrvUavHeap)));
	D3D12_CPU_DESCRIPTOR_HANDLE uavSrvHandle = mpSrvUavHeap->GetCPUDescriptorHandleForHeapStart();

	// Create the UAV for GShadowOutput
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavGOutputDesc = {};
	uavGOutputDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	pDevice->CreateUnorderedAccessView(mpRtShadowOutputTexture->GetD3D12Resource().Get(), nullptr, &uavGOutputDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the UAV for GReflectOutput
	pDevice->CreateUnorderedAccessView(mpRtReflectOutputTexture->GetD3D12Resource().Get(), nullptr, &uavGOutputDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create srv for TLAS
	D3D12_SHADER_RESOURCE_VIEW_DESC srvTLASDesc = {};
	srvTLASDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvTLASDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvTLASDesc.RaytracingAccelerationStructure.Location = mpTopLevelAS->GetGPUVirtualAddress();
	pDevice->CreateShaderResourceView(nullptr, &srvTLASDesc, uavSrvHandle);
	uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//GBuffer
	for (int i = 0; i < 4; i++) {
		UINT pDestDescriptorRangeSizes[] = { 1 };
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &m_GBuffer.GetTexture((AttachmentPoint)i).GetShaderResourceView(), pDestDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	objectToRT = 0;
	for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		auto pLobject = it->second;

		UINT pDestDescriptorRangeSizes[] = { 1 };
		//pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
		//	1,              , pDestDescriptorRangeSizes, 
		//	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// indices 0
		pDevice->CreateShaderResourceView(nullptr, &srvTLASDesc, uavSrvHandle);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// indices 1
		D3D12_SHADER_RESOURCE_VIEW_DESC indexSrvDesc = {};
		indexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		indexSrvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		indexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		indexSrvDesc.Buffer.NumElements = (UINT)((meshPool[pLobject->mesh]->getIndexCount()+1)/2);
		indexSrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		pDevice->CreateShaderResourceView(meshPool[pLobject->mesh]->getIndexBuffer().GetD3D12Resource().Get(), &indexSrvDesc, uavSrvHandle);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		// vetices 2
		D3D12_SHADER_RESOURCE_VIEW_DESC vertexSrvDesc = {};
		vertexSrvDesc.Format = DXGI_FORMAT_UNKNOWN;
		vertexSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		vertexSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		vertexSrvDesc.Buffer.FirstElement = 0;
		vertexSrvDesc.Buffer.NumElements = meshPool[pLobject->mesh]->getVertexCount();
		vertexSrvDesc.Buffer.StructureByteStride = sizeof(VertexPositionNormalTexture);
		pDevice->CreateShaderResourceView(meshPool[pLobject->mesh]->getVertexBuffer().GetD3D12Resource().Get(), &vertexSrvDesc, uavSrvHandle);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//albedo 3
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &texturePool[pLobject->material.tex.AlbedoTexture].GetShaderResourceView() , pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//metallic 4
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &texturePool[pLobject->material.tex.MetallicTexture].GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//normal 5
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &texturePool[pLobject->material.tex.NormalTexture].GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//roughness 6
		pDevice->CopyDescriptors(1, &uavSrvHandle, pDestDescriptorRangeSizes,
			1, &texturePool[pLobject->material.tex.RoughnessTexture].GetShaderResourceView(), pDestDescriptorRangeSizes,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		uavSrvHandle.ptr += pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		objectToRT++;
	}
}

void HybridPipeline::createShaderTable()
{
	const D3D12_HEAP_PROPERTIES kUploadHeapProps =
	{
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0,
	};

	const D3D12_HEAP_PROPERTIES kDefaultHeapProps =
	{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};

	auto mpDevice = Application::Get().GetDevice();

	int objectToRT = 0;
	for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		objectToRT++;
	}

	// Calculate the size and create the buffer
	mShaderTableEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	mShaderTableEntrySize += 48; // The ray-gen's descriptor table
	mShaderTableEntrySize = Math::AlignUp(mShaderTableEntrySize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
	uint32_t shaderTableSize = mShaderTableEntrySize * (3 + objectToRT *2);

	// For simplicity, we create the shader-table on the upload heap. You can also create it on the default heap
	mpShaderTable = createBuffer(mpDevice, shaderTableSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, kUploadHeapProps);
	NAME_D3D12_OBJECT(mpShaderTable);
	ResourceStateTracker::AddGlobalResourceState(mpShaderTable.Get(), D3D12_RESOURCE_STATE_GENERIC_READ);

	// Map the buffer
	uint8_t* pData;
	ThrowIfFailed(mpShaderTable->Map(0, nullptr, (void**)& pData));

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> pRtsoProps;
	mpPipelineState->QueryInterface(IID_PPV_ARGS(&pRtsoProps));

	// Entry 0 - ray-gen program ID and descriptor data
	memcpy(pData + mShaderTableEntrySize * 0, pRtsoProps->GetShaderIdentifier(kRayGenShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
	D3D12_GPU_VIRTUAL_ADDRESS heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr;
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8*0) = heapStart;
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8*1) = mpRTPointLightCB->GetGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8*2) = mpRTCameraCB->GetGPUVirtualAddress();
	*(D3D12_GPU_VIRTUAL_ADDRESS*)(pData + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8*3) = mpRTFrameIndexCB->GetGPUVirtualAddress();
	
	// Entry 1 - shadow miss program
	memcpy(pData + mShaderTableEntrySize*1, pRtsoProps->GetShaderIdentifier(kShadowMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	// entry 2 - secondary miss program
	memcpy(pData + mShaderTableEntrySize*2, pRtsoProps->GetShaderIdentifier(kSecondaryMissShader), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

	objectToRT = 0;
	uint32_t srvuavBias = 7;
	uint32_t srvuavPerHitSize = 7;
	for (auto it = gameObjectPool.begin(); it != gameObjectPool.end(); it++)
	{
		// 特殊处理剔除光源不加入光追
		std::string objIndex = it->first;
		if (objIndex.find("light") != std::string::npos) {
			continue;
		}
		// Entry 3+i*2 -  shadow hit program
		uint8_t* pHitEntry = pData + mShaderTableEntrySize * (3 + objectToRT * 2); // +2 skips the ray-gen and miss entries
		memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kShadowHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		// entry 3+i*2+1 secondary hit program
		pHitEntry = pData + mShaderTableEntrySize * (3 + objectToRT * 2 + 1); // +2 skips the ray-gen and miss entries
		memcpy(pHitEntry, pRtsoProps->GetShaderIdentifier(kSecondaryHitGroup), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		D3D12_GPU_VIRTUAL_ADDRESS heapStart = mpSrvUavHeap->GetGPUDescriptorHandleForHeapStart().ptr
			+ mpDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * (srvuavBias + objectToRT * srvuavPerHitSize);
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 0) = heapStart;
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 1) = mpRTMaterialCBList[objectToRT]->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 2) = mpRTPBRMaterialCBList[objectToRT]->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 3) = mpRTGameObjectIndexCBList[objectToRT]->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 4) = mpRTPointLightCB->GetGPUVirtualAddress();
		*(D3D12_GPU_VIRTUAL_ADDRESS*)(pHitEntry + D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8 * 5) = mpRTFrameIndexCB->GetGPUVirtualAddress();

		objectToRT++;
	}
	// Unmap
	mpShaderTable->Unmap(0, nullptr);
}

