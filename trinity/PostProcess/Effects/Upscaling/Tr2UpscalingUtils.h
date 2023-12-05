////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#pragma once
namespace AMDUpscaling
{
	typedef union
	{
		uint32_t u[4];
		float f[4];
	} uintfloat4;

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
}

namespace Jitter {
    float Halton(uint32_t index, uint32_t base);
}

namespace UpscalingUtils
{
	uint32_t ConvertDisplaySizeToRenderSize( uint32_t displaySize, float upscaling );
}
