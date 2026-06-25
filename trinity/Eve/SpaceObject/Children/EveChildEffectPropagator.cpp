// Copyright © 2020 CCP ehf.

#include "StdAfx.h"
#include "EveChildEffectPropagator.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2SphereShapeAttributeGenerator.h"
#include "include/TriMath.h"
#include "Curves/Tr2CurveScalar.h"
#include "EveChildInstanceContainer.h"

EveChildEffectPropagator::EveChildEffectPropagator( IRoot* lockobj ) :
	EveChildContainer( lockobj ),
	m_effectScaling( 1.0f, 1.0f, 1.0f ),
	m_triggerSphereOffset( 0.0f, 0.0f, 0.0f ),
	m_type( LOCAL_LOCATORS ),
	m_triggerMethod( TRIGGER_SPHERE_CURVE ),
	m_isPlaying( false ),
	m_playTime( 0 ),
	m_currentTriggerIndex( 0 ),
	m_numTriggers( 10 ),
	m_rndRange( 500 ),
	m_rndClosenessPreference( 0.25 ),
	m_rndMinRangeThreshold( 0 ),
	m_skipCleanup( false ),
	m_stopToClearDelay( 0 ),
	m_delayTimer( 0 ),
	m_replayAfterDelay( false ),
	m_triggerSphereScalarMulti( 1 ),
	m_completeness( 1 ),
	m_randScaleMin( 1 ),
	m_randScaleMax( 1 ),
	m_frequency( 1 ),
	m_numDeleted( 0 ),
	m_effectDuration( 3 ),
	m_stopAfterNumTriggers( -1.f ),
	m_trigger( false )
{
}

EveChildEffectPropagator::~EveChildEffectPropagator()
{
}


void EveChildEffectPropagator::RegisterComponents()
{
	if( IsInRegistry() && m_effect )
	{
		m_effect->Register( this->GetComponentRegistry() );
	}
}

void EveChildEffectPropagator::UnRegisterComponents()
{
	if( IsInRegistry() && m_effect )
	{
		m_effect->UnRegister( this->GetComponentRegistry() );
	}
}

bool EveChildEffectPropagator::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_effect ) )
	{
		if( m_effect != nullptr )
		{
			m_effect->DisableEditMode( true );
		}
	}

	if( IsMatch( value, m_completeness ) )
	{
		m_completeness = min( 1.f, max( 0.f, m_completeness ) );
	}

	if( IsMatch( value, m_randScaleMin ) )
	{
		m_randScaleMin = min( m_randScaleMax, max( 0.f, m_randScaleMin ) );
	}

	if( IsMatch( value, m_randScaleMax ) )
	{
		m_randScaleMax = max( m_randScaleMax, m_randScaleMin );
	}

	if( IsMatch( value, m_frequency ) )
	{
		if( m_frequency != 0.f )
		{
			m_playTime = float( m_currentTriggerIndex ) / m_frequency;
		}
	}

	return EveChildContainer::OnModified( value );
}

// --------------------------------------------------------------------------------------
// Description:
//   Starts effect playback
// --------------------------------------------------------------------------------------
void EveChildEffectPropagator::Play()
{
	Stop();

	if( !m_effect )
	{
		return;
	}

	m_trigger = false;
	m_isPlaying = true;
	m_delayTimer = m_stopToClearDelay;
}

// --------------------------------------------------------------------------------------
// Description:
//   Stops effect playback
// --------------------------------------------------------------------------------------
void EveChildEffectPropagator::Stop()
{
	m_isPlaying = false;
	m_playTime = 0;
	m_currentTriggerIndex = 0;
	m_numDeleted = 0;
	if( m_effect != nullptr )
	{
		m_effect->ClearInstanceList();
	}
}

bool EveChildEffectPropagator::Initialize()
{
	if( m_effect != nullptr )
	{
		m_effect->DisableEditMode( true );
	}
	return EveChildContainer::Initialize();
}

void EveChildEffectPropagator::ManageTriggers()
{
	if( m_triggerSphereRadiusCurve == nullptr )
	{
		return;
	}

	if( nullptr == m_effect )
	{
		return;
	}

	float currentRadSqr = m_triggerSphereRadiusCurve->GetValueAt( m_playTime ) * m_triggerSphereScalarMulti;
	currentRadSqr = currentRadSqr * currentRadSqr;

	for( auto it = m_processedTransforms.begin() + m_currentTriggerIndex; it != m_processedTransforms.end(); ++it )
	{
		if( it->sqrDistToSphereCenter < currentRadSqr )
		{
			m_effect->CreateInstance( it->scale, it->rotation, it->position );
			m_currentTriggerIndex++;
		}
		else
		{
			break;
		}
	}
}

// --------------------------------------------
// Description:
//   Implements IEveSpaceObjectChild interface.
// --------------------------------------------
void EveChildEffectPropagator::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_trigger )
	{
		ProcessLocators( params.spaceObjectParent );
		Play();

		if( m_triggerMethod == INTERVAL_TRIGGERS )
		{
			int size = (int)max( floor( m_effectDuration * m_frequency ), 0.f );
			std::vector<int> arr( size );
			for( int x = 0; x < size; ++x )
			{
				arr[x] = -1;
			}
			m_lastTriggered = arr;
		}
	}

	if( m_effect != nullptr )
	{
		m_effect->UpdateSyncronous( updateContext, params );
	}

	if( !m_isPlaying )
	{
		return;
	}

	switch( m_triggerMethod )
	{
	case TRIGGER_SPHERE_CURVE:
		UpdateTriggerCurve( updateContext );
		break;
	case INTERVAL_TRIGGERS:
		if( m_frequency != 0.f )
		{
			UpdateTriggerInterval( updateContext );
		}
		else
		{
			Stop();
		}
		break;
	case INSTANT_PERMANENT:
		m_playTime += updateContext.GetDeltaT();
		if( m_currentTriggerIndex == 0 )
		{
			for( auto it = m_processedTransforms.begin(); it != m_processedTransforms.end(); ++it )
			{
				m_effect->CreateInstance( it->scale, it->rotation, it->position );
			}
			m_currentTriggerIndex++;
		}
		break;
	default:
		break;
	}
}

void EveChildEffectPropagator::UpdateTriggerCurve( const EveUpdateContext& updateContext )
{
	auto dt = updateContext.GetDeltaT();
	m_playTime += dt;

	if( nullptr != m_effect )
	{
		ManageTriggers();
	}

	if( nullptr == m_triggerSphereRadiusCurve )
	{
		Stop();
		return;
	}

	if( m_playTime > m_triggerSphereRadiusCurve->Length() )
	{
		if( m_skipCleanup )
		{
			return;
		}

		if( m_replayAfterDelay )
		{
			if( m_delayTimer > 0 )
			{
				m_delayTimer -= dt;
			}
			else
			{
				RecalculateLocatorSizes();
				Play();
			}
		}
		else
		{
			Stop();
		}
	}
}

void EveChildEffectPropagator::UpdateTriggerInterval( const EveUpdateContext& updateContext )
{
	auto dt = updateContext.GetDeltaT();
	m_playTime += dt;

	if( m_processedTransforms.empty() )
	{
		return;
	}

	if( m_stopAfterNumTriggers > 0.0 && m_effectDuration != -1.f && m_playTime > ( m_stopAfterNumTriggers / m_frequency + m_effectDuration ) )
	{
		Stop();
		return;
	}

	// triggers based on the frequency interval unless maximum amount of spawns has been reached
	if( m_playTime > (float)m_currentTriggerIndex / m_frequency && ( (float)m_currentTriggerIndex < m_stopAfterNumTriggers || m_stopAfterNumTriggers < 0 ) )
	{
		int locatorIndex = GetSmartRandomLocatorIndex();
		if( !m_lastTriggered.empty() )
		{
			m_lastTriggered.erase( m_lastTriggered.begin() );
		}
		m_lastTriggered.push_back( locatorIndex );

		auto it = &m_processedTransforms.at( locatorIndex );
		m_effect->CreateInstance( it->scale, it->rotation, it->position );
		m_currentTriggerIndex++;
	}

	if( m_effectDuration != -1.f && m_playTime > ( (float)m_numDeleted / m_frequency ) + m_effectDuration )
	{
		m_effect->PopFront();
		m_numDeleted++;

		if( m_numDeleted == m_currentTriggerIndex )
		{
			m_currentTriggerIndex = 0; // prevent debug rendering on a running loop after it finishes (see InstanceContainers)
			m_lastTriggered.clear();
		}
	}
}

int EveChildEffectPropagator::GetSmartRandomLocatorIndex()
{
	int locatorIndex = -1;
	int ptSize = (int)m_processedTransforms.size();
	int ltSize = (int)m_lastTriggered.size();

	if( ltSize >= ptSize || m_frequency * m_effectDuration > 0.75f * float( ptSize ) ) // (*)
	{
		locatorIndex = TriRandInt( ptSize );
	}
	else
	{
		// this loop should never repeat more than 2-3 times and most often no
		// repeats at all. I also made an early exit condition ->(*).
		// The assumption is that this is faster than keeping an organized
		// index array barring excluded indexes. The only bad scenario is when all of these
		// 3 things line up: small locatorSet, long effect duration and very frequent triggers
		// and that's the reasoning for the early exit
		while( locatorIndex == -1 )
		{
			locatorIndex = TriRandInt( ptSize );
			for( int i = 0; i < ltSize; i++ )
			{
				if( locatorIndex == m_lastTriggered[i] )
				{
					locatorIndex = -1;
					break;
				}
			}
		}
	}
	return locatorIndex;
}

void EveChildEffectPropagator::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( !IsRendering() )
	{
		return;
	}

	if( m_effect != nullptr )
	{
		m_effect->UpdateAsyncronous( updateContext, params );
	}

	EveChildContainer::UpdateAsyncronous( updateContext, params );
}

void EveChildEffectPropagator::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !IsRendering() )
	{
		return;
	}

	if( m_effect != nullptr )
	{
		m_effect->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildEffectPropagator::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_effect != nullptr )
	{
		m_effect->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildEffectPropagator::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( m_effect != nullptr )
	{
		m_effect->UpdateVisibility( updateContext, parentTransform, parentLod );
	}
}

void EveChildEffectPropagator::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_currentTriggerIndex == 0 )
	{
		return;
	}

	if( m_effect != nullptr )
	{
		m_effect->GetRenderables( renderables );
	}
}

void EveChildEffectPropagator::ProcessLocalLocators()
{
	if( m_localLocators == nullptr )
	{
		return;
	}

	const LocatorStructureList* ls = m_localLocators->GetLocators();

	m_triggerSphereScalarMulti = 0;
	for( auto it = ls->begin(); it != ls->end(); ++it )
	{
		if( TriRand() > m_completeness )
		{
			continue;
		}

		Transform t;
		t.position = it->position;
		float l = LengthSq( t.position );
		m_triggerSphereScalarMulti = m_triggerSphereScalarMulti > l ? m_triggerSphereScalarMulti : l;
		t.rotation = it->direction;
		float rand = m_randScaleMin + TriRand() * ( m_randScaleMax - m_randScaleMin );
		t.scale = m_effectScaling * rand;
		m_processedTransforms.emplace_back( t );
	}

	m_triggerSphereScalarMulti = std::sqrt( m_triggerSphereScalarMulti ) * 2.f;
}

void EveChildEffectPropagator::ProcessRefLocators( IEveSpaceObject2* parent )
{
	if( m_locatorSetName.empty() )
	{
		m_locatorSetName = BlueSharedString( "damage" );
	}

	const LocatorStructureList* locators;

	if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( parent ) )
	{
		locators = spaceObject->GetLocatorsForSet( m_locatorSetName );
		Vector4 bounds;
		spaceObject->GetBoundingSphere( bounds );
		m_triggerSphereScalarMulti = bounds.w;
	}
	else
	{
		return;
	}

	if( locators )
	{
		for( auto locator = locators->begin(); locator != locators->end(); ++locator )
		{
			if( TriRand() > m_completeness )
			{
				continue;
			}

			Transform t;
			t.position = locator->position;
			t.rotation = locator->direction;
			float rand = m_randScaleMin + TriRand() * ( m_randScaleMax - m_randScaleMin );
			t.scale = m_effectScaling * rand;
			m_processedTransforms.emplace_back( t );
		}
	}
}

void EveChildEffectPropagator::ProcessRandomSpreadLocators()
{
	for( int i = 0; i < m_numTriggers; i++ )
	{
		if( TriRand() > m_completeness )
		{
			continue;
		}

		float dist = TriRand();
		dist += ( m_rndClosenessPreference - dist ) * TriRand();
		dist = m_rndMinRangeThreshold + ( m_rndRange - m_rndMinRangeThreshold ) * dist;

		float a = TRI_2PI * TriRand();
		float z = TriRand() * 2.f - 1.f;
		Vector3 angle( sqrt( 1.f - z * z ) * cos( a ), sqrt( 1.f - z * z ) * sin( a ), z );

		Transform t;

		t.position = angle * dist;
		TriQuaternionDirVector( &t.rotation, &angle );
		float rand = m_randScaleMin + TriRand() * ( m_randScaleMax - m_randScaleMin );
		t.scale = m_effectScaling * rand;
		m_processedTransforms.emplace_back( t );
	}
	m_triggerSphereScalarMulti = m_rndRange;
}

// --------------------------------------------------------------------------------------
// Description:
//   Based on a menu selection this function populates the vector maintaining all the
//   location references for where an effect should trigger
// --------------------------------------------------------------------------------------
void EveChildEffectPropagator::ProcessLocators( IEveSpaceObject2* parent )
{
	m_processedTransforms.clear();
	const Vector3 zAxis( 0.f, 0.f, 1.f );

	switch( m_type )
	{
	case LOCAL_LOCATORS:
		ProcessLocalLocators();
		break;
	case LOCATOR_SET_BY_REF:
		ProcessRefLocators( parent );
		break;
	case RANDOM_SPREAD:
		ProcessRandomSpreadLocators();
		break;
	default:
		break;
	}

	if( m_processedTransforms.empty() )
	{
		Stop();
		return;
	}

	DistanceSortLocators();
}

void EveChildEffectPropagator::RecalculateLocatorSizes()
{
	for( auto it = m_processedTransforms.begin(); it != m_processedTransforms.end(); ++it )
	{
		float rand = m_randScaleMin + TriRand() * ( m_randScaleMax - m_randScaleMin );
		it->scale = m_effectScaling * rand;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Based on a menu selection this function populates the vector maintaining all the
//   location references for where an effect should trigger
// --------------------------------------------------------------------------------------
void EveChildEffectPropagator::DistanceSortLocators()
{
	for( auto it = m_processedTransforms.begin(); it != m_processedTransforms.end(); ++it )
	{
		it->sqrDistToSphereCenter = LengthSq( it->position - m_triggerSphereOffset * m_triggerSphereScalarMulti );
	}

	std::sort( m_processedTransforms.begin(), m_processedTransforms.end(), SortByCircleDist() );
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable

void EveChildEffectPropagator::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "EffectPropagator" );

	if( m_effect != nullptr )
	{
		m_effect->GetDebugOptions( options );
	}
}

void EveChildEffectPropagator::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !renderer.HasOption( this, "EffectPropagator" ) )
	{
		return;
	}

	if( m_triggerSphereRadiusCurve != nullptr )
	{
		float currentRad = m_triggerSphereRadiusCurve->GetValueAt( m_playTime ) * m_triggerSphereScalarMulti;
		if( m_type == LOCATOR_SET_BY_REF )
		{
			currentRad *= 2;
		}
		renderer.DrawSphere( this, TranslationMatrix( m_triggerSphereOffset * m_triggerSphereScalarMulti ) * m_worldTransform, currentRad, 12, Tr2DebugRenderer::Wireframe, 0xbbffbbff );
	}

	if( m_type == LOCAL_LOCATORS )
	{
		if( m_localLocators == nullptr )
		{
			return;
		}

		const LocatorStructureList& locators = *m_localLocators->GetLocators();
		int i = 0;
		for( auto it = m_processedTransforms.begin(); it != m_processedTransforms.end(); ++it, i++ )
		{
			renderer.DrawSphereArrow(
				Tr2DebugObjectReference( &locators, uint32_t( i ) ),
				Vector3( XMVector3TransformCoord( it->position, m_worldTransform ) ),
				Vector3( it->rotation ),
				Length( it->scale ) * m_triggerSphereScalarMulti / 50.f,
				8,
				Tr2DebugRenderer::Lit,
				0x990088ff );
		}
	}
	else
	{
		for( auto it = m_processedTransforms.begin(); it != m_processedTransforms.end(); ++it )
		{
			renderer.DrawSphereArrow(
				this,
				Vector3( XMVector3TransformCoord( it->position, m_worldTransform ) ),
				Vector3( it->rotation ),
				Length( it->scale ) * m_triggerSphereScalarMulti / 50.f,
				8,
				Tr2DebugRenderer::Lit,
				0x990088ff );
		}
	}


	if( m_effect != nullptr )
	{
		m_effect->RenderDebugInfo( renderer );
	}
}


EveChildInstanceContainer* EveChildEffectPropagator::GetEffect() const
{
	return m_effect;
}

void EveChildEffectPropagator::SetEffect( EveChildInstanceContainer* effect )
{
	// unregister the current effect
	UnRegisterComponents();

	m_effect = effect;

	// register again
	RegisterComponents();
}

void EveChildEffectPropagator::SetControllerVariable( const char* name, float value )
{
	if( m_effect )
	{
		m_effect->SetControllerVariable( name, value );
	}
}