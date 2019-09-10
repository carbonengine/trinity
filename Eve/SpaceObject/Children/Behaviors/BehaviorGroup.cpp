#include "StdAfx.h"
#include "BehaviorGroup.h"
#include "include/TriMath.h"
#include "IBehavior.h"
#include "Tr2InstancedMesh.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierTransformCommon.h" 
#include "Resources/TriGeometryRes.h"

//temp
//#include "float.h"

BehaviorGroup::BehaviorGroup( IRoot* lockobj ) :
	PARENTLOCK( m_behaviors ),
	PARENTLOCK( m_volumes ),
	PARENTLOCK( m_exclusionVolumes ),
	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_count( 0 ),
	m_display( true ),
	m_meshToggle( false ),
	m_maxVelocity( 100 ),
	m_changeBufferVertexCount( nullptr ),
	m_scale( Vector3( 1, 1, 1 ) ),
	m_spriteScale( Vector3( 7.0, 7.0, 7.0 ) ),
	m_currentScreenSize( 0.0 ),
	m_renderThreshold( 1.0 ),
	m_blendScreenSizeMin( 5.0 ),
	m_blendScreenSizeMax( 15.0 ),
	m_xfadeValue( 1.0 ),
	m_boundingSphereCenter( 0.f, 0.f, 0.f ),
	m_boundingSphereRadius( 5.f ),
	m_blendRangeMax( 1000 ), // LOD system
	m_blendRangeMin( 500 ),
	m_blendRangeValue( 1.0 ),

	m_TEMPDEBUGVECTORTOFINDCLOSEDRONES( Vector3( 0, 0, 0 ) )
{
	m_behaviors.SetNotify( this );
	m_tree = nullptr;
	m_tree.CreateInstance();
}

bool BehaviorGroup::Initialize()
{
	m_scratchData.resize( m_behaviors.size() );
	return true;
}

void BehaviorGroup::OnListModified(
	long event,		// BLUELISTEVENT values
	ssize_t key,
	ssize_t key2,
	IRoot* value,
	const struct IList* list
)
{
	if( list == &m_behaviors && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
			case BELIST_INSERTED:
			{
				m_scratchData.insert( m_scratchData.begin() + key, CcpMallocBuffer() );
				if( !m_agents.empty() )
				{
					auto size = m_behaviors[key]->GetScratchMemorySize();
					auto& scratchData = m_scratchData[key];
					scratchData.resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
					for( size_t i = 0; i < m_agents.size(); ++i )
					{
						m_behaviors[key]->InitializeScratch( m_agents[i], scratchData.get() + size * i );
					}
				}
				CreateAgentTree();
				break;
			}
			case BELIST_REMOVED:
				m_scratchData.erase( m_scratchData.begin() + key );
				CreateAgentTree();
				break;
			case BELIST_SWAPPED:
				std::swap( m_scratchData[key], m_scratchData[key2] );
				break;
			case BELIST_UNLOADSTART:
				m_scratchData.clear();
				break;
			default:
				break;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Load the geometry resource, might be a re-load
// SeeAlso:
//   IBlueResource, TriGeometryRes
// --------------------------------------------------------------------------------
void BehaviorGroup::InitializeGeometryResource()
{
	//to make resource load correctly they must be regenerated for this instance
	m_agents.clear();
	for( auto it = begin( m_scratchData ); it != end( m_scratchData ); ++it )
	{
		it->clear();
	}

	const int t = m_count;
	m_count = 0;
	SetCount( t );
}

void BehaviorGroup::SetVertexFunctionReferance( const std::function<void( void )> &F )
{
	m_changeBufferVertexCount = F;
}

BehaviorGroup::~BehaviorGroup()
{
}

size_t BehaviorGroup::GetSize()
{
	return m_agents.size();
}

unsigned int BehaviorGroup::GetCount()
{
	return unsigned( m_count );
}

void BehaviorGroup::CreateAgentTree()
{
	m_tree = nullptr;
	if ( !m_tree.CreateInstance() )
	{
		return;
	}
	m_tree->CreateTree( m_agents, m_behaviors.size());
}

// For Artists when they are creating the sprite to easily swap between mesh's
void BehaviorGroup::ToggleMesh()
{
	m_meshToggle = !m_meshToggle;
}

Tr2MeshPtr BehaviorGroup::GetMesh() const
{
	auto mesh = m_meshToggle ? m_spriteMesh : m_mesh;
	return mesh;
}

Tr2MeshPtr BehaviorGroup::GetSpriteMesh() const
{
	return m_spriteMesh;
}

unsigned int BehaviorGroup::GetVertexDeclarationHandle() const
{
	return m_vertexDeclarationHandle;
}

unsigned int BehaviorGroup::GetSpriteVertexDeclarationHandle() const
{
	return m_spriteVertexDeclarationHandle;
}

void BehaviorGroup::SetGroupIndexIndicator( int index )
{
	m_groupIndex = index;
}

int BehaviorGroup::GetGroupIndexIndicator() const
{
	return m_groupIndex;
}

void BehaviorGroup::AddAgent()
{
	// The function without arguments to be called from the UI
	AddAgentPrivate();
	if ( m_changeBufferVertexCount )
	{
		(m_changeBufferVertexCount)();
	}
	CreateAgentTree();
}


void BehaviorGroup::AddAgentPrivate()
{

	//This function should change to resize the m_agents once and the buffer once, because if we setCount(1000) this is gonna be really slow
	//Now we are adding an agent, repositioning it and repositioning the buffer as well. 
 	DroneAgent agent;
	agent.position = Vector3( 0, 0, 0 ); // TODO: We might want to find a 'smart' spawn location
	agent.id = TriRandInt( 500 ); //TODO: look better into parameter, could the same ID be generate more than once?
	m_agents.push_back( agent );

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[i]->GetScratchMemorySize();
		if( m_scratchData.size() <= i )
		{
			m_scratchData.push_back( CcpMallocBuffer() );
		}
		if( size > 0)
		{
			m_scratchData[i].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
			m_behaviors[i]->InitializeScratch( agent, m_scratchData[i].get() + size * ( m_agents.size() - 1 ) );
		}
	}

	m_count++;
}

void BehaviorGroup::SetCount( int count )
{
	if ( count == m_count || count < 0 )
	{
		return;
	}

	if ( m_count < count )
	{
		int difference = count - m_count;
		for ( int i = 0; i < difference; i++ )
		{
			AddAgentPrivate();
		}
	}
	else
	{
		int difference = m_count - count;
		for ( int i = 0; i < difference; i++ )
		{
			RemoveSpecificAgent( TriRandInt( m_count ) );
		}
	}

	CreateAgentTree();

	if ( m_changeBufferVertexCount == nullptr )
	{
		return;
	}
	(m_changeBufferVertexCount)();
	CreateAgentTree();
}

float BehaviorGroup::AllTheSame()
{
	// Returns 1 if all agents are sprites
	// Returns 0 if all agents are meshes
	// Returns -1 if they are not the same or neither (no agents)
	float same = -1;
	// if none of the agents need either of the meshes we let the system know
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if ( same == -1 ) same = (agent->xfade);
		if ( same != (agent->xfade) ) return -1;
	}
	return same;
}

// The function without arguments to be called from the UI
void BehaviorGroup::RemoveAgent()
{
	RemoveSpecificAgent( TriRandInt( m_count ) );

	if ( m_changeBufferVertexCount )
	{
		(m_changeBufferVertexCount)();
	}
	CreateAgentTree();
}

// returns a vector so we can do something with the location. explosion etc
Vector3 BehaviorGroup::RemoveSpecificAgent( int index )
{
	if ( m_agents.empty() )
	{
		return Vector3( 0, 0, 0 );
	}

	Vector3 pos = m_agents[ index ].position;
	m_agents[ index ] = m_agents.back();
	m_agents.pop_back();

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[i]->GetScratchMemorySize();
		if( size == 0 )
		{
			continue;
		}

		memcpy( m_scratchData[i].get() + size * index, m_scratchData[i].get() + size * m_agents.size(), size );
		m_scratchData[i].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
	}

	m_count--;

	return pos;
}

void BehaviorGroup::UpdateAgents(const float dt, EveChildBehaviorSystem& system )
{
	if ( m_agents.empty() )
	{
		return;
	}

	std::vector<float> ranges;

	float searchRad = 0;
	for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior )
	{
		searchRad = ( *behavior )->GetBehaviorSearchRadius();
		if( searchRad == -1.f)
		{
			ranges.push_back( -1.f );
		}
		else
		{
			ranges.push_back( searchRad + m_boundingSphereRadius );
		}
	}

	const std::vector < std::vector<std::vector<DroneAgent*>>>* dronesInRange = m_tree->FindDronesInRange( m_agents, ranges, m_boundingSphereRadius );
	int groupIndex = 0;

	//Calculate the behaviors
	if( m_collectForces )
	{
		m_forces.clear();

		auto scratch = m_scratchData.begin();
		for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior, scratch++ )
		{
			
			std::vector<Vector3> forces = ( *behavior )->CalculateBehavior( m_agents, scratch->get(), dt, *this, system, (*dronesInRange)[groupIndex]);
			groupIndex++;
			for ( auto force = forces.begin(); force != forces.end(); ++force )
			{
				m_forces.push_back( *force );
			}
		}
	}
	else
	{
		auto scratch = m_scratchData.begin();
		for( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior, scratch++ )
		{
			( *behavior )->CalculateBehavior( m_agents, scratch->get(), dt, *this, system, (*dronesInRange)[ groupIndex ]);

			// XXX TEMP DEBUG XXX breaks Mac build but is good to catch unchecked behaviors
			/*if ( _isnan( m_agents[0].acceleration[ 0 ] ) )
			{
				m_agents[ 0 ].velocity += Vector3( 0, 0, 0 );
			}*/

			groupIndex++;
		}
	}

	//Move the agents based on the behaviors
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		agent->lifetime += dt;

		static const Vector3 zAxis( 0.f, 0.f, 1.f );
		Vector3 test = agent->velocity - agent->acceleration;
		
		// only apply force if boosters are facing the correct direction
		//float angle = Dot( Normalize( Vector3( agent->acceleration ) ), Normalize( agent->target ) );
		
		//if( angle > 0.7 )
		//{
		//	angle = ( angle - 0.7f) * 10.f / 3.f;
		//	// TODO Here we can enable booster effect 
		//	agent->velocity += agent->acceleration * angle * angle;
		//} 
		// Vector3 facingDir =  agent->acceleration;

		
		agent->velocity += agent->acceleration;
		Vector3 facingDir = agent->velocity;
		TriQuaternionRotationArc( &agent->rotation, &zAxis, &facingDir );

		agent->velocity = ClampLength( agent->velocity, m_maxVelocity );
		agent->position = agent->position + agent->velocity * dt;
		agent->acceleration = Vector3( 0, 0, 0 );
		agent->target = Vector3( 0, 0, 0 );
	}
	m_tree->UpdateTree( dt );
}

void BehaviorGroup::UpdateVisibility( const TriFrustum & frustum, const Matrix & parentTransform )
{
	// Check if an agent is visible and calculate the xfade value
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if( frustum.IsSphereVisible( agent->position, m_boundingSphereRadius ) )
		{
			float pixelSize = frustum.GetPixelSizeAccross( agent->position, m_boundingSphereRadius );
			agent->screenSize = pixelSize * m_scale[0]; // Store the screen size for each agent

			if( pixelSize >= m_blendScreenSizeMax )
			{
				agent->xfade = 0.0; // Render as mesh
			}
			else if( pixelSize <= m_blendScreenSizeMin )
			{
				agent->xfade = 1.0; // Render as sprite
			}
			else
			{
				float s = 1.0;
				agent->xfade = s - ( pixelSize - m_blendScreenSizeMin ) / ( m_blendScreenSizeMax - m_blendScreenSizeMin );
			}
			agent->isVisible = true;
		}
		else
		{
			agent->isVisible = false;
		}
	}

	this->UpdateCurrentScreenSize();
}

void BehaviorGroup::UpdateCurrentScreenSize()
{
	// Update the current screensize read-only attribute in Jessica using the first agent.
	if ( !m_agents.empty() )
	{
		m_currentScreenSize = m_agents.begin()->screenSize;
	}
}

bool BehaviorGroup::IsGroupVisible()
{
	bool isAnyAgentVisible = false;

	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if ( agent->screenSize >= m_renderThreshold ) {
			isAnyAgentVisible = true;
			break;
		}
	}
	return isAnyAgentVisible;
}

void BehaviorGroup::GetInfoForBuffer( uint8_t* data, Matrix& parentWorldLocation )
{
	for( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		float LOD = ( *agent ).xfade;
		float LODmod;
		if( LOD != 1 && agent->isVisible )
		{
			LODmod = ( 1 - LOD ) *( 0.5f + ( 1 - LOD ) * 0.5f );
			Vector3 meshScale = m_scale * Vector3( LODmod, LODmod, LODmod );

			if( m_meshToggle )
			{
				meshScale *= m_spriteScale;
			}

			Matrix m = Transpose( TransformationMatrix( meshScale, agent->rotation, agent->position ) );
			memcpy( data, &m, 12 * sizeof( float ) );
		}
		else
		{
			Matrix m = Transpose( TransformationMatrix( Vector3( 0, 0, 0 ), Quaternion( 0, 0, 0, 0 ), Vector3( 0, 0, 0 ) ) );
			memcpy( data, &m, 12 * sizeof( float ) );
		}

		data += 12 * sizeof( float );

		// sprite
		if( LOD != 0 && agent->isVisible )
		{
			LODmod = ( 1.0f - LOD ) * ( LOD * 0.3f ) + ( LOD * 1.0f );
			Matrix agentMatrix = TransformationMatrix( m_scale * m_spriteScale * Vector3( LODmod, LODmod, LODmod ),
				agent->rotation, agent->position );
			agentMatrix = Billboard2D( agentMatrix );
			Matrix m2 = Transpose( agentMatrix );
			memcpy( data, &m2, 12 * sizeof( float ) );
		}
		data += 12 * sizeof( float );
	}
}

void BehaviorGroup::CreateSpriteVertexDeclaration()
{
	Tr2MeshPtr meshPtr = GetSpriteMesh();

	if ( meshPtr )
	{
		if ( nullptr == meshPtr->GetGeometryResource() )
		{
			return;
		}

		if ( (meshPtr->GetGeometryResource())->IsGood() )
		{
			TriGeometryResMeshData* meshData = meshPtr->GetGeometryResource()->GetMeshData( meshPtr->GetMeshIndex() );
			if ( meshData->m_vertexDeclaration != m_cachedSVD )
			{
				Tr2VertexDefinition s_agentInstancedVertex;
				Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, s_agentInstancedVertex );

				Tr2VertexDefinition& def = s_agentInstancedVertex;
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 8, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 9, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 10, 1, 1 );

				// create vertex-declarartion
				m_spriteVertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_agentInstancedVertex );

				m_cachedSVD = meshData->m_vertexDeclaration;
			}
			return;
		}
	}
	m_cachedSVD = -1;
	m_spriteVertexDeclarationHandle = 0;
}

void BehaviorGroup::CreateVertexDeclaration()
{
	Tr2MeshPtr meshPtr = GetMesh();

	if ( meshPtr )
	{
		if ( nullptr == meshPtr->GetGeometryResource() )
		{
			return;
		}

		if ( (meshPtr->GetGeometryResource())->IsGood() )
		{
			TriGeometryResMeshData* meshData = meshPtr->GetGeometryResource()->GetMeshData( meshPtr->GetMeshIndex() );
			if ( meshData->m_vertexDeclaration != m_cachedVD )
			{
				Tr2VertexDefinition s_agentInstancedVertex;
				Tr2EffectStateManager::GetVertexDeclarationElements( meshData->m_vertexDeclaration, s_agentInstancedVertex );

				Tr2VertexDefinition& def = s_agentInstancedVertex;
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 8, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 9, 1, 1 );
				def.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 10, 1, 1 );

				// create vertex-declarartion
				m_vertexDeclarationHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_agentInstancedVertex );

				m_cachedVD = meshData->m_vertexDeclaration;
			}
			return;
		}
	}
	m_cachedVD = -1;
	m_vertexDeclarationHandle = 0;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void BehaviorGroup::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "AgentsKDTree" );
	options.insert( "Volumes" );
	options.insert( "ExclusionVolumes" );
	options.insert( "Bounding Sphere" );
	options.insert( "Locators" );
	options.insert( "DebugBehaviors" );
	options.insert( "BehaviorVisionRanges" );
	options.insert( "Wander" );
	options.insert( "LocatorRadius" );
}

float BehaviorGroup::GetBoundingSphereRadius()
{
	return m_boundingSphereRadius;
}


void BehaviorGroup::RenderDebugInfo( Tr2DebugRenderer& renderer, Matrix& parentWorldLocation )
{
	for ( auto group = m_behaviors.begin(); group != m_behaviors.end(); ++group )
	{
		( *group )->RenderDebugInfo( renderer, m_agents, parentWorldLocation );
	}

	if ( renderer.HasOption( this, "AgentsKDTree" ) )
	{
		if ( m_tree != nullptr )
		{
			m_tree->RenderDebugInfo( renderer, parentWorldLocation );
		}
		else
		{
			CreateAgentTree();
		}
	}

	if ( renderer.HasOption( this, "Volumes" ) )
	{
		for ( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
		{
			( *volume )->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}

	if( m_TEMPDEBUGVECTORTOFINDCLOSEDRONES != Vector3(0,0,0)) // TODO remove, gona leave it here for a while to debug interaction with BHgroups
	{
		DroneAgent* p = m_tree->FindClosestAgent( m_TEMPDEBUGVECTORTOFINDCLOSEDRONES );
		if( p != nullptr )
			renderer.DrawSphere( this, TranslationMatrix( ( p->position ) ) * parentWorldLocation,
													72, 6, Tr2DebugRenderer::Wireframe, 0xffcc11ff );
	}

	if ( renderer.HasOption( this, "ExclusionVolumes" ) )
	{
		for ( auto volume = m_exclusionVolumes.begin(); volume != m_exclusionVolumes.end(); ++volume )
		{
			(*volume)->RenderDebugInfo( renderer, parentWorldLocation );
		}
	}

	if (renderer.HasOption( this, "Bounding Sphere" ))
	{
		for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent) 
		{
			renderer.DrawSphere( this, TranslationMatrix( agent->position ) * parentWorldLocation, m_boundingSphereRadius, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
		}
	}

	if ( renderer.HasOption( this, "DebugBehaviors" ) )
	{
		if( m_collectForces )
		{
			bool loopToggle = true;
			Vector3 point;
			for ( auto force = m_forces.begin(); force != m_forces.end(); ++force )
			{
				if(loopToggle)
				{
					point = ( *force ) + parentWorldLocation.GetTranslation();
					loopToggle = false;
				}
				else
				{
					float lengthLerp = min( 1.f, max( 0.f, Length( *force ) / m_maxVelocity ) );
					renderer.DrawLine(this, point, point + (*force),
					                  Lerp(Color(0xff11ff11), Color(0xffff1111), lengthLerp));
					Color(0xff11ff11);
					loopToggle = true;
				}
			}
		}
		else
		{
			m_collectForces = true;
		}
	}
	else
	{
		m_collectForces = false;
	}
}

