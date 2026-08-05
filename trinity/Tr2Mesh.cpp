// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Mesh.h"
#include "Resources/TriGeometryRes.h"


Tr2Mesh::Tr2Mesh( IRoot* lockobj ) :
	PARENTLOCK( m_serializedMorphAnimations ),
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
	else if( IsMatch( value, m_meshIndex ) )
	{
		InitializeMorphTargets();
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

void Tr2Mesh::InitializeMorphTargets()
{
	m_morphAnimations.clear();
	auto names = GetMorphTargetNames();

	if( names )
	{
		std::vector<std::string>& morphTargetNames = *names;

		bool clearSerializedData = false;
		if( morphTargetNames.size() == m_serializedMorphAnimations.size() )
		{
			for( int32_t i = 0; i < morphTargetNames.size(); i++ )
			{
				if( morphTargetNames[i] != m_serializedMorphAnimations[i]->m_name )
				{
					clearSerializedData = true;
					break;
				}
			}
		}
		else
		{
			clearSerializedData = true;
		}
		if( clearSerializedData )
		{
			m_serializedMorphAnimations.Clear();
			for( int32_t i = 0; i < morphTargetNames.size(); i++ )
			{
				Tr2SerializedMorphAnimationPtr serializedWeight;
				serializedWeight.CreateInstance();
				serializedWeight->m_name = morphTargetNames[i];
				serializedWeight->m_weight = 0.f;
				m_serializedMorphAnimations.Append( serializedWeight );
			}
		}

		for( int32_t i = 0; i < morphTargetNames.size(); i++ )
		{
			m_morphAnimations[morphTargetNames[i]] = Tr2MorphTargetAnimationData( i, m_serializedMorphAnimations[i]->m_weight );
		}
	}
}

void Tr2Mesh::RebuildCachedData( BlueAsyncRes* p )
{
	if( p == m_geometryResource || p == m_lowResGeometryResource )
	{
		CacheBounds();
		InitializeMorphTargets();
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

std::vector<std::string>* Tr2Mesh::GetMorphTargetNames() const
{
	if( !GetGeometryResource() )
	{
		return nullptr;
	}

	auto mesh = GetGeometryResource()->GetMeshLod( GetMeshIndex(), 0 );

	if( !mesh )
	{
		return nullptr;
	}

	return &mesh->m_morphTargetNames;
}

bool Tr2Mesh::IsBakedMorph( int index ) const
{
	if( !GetGeometryResource() )
	{
		return false;
	}

	auto mesh = GetGeometryResource()->GetMeshLod( GetMeshIndex(), 0 );

	if( !mesh )
	{
		return false;
	}

	return mesh->m_isBakedMorphTarget[index];
}

void Tr2Mesh::SetMorphTargetWeight( const char* name, float weight )
{
	auto anim = m_morphAnimations.find( name );

	if( anim != m_morphAnimations.end() )
	{
		anim->second.m_weight = weight;
		m_serializedMorphAnimations[anim->second.m_index]->m_weight = weight;
	}
	else
	{
		CCP_LOGWARN( "Tr2Mesh::SetMorphTargetWeight cannot find %s", name );
	}
}

float Tr2Mesh::GetMorphTargetWeight( const char* name )
{
	auto anim = m_morphAnimations.find( name );

	if( anim != m_morphAnimations.end() )
	{
		return anim->second.m_weight;
	}
	else
	{
		CCP_LOGWARN( "Tr2Mesh::GetMorphTargetWeight cannot find %s", name );
	}
	return 0.f;
}

void Tr2Mesh::SetBakedMorphTarget( const char* name, bool isBaked )
{
	if( !GetGeometryResource() )
	{
		return;
	}

	auto mesh = GetGeometryResource()->GetMeshLod( GetMeshIndex(), 0 );
	if( !mesh )
	{
		return;
	}

	for( int i = 0; i < mesh->m_morphTargetNames.size(); i++ )
	{
		if( mesh->m_morphTargetNames[i] == name )
		{
			mesh->m_isBakedMorphTarget[i] = isBaked;
			return;
		}
	}
}

bool Tr2Mesh::GetBakedMorphTarget( const char* name )
{
	if( !GetGeometryResource() )
	{
		return false;
	}

	auto mesh = GetGeometryResource()->GetMeshLod( GetMeshIndex(), 0 );
	if( !mesh )
	{
		return false;
	}

	for( int i = 0; i < mesh->m_morphTargetNames.size(); i++ )
	{
		if( mesh->m_morphTargetNames[i] == name )
		{
			return mesh->m_isBakedMorphTarget[i];
		}
	}
	return false;
}

std::vector<bool>* Tr2Mesh::GetAllBakedMorphTargetStates() const
{
	if( !GetGeometryResource() )
	{
		return nullptr;
	}

	auto mesh = GetGeometryResource()->GetMeshLod( GetMeshIndex(), 0 );

	if( !mesh )
	{
		return nullptr;
	}

	return &mesh->m_isBakedMorphTarget;
}

const std::unordered_map<std::string, Tr2MorphTargetAnimationData>& Tr2Mesh::GetMorphAnimations() const
{
	return m_morphAnimations;
}