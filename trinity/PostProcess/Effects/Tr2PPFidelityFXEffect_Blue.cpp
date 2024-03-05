////////////////////////////////////////////////////////////////////////////////
//
// Created:		November 2019
// Copyright:	CCP 2019
//

#include "StdAfx.h"
#include "Tr2PPFidelityFXEffect.h"

BLUE_DEFINE( Tr2PPFidelityFXEffect );

Be::VarChooser FsrQualityChooser[] = {
	{ "Off",
	  BeCast( FidelityFX::FSR_OFF),
	  "FSR is off" },
	{ "Performance",
	  BeCast( FidelityFX::FSR_PERFORMANCE ),
	  "FSR is set to performance mode" },
	{ "Balanced",
	  BeCast( FidelityFX::FSR_BALANCED ),
	  "FSR is set to balanced mode" },
	{ "Quality",
	  BeCast( FidelityFX::FSR_QUALITY),
	  "FSR is set to quality mode" },
	{ "Balanced",
	  BeCast( FidelityFX::FSR_ULTRA_QUALITY),
	  "FSR is set to ultra quality mode" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "FsrQuality", FidelityFX::FsrQuality, FsrQualityChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

const Be::ClassInfo* Tr2PPFidelityFXEffect::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2PPFidelityFXEffect, "" )
		MAP_INTERFACE( Tr2PPEffect )

		MAP_ATTRIBUTE( "intensity", m_intensity, "Intensity of sharpness filter", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "fsrEnabled", m_fsrEnabled, "Do we want to use fsr\n"
			":jessica-group: FSR", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "useRcas", m_useRcas, "Do we want Robust Contrast Adaptive Sharpening\n"
			":jessica-group: FSR", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "slowFSR", m_slowFSR, "Use slow FSR (full floats instead of halfs)\n"
			":jessica-group: FSR", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "sharpness", m_sharpness, "Sharpness of FSR\n"
			":jessica-group: FSR", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "upsamplingFactor", m_upsampling, "How much is the output upsampled\n"
			":jessica-group: FSR", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "miplodbias", m_mipLodBias, "How much do we want the mip lod bias to be\n"
			":jessica-group: FSR", Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "debug", m_debug, "Show the deubg without upsampling\n"
			":jessica-group: FSR", Be::READWRITE)

		MAP_METHOD_AND_WRAP(
			"SetFSRQuality",
			SetFSRQuality,
			"Sets the quality of FSR based\n"
			":param quality: The quality flag for FSR (FsrQuality enum)\n"
		)
		
		EXPOSURE_CHAINTO( Tr2PPEffect )
}