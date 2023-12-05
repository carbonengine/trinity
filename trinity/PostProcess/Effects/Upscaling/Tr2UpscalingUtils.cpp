////////////////////////////////////////////////////////////////////////////////
//
// Created:		March 2023
// Copyright:	CCP 2023
//
#include "Tr2UpscalingUtils.h"
namespace AMDUpscaling
{
	Vector4 AsVector( uintfloat4 v )
	{
		return Vector4( v.f[0], v.f[1], v.f[2], v.f[3] );
	}
}

namespace Jitter{
    float Halton(uint32_t index, uint32_t base)
    {
       float result = 0.0;
       float fractional = 1.0;
       while( index > 0 ){
           fractional /= float(base);
           result += fractional * float(index % base);
           index /= base;
       }
       return result;
    }
}

namespace UpscalingUtils
{
    uint32_t ConvertDisplaySizeToRenderSize( uint32_t displaySize, float upscaling )
    {
		uint32_t renderSize = uint32_t(displaySize / (float)upscaling);
		uint32_t addition = displaySize % 2 != renderSize % 2;
		return renderSize + addition;
    }
}
