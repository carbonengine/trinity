// Copyright © 2012 CCP ehf.

#include "StdAfx.h"
#include "Tr2GeometryBufferParameter.h"

BLUE_DEFINE( Tr2GeometryBufferParameter );

const Be::ClassInfo* Tr2GeometryBufferParameter::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2GeometryBufferParameter, "" )
		MAP_INTERFACE( ITriEffectParameter )
		MAP_INTERFACE( ITriEffectResourceParameter )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Parameter name",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"resourcePath",
			m_resourcePath,
			"Resource path to granny file",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST,
			NULL )
		MAP_ATTRIBUTE(
			"meshIndex",
			m_meshIndex,
			"Mesh index in granny file",
			Be::READWRITE | Be::NOTIFY | Be::PERSIST )
		MAP_PROPERTY_READONLY(
			"isValid",
			IsValid,
			"Is the geometry loaded and mesh is a shader resource" )
		MAP_ATTRIBUTE(
			"gpuBuffer",
			m_gpuBuffer,
			"GPU buffer (can be specified instead of geometry res path)",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"usedByCurrentEffect",
			m_isUsedByEffect,
			"If the parameter is actually used by the effect",
			Be::READ )
	EXPOSURE_END()
}
