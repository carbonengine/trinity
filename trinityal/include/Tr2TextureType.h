#pragma once
#ifndef Tr2TextureType_H
#define Tr2TextureType_H

namespace Tr2RenderContextEnum
{
// Texture types
enum TextureType
{
	TEX_TYPE_1D		= 1,
	TEX_TYPE_2D,
	TEX_TYPE_3D,
	TEX_TYPE_CUBE,

	TEX_TYPE_TYPELESS,	// valid but unknown dimensions

	TEX_TYPE_INVALID
};

}

#endif