// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"
#include "TriConstants.h"

BLUE_DEFINE( Tr2TextureAtlas );

const Be::ClassInfo* Tr2TextureAtlas::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2TextureAtlas, "" )
		MAP_INTERFACE( Tr2TextureAtlas )
		MAP_INTERFACE( ITr2TextureProvider )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"format",
			m_format,
			"Texture format",
			Be::READ | Be::ENUM,
			Tr2RenderContextEnum_PixelFormat_Chooser )

		MAP_ATTRIBUTE(
			"width",
			m_width,
			"Width of the texture used for the atlas",
			Be::READ )

		MAP_ATTRIBUTE(
			"height",
			m_height,
			"Height of the texture used for the atlas",
			Be::READ )
		MAP_ATTRIBUTE( "mipCount", m_mipLevels, "", Be::READ );
		MAP_PROPERTY_READONLY( "multiSampleType", GetMsaaSamples, "" );
		MAP_PROPERTY_READONLY( "multiSampleQuality", GetMsaaQuality, "" );

		MAP_ATTRIBUTE(
			"optimizeOnRemoval",
			m_optimizeOnRemoval,
			"If true, the atlas attempts to collapse free areas and pull in outsiders\n"
			"whenever free space opens up in the atlas.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"paintEmptyAreas",
			m_paintEmptyAreas,
			"If true, empty areas are painted in unique colors. This is useful for visualizing\n"
			"fragmentation in the atlas.",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"margin",
			m_margin,
			"Margin for each texture in the atlas, in pixels.",
			Be::READWRITE )

		MAP_PROPERTY_READONLY(
			"texturesInAtlasCount",
			GetTexturesInAtlasCount,
			"How many textures are in the atlas texture?" )

		MAP_PROPERTY_READONLY(
			"texturesOutsideAtlasCount",
			GetTexturesOutsideAtlasCount,
			"How many textures are registered with the atlas but didn't fit?" )

		MAP_METHOD_AND_WRAP(
			"GetFreeTexels",
			GetFreeTexels,
			"How many texels are not assigned to a texture in the atlas" )

		MAP_METHOD_AND_WRAP(
			"GetFreeTexelPercentage",
			GetFreeTexelPercentage,
			"Percentage of the atlas that is unused" )

		MAP_METHOD_AND_WRAP(
			"CollapseFreeAreas",
			CollapseFreeAreas,
			"Collapses adjacent free areas where possible to make room for larger textures" )

		MAP_METHOD_AND_WRAP(
			"ConsolidateFreeAreas",
			ConsolidateFreeAreas,
			"Tries to consolidate adjacent free areas, improving the 'squareness' of free regions" )

		MAP_METHOD_AND_WRAP(
			"PullInOutsiders",
			PullInOutsiders,
			"Moves any textures that were registered with the atlas and left outside\n"
			"into the atlas, as much as free space allows\n"
			":param optimizeInsertion: \n" )

		MAP_METHOD_AND_WRAP(
			"GetFreeMaxWidth",
			GetFreeMaxWidth,
			"Get the width of the largest free area in the atlas" )

		MAP_METHOD_AND_WRAP(
			"GetFreeMaxHeight",
			GetFreeMaxHeight,
			"Get the height of the largest free area in the atlas" )

		MAP_METHOD_AND_WRAP(
			"EjectAllTextures",
			EjectAllTextures,
			"Eject all textures from the atlas" )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"CreateTexture",
			CreateTexture,
			1,
			"Creates a texture of the given dimensions in the atlas.\n\n"
			":param width: Width of the texture\n"
			":param height: Height of the texture\n"
			":param textureType: Texture type" )

#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP(
			"GetTexturesOutsideAtlas",
			GetTexturesOutsideAtlas,
			"Returns a list of all textures that were registered with the atlas\n"
			"and left outside." )
#endif

		MAP_METHOD_AND_WRAP(
			"HasALObject",
			HasALObject,
			"Returns True iff Tr2TextureAtlas contains a reference to passed AL object ID.\n"
			"Used for debugging along with trinity.GetLiveALResources.\n"
			":param alType: AL object type (trinity.AL_OBJECT_TYPE)\n"
			":param alObject: AL object ID" )
	EXPOSURE_END()
}
