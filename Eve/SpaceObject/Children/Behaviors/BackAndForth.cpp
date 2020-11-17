#include "StdAfx.h"
#include "BackAndForth.h"
#include "include/TriMath.h"

const BlueSharedString DELIVER_LOCATOR_SET_NAME( "deliver" );
const BlueSharedString SEEK_LOCATOR_SET_NAME( "seek" );

BackAndForth::BackAndForth( IRoot* lockobj ) :
	PARENTLOCK( m_locatorSets ),
	m_arrivedRadius( 50.f ),
	m_slowDownRadius( 200.f ),
	m_backAndForthWeight( 100.f ),
	m_seconds( 0.25f ),
	m_fxBehavior( nullptr )
{
}

BackAndForth::~BackAndForth()
{
}

int BackAndForth::GetProcessPriority()
{
	return LEAST_PRIORITY;
}

size_t BackAndForth::GetScratchMemorySize() const
{
	return sizeof( BackAndForthData );
}

void BackAndForth::InitializeScratch( void* scratchMemory )
{
	*static_cast<BackAndForthData*>( scratchMemory ) = BackAndForthData();
}

std::vector<Vector3> BackAndForth::CalculateBehavior(std::vector<DroneAgent>& agents, void* scratchData, const float deltaTime,
                                                     BehaviorGroup& group, EveChildBehaviorSystem& system, const std::vector<std::vector<DroneAgent*>>& dronesInSearchRadius)
{	
	if( m_fxBehavior == nullptr )
	{
		m_fxBehavior = group.GetBehaviorByName( "PlayFX" );
	}

	auto data = static_cast<BackAndForthData*>( scratchData );

	for( auto agent = agents.begin(); agent != agents.end(); ++agent, ++data )
	{
		if( data->arrived )
		{
			if( data->seek )
			{
				//Get count of locators under the "seek" locatorSet
				auto seekLocators = GetLocatorsForSet( SEEK_LOCATOR_SET_NAME );
				if (seekLocators != NULL && seekLocators[0].size() > 0)
				{
					m_rand = TriRandInt( 0, (int)seekLocators->size() );
					data->locatorTarget = seekLocators[0][m_rand].position;
					data->locatorDirection = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), seekLocators[0][m_rand].direction );
				}
			}
			//If the deliver behavior is active
			else if (data->deliver)
			{
				//Get count locators under the "deliver" locatorSet
				auto deliverLocators = GetLocatorsForSet( DELIVER_LOCATOR_SET_NAME );
				if (deliverLocators != NULL && deliverLocators[0].size() > 0 )
				{
					m_rand = TriRandInt( 0, (int)deliverLocators->size() );
					data->locatorTarget = deliverLocators[0][m_rand].position;
					data->locatorDirection = (Vector3)XMVector3Rotate( Vector3( 0.f, 1.f, 0.f ), deliverLocators[0][m_rand].direction );
				}
			}
			data->arrived = false;
		}

		agent->target = data->locatorTarget;

		Vector3 targetPoint = data->locatorDirection;
		targetPoint = Normalize( targetPoint );
		targetPoint *= m_arrivedRadius;
		targetPoint += data->locatorTarget;

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
	std::vector<Vector3> todo;
	return todo;
}

void BackAndForth::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Locators" );
	options.insert( "LocatorRadius" );
}

void BackAndForth::RenderDebugInfo( ITr2DebugRenderer2& renderer, std::vector<DroneAgent>& agents, Matrix& parentWorldLocation)
{
	if( renderer.HasOption( this, "Locators" ) )
	{
		float boundingSphereRadius = 100.f;
		float modelScale = 10;
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

	if( renderer.HasOption( this, "LocatorRadius" ) )
	{
		for( auto it = m_locatorSets.begin(); it != m_locatorSets.end(); ++it )
		{
			const LocatorStructureList& locators = ( *( *it )->GetLocators() );
			for( size_t i = 0; i < locators.size(); ++i )
			{
				auto& locator = locators[i];
				auto position = locator.position;
				renderer.DrawSphere( this, position, m_arrivedRadius, 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
				renderer.DrawSphere( this, position, m_slowDownRadius, 8, Tr2DebugRenderer::Wireframe, 0xff86d2fd );
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
	seekSet->Set( "seek", NULL, 0 );

	EveLocatorSetsPtr deliverSet;
	deliverSet.CreateInstance();
	deliverSet->Set( "deliver", NULL, 0 );

	m_locatorSets.Append( seekSet );
	m_locatorSets.Append( deliverSet );
	
}