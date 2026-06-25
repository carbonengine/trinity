// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2SkinnedModel.h"

BLUE_DEFINE( Tr2SkinnedModel );

const Be::ClassInfo* Tr2SkinnedModel::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2SkinnedModel, "" )

		MAP_INTERFACE( Tr2SkinnedModel )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE( "geometryRes", m_geometryRes, "na", Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER( "geometryResPath", m_geometryResPath, "Resource path to geometry resource containing skeleton", Be::READWRITE | Be::NOTIFY | Be::PERSIST, NULL )
		MAP_ATTRIBUTE( "skeletonName", m_skeletonName, "Name of skeleton to use", Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE( "skinScale", m_skinScale, "This is a scaling factor that is applied only to the skin. This is a workaround for an issue with rigs being built to a different scale from the meshes - will hopefully be fixed before too long", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "ResetAnimationBindings",
							 ResetBindings,
							 "ResetAnimationBindings()\n"
							 "Reset the animation bindings to update binding to meshes on the object." )

	EXPOSURE_CHAINTO( Tr2Model )
}
