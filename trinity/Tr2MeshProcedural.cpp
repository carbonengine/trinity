// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2MeshProcedural.h"
#include "Resources/Tr2GeometryProvider.h"


Tr2MeshProcedural::Tr2MeshProcedural( IRoot* lockobj )
{
}

Tr2MeshProcedural::~Tr2MeshProcedural()
{
}

void Tr2MeshProcedural::SetGeometryRes( Tr2GeometryProvider* res )
{
	m_geometryResource = res;
}

void Tr2MeshProcedural::PySetGeometryRes( Tr2GeometryProvider* geometryRes )
{
	SetGeometryRes( geometryRes );
}

int Tr2MeshProcedural::GetAreasCount() const
{
	return TRIBATCHTYPE_COUNT_OF_BATCH_TYPES;
}

Tr2GeometryProvider* Tr2MeshProcedural::GetGeometryResource() const
{
	if( m_geometryResource && m_geometryResource->IsGood() )
	{
		return m_geometryResource;
	}
	return nullptr;
}

bool Tr2MeshProcedural::IsLoading() const
{
	return m_geometryResource && m_geometryResource->IsLoading();
}
