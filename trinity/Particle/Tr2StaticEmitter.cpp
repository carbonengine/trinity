////////////////////////////////////////////////////////////
//
//    Created:   December 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2StaticEmitter.h"
#include "Tr2ParticleSystem.h"
#include "Resources/TriGrannyRes.h"
#include "Tr2Renderer.h"
#include "Tr2VertexDefinitionUtilities.h"


// --------------------------------------------------------------------------------------
// Description:
//   Tr2StaticEmitter default constructor
// --------------------------------------------------------------------------------------
Tr2StaticEmitter::Tr2StaticEmitter( IRoot* lockobj )
	:m_hasSpawnedParticles( false ),
	m_meshIndex( 0 ),
	m_isThreadSafe( false )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2StaticEmitter destructor
// --------------------------------------------------------------------------------------
Tr2StaticEmitter::~Tr2StaticEmitter()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Starts loading particle geometry file.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2StaticEmitter::Initialize()
{
	if( !m_geometryResourcePath.empty() )
	{
		BeResMan->GetResource( m_geometryResourcePath, 
							   "raw", 
							   m_geometryResource );
	}
	if( m_particleSystem && m_isThreadSafe )
	{
		m_particleSystem->SetThreadSafeFlag();
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface.  Allows the emitter o respond to parameter changes 
//   generated in Python.  If the particle geometry resource path changes, the emitter 
//   starts loading new particle data.  
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2StaticEmitter::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_geometryResourcePath ) )
	{
		BeResMan->GetResource( m_geometryResourcePath, 
							   "raw", 
							   m_geometryResource );
	}
	else if( IsMatch( value, m_particleSystem ) )
	{
		if( m_isThreadSafe && m_particleSystem )
		{
			m_particleSystem->SetThreadSafeFlag();
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter. Notifies the linked particle system that it needs
//   additional syncronization when inserting particles.
// --------------------------------------------------------------------------------------
void Tr2StaticEmitter::SetThreadSafeFlag()
{
	m_isThreadSafe = true;
	if( m_particleSystem )
	{
		m_particleSystem->SetThreadSafeFlag();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Spawns particles from geometry resource if resource is loaded.  
// --------------------------------------------------------------------------------------
void Tr2StaticEmitter::DoSpawn()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_particleSystem && m_particleSystem->IsValid() && 
		m_geometryResource && m_geometryResource->IsGood() && Tr2Renderer::IsResourceCreationAllowed() )
	{
		m_hasSpawnedParticles = true;

		if( int( m_meshIndex ) >= m_geometryResource->GetMeshCount() )
		{
			CCP_LOGERR( "Invalid mesh index for Tr2StaticEmitter with geometry path %s", 
						m_geometryResourcePath.c_str() );
			return;
		}
		auto meshData = m_geometryResource->GetGrannyMesh( m_meshIndex );
		auto vertexDefinition = m_geometryResource->GetGrannyVertexType( m_meshIndex );

		if( !meshData || !meshData->PrimaryVertexData || !vertexDefinition )
		{
			CCP_LOGERR( "Invalid granny file for Tr2StaticEmitter with geometry path %s", 
						m_geometryResourcePath.c_str() );
			return;
		}

		Tr2VertexDefinition elements = BuildFromGrannyVertexDecl( vertexDefinition );

		// Validate geometry vertex declaration against particle system element declaration
		const Tr2ParticleElementDataMap &particleElements = m_particleSystem->GetElementDeclaration();
		std::vector<DeclarationMapping> geometryDeclarationMap;
		for( auto i = particleElements.begin(); i != particleElements.end(); ++i )
		{
			bool found = false;
			for( size_t j = 0; j != elements.m_items.size(); ++j )
			{
				unsigned usageIndex = 0;
				if( i->first.m_type == Tr2ParticleElementDeclarationName::CUSTOM )
				{
					usageIndex = i->second.m_usageIndex;
				}
				if( elements.m_items[j].m_usage == i->first.GetD3DUsage() && elements.m_items[j].m_usageIndex == usageIndex )
				{
					DeclarationMapping mapping;
					mapping.inOffset = elements.m_items[j].m_offset;
					mapping.buffer = i->second.m_bufferType;
					mapping.offset = i->second.m_offset;
					mapping.length = i->second.m_dimension;
					mapping.isEmpty = false;
					
					if( ( elements.m_items[j].m_dataType & elements.DT_TYPE_MASK ) == elements.DT_FLOAT32 )
					{
						mapping.isFloat16 = false;
					}
					else if( ( elements.m_items[j].m_dataType & elements.DT_TYPE_MASK ) == elements.DT_FLOAT16 )
					{
						mapping.isFloat16 = true;
					}
					else
					{
						CCP_LOGERR( "Incompatible type for particle elements \"%s\" in Tr2StaticEmitter geometry %s", 
									i->first.GetName().c_str(), 
									m_geometryResourcePath.c_str() );
						return;
					}
					geometryDeclarationMap.push_back( mapping );
					found = true;
					break;
				}
			}
			if( !found )
			{
				DeclarationMapping mapping;
				mapping.inOffset = 0;
				mapping.buffer = i->second.m_bufferType;
				mapping.offset = i->second.m_offset;
				mapping.length = i->second.m_dimension;
				mapping.isEmpty = true;
				mapping.isFloat16 = false;
				geometryDeclarationMap.push_back( mapping );
				CCP_LOG( "No data for particle elements \"%s\" in Tr2StaticEmitter geometry %s - filling with zeroes",
					i->first.GetName().c_str(),
					m_geometryResourcePath.c_str() );
			}
		}

		m_particleSystem->ClearParticles();

		// Spawn particles
		uint8_t* data = meshData->PrimaryVertexData->Vertices;
		uint32_t stride = m_geometryResource->GetVertexSize( m_meshIndex );
		for( int i = 0; i < meshData->PrimaryVertexData->VertexCount; ++i )
		{
			float* particle[Tr2ParticleElementData::COUNT];
			if( m_particleSystem->InsertParticle( particle ) )
			{
				for( auto j = geometryDeclarationMap.begin(); j != geometryDeclarationMap.end(); ++j )
				{
					if( j->isEmpty )
					{
						std::fill_n( particle[j->buffer] + j->offset, j->length, 0.0f );
					}
					else if( j->isFloat16 )
					{
						std::copy( 
							reinterpret_cast<Float_16*>( data + j->inOffset ),
							reinterpret_cast<Float_16*>( data + j->inOffset ) + j->length,
							particle[j->buffer] + j->offset );
					}
					else
					{
						std::copy( 
							reinterpret_cast<float*>( data + j->inOffset ), 
							reinterpret_cast<float*>( data + j->inOffset ) + j->length, 
							particle[j->buffer] + j->offset );
					}
				}
				m_particleSystem->DoneInsertingParticle();
			}
			data += stride;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Spawns particles from geometry resource
//   if resource is loaded and particles were not alread spawned.  
// Arguments:
//   arguments - Update arguments
// --------------------------------------------------------------------------------------
void Tr2StaticEmitter::Update( const ITr2GenericEmitter::UpdateArguments& arguments )
{
	if( !m_hasSpawnedParticles )
	{
		DoSpawn();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2GenericEmitter interface. Does nothing since this emitter only spawns 
//   particles once during Update method.
// Arguments:
//   arguments - Update arguments
//   position - Position of the "parent" particle (unused).
//   velocity - Velocity of the "parent" particle (unused).
//   rateModifier - Modifies the number of particles spawned as opposed to emitter's
//		defined rate value (unused).
// --------------------------------------------------------------------------------------
void Tr2StaticEmitter::SpawnParticles( const ITr2GenericEmitter::UpdateArguments& arguments, 
									   const Vector3* position, 
									   const Vector3* velocity, 
									   float rateModifier )
{
}

void Tr2StaticEmitter::SpawnParticles( const ITr2GenericEmitter::UpdateArguments& arguments, 
						const Vector3 *positionStart, const Vector3 *positionEnd,
						const Vector3 *velocityStart, const Vector3 *velocityEnd,
						float deltaTime )
{
	SpawnParticles( arguments, positionEnd, velocityEnd, deltaTime );
}

// --------------------------------------------------------------------------------------
// Description:
//   Python-exposed method that forces re-spawning of particles.
// --------------------------------------------------------------------------------------
void Tr2StaticEmitter::Spawn()
{
	m_hasSpawnedParticles = false;
}
