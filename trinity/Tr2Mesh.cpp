#include "StdAfx.h"
#include "Tr2Mesh.h"
#include "Resources/TriGeometryRes.h"


Tr2Mesh::Tr2Mesh( IRoot* lockobj ) :
	m_deferGeometryLoad( false )
{
}

Tr2Mesh::~Tr2Mesh()
{
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}
	if( m_lowResGeometryResource )
	{
		m_lowResGeometryResource->RemoveNotifyTarget( this );
	}
}


// ---------------------------------------------------------------
bool Tr2Mesh::Initialize()
{
	if( !m_deferGeometryLoad )
	{
		InitializeGeometryResource();
	}

	return true;
}

// ---------------------------------------------------------------
bool Tr2Mesh::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_meshResPath ) )
	{
		InitializeGeometryResource();
	}
	else if( IsMatch( value, m_deferGeometryLoad ) )
	{
		if( !m_deferGeometryLoad && !m_geometryResource )
		{
			Initialize();
		}
	}

	return true;
}

void Tr2Mesh::SetGeometryRes( TriGeometryRes* res )
{
	// Remove existing callback setup if any, set new geometry resource and attach callback
	if( m_geometryResource )
	{
		m_geometryResource->RemoveNotifyTarget( this );
	}

	m_geometryResource = res;

	if( m_geometryResource )
	{
		m_geometryResource->AddNotifyTarget( this );
	}
}

void Tr2Mesh::SetLowResGeometryRes( TriGeometryRes* res )
{
	// Remove existing callback setup if any, set new geometry resource and attach callback
	if( m_lowResGeometryResource )
	{
		m_lowResGeometryResource->RemoveNotifyTarget( this );
	}

	m_lowResGeometryResource = res;

	if( m_lowResGeometryResource )
	{
		m_lowResGeometryResource->AddNotifyTarget( this );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Set a new geometry path from the outside. This will trigger an initialize of
//   the new geometry resource!
// Arguments:
//   path - gr2 res path
// --------------------------------------------------------------------------------------
void Tr2Mesh::SetMeshResPath( const char* path )
{
	m_meshResPath = path;

	// trigger change, this will automatically be triggered when set through python
	OnModified( (Be::Var*)&m_meshResPath );
}

void Tr2Mesh::InitializeGeometryResource()
{
	TriGeometryResPtr lowRes;
	TriGeometryResPtr res;

	bool loadingLowRes = false;

	if( !m_meshResPath.empty() && !BePaths->FileExistsLocally( CA2W( m_meshResPath.c_str() ) ) )
	{
		auto dot = m_meshResPath.rfind( '.' );
		if( dot != std::string::npos )
		{
			auto lowResPath = m_meshResPath.substr( 0, dot ) + "_lowdetail" + m_meshResPath.substr( dot );
			if( BePaths->FileExistsLocally( CA2W( lowResPath.c_str() ) ) )
			{
				BeResMan->GetResource( lowResPath.c_str(), m_geomResourceEx.c_str(), lowRes );
				m_loadFence.Put();
				BeResMan->GetResource( m_meshResPath.c_str(), m_geomResourceEx.c_str(), res );
				loadingLowRes = true;
			}
		}
	}

	if( !loadingLowRes )
	{
		BeResMan->GetResource( m_meshResPath.c_str(), m_geomResourceEx.c_str(), res );
		m_loadFence.Put();
	}

	SetLowResGeometryRes( lowRes );
	SetGeometryRes( res );
}

void Tr2Mesh::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource || p == m_lowResGeometryResource )
	{
		CacheBounds();
	}
	if( p == m_geometryResource )
	{
		SetLowResGeometryRes( nullptr );
	}
}

void Tr2Mesh::ReleaseCachedData( BlueAsyncRes* p )
{
}

void Tr2Mesh::PySetGeometryRes( TriGeometryRes* geometryRes )
{
	SetMeshResPath( "" );
	SetGeometryRes( geometryRes );
}

int Tr2Mesh::GetAreasCount() const
{
	return TRIBATCHTYPE_COUNT_OF_BATCH_TYPES;
}

TriGeometryRes* Tr2Mesh::GetGeometryResource() const
{
	if( m_geometryResource && m_geometryResource->IsGood() )
	{
		return m_geometryResource;
	}
	return m_lowResGeometryResource ? m_lowResGeometryResource : m_geometryResource;
}

bool Tr2Mesh::IsLoading() const
{
	return !m_loadFence.IsReached();
}