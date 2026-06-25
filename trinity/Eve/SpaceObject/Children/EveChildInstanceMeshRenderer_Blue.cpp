// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveChildInstanceMeshRenderer.h"


Be::VarChooser RotationalConstraintsChooser[] = {
	{ "none", BeCast( EveChildInstanceMeshRenderer::RotationalConstraints::NONE ), "normal rendering" },
	{ "billboard", BeCast( EveChildInstanceMeshRenderer::RotationalConstraints::BILLBOARD ), "face the camera" },
	{ "lockZ", BeCast( EveChildInstanceMeshRenderer::RotationalConstraints::BILLBOARD_WITH_Z_LOCKED ), "face the camera but keep the original UP" },
	{ 0 }
};

BLUE_REGISTER_ENUM_EX( "RotationalConstraint", EveChildInstanceMeshRenderer::RotationalConstraints, RotationalConstraintsChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );

BLUE_DEFINE( EveChildInstanceMeshRenderer );

const Be::ClassInfo* EveChildInstanceMeshRenderer::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveChildInstanceMeshRenderer, ":jessica-icon: stars\n:jessica-icon-color: (192, 17, 0)\n" )
		MAP_INTERFACE( EveChildInstanceMeshRenderer )
		MAP_INTERFACE( EveEntity )
		MAP_INTERFACE( EveChildMesh )
		MAP_INTERFACE( IEveSpaceObjectChild )
		MAP_INTERFACE( ITr2Renderable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( IEveShadowCaster )

		MAP_ATTRIBUTE( "name", m_name, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "display", m_display, "", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "distribution", m_distribution, "distributionMethod for entities\n:jessica-icon: map-location-dot\n", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "mesh", m_mesh, "the rendered mesh", Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE_WITH_CHOOSER( "rotationConstraint", m_rotationConstraint, "", Be::READWRITE | Be::PERSIST | Be::ENUM, RotationalConstraintsChooser );

		MAP_ATTRIBUTE( "staticOffsetTranslation", m_staticOffsetTranslation, "an positional offset from the distribution placementData", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticOffsetRotation", m_staticOffsetRotation, "additional rotation in place at the location distribution placementData", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "staticOffsetScale", m_staticOffsetScale, "scale all rendered meshes", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "minScreenSize", m_minScreenSize, "", Be::READWRITE | Be::PERSIST )

		MAP_METHOD_AND_WRAP( "RefreshStaticGeometry", RefreshStaticGeometry, "if static geo parameters were changed during authoring: refresh here\n:jessica-placement: TOOLBAR" )

	EXPOSURE_END()
}
