////////////////////////////////////////////////////////////////////////////////
//
// Created:		November 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFidelityFXEffect.h"
// FidelityFX headers
#define A_CPU
#include "ffx_a.h"
#include "ffx_fsr1.h"
#include "ffx_cas.h"


namespace FidelityFX
{
	Vector4 AsVector( uintfloat4 v )
	{
		return Vector4( v.f[0], v.f[1], v.f[2], v.f[3] );
	}
}

Tr2PPFidelityFXEffect::Tr2PPFidelityFXEffect( IRoot* lockobj ) :
	m_intensity( 0.5f ),
	m_useRcas( true ),
	m_fsrEnabled( false ),
	m_slowFSR( false ),
	m_mipLodBias( 0.0f ),
	m_debug( false ),
	m_upsampling( 1.0f ),
	m_sharpness( 0.2f )
{
}

Tr2PPFidelityFXEffect::~Tr2PPFidelityFXEffect()
{

}


void Tr2PPFidelityFXEffect::SetFSRQuality( FidelityFX::FsrQuality quality )
{
	m_fsrEnabled = true;
	m_useRcas = true;
	m_sharpness = 0.2f;

	switch( quality )
	{
	case FidelityFX::FSR_ULTRA_QUALITY:
		m_upsampling = 1.3;
		break;
	case FidelityFX::FSR_QUALITY:
		m_upsampling = 1.5;
		break;
	case FidelityFX::FSR_BALANCED:
		m_upsampling = 1.7;
		break;
	case FidelityFX::FSR_PERFORMANCE:
		m_upsampling = 2.0;
		break;
	case FidelityFX::FSR_OFF:
		m_upsampling = 1.0;
		m_fsrEnabled = false;
		break;
	default:
		break;
	}
	m_mipLodBias = -log2f( m_upsampling );
	m_isDirty = true;
}

bool Tr2PPFidelityFXEffect::OnModified( Be::Var* value )
{
	if( IsMatch(value, m_sharpness) )
	{
		// keep sharpness in 0 - 2 range (0 is sharpest)
		m_sharpness = min( 2.0f, max( m_sharpness, 0.0f ) );
	}
	if( IsMatch( value, m_upsampling ) )
	{
		// keep upsampling factor above 1.0
		m_upsampling = max(1.0f, m_upsampling );
		m_mipLodBias = -log2f( m_upsampling );
	}
	return Tr2PPEffect::OnModified( value );
}

void Tr2PPFidelityFXEffect::PrepEasuConsts( uint32_t displayWidth, uint32_t displayHeight, FidelityFX::FSRConstants& constants ) const
{
	float displayWidth_f = float( displayWidth );
	float displayHeight_f = float( displayHeight );

	FsrEasuCon( 
		constants.Const0.u, 
		constants.Const1.u, 
		constants.Const2.u, 
		constants.Const3.u, 
		displayWidth_f,
		displayHeight_f,
		displayWidth_f,
		displayHeight_f,
		displayWidth_f * m_upsampling, 
		displayHeight_f * m_upsampling);

	constants.Sample.u[0] = 0; // this should be 1 if the output is hdr and we don´t have RCAS
}

void Tr2PPFidelityFXEffect::PrepRcasConsts( FidelityFX::FSRConstants& constants ) const
{
	FsrRcasCon( constants.Const0.u, m_sharpness );
	
	constants.Sample.u[0] = 0; // this should be 1 if the output is hdr
}

void Tr2PPFidelityFXEffect::PrepCasConsts( uint32_t displayWidth, uint32_t displayHeight, FidelityFX::CASConstants& constants ) const
{
	AF1 outWidth = static_cast<AF1>( displayWidth );
	AF1 outHeight = static_cast<AF1>( displayHeight );

	CasSetup( constants.const0.u, constants.const1.u, m_intensity, outWidth, outHeight, outWidth, outHeight );
}

bool Tr2PPFidelityFXEffect::IsActive()
{
	return m_display && m_intensity > 0;
}

float Tr2PPFidelityFXEffect::GetFSRMipLodBias() const
{
	if( m_display && m_fsrEnabled )
	{
		return m_mipLodBias;
	}
	return 0.0f;
}