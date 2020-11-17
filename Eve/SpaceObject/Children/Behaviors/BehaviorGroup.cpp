#include "StdAfx.h"
#include "BehaviorGroup.h"
#include "BehaviorGroupBooster.h"
#include "Tr2LightManager.h"
#include "include/TriMath.h"
#include "IBehavior.h"
#include "Tr2InstancedMesh.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierTransformCommon.h" 
#include "Resources/TriGeometryRes.h"
#include "PlayFX.h"


BehaviorGroup::BehaviorGroup( IRoot* lockobj ) :
	PARENTLOCK( m_behaviors ),
	m_vertexDeclarationHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_cachedVD( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_count( 0 ),
	m_display( true ),
	m_maxVelocity( 100 ),
	m_changeBufferVertexCount( nullptr ),
	m_scale( 1.0 ),
	m_currentScreenSize( 0.0 ),
	m_renderThreshold( 1.0 ),
	m_blendScreenSizeMin( 5.0 ),
	m_blendScreenSizeMax( 15.0 ),
	m_boundingSphereRadius( 5.f ),
	m_spawnPosition( 0.f, 0.f, 0.f ),
	m_debugMode( false ),
	m_debugLodLevel( 0.f ),
	m_debugIntensity( 0.f )
{
	m_behaviors.SetNotify( this );
	m_tree = nullptr;
	m_tree.CreateInstance();
}

bool BehaviorGroup::Initialize()
{
	m_scratchData.resize( m_behaviors.size() );
	CreateVertexDeclaration();
	
	if( m_booster )
	{
		m_booster->RebuildFlareBuffer( m_count );
	}
	return true;
}

bool BehaviorGroup::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_mesh ) )
	{
		CreateVertexDeclaration();
	}
	if( IsMatch( value, m_booster ) && m_booster != nullptr )
	{
		m_booster->RebuildFlareBuffer( m_count );
	}

	return true;
}

BehaviorGroup::~BehaviorGroup()
{
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
						m_behaviors[key]->InitializeScratch( scratchData.get() + size * i );
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
	InitializeGeometryResource();
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

	SortBehaviorIndexes();

	const int t = m_count;
	m_count = 0;
	SetCount( t );
}

void BehaviorGroup::SetVertexFunctionReferance( const std::function<void( void )> &F )
{
	m_changeBufferVertexCount = F;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return how many agents have been created
// --------------------------------------------------------------------------------------
size_t BehaviorGroup::GetSize()
{
	return m_agents.size();
}

// --------------------------------------------------------------------------------------
// Description:
//   Return local counter of agent count
// --------------------------------------------------------------------------------------
unsigned int BehaviorGroup::GetCount()
{
	return unsigned( m_count );
}

// --------------------------------------------------------------------------------------
// Description:
//   Create a local KD tree
// --------------------------------------------------------------------------------------
void BehaviorGroup::CreateAgentTree()
{
	m_tree = nullptr;
	if ( !m_tree.CreateInstance() )
	{
		return;
	}
	m_tree->CreateTree( m_agents, m_behaviors.size());
}

// --------------------------------------------------------------------------------------
// Description:
//   Loop through behaviors and returns the one that matches the name
// Argument:
//	name - name of the behavior you want to find
// Returns:
//	The behavior object if it found a match, if not return a nullptr
// --------------------------------------------------------------------------------------
IBehavior* BehaviorGroup::GetBehaviorByName( const std::string& name )
{
	for( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior )
	{
		if( name == ( *behavior )->GetBehaviorName() )
		{
			return *behavior;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Loop through elements in ProcessPriority enum and re-arranges behavior indices 
//   based on priority
// Returns:
//	A vector of sorted indices
// --------------------------------------------------------------------------------------
void BehaviorGroup::SortBehaviorIndexes()
{
	m_sortedBehaviorIndexes.clear();

	for ( int i = 0; i < IBehavior::ProcessPriority::COUNT; ++i )
	{
		int p = 0;
		for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior, ++p )
		{
			if ( ( *behavior )->GetProcessPriority() == i )
			{
				m_sortedBehaviorIndexes.push_back( p );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the instanced mesh
// --------------------------------------------------------------------------------------
Tr2MeshPtr BehaviorGroup::GetMesh() const
{
	return m_mesh;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the max velocity set for agents in this behaviorGroup
// --------------------------------------------------------------------------------------
float BehaviorGroup::GetMaxVelocity() const
{
	return m_maxVelocity;
}

// --------------------------------------------------------------------------------------
// Description:
//   Return the vertex declaration handle for the BehaviorGroup instanced mesh 
// --------------------------------------------------------------------------------------
unsigned int BehaviorGroup::GetVertexDeclarationHandle() const
{
	return m_vertexDeclarationHandle;
}

// --------------------------------------------------------------------------------------
// Description:
//   Set the group index, used by BehaviorSystem
// --------------------------------------------------------------------------------------
void BehaviorGroup::SetGroupIndexIndicator( int index )
{
	m_groupIndex = index;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get the group index, used by BehaviorSystem
// --------------------------------------------------------------------------------------
int BehaviorGroup::GetGroupIndexIndicator() const
{
	return m_groupIndex;
}

// --------------------------------------------------------------------------------------
// Description:
//   Exposed function used to add an agent, calls the private function which does all the work
// --------------------------------------------------------------------------------------
void BehaviorGroup::AddAgent()
{
	// The function without arguments to be called from the UI
	AddAgentPrivate();
	
	OnAgentCountChanged();
}

// --------------------------------------------------------------------------------------
// Description:
//   Create an agent and add it to the vector. Updates scratchdata for that new agent
// --------------------------------------------------------------------------------------
void BehaviorGroup::AddAgentPrivate()
{
 	DroneAgent agent;
	// This is because the process priority behavior can also affect where drones spawn
	if( m_spawnPosition != Vector3( 0.f, 0.f, 0.f ) )
	{
		agent.position = m_spawnPosition;
	}

	m_agents.push_back( agent );

	while( m_scratchData.size() < m_behaviors.size() )
	{
		m_scratchData.push_back( CcpMallocBuffer() );
	}

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[ m_sortedBehaviorIndexes[i] ]->GetScratchMemorySize();
		if( size > 0 )
		{
			m_scratchData[ m_sortedBehaviorIndexes[i] ].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
			m_behaviors[ m_sortedBehaviorIndexes[i] ]->InitializeScratch( m_scratchData[ m_sortedBehaviorIndexes[i] ].get() + size * ( m_agents.size() - 1 ) );
		}
	}

	m_count++;
}

// --------------------------------------------------------------------------------------
// Description:
//   Exposed function to set agent count. Based on if adding multiple agents or removing
//	 them, calls the appropriate function
// --------------------------------------------------------------------------------------
void BehaviorGroup::SetCount( int count )
{
	if ( count == m_count || count < 0 )
	{
		return;
	}

	if ( m_count < count )
	{
		AddAgentsByCount( count );
	}
	else
	{
		RemoveAgentsByCount( count );
	}
		
	m_count = count;
	OnAgentCountChanged();
}

void BehaviorGroup::OnAgentCountChanged()
{
	if( m_changeBufferVertexCount )
	{
		( m_changeBufferVertexCount )( );
	}
	CreateAgentTree();
	
	if( m_booster )
	{
		m_booster->RebuildFlareBuffer( m_count );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if all the agents have lodded out or not.
// Returns:
//	 1 if all agents are sprites, 0 if all agents are instanced meshes, -1 if they are
//	 not the same or neither (no agents)
// --------------------------------------------------------------------------------------
float BehaviorGroup::AllTheSame()
{
	float same = -1;
	// if none of the agents need either of the meshes we let the system know
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		if ( same == -1 ) same = (agent->xfade);
		if ( same != (agent->xfade) ) return -1;
	}
	return same;
}

// --------------------------------------------------------------------------------------
// Description:
//   Exposed function used to remove an agent, calls the private function which does all the work
// --------------------------------------------------------------------------------------
void BehaviorGroup::RemoveAgent()
{
	if( m_agents.size() == 0 )
	{
		return;
	}
	// Removes the last agent
	RemoveSpecificAgent( m_count - 1 );

	OnAgentCountChanged();
}

// --------------------------------------------------------------------------------------
// Description:
//   Remove the last agent from the vector and update scratch data.
// --------------------------------------------------------------------------------------
void BehaviorGroup::RemoveSpecificAgent( int index )
{
	m_agents[index] = m_agents.back();
	m_agents.pop_back();

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[m_sortedBehaviorIndexes[i]]->GetScratchMemorySize();
		if( size == 0 )
		{
			continue;
		}

		memcpy( m_scratchData[m_sortedBehaviorIndexes[i]].get() + size * index, m_scratchData[m_sortedBehaviorIndexes[i]].get() + size * m_agents.size(), size );
		m_scratchData[m_sortedBehaviorIndexes[i]].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
	}

	m_count--;
}

// --------------------------------------------------------------------------------------
// Description:
//   Set agent count to size. Update scratch data for how many agents were added
// Argument:
//	 size: The new size of agent vector
// --------------------------------------------------------------------------------------
void BehaviorGroup::AddAgentsByCount( int count )
{
	size_t sizeBeforeResize = m_agents.size();
	m_agents.resize( count );

	// This is because the process priority behavior can also affect where drones spawn
	if( m_spawnPosition != Vector3( 0.f, 0.f, 0.f ) )
	{
		for( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
		{
			agent->position = m_spawnPosition;
		}
	}

	while( m_scratchData.size() < m_behaviors.size() )
	{
		m_scratchData.push_back( CcpMallocBuffer() );
	}

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
 		auto size = m_behaviors[m_sortedBehaviorIndexes[i]]->GetScratchMemorySize();
		if( size > 0 )
		{
			m_scratchData[m_sortedBehaviorIndexes[i]].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
			for( size_t j = sizeBeforeResize; j < m_agents.size(); j++ )
			{
				m_behaviors[m_sortedBehaviorIndexes[i]]->InitializeScratch( m_scratchData[m_sortedBehaviorIndexes[i]].get() + size * ( j ) );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Set agent count to size. Update scratch data for how many agents were removed
// Argument:
//	 size: The new size of agent vector
// --------------------------------------------------------------------------------------
void BehaviorGroup::RemoveAgentsByCount( int count )
{
	m_agents.resize( count );

	for( size_t i = 0; i < m_behaviors.size(); ++i )
	{
		auto size = m_behaviors[m_sortedBehaviorIndexes[i]]->GetScratchMemorySize();
		if( size == 0 )
		{
			continue;
		}

		m_scratchData[m_sortedBehaviorIndexes[i]].resize( "BehaviorGroup::m_scratchData", m_agents.size() * size );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Update agents forces based on behaviors calculations.
// Arguments:
//	 dt: delta time, system: parent object
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateAgents(const float dt, EveChildBehaviorSystem& system )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if ( m_agents.empty() )
	{
		return;
	}

	std::vector<float> ranges;

	for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior )
	{
		float searchRad = ( *behavior )->GetBehaviorSearchRadius();
		if ( searchRad == -1.f )
		{
			ranges.push_back( -1.f );
		}
		else
		{
			ranges.push_back( searchRad + m_boundingSphereRadius * m_scale );
		}
	}
	
	auto bs = m_boundingSphereRadius * m_scale;
	const std::vector<std::vector<std::vector<DroneAgent*>>>* dronesInRange = m_tree->FindDronesInRange( m_agents, ranges, bs );

	//Calculate the behaviors
	if( m_collectForces )
	{
		m_forces.clear();
		auto scratch = m_scratchData.begin();

		for ( int i = 0; i < static_cast< int >(m_behaviors.size()); ++i)
		{
			std::vector<Vector3> forces = m_behaviors[ m_sortedBehaviorIndexes[i] ]->CalculateBehavior( m_agents, (scratch + m_sortedBehaviorIndexes[i])->get(), dt, *this, system, (*dronesInRange)[ m_sortedBehaviorIndexes[i] ]);
			for ( auto force = forces.begin(); force != forces.end(); ++force )
			{
				m_forces.push_back( *force );
			}
		}
	}
	else
	{
		auto scratch = m_scratchData.begin();
		for ( int i = 0; i < static_cast< int >( m_behaviors.size()); ++i )
		{
			m_behaviors[ m_sortedBehaviorIndexes[ i ] ]->CalculateBehavior( m_agents, ( scratch + m_sortedBehaviorIndexes[ i ] )->get(), dt, *this, system, ( *dronesInRange )[ m_sortedBehaviorIndexes[i] ] );
		}
	}

	// Move the agents based on the behaviors
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		agent->lifetime += dt;

		static const Vector3 zAxis( 0.f, 0.f, 1.f );
		Vector3 test = agent->velocity - agent->acceleration;
		
		agent->velocity += agent->acceleration;
		Vector3 facingDir = agent->velocity;
		Vector3 interestPoint = Vector3();
		TriVectorRotateQuaternion( &interestPoint, &zAxis, &agent->rotation );
		Vector3 actualFacingDir = Lerp( interestPoint, facingDir, LengthSq(agent->velocity) / max(1.0f, m_maxVelocity * m_maxVelocity) );
		TriQuaternionRotationArc( &agent->rotation, &zAxis, &actualFacingDir);
		agent->targetDirection = actualFacingDir;

		agent->velocity = ClampLength( agent->velocity, m_maxVelocity );
		agent->position = agent->position + agent->velocity * dt;
		agent->acceleration = Vector3( 0, 0, 0 );
	}

	// later on we could have the updateTree input dynamically adjust based on dt
	// one of my ideas was input = max( const - dt , minimumUpdateFreq )
	// this would make it update less often on big dt-s
	m_tree->UpdateTree( 0.015 );
}

// --------------------------------------------------------------------------------------
// Description:
//   Check if each agent is still visible or not and update it's visibility based on that.
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateVisibility( const TriFrustum & frustum, const Matrix & worldTransform )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Check if an agent is visible and calculate the xfade value
	for ( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		auto agentPosInWorld = TransformCoord( agent->position, worldTransform );
		if( frustum.IsSphereVisible( agentPosInWorld, m_boundingSphereRadius * m_scale ) )
		{
			float pixelSize = frustum.GetPixelSizeAccross( agentPosInWorld, m_boundingSphereRadius * m_scale );
			agent->screenSize = pixelSize; // Store the screen size for each agent

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
			agent->isVisible = agent->screenSize >= m_renderThreshold;
		}
		else
		{
			agent->isVisible = false;
		}
	}

	this->UpdateCurrentScreenSize();

	m_frustum = frustum;
	m_parentTransform = worldTransform;
}

// --------------------------------------------------------------------------------------
// Description:
//   Update the current screensize read-only attribute in Jessica using the first agent.
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateCurrentScreenSize()
{
	if ( !m_agents.empty() )
	{
		m_currentScreenSize = m_agents.begin()->screenSize;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//  Check if any of the agents are visible.
// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
// Description:
//   Get LOD info for buffer. Called from BehaviorSystem
// --------------------------------------------------------------------------------------
void BehaviorGroup::GetInfoForBuffer( uint8_t* data, const Matrix& parentWorldLocation )
{	
	CCP_STATS_ZONE( __FUNCTION__ );
	m_lightInfo.clear();
	auto agentScale = Vector3( 1.0, 1.0, 1.0 );
	Matrix zeroMatrix = ScalingMatrix( Vector3( 0.0, 0.0, 0.0 ) );
	unsigned int agentIndex = 0;

	for( auto agent = m_agents.begin(); agent != m_agents.end(); ++agent )
	{
		float LOD = m_debugMode ? m_debugLodLevel : ( *agent ).xfade;
		Matrix agentTransform = TransformationMatrix( agentScale, agent->rotation, agent->position );
		if( agent->isVisible )
		{
			float LODmod = 0;
			Matrix meshData = Matrix( zeroMatrix );
			// agent mesh
			if( LOD < 0.75f )
			{
				LODmod = ( 1 - LOD ) * ( 0.5f + ( 1 - LOD ) * 0.5f );
				Vector3 meshScale = m_scale * Vector3( LODmod, LODmod, LODmod );
				meshData = Transpose( ScalingMatrix( meshScale ) * agentTransform );
			}
			memcpy( data, &meshData, 12 * sizeof( float ) );
			data += 12 * sizeof( float );

			// boosters
			Vector4 boosterPos = Vector4( 0, 0, 0, 0 );
			Quaternion boosterQuaternion = Quaternion( 0, 0, 0, 1 );
			Vector4 boosterInfo = Vector4( 0, 0, 0, 0 );
			if( m_booster != nullptr )
			{
				boosterPos.GetXYZ() = TransformCoord( m_booster->GetOffset() * LODmod, agentTransform );
				boosterPos.w = 1.0f;
				boosterQuaternion = agent->rotation;
				
				float intensity = m_debugMode ? m_debugIntensity : Length( agent->velocity ) / max( 1.0f, m_maxVelocity );

				if( LOD < 0.3 )
				{
					boosterPos.GetXYZ() = TransformCoord( m_booster->GetOffset(), agentTransform );
					boosterPos.w = 1.0f;
					boosterQuaternion = agent->rotation;
					srand( agent->id );
					boosterInfo.x = intensity;
					boosterInfo.y = ( float )rand() / ( float )RAND_MAX; // random stuff
					boosterInfo.z = float( m_booster->GetAtlasIndex0() );
					boosterInfo.w = float( m_booster->GetAtlasIndex1() );

					if( LOD < 0.25 )
					{
						float lightScale = intensity * ( 1.0f - 4.0f * LOD ) * ( 2.0f * LOD + 1.0f ); // early scale down for lights
						m_lightInfo[agentIndex] = Vector4( boosterPos.GetXYZ(), lightScale );
					}
				}
				auto m = agentTransform * parentWorldLocation;
				m_booster->AddFlare( m, LOD, intensity, agentIndex, m_boundingSphereRadius * m_scale );
			}

			memcpy( data, &boosterPos, 4 * sizeof( float ) );
			data += 4 * sizeof( float );
			memcpy( data, &boosterQuaternion, 4 * sizeof( float ) );
			data += 4 * sizeof( float );
			memcpy( data, &boosterInfo, 4 * sizeof( float ) );
			data += 4 * sizeof( float );
		}
		else
		{
			memcpy( data, &zeroMatrix, 12 * sizeof( float ) );
			data += 12 * sizeof( float );	

			// boosters
			memcpy( data, &zeroMatrix, 12 * sizeof( float ) );
			data += 12 * sizeof( float );
			if( m_booster != nullptr )
			{ 
				m_booster->AddFlare( IdentityMatrix(), 0, 0, agentIndex, m_boundingSphereRadius * m_scale );
			}
		}
		agentIndex++;
	}
}


// --------------------------------------------------------------------------------------
// Description:
//   Create instanced mesh vertex declaration
// --------------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------------
// Description:
//   For the effect in PlayFX to be rendered this is needed.
// --------------------------------------------------------------------------------------
void BehaviorGroup::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	auto behavior = GetBehaviorByName( "PlayFX" );

	if( behavior != nullptr )
	{
		auto tmp = dynamic_cast<PlayFX*>( behavior );
		if( tmp )
		{
			tmp->GetRenderables( renderables );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   For the effect in PlayFX to be updated this is needed.
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateAsyncronous( EveUpdateContext& updateContext )
{
	auto behavior = GetBehaviorByName( "PlayFX" );

	if( behavior != nullptr )
	{
		auto tmp = dynamic_cast<PlayFX*>( behavior );
		if( tmp )
		{
			tmp->UpdateAsyncronous( updateContext, m_frustum, m_parentTransform );
		}
	}
}


// --------------------------------------------------------------------------------------
// Description:
//   For the effect in PlayFX to be updated this is needed.
// --------------------------------------------------------------------------------------
void BehaviorGroup::UpdateSyncronous( EveUpdateContext& updateContext )
{
	auto behavior = GetBehaviorByName( "PlayFX" );

	if( behavior != nullptr )
	{
		auto tmp = dynamic_cast<PlayFX*>( behavior );
		if( tmp )
		{
			tmp->UpdateSyncronous( updateContext );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void BehaviorGroup::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "AgentsKDTree" );
	options.insert( "Bounding Sphere" );
	options.insert( "DebugBehaviors" );
	
        if( m_booster != nullptr )
	{
		options.insert( "Boosters" );
		options.insert( "Lights" );
	}

	for( auto group = m_behaviors.begin(); group != m_behaviors.end(); ++group )
	{
		( *group )->GetDebugOptions( options );
	}
}

float BehaviorGroup::GetBoundingSphereRadius()
{
	return m_boundingSphereRadius * m_scale;
}

EveKDdroneManagementTreePtr BehaviorGroup::GetKDTree()
{
	return m_tree;
}

BehaviorGroupBoosterPtr BehaviorGroup::GetBooster() const
{
	return m_booster;
}

void BehaviorGroup::AddLights( Tr2LightManager& lightManager, const Matrix& parentTransform )
{
	if( m_booster && m_booster->GetDisplay() )
	{
		for( auto it = m_lightInfo.begin(); it != m_lightInfo.end(); ++it )
		{
			Vector4 info = it->second;

			m_booster->AddLight( lightManager, info.GetXYZ(), info.w, it->first, parentTransform );			
		}
	}
}

void BehaviorGroup::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_booster ) 
	{
		m_booster->RegisterWithQuadRenderer( quadRenderer );
	}
}


void BehaviorGroup::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_booster && m_booster->GetDisplay() )
	{
		m_booster->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void BehaviorGroup::RenderDebugInfo( ITr2DebugRenderer2& renderer, Matrix& parentWorldLocation )
{
	for ( auto behavior = m_behaviors.begin(); behavior != m_behaviors.end(); ++behavior )
	{
		( *behavior )->RenderDebugInfo( renderer, m_agents, parentWorldLocation );
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

	bool drawBS = renderer.HasOption( this, "Bounding Sphere" );
	bool drawBoosters = renderer.HasOption( this, "Boosters" ) && m_booster != nullptr;

	if( drawBoosters || drawBS )
	{
		for (auto agent = m_agents.begin(); agent != m_agents.end(); ++agent) 
		{
			if( drawBS )
			{
				renderer.DrawSphere( this, TranslationMatrix( agent->position ) * parentWorldLocation, m_boundingSphereRadius * m_scale, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
			}
			if( drawBoosters )
			{
				m_booster->RenderBoosterDebug( renderer, this, TransformationMatrix(Vector3(1, 1, 1), agent->rotation, agent->position ) * parentWorldLocation );
			}
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


	if( renderer.HasOption( this, "Lights" ) )
	{
		auto iq = IdentityQuaternion();
		auto scale = Vector3( 1, 1, 1 );
		
		for( auto it = m_lightInfo.begin(); it != m_lightInfo.end(); ++it )
		{
			Vector4 info = it->second;
			m_booster->RenderLightDebug( renderer, this, TransformationMatrix( info.w * scale, iq, info.GetXYZ() ) * parentWorldLocation );
		}
	}
}

