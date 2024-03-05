#include "StdAfx.h"
#include "BackAndForth.h"
#include "include/TriMath.h"

const BlueSharedString DELIVER_LOCATOR_SET_NAME( "deliver" );
const BlueSharedString SEEK_LOCATOR_SET_NAME( "seek" );

BackAndForth::BackAndForth( IRoot* lockobj ) :
	PARENTLOCK( m_locatorSets ),
	m_enabled( true ),
	m_arrivedRadius( 50.f ),
	m_distFromOrigin( 20.f ),
	m_slowDownRadius( 200.f ),
	m_backAndForthWeight( 100.f ),
	m_seconds( 0.25f ),
	m_fxBehavior( nullptr ),
	m_locatorType( LOCAL_LOCATORS ),
	m_priority( LEAST_PRIORITY ),
	m_locatorSetName( "damage" ),
	m_target( nullptr ),
	m_parent( nullptr )
{
}

BackAndForth::~BackAndForth()
{
}

int BackAndForth::GetProcessPriority()
{
	return m_priority;
}

std::string BackAndForth::GetBehaviorName()
{
	return "BackAndForth";
}

size_t BackAndForth::GetScratchMemorySize() const
{
	return sizeof( BackAndForthData );
}

void BackAndForth::InitializeScratch( void* scratchMemory )
{
	*static_cast<BackAndForthData*>( scratchMemory ) = BackAndForthData();
}

std::vector<Vector3> BackAndForth::CalculateBehavior( std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime, BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	std::vector<Vector3> todo;
	if( !m_enabled )
	{
		return todo;
	}

	if( m_fxBehavior == nullptr )
	{
		m_fxBehavior = group.GetBehaviorByName( "PlayFX" );
	}

	auto data = static_cast<BackAndForthData*>( scratchData );

	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		if( m_locatorType == LOCAL_LOCATORS )
		{
			if( data->arrived )
			{
				if( data->seek )
				{
					//Get count of locators under the "seek" locatorSet
					auto seekLocators = GetLocatorsForSet( SEEK_LOCATOR_SET_NAME );
					if( seekLocators != NULL && seekLocators[0].size() > 0 )
					{
						m_rand = TriRandInt( 0, (int)seekLocators->size() );
						data->locatorTarget = seekLocators[0][m_rand].position;
						data->locatorDirection = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), seekLocators[0][m_rand].direction );
					}
				}
				//If the deliver behavior is active
				else if( data->deliver )
				{
					//Get count locators under the "deliver" locatorSet
					auto deliverLocators = GetLocatorsForSet( DELIVER_LOCATOR_SET_NAME );
					if( deliverLocators != NULL && deliverLocators[0].size() > 0 )
					{
						m_rand = TriRandInt( 0, (int)deliverLocators->size() );
						data->locatorTarget = deliverLocators[0][m_rand].position;
						data->locatorDirection = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), deliverLocators[0][m_rand].direction );
					}
				}
				data->arrived = false;
			}
		}
		else if( m_locatorType == PARENT_LOCATORS )
		{
			if( m_parent == nullptr )
			{
				m_parent = group.GetParent();
			}

			if( data->arrived && m_parent )
			{
				// Pick a new locator to go to
				unsigned int count = m_parent->GetLocatorCount( m_locatorSetName );
				int rand = TriRandInt( count );
				data->locatorIndex = rand;
			}

			GetParentLocatorPosition( data->locatorIndex, &data->locatorTarget, &data->locatorDirection );
			data->arrived = false;
		}
		else if( m_locatorType == TARGET_LOCATORS )
		{
			if( data->arrived && m_target )
			{
				// Pick a new locator to go to
				unsigned int count = m_target->GetLocatorCount( m_locatorSetName );
				int rand = TriRandInt( count );
				data->locatorIndex = rand;
			}

			GetTargetLocatorPosition( data->locatorIndex, &data->locatorTarget, &data->locatorDirection );
			data->arrived = false;
		}

		agent->target = data->locatorTarget;

		// If the direction is (0,0,0) it's pointing up but then the slowDown radius won't work
		if( data->locatorDirection == Vector3( 0.0, 0.0, 0.0 ) )
		{
			data->locatorDirection = Vector3( 0.f, 1.f, 0.f );
		}

		Vector3 targetPoint = data->locatorDirection;
		targetPoint = Normalize( targetPoint );
		targetPoint *= m_distFromOrigin;
		targetPoint += data->locatorTarget;

		// For debugging
		m_arrivalPoint = targetPoint;

		Matrix worldTransform = system.GetWorldTransform();
		Vector3 agentPositionWS = XMVector3TransformCoord( agent->position, worldTransform );

		Vector3 desiredVelocity = targetPoint - agentPositionWS;
		float distance = Length( desiredVelocity );
		desiredVelocity = Normalize( desiredVelocity );
		static const Vector3 zAxis( 0.f, 0.f, 1.f );

		//If we are approaching the target
		if( distance < m_slowDownRadius )
		{
			// make the agent slow down before arriving at target
			desiredVelocity = desiredVelocity * m_backAndForthWeight * ( distance / m_slowDownRadius );

			// Set the rotation of the drone
			Quaternion newRotation;
			auto invDir = Normalize( data->locatorTarget - agentPositionWS );
			TriQuaternionRotationArc( &newRotation, &zAxis, &invDir );
			agent->rotation = newRotation;
			data->timePassed = 0.f;

			// Start playing fx when slowing down
			if( !agent->playFX && m_fxBehavior != nullptr )
			{
				agent->fxStartTime = BeOS->GetActualTime();
				agent->playFX = true;
			}

			//If the agent has arrived to the target, then switch targets
			if( distance < m_arrivedRadius )
			{
				std::swap( data->seek, data->deliver );
				data->arrived = true;
			}
		}
		else
		{
			// Have the drone slowly start moving based on time passed
			data->timePassed += deltaTime;
			data->timePassed = max( data->timePassed, m_seconds );
			desiredVelocity *= Lerp( 0, 1, max( data->timePassed, m_seconds ) / m_seconds );
		}
		agent->acceleration += desiredVelocity - agent->velocity;
	}
	
	return todo;
}

void BackAndForth::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Locators" );
	options.insert( "Locator Radius" );
}

void BackAndForth::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation )
{
	if( renderer.HasOption( this, "Locators" ) )
	{
		float boundingSphereRadius = 100.f;
		float modelScale = 10;
		if( m_locatorType == LOCAL_LOCATORS )
		{
			for( auto it = m_locatorSets.begin(); it != m_locatorSets.end(); ++it )
			{
				const LocatorStructureList& locators = ( *( *it )->GetLocators() );
				for( size_t i = 0; i < locators.size(); ++i )
				{
					auto& locator = locators[i];
					auto position = locator.position;
					auto rotation = locator.direction;
					uint32_t color = 0x990088ff;

					renderer.DrawSphereArrow(
						Tr2DebugObjectReference( &locators, uint32_t( i ) ),
						Vector3( XMVector3TransformCoord( position, parentWorldLocation ) ),
						Vector3( XMVector3TransformNormal( Vector3( 0, 1, 0 ), Matrix( XMMatrixRotationQuaternion( rotation ) ) * parentWorldLocation ) ),
						boundingSphereRadius * modelScale / 50.f,
						8,
						Tr2DebugRenderer::Lit,
						color );
				}
			}
		}
		else if( m_locatorType == PARENT_LOCATORS && m_parent )
		{
			const LocatorStructureList& locatorList = *m_parent->GetLocatorsForSet( m_locatorSetName );
			for( size_t i = 0; i < locatorList.size(); ++i )
			{
				auto locator = locatorList[i];
				auto position = locator.position;
				auto rotation = locator.direction;
				uint32_t color = 0x990088ff;

				renderer.DrawSphereArrow(
					Tr2DebugObjectReference( &locatorList, uint32_t( i ) ),
					Vector3( XMVector3TransformCoord( position, parentWorldLocation ) ),
					Vector3( XMVector3TransformNormal( Vector3( 0, 1, 0 ), Matrix( XMMatrixRotationQuaternion( rotation ) ) * parentWorldLocation ) ),
					boundingSphereRadius * modelScale / 50.f,
					8,
					Tr2DebugRenderer::Lit,
					color );
			}
		}
		else if( m_locatorType == TARGET_LOCATORS && m_target )
		{
			const LocatorStructureList& locatorList = *m_target->GetLocatorsForSet( m_locatorSetName );
			for( size_t i = 0; i < locatorList.size(); ++i )
			{
				auto locator = locatorList[i];
				auto position = locator.position;
				auto rotation = locator.direction;
				uint32_t color = 0x990088ff;

				renderer.DrawSphereArrow(
					Tr2DebugObjectReference( &locatorList, uint32_t( i ) ),
					Vector3( XMVector3TransformCoord( position, parentWorldLocation ) ),
					Vector3( XMVector3TransformNormal( Vector3( 0, 1, 0 ), Matrix( XMMatrixRotationQuaternion( rotation ) ) * parentWorldLocation ) ),
					boundingSphereRadius * modelScale / 50.f,
					8,
					Tr2DebugRenderer::Lit,
					color );
			}
		}
	}

	if( renderer.HasOption( this, "Locator Radius" ) )
	{
		if( m_locatorType == LOCAL_LOCATORS )
		{
			renderer.DrawSphere( this, m_arrivalPoint, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
			renderer.DrawSphere( this, m_arrivalPoint, 5, 8, Tr2DebugRenderer::Wireframe, 0xff0000ff );
			renderer.DrawSphere( this, m_arrivalPoint, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xff86d2fd );
		}
		else if( m_locatorType == PARENT_LOCATORS && m_parent )
		{
			unsigned int count = m_parent->GetLocatorCount( m_locatorSetName );
			for( unsigned int i = 0; i < count; ++i )
			{
				Vector3 pos( 0, 0, 0 );
				Vector3 dir( 0, 0, 0 );
				m_parent->GetLocatorPosition( &pos, i, true, m_locatorSetName );
				m_parent->GetLocatorDirection( &dir, i, true, m_locatorSetName );
				Vector3 targetPoint = dir;
				targetPoint = Normalize( targetPoint );
				targetPoint *= m_arrivedRadius;
				targetPoint += pos;
				renderer.DrawSphere( this, targetPoint, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
				renderer.DrawSphere( this, targetPoint, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xff86d2fd );
			}
		}
		else if( m_locatorType == TARGET_LOCATORS && m_target )
		{
			unsigned int count = m_target->GetLocatorCount( m_locatorSetName );
			for( unsigned int i = 0; i < count; ++i )
			{
				Vector3 pos( 0, 0, 0 );
				Vector3 dir( 0, 0, 0 );
				m_target->GetLocatorPosition( &pos, i, true, m_locatorSetName );
				m_target->GetLocatorDirection( &dir, i, true, m_locatorSetName );
				Vector3 targetPoint = dir;
				targetPoint = Normalize( targetPoint );
				targetPoint *= m_arrivedRadius;
				targetPoint += pos;
				renderer.DrawSphere( this, targetPoint, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
				renderer.DrawSphere( this, targetPoint, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xff86d2fd );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Try to find the specified locator set and return a pointer to it
// --------------------------------------------------------------------------------
const LocatorStructureList* BackAndForth::GetLocatorsForSet( const BlueSharedString& setName ) const
{
	for( auto it = m_locatorSets.cbegin(); it != m_locatorSets.cend(); ++it )
	{
		if( ( *it )->HasName( setName ) )
		{
			return ( *it )->GetLocators();
		}
	}
	return nullptr;
}

void BackAndForth::AddLocatorSet()
{
	EveLocatorSetsPtr seekSet;
	seekSet.CreateInstance();
	seekSet->SetName( SEEK_LOCATOR_SET_NAME );

	EveLocatorSetsPtr deliverSet;
	deliverSet.CreateInstance();
	deliverSet->SetName( DELIVER_LOCATOR_SET_NAME );

	m_locatorSets.Append( seekSet );
	m_locatorSets.Append( deliverSet );
}

void BackAndForth::SetParent( IEveSpaceObject2* parent )
{
	if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( parent ) )
	{
		m_parent = spaceObject;
	}
}

void BackAndForth::GetParentLocatorPosition( int locatorIndex, Vector3* locatorPosition, Vector3* locatorDirection )
{
	if( m_parent != nullptr )
	{
		m_parent->GetLocatorPosition( locatorPosition, locatorIndex, true, m_locatorSetName );
		m_parent->GetLocatorDirection( locatorDirection, locatorIndex, true, m_locatorSetName );
	}
}

void BackAndForth::GetTargetLocatorPosition( int locatorIndex, Vector3* locatorPosition, Vector3* locatorDirection )
{
	if( m_target != nullptr )
	{
		m_target->GetLocatorPosition( locatorPosition, locatorIndex, true, m_locatorSetName );
		m_target->GetLocatorDirection( locatorDirection, locatorIndex, true, m_locatorSetName );
	}
}