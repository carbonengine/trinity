////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "StdAfx.h"
#include "Tr2Fsr1Upscaling.h"
#include "Tr2Renderer.h"

// FidelityFX headers
#define A_CPU
#include "ffx_a.h"
#include "ffx_fsr1.h"


const float FSR_SHARPENING = 0.2f;
const uint32_t CAS_THREAD_GROUP_WORK_REGION_DIM = 16;

Tr2Fsr1Upscaling::Tr2Fsr1Upscaling( IRoot* lockobj ) :
	m_renderWidth( 0 ),
	m_renderHeight( 0 ),
	m_displayWidth( 0 ),
	m_displayHeight( 0 ),
	m_upscaling( 1.0f )
{
	m_fidelityFXFsrEASUShader.CreateInstance();
	m_fidelityFXFsrEASUShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/fsr/fsr.fx" );
	m_fidelityFXFsrEASUShader->SetOption( BlueSharedString( "FSR_STEP" ), BlueSharedString( "EDGE_ADAPTIVE_SPATIAL_UPSAMPLING" ) );

	m_fidelityFXFsrRCASShader.CreateInstance();
	m_fidelityFXFsrRCASShader->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/PostProcess/fsr/fsr.fx" );
	m_fidelityFXFsrRCASShader->SetOption( BlueSharedString( "FSR_STEP" ), BlueSharedString( "ROBUST_CONTRAST_ADAPTIVE_SHARPENING" ) );
}

Tr2Fsr1Upscaling::~Tr2Fsr1Upscaling()
{
}

bool Tr2Fsr1Upscaling::IsApplicable() const
{
	// FSR 1 is available on all platforms
	return true;
}

void Tr2Fsr1Upscaling::GetJitter( float& x, float& y )
{
	x = 0;
	y = 0;
}

void Tr2Fsr1Upscaling::GetJitterOffset( float& x, float& y )
{
	x = 0;
	y = 0;
}

const std::vector<Tr2Upscaling::Setting> Tr2Fsr1Upscaling::GetAvailableSettings() const
{
	static std::vector<Tr2Upscaling::Setting> availableSettings = { 
		Tr2Upscaling::PERFORMANCE,
		Tr2Upscaling::BALANCED,
		Tr2Upscaling::QUALITY,
		Tr2Upscaling::ULTRA_QUALITY };
	return availableSettings;
}

float Tr2Fsr1Upscaling::GetMipLevelBias() const
{
	return -log2f( m_upscaling );
}

void Tr2Fsr1Upscaling::GetRenderSize(uint32_t& width, uint32_t& height) const
{
	width = m_renderWidth;
	height = m_renderHeight;
}

Tr2Upscaling::UpscalingType Tr2Fsr1Upscaling::GetUpscalingType() const
{
	return Tr2Upscaling::UT_SPATIAL;
}

void Tr2Fsr1Upscaling::ApplySetting( const Tr2Upscaling::Setting& setting, uint32_t displayWidth, uint32_t displayHeight )
{
	m_upscaling = 1.0f;
	switch( setting )
	{
	case Tr2Upscaling::ULTRA_QUALITY:
		m_upscaling = 1.3;
		break;
	case Tr2Upscaling::QUALITY:
		m_upscaling = 1.5;
		break;
	case Tr2Upscaling::BALANCED:
		m_upscaling = 1.7;
		break;
	case Tr2Upscaling::PERFORMANCE:
	case Tr2Upscaling::ULTRA_PERFORMANCE:
		m_upscaling = 2.0;
		break;
	default:
		CCP_LOGERR( "Invalid Setting Applied to Tr2FSR1Upscaling: %d", setting );
		break;
	}

	m_displayWidth = displayWidth;
	m_displayHeight = displayHeight;
	
	m_renderWidth = UpscalingUtils::ConvertDisplaySizeToRenderSize( displayWidth, m_upscaling );
	m_renderHeight = UpscalingUtils::ConvertDisplaySizeToRenderSize( displayHeight, m_upscaling );
}

void Tr2Fsr1Upscaling::Setup( Tr2Upscaling::UpscalingSetupContext setupContext, Tr2RenderContext& renderContext)
{
	float renderWidth_f = float( m_renderWidth );
	float renderHeight_f = float( m_renderHeight );
	float displayWidth_f = float( m_displayWidth );
	float displayHeight_f = float( m_displayHeight );

	FsrEasuCon(
		m_easuConst.Const0.u,
		m_easuConst.Const1.u,
		m_easuConst.Const2.u,
		m_easuConst.Const3.u,
		renderWidth_f,
		renderHeight_f,
		renderWidth_f,
		renderHeight_f,
		displayWidth_f,
		displayHeight_f );
	m_easuConst.Sample.u[0] = 0; // this should be 1 if the output is hdr and we don´t have RCAS

	FsrRcasCon( m_rcasConst.Const0.u, FSR_SHARPENING );
	m_rcasConst.Sample.u[0] = 0; // this should be 1 if the output is hdr

	m_fidelityFXFsrEASUShader->StartUpdate();
	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "Const0" ), AMDUpscaling::AsVector( m_easuConst.Const0 ) );
	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "Const1" ), AMDUpscaling::AsVector( m_easuConst.Const1 ) );
	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "Const2" ), AMDUpscaling::AsVector( m_easuConst.Const2 ) );
	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "Const3" ), AMDUpscaling::AsVector( m_easuConst.Const3 ) );
	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "Sample" ), AMDUpscaling::AsVector( m_easuConst.Sample ) );
	m_fidelityFXFsrEASUShader->EndUpdate();
	
	m_fidelityFXFsrRCASShader->StartUpdate();
	m_fidelityFXFsrRCASShader->SetParameter( BlueSharedString( "Const0" ), AMDUpscaling::AsVector( m_rcasConst.Const0 ) );
	m_fidelityFXFsrRCASShader->SetParameter( BlueSharedString( "Sample" ), AMDUpscaling::AsVector( m_rcasConst.Sample ) );
	m_fidelityFXFsrRCASShader->EndUpdate();
}

void Tr2Fsr1Upscaling::Dispatch( Tr2RenderContext& renderContext, Tr2PostProcessRenderInfo& renderInfo, Tr2Upscaling::Textures& textures, const Tr2Upscaling::SceneInformation& sceneInformation )
{
	GPU_REGION( renderContext, "Fsr1 Upscaling" );

	auto easuOutput = renderInfo.GetTempTexture( m_upscaling, Tr2RenderContextEnum::EX_BIND_UNORDERED_ACCESS );

	int dispatchX = ( easuOutput->GetWidth() + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;
	int dispatchY = ( easuOutput->GetHeight() + ( CAS_THREAD_GROUP_WORK_REGION_DIM - 1 ) ) / CAS_THREAD_GROUP_WORK_REGION_DIM;

	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "InputTexture" ), textures.input );
	m_fidelityFXFsrEASUShader->SetParameter( BlueSharedString( "OutputTexture" ), easuOutput );

	m_fidelityFXFsrRCASShader->SetParameter( BlueSharedString( "InputTexture" ), easuOutput );
	m_fidelityFXFsrRCASShader->SetParameter( BlueSharedString( "OutputTexture" ), textures.output );

	Tr2Renderer::RunComputeShader( m_fidelityFXFsrEASUShader, dispatchX, dispatchY, 1, renderContext );
	Tr2Renderer::RunComputeShader( m_fidelityFXFsrRCASShader, dispatchX, dispatchY, 1, renderContext );
}
