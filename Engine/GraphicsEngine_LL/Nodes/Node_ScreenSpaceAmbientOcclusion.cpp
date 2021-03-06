#include "Node_ScreenSpaceAmbientOcclusion.hpp"

#include "NodeUtility.hpp"

#include "../MeshEntity.hpp"
#include "../Mesh.hpp"
#include "../Image.hpp"
#include "../DirectionalLight.hpp"
#include "../PerspectiveCamera.hpp"
#include "../GraphicsCommandList.hpp"
#include "../EntityCollection.hpp"

#include "DebugDrawManager.hpp"

#include <array>

namespace inl::gxeng::nodes {

struct Uniforms
{
	Mat44_Packed invVP, oldVP;
	Vec4_Packed farPlaneData0, farPlaneData1;
	float nearPlane, farPlane, wsRadius, scaleFactor;
	float temporalIndex;
};


static int temporalIndex = 0;

ScreenSpaceAmbientOcclusion::ScreenSpaceAmbientOcclusion() {
	this->GetInput<0>().Set({});
}


void ScreenSpaceAmbientOcclusion::Initialize(EngineContext & context) {
	GraphicsNode::SetTaskSingle(this);
}

void ScreenSpaceAmbientOcclusion::Reset() {
	m_depthTexSrv = TextureView2D();

	GetInput<0>().Clear();
}


void ScreenSpaceAmbientOcclusion::Setup(SetupContext& context) {
	gxapi::SrvTexture2DArray srvDesc;
	srvDesc.activeArraySize = 1;
	srvDesc.firstArrayElement = 0;
	srvDesc.mipLevelClamping = 0;
	srvDesc.mostDetailedMip = 0;
	srvDesc.numMipLevels = 1;
	srvDesc.planeIndex = 0;

	Texture2D depthTex = this->GetInput<0>().Get();
	m_depthTexSrv = context.CreateSrv(depthTex, FormatDepthToColor(depthTex.GetFormat()), srvDesc);
	

	m_camera = this->GetInput<1>().Get();

	if (!m_binder.has_value()) {
		BindParameterDesc uniformsBindParamDesc;
		m_uniformsBindParam = BindParameter(eBindParameterType::CONSTANT, 0);
		uniformsBindParamDesc.parameter = m_uniformsBindParam;
		uniformsBindParamDesc.constantSize = sizeof(Uniforms);
		uniformsBindParamDesc.relativeAccessFrequency = 0;
		uniformsBindParamDesc.relativeChangeFrequency = 0;
		uniformsBindParamDesc.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		BindParameterDesc sampBindParamDesc0;
		sampBindParamDesc0.parameter = BindParameter(eBindParameterType::SAMPLER, 0);
		sampBindParamDesc0.constantSize = 0;
		sampBindParamDesc0.relativeAccessFrequency = 0;
		sampBindParamDesc0.relativeChangeFrequency = 0;
		sampBindParamDesc0.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		BindParameterDesc sampBindParamDesc1;
		sampBindParamDesc1.parameter = BindParameter(eBindParameterType::SAMPLER, 1);
		sampBindParamDesc1.constantSize = 0;
		sampBindParamDesc1.relativeAccessFrequency = 0;
		sampBindParamDesc1.relativeChangeFrequency = 0;
		sampBindParamDesc1.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		BindParameterDesc dethBindParamDesc;
		m_depthTexBindParam = BindParameter(eBindParameterType::TEXTURE, 0);
		dethBindParamDesc.parameter = m_depthTexBindParam;
		dethBindParamDesc.constantSize = 0;
		dethBindParamDesc.relativeAccessFrequency = 0;
		dethBindParamDesc.relativeChangeFrequency = 0;
		dethBindParamDesc.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		BindParameterDesc inputBindParamDesc;
		m_inputTexBindParam = BindParameter(eBindParameterType::TEXTURE, 1);
		inputBindParamDesc.parameter = m_inputTexBindParam;
		inputBindParamDesc.constantSize = 0;
		inputBindParamDesc.relativeAccessFrequency = 0;
		inputBindParamDesc.relativeChangeFrequency = 0;
		inputBindParamDesc.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		BindParameterDesc temporalBindParamDesc;
		m_temporalTexBindParam = BindParameter(eBindParameterType::TEXTURE, 2);
		temporalBindParamDesc.parameter = m_temporalTexBindParam;
		temporalBindParamDesc.constantSize = 0;
		temporalBindParamDesc.relativeAccessFrequency = 0;
		temporalBindParamDesc.relativeChangeFrequency = 0;
		temporalBindParamDesc.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		gxapi::StaticSamplerDesc samplerDesc0;
		samplerDesc0.shaderRegister = 0;
		samplerDesc0.filter = gxapi::eTextureFilterMode::MIN_MAG_MIP_POINT;
		samplerDesc0.addressU = gxapi::eTextureAddressMode::CLAMP;
		samplerDesc0.addressV = gxapi::eTextureAddressMode::CLAMP;
		samplerDesc0.addressW = gxapi::eTextureAddressMode::CLAMP;
		samplerDesc0.mipLevelBias = 0.f;
		samplerDesc0.registerSpace = 0;
		samplerDesc0.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		gxapi::StaticSamplerDesc samplerDesc1;
		samplerDesc1.shaderRegister = 1;
		samplerDesc1.filter = gxapi::eTextureFilterMode::MIN_MAG_MIP_LINEAR;
		samplerDesc1.addressU = gxapi::eTextureAddressMode::CLAMP;
		samplerDesc1.addressV = gxapi::eTextureAddressMode::CLAMP;
		samplerDesc1.addressW = gxapi::eTextureAddressMode::CLAMP;
		samplerDesc1.mipLevelBias = 0.f;
		samplerDesc1.registerSpace = 0;
		samplerDesc1.shaderVisibility = gxapi::eShaderVisiblity::ALL;

		m_binder = context.CreateBinder({ uniformsBindParamDesc, sampBindParamDesc0, sampBindParamDesc1, dethBindParamDesc, inputBindParamDesc, temporalBindParamDesc },{ samplerDesc0, samplerDesc1 });
	}

	if (!m_fsq.HasObject()) {
		std::vector<float> vertices = {
			-1, -1, 0,  0, +1,
			+1, -1, 0, +1, +1,
			+1, +1, 0, +1,  0,
			-1, +1, 0,  0,  0
		};
		std::vector<uint16_t> indices = {
			0, 1, 2,
			0, 2, 3
		};
		m_fsq = context.CreateVertexBuffer(vertices.data(), sizeof(float)*vertices.size());
		m_fsq.SetName("Screen space ambient occlusion full screen quad vertex buffer");
		m_fsqIndices = context.CreateIndexBuffer(indices.data(), sizeof(uint16_t)*indices.size(), indices.size());
		m_fsqIndices.SetName("Screen space ambient occlusion full screen quad index buffer");
	}

	if (!m_PSO) {
		InitRenderTarget(context);

		ShaderParts shaderParts;
		shaderParts.vs = true;
		shaderParts.ps = true;

		std::vector<gxapi::InputElementDesc> inputElementDesc = {
			gxapi::InputElementDesc("POSITION", 0, gxapi::eFormat::R32G32B32_FLOAT, 0, 0),
			gxapi::InputElementDesc("TEX_COORD", 0, gxapi::eFormat::R32G32_FLOAT, 0, 12)
		};

		{
			m_shader = context.CreateShader("ScreenSpaceAmbientOcclusion", shaderParts, "");

			gxapi::GraphicsPipelineStateDesc psoDesc;
			psoDesc.inputLayout.elements = inputElementDesc.data();
			psoDesc.inputLayout.numElements = (unsigned)inputElementDesc.size();
			psoDesc.rootSignature = m_binder->GetRootSignature();
			psoDesc.vs = m_shader.vs;
			psoDesc.ps = m_shader.ps;
			psoDesc.rasterization = gxapi::RasterizerState(gxapi::eFillMode::SOLID, gxapi::eCullMode::DRAW_ALL);
			psoDesc.primitiveTopologyType = gxapi::ePrimitiveTopologyType::TRIANGLE;

			psoDesc.depthStencilState.enableDepthTest = false;
			psoDesc.depthStencilState.enableDepthStencilWrite = false;
			psoDesc.depthStencilState.enableStencilTest = false;
			psoDesc.depthStencilState.cwFace = psoDesc.depthStencilState.ccwFace;

			psoDesc.numRenderTargets = 1;
			psoDesc.renderTargetFormats[0] = m_ssao_rtv.GetResource().GetFormat();

			m_PSO.reset(context.CreatePSO(psoDesc));
		}

		{
			m_horizontalShader = context.CreateShader("SsaoBilateralHorizontalBlur", shaderParts, "");

			gxapi::GraphicsPipelineStateDesc psoDesc;
			psoDesc.inputLayout.elements = inputElementDesc.data();
			psoDesc.inputLayout.numElements = (unsigned)inputElementDesc.size();
			psoDesc.rootSignature = m_binder->GetRootSignature();
			psoDesc.vs = m_horizontalShader.vs;
			psoDesc.ps = m_horizontalShader.ps;
			psoDesc.rasterization = gxapi::RasterizerState(gxapi::eFillMode::SOLID, gxapi::eCullMode::DRAW_ALL);
			psoDesc.primitiveTopologyType = gxapi::ePrimitiveTopologyType::TRIANGLE;

			psoDesc.depthStencilState.enableDepthTest = false;
			psoDesc.depthStencilState.enableDepthStencilWrite = false;
			psoDesc.depthStencilState.enableStencilTest = false;
			psoDesc.depthStencilState.cwFace = psoDesc.depthStencilState.ccwFace;

			psoDesc.numRenderTargets = 1;
			psoDesc.renderTargetFormats[0] = m_blur_horizontal_rtv.GetResource().GetFormat();

			m_horizontalPSO.reset(context.CreatePSO(psoDesc));
		}

		{
			m_verticalShader = context.CreateShader("SsaoBilateralVerticalBlur", shaderParts, "");

			gxapi::GraphicsPipelineStateDesc psoDesc;
			psoDesc.inputLayout.elements = inputElementDesc.data();
			psoDesc.inputLayout.numElements = (unsigned)inputElementDesc.size();
			psoDesc.rootSignature = m_binder->GetRootSignature();
			psoDesc.vs = m_verticalShader.vs;
			psoDesc.ps = m_verticalShader.ps;
			psoDesc.rasterization = gxapi::RasterizerState(gxapi::eFillMode::SOLID, gxapi::eCullMode::DRAW_ALL);
			psoDesc.primitiveTopologyType = gxapi::ePrimitiveTopologyType::TRIANGLE;

			psoDesc.depthStencilState.enableDepthTest = false;
			psoDesc.depthStencilState.enableDepthStencilWrite = false;
			psoDesc.depthStencilState.enableStencilTest = false;
			psoDesc.depthStencilState.cwFace = psoDesc.depthStencilState.ccwFace;

			psoDesc.numRenderTargets = 1;

			psoDesc.renderTargetFormats[0] = m_blur_vertical0_rtv.GetResource().GetFormat();
			m_vertical0PSO.reset(context.CreatePSO(psoDesc));

			psoDesc.renderTargetFormats[0] = m_blur_vertical1_rtv.GetResource().GetFormat();
			m_vertical1PSO.reset(context.CreatePSO(psoDesc));
		}
	}

	this->GetOutput<0>().Set(temporalIndex % 2 ? m_blur_vertical0_rtv.GetResource() : m_blur_vertical1_rtv.GetResource());
	//this->GetOutput<0>().Set(m_ssao_rtv.GetResource());
	//this->GetOutput<0>().Set(m_blur_horizontal_rtv.GetResource());
}


void ScreenSpaceAmbientOcclusion::Execute(RenderContext& context) {
	GraphicsCommandList& commandList = context.AsGraphics();

	Uniforms uniformsCBData;

	//DebugDrawManager::GetInstance().AddSphere(m_camera->GetPosition() + m_camera->GetLookDirection() * 5, 1, 1);

	//create single-frame only cb
	/*gxeng::VolatileConstBuffer cb = context.CreateVolatileConstBuffer(&uniformsCBData, sizeof(Uniforms));
	cb.SetName("Bright Lum pass volatile CB");
	gxeng::ConstBufferView cbv = context.CreateCbv(cb, 0, sizeof(Uniforms));
	*/

	gxeng::VertexBuffer* pVertexBuffer = &m_fsq;
	unsigned vbSize = (unsigned)m_fsq.GetSize();
	unsigned vbStride = 5 * sizeof(float);

	uniformsCBData.temporalIndex = temporalIndex;
	temporalIndex = (temporalIndex + 1) % 6;

	Mat44 v = m_camera->GetViewMatrix();
	Mat44 p = m_camera->GetProjectionMatrix();
	Mat44 vp = v * p;
	Mat44 invP = p.Inverse();
	Mat44 invVP = vp.Inverse();
	uniformsCBData.invVP = invVP;
	uniformsCBData.oldVP = m_prevVP;
	uniformsCBData.nearPlane = m_camera->GetNearPlane();
	uniformsCBData.farPlane = m_camera->GetFarPlane();

	uniformsCBData.wsRadius = 0.5f;
	uniformsCBData.scaleFactor = 0.5f * (m_depthTexSrv.GetResource().GetHeight() / (2.0f*p(0,0)));

	//far ndc corners
	Vec4 ndcCorners[] =
	{
		Vec4(-1.f, -1.f, 1.f, 1.f),
		Vec4(1.f, 1.f, 1.f, 1.f),
	};

	//convert to world space frustum corners
	ndcCorners[0] = ndcCorners[0] * invP;
	ndcCorners[1] = ndcCorners[1] * invP;
	ndcCorners[0] /= ndcCorners[0].w;
	ndcCorners[1] /= ndcCorners[1].w;

	uniformsCBData.farPlaneData0 = Vec4(ndcCorners[0].xyz, ndcCorners[1].x);
	uniformsCBData.farPlaneData1 = Vec4(ndcCorners[1].y, ndcCorners[1].z, 0.0f, 0.0f);

	{ //SSAO pass
		commandList.SetResourceState(m_ssao_rtv.GetResource(), gxapi::eResourceState::RENDER_TARGET);
		commandList.SetResourceState(m_depthTexSrv.GetResource(), { gxapi::eResourceState::PIXEL_SHADER_RESOURCE, gxapi::eResourceState::NON_PIXEL_SHADER_RESOURCE });

		RenderTargetView2D* pRTV = &m_ssao_rtv;
		commandList.SetRenderTargets(1, &pRTV, 0);

		gxapi::Rectangle rect{ 0, (int)m_ssao_rtv.GetResource().GetHeight(), 0, (int)m_ssao_rtv.GetResource().GetWidth() };
		gxapi::Viewport viewport;
		viewport.width = (float)rect.right;
		viewport.height = (float)rect.bottom;
		viewport.topLeftX = 0;
		viewport.topLeftY = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandList.SetScissorRects(1, &rect);
		commandList.SetViewports(1, &viewport);

		commandList.SetPipelineState(m_PSO.get());
		commandList.SetGraphicsBinder(&m_binder.value());
		commandList.SetPrimitiveTopology(gxapi::ePrimitiveTopology::TRIANGLELIST);

		commandList.BindGraphics(m_depthTexBindParam, m_depthTexSrv);
		commandList.BindGraphics(m_uniformsBindParam, &uniformsCBData, sizeof(Uniforms));

		commandList.SetResourceState(*pVertexBuffer, gxapi::eResourceState::VERTEX_AND_CONSTANT_BUFFER);
		commandList.SetResourceState(m_fsqIndices, gxapi::eResourceState::INDEX_BUFFER);
		commandList.SetVertexBuffers(0, 1, &pVertexBuffer, &vbSize, &vbStride);
		commandList.SetIndexBuffer(&m_fsqIndices, false);
		commandList.DrawIndexedInstanced((unsigned)m_fsqIndices.GetIndexCount());
	}


	{ //Bilateral horizontal blur pass
		commandList.SetResourceState(m_blur_horizontal_rtv.GetResource(), gxapi::eResourceState::RENDER_TARGET);
		commandList.SetResourceState(m_ssao_srv.GetResource(), { gxapi::eResourceState::PIXEL_SHADER_RESOURCE, gxapi::eResourceState::NON_PIXEL_SHADER_RESOURCE });
		commandList.SetResourceState(m_depthTexSrv.GetResource(), { gxapi::eResourceState::PIXEL_SHADER_RESOURCE, gxapi::eResourceState::NON_PIXEL_SHADER_RESOURCE });

		RenderTargetView2D* pRTV = &m_blur_horizontal_rtv;
		commandList.SetRenderTargets(1, &pRTV, 0);

		gxapi::Rectangle rect{ 0, (int)m_blur_horizontal_rtv.GetResource().GetHeight(), 0, (int)m_blur_horizontal_rtv.GetResource().GetWidth() };
		gxapi::Viewport viewport;
		viewport.width = (float)rect.right;
		viewport.height = (float)rect.bottom;
		viewport.topLeftX = 0;
		viewport.topLeftY = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandList.SetScissorRects(1, &rect);
		commandList.SetViewports(1, &viewport);

		commandList.SetPipelineState(m_horizontalPSO.get());
		commandList.SetGraphicsBinder(&m_binder.value());
		commandList.SetPrimitiveTopology(gxapi::ePrimitiveTopology::TRIANGLELIST);

		commandList.BindGraphics(m_depthTexBindParam, m_depthTexSrv);
		commandList.BindGraphics(m_inputTexBindParam, m_ssao_srv);
		commandList.BindGraphics(m_uniformsBindParam, &uniformsCBData, sizeof(Uniforms));

		commandList.SetResourceState(*pVertexBuffer, gxapi::eResourceState::VERTEX_AND_CONSTANT_BUFFER);
		commandList.SetResourceState(m_fsqIndices, gxapi::eResourceState::INDEX_BUFFER);
		commandList.SetVertexBuffers(0, 1, &pVertexBuffer, &vbSize, &vbStride);
		commandList.SetIndexBuffer(&m_fsqIndices, false);
		commandList.DrawIndexedInstanced((unsigned)m_fsqIndices.GetIndexCount());
	}

	{ //Bilateral vertical blur pass
		auto write_rtv = temporalIndex % 2 ? m_blur_vertical0_rtv : m_blur_vertical1_rtv;
		auto pso = temporalIndex % 2 ? &m_vertical0PSO : &m_vertical1PSO;
		auto read_srv = temporalIndex % 2 ? m_blur_vertical1_srv : m_blur_vertical0_srv;

		commandList.SetResourceState(write_rtv.GetResource(), gxapi::eResourceState::RENDER_TARGET);
		commandList.SetResourceState(m_blur_horizontal_srv.GetResource(), { gxapi::eResourceState::PIXEL_SHADER_RESOURCE, gxapi::eResourceState::NON_PIXEL_SHADER_RESOURCE });
		commandList.SetResourceState(read_srv.GetResource(), { gxapi::eResourceState::PIXEL_SHADER_RESOURCE, gxapi::eResourceState::NON_PIXEL_SHADER_RESOURCE });
		commandList.SetResourceState(m_depthTexSrv.GetResource(), { gxapi::eResourceState::PIXEL_SHADER_RESOURCE, gxapi::eResourceState::NON_PIXEL_SHADER_RESOURCE });

		RenderTargetView2D* pRTV = &write_rtv;
		commandList.SetRenderTargets(1, &pRTV, 0);

		gxapi::Rectangle rect{ 0, (int)write_rtv.GetResource().GetHeight(), 0, (int)write_rtv.GetResource().GetWidth() };
		gxapi::Viewport viewport;
		viewport.width = (float)rect.right;
		viewport.height = (float)rect.bottom;
		viewport.topLeftX = 0;
		viewport.topLeftY = 0;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandList.SetScissorRects(1, &rect);
		commandList.SetViewports(1, &viewport);

		commandList.SetPipelineState(pso->get());
		commandList.SetGraphicsBinder(&m_binder.value());
		commandList.SetPrimitiveTopology(gxapi::ePrimitiveTopology::TRIANGLELIST);

		commandList.BindGraphics(m_depthTexBindParam, m_depthTexSrv);
		commandList.BindGraphics(m_temporalTexBindParam, read_srv);
		commandList.BindGraphics(m_inputTexBindParam, m_blur_horizontal_srv);
		commandList.BindGraphics(m_uniformsBindParam, &uniformsCBData, sizeof(Uniforms));

		commandList.SetResourceState(*pVertexBuffer, gxapi::eResourceState::VERTEX_AND_CONSTANT_BUFFER);
		commandList.SetResourceState(m_fsqIndices, gxapi::eResourceState::INDEX_BUFFER);
		commandList.SetVertexBuffers(0, 1, &pVertexBuffer, &vbSize, &vbStride);
		commandList.SetIndexBuffer(&m_fsqIndices, false);
		commandList.DrawIndexedInstanced((unsigned)m_fsqIndices.GetIndexCount());
	}

	m_prevVP = vp;
}


void ScreenSpaceAmbientOcclusion::InitRenderTarget(SetupContext& context) {
	if (!m_outputTexturesInited) {
		m_outputTexturesInited = true;

		using gxapi::eFormat;

		auto formatSSAO = eFormat::R8G8B8A8_UNORM;

		gxapi::RtvTexture2DArray rtvDesc;
		rtvDesc.activeArraySize = 1;
		rtvDesc.firstArrayElement = 0;
		rtvDesc.firstMipLevel = 0;
		rtvDesc.planeIndex = 0;

		gxapi::SrvTexture2DArray srvDesc;
		srvDesc.activeArraySize = 1;
		srvDesc.firstArrayElement = 0;
		srvDesc.numMipLevels = -1;
		srvDesc.mipLevelClamping = 0;
		srvDesc.mostDetailedMip = 0;
		srvDesc.planeIndex = 0;

		Texture2DDesc desc{
			m_depthTexSrv.GetResource().GetWidth(),
			m_depthTexSrv.GetResource().GetHeight(),
			formatSSAO
		};

		Texture2D ssao_tex = context.CreateTexture2D(desc, { true, true, false, false });
		ssao_tex.SetName("Screen space ambient occlusion tex");
		m_ssao_rtv = context.CreateRtv(ssao_tex, formatSSAO, rtvDesc);
		m_ssao_srv = context.CreateSrv(ssao_tex, formatSSAO, srvDesc);

		Texture2D blur_vertical1_tex = context.CreateTexture2D(desc, { true, true, false, false });
		blur_vertical1_tex.SetName("Screen space ambient occlusion vertical blur tex");
		m_blur_vertical1_rtv = context.CreateRtv(blur_vertical1_tex, formatSSAO, rtvDesc);
		m_blur_vertical1_srv = context.CreateSrv(blur_vertical1_tex, formatSSAO, srvDesc);

		Texture2D blur_vertical0_tex = context.CreateTexture2D(desc, { true, true, false, false });
		blur_vertical0_tex.SetName("Screen space ambient occlusion vertical blur tex");
		m_blur_vertical0_rtv = context.CreateRtv(blur_vertical0_tex, formatSSAO, rtvDesc);
		m_blur_vertical0_srv = context.CreateSrv(blur_vertical0_tex, formatSSAO, srvDesc);

		Texture2D blur_horizontal_tex = context.CreateTexture2D(desc, { true, true, false, false });
		blur_horizontal_tex.SetName("Screen space ambient occlusion horizontal blur tex");
		m_blur_horizontal_rtv = context.CreateRtv(blur_horizontal_tex, formatSSAO, rtvDesc);
		m_blur_horizontal_srv = context.CreateSrv(blur_horizontal_tex, formatSSAO, srvDesc);
	}
}


} // namespace inl::gxeng::nodes
