// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2MeshProcedural.h"
#include "Resources/Tr2GeometryProvider.h"


Tr2MeshProcedural::Tr2MeshProcedural( IRoot* lockobj )
{
}

Tr2MeshProcedural::~Tr2MeshProcedural()
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}
}

bool Tr2MeshProcedural::Initialize()
{
	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
		if( !m_geometryResource->IsLoading() && !m_geometryResource->IsPrepared() )
		{
			m_geometryResource->Rebuild();
		}
	}
	return true;
}

Tr2ProceduralGeometryProvider* Tr2MeshProcedural::GetGeometry()
{
	return m_geometryResource;
}

void Tr2MeshProcedural::SetGeometryResource( Tr2ProceduralGeometryProvider* res )
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}

	m_geometryResource = res;

	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
	}

	if( m_geometryResource && !m_geometryResource->IsLoading() && !m_geometryResource->IsPrepared() )
	{
		m_geometryResource->Rebuild();
	}
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

void Tr2MeshProcedural::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		CacheBounds();
	}
}

void Tr2MeshProcedural::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource )
	{
		CacheBounds();
	}
}
