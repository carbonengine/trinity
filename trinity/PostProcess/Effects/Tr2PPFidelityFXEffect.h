////////////////////////////////////////////////////////////////////////////////
//
// Created:		November 2019
// Copyright:	CCP 2019
//

#pragma once

#include "PostProcess/Effects/Tr2PPEffect.h"

BLUE_DECLARE( Tr2RenderTarget );

typedef union
{
	uint32_t u[4];
	float f[4];
} uintfloat4;

namespace FidelityFX
{
	Vector4 AsVector( uintfloat4 v );

	struct FSRConstants
	{
		uintfloat4 Const0 = {};
		uintfloat4 Const1 = {};
		uintfloat4 Const2 = {};
		uintfloat4 Const3 = {};
		uintfloat4 Sample = {};
	};

	struct CASConstants
	{
		uintfloat4 const0 = {};
		uintfloat4 const1 = {};
	};

	enum FsrQuality
	{
		FSR_OFF,
		FSR_ULTRA_QUALITY,
		FSR_QUALITY,
		FSR_BALANCED,
		FSR_PERFORMANCE,
	};

}

BLUE_CLASS( Tr2PPFidelityFXEffect ) :
	public Tr2PPEffect
{
public:
	EXPOSE_TO_BLUE();

	Tr2PPFidelityFXEffect( IRoot* lockobj = NULL );
	~Tr2PPFidelityFXEffect();

	void SetFSRQuality( FidelityFX::FsrQuality quality );

	void PrepEasuConsts( uint32_t displayWidth, uint32_t displayHeight, FidelityFX::FSRConstants & constants ) const;
	void PrepRcasConsts( FidelityFX::FSRConstants & constants ) const;
	void PrepCasConsts( uint32_t displayWidth, uint32_t displayHeight, FidelityFX::CASConstants & constants ) const;

	float GetFSRMipLodBias() const;

	// Tr2PPEffect
	bool IsActive() override;

	bool OnModified( Be::Var * value );

	float m_intensity;
	float m_sharpness;
	bool m_useRcas;
	bool m_fsrEnabled;
	float m_upsampling;
	bool m_slowFSR;
	float m_mipLodBias;
	bool m_debug;
};

TYPEDEF_BLUECLASS( Tr2PPFidelityFXEffect );