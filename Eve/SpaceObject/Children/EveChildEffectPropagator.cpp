#include "StdAfx.h"
#include "EveChildEffectPropagator.h"
#include "Eve/EveUpdateContext.h"
#include "Particle/Tr2SphereShapeAttributeGenerator.h"
#include "include/TriMath.h"
#include "Curves/Tr2CurveScalar.h"

EveChildEffectPropagator::EveChildEffectPropagator( IRoot* lockobj )
	:EveChildContainer( lockobj ),
	m_effectScaling( 1.0f, 1.0f, 1.0f ),
	m_triggerSphereOffset( 0.0f, 0.0f, 0.0f ),
	m_type( LOCAL_LOCATORS ),
	m_isPlaying( false ),
	m_playTime( 0 ),
	m_currentTriggerIndex( 0 ),
	m_numTriggers( 10 ),
	m_rndRange( 500 ),
	m_rndClosenessPreference( 0.25 ),
	m_rndMinRangeThreshold( 0 ),
	m_stopToClearDelay( 0 ),
	m_delayTimer( 0 ),
	m_replayAfterDelay( false ),
	m_triggerSphereScalarMulti( 1 ),
	m_completeness( 1 ),
	m_randScaleMin( 1 ),
	m_randScaleMax( 1 ),
	m_trigger( false )
{
}

EveChildEffectPropagator::~EveChildEffectPropagator()
{
}

bool EveChildEffectPropagator::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_triggerSphereOffset ) )
	{
		DistanceSortLocators();
	}

	if( IsMatch( value, m_completeness) )
	{
		m_completeness = min( 1.f, max( 0.f, m_completeness) );
	}

	if( IsMatch( value, m_randScaleMin ) )
	{
		m_randScaleMin = min( m_randScaleMax, max( 0.f, m_randScaleMin ) );
	}

	if( IsMatch( value, m_randScaleMax ) )
	{
		m_randScaleMax = max( m_randScaleMax, m_randScaleMin );
	}

	return true;
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
	if( m_effect != nullptr )
	{
		m_effect->ClearInstanceList();
	}
}

void EveChildEffectPropagator::ManageTriggers()
{
	if( m_triggerSphereRadiusCurve == nullptr )
	{
		return;
	}

	if (nullptr == m_effect)
	{
		return;
	}

	float currentRadSqr = m_triggerSphereRadiusCurve->GetValueAt( m_playTime ) * m_triggerSphereScalarMulti;
	currentRadSqr = currentRadSqr * currentRadSqr;

	for( auto it = m_processedTransforms.begin() + m_currentTriggerIndex; it != m_processedTransforms.end(); ++it )
	{
		if( it->sqrDistToSphereCenter < currentRadSqr  )
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
void EveChildEffectPropagator::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_trigger )
	{
		ProcessLocators( params.spaceObjectParent );
		Play();
	}

	if( m_isPlaying )
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
			if( m_replayAfterDelay )
			{
				if( m_delayTimer > 0 )
				{
					m_delayTimer -= dt;
				}

				Play();
			}
			else
			{
				Stop();
			}
		}
	}

	if( m_effect != nullptr )
	{
		m_effect->UpdateSyncronous( updateContext, params );
	}
}

void EveChildEffectPropagator::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( !IsRendering() )
	{
		return;
	}

	if( m_effect != nullptr )
	{
		m_effect->UpdateAsyncronous( updateContext, params );
	}
}

void EveChildEffectPropagator::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if(!IsRendering())
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

void EveChildEffectPropagator::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( m_effect != nullptr )
	{
		m_effect->UpdateVisibility( frustum, parentTransform, parentLod );
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

// --------------------------------------------------------------------------------------
// Description:
//   Based on a menu selection this function populates the vector maintaining all the
//   location references for where an effect should trigger
// --------------------------------------------------------------------------------------
void EveChildEffectPropagator::ProcessLocators( IEveSpaceObject2* parent )
{
	m_processedTransforms.clear();
	const Vector3 zAxis(0.f, 0.f, 1.f);
	const LocatorStructureList* ls;

	if( m_localLocators != nullptr )
	{
		ls = m_localLocators->GetLocators();
	}

	switch( m_type )
	{
	case LOCAL_LOCATORS:
		if( m_localLocators == nullptr )
		{
			break;
		}
		m_triggerSphereScalarMulti = 0;
		for( auto it = ls->begin(); it != ls->end(); ++it )
		{
			if( TriRand() > m_completeness)
			{
				continue;
			}

			Transform t;
			t.position = ( TranslationMatrix( it->position ) * m_worldTransform ).GetTranslation();
			float l = LengthSq(t.position);
			m_triggerSphereScalarMulti = m_triggerSphereScalarMulti > l ? m_triggerSphereScalarMulti : l;
			t.rotation = it->direction; 
			float rand = m_randScaleMin + TriRand() * ( m_randScaleMax - m_randScaleMin );
			t.scale = m_effectScaling * rand;
			m_processedTransforms.emplace_back( t );
		}
		m_triggerSphereScalarMulti = std::sqrt(m_triggerSphereScalarMulti) * 2.f;

		break;
	case LOCATOR_SET_BY_REF:
		if( !m_locatorSetName.empty() )
		{
			const LocatorStructureList* locators;
			
			if( nullptr != m_refObject )
			{
				locators = m_refObject->GetLocatorsForSet( m_locatorSetName );
			}
			else
			{
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
			}

			if( locators )
			{
				for ( auto locator = locators->begin(); locator != locators->end(); ++locator  )
				{
					if ( TriRand() > m_completeness )
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
		break;
	case RANDOM_SPREAD:
		for( int i = 0; i < m_numTriggers; i++ )
		{
			float dist = TriRand();
			dist += ( m_rndClosenessPreference - dist ) * TriRand();
			dist = m_rndMinRangeThreshold + ( m_rndRange - m_rndMinRangeThreshold ) * dist;

			float a = TRI_2PI * TriRand();
			float z = TriRand() * 2.f - 1.f;
			Vector3 angle( sqrt( 1.f - z*z ) * cos( a ), sqrt( 1.f - z*z ) * sin( a ), z );
			
			Transform t;
			
 			t.position = ( TranslationMatrix( angle * dist ) * m_worldTransform ).GetTranslation();
			
			TriQuaternionDirVector( &t.rotation, &angle );
			
			float rand = m_randScaleMin + TriRand() * ( m_randScaleMax - m_randScaleMin );
			t.scale = m_effectScaling * rand;
			m_processedTransforms.emplace_back( t );
		}
		m_triggerSphereScalarMulti = m_rndRange;
		break;
	default:
		break;
	}

	DistanceSortLocators();
}

void EveChildEffectPropagator::GetLights( Tr2LightManager& lightManager ) const
{
	if (m_effect != nullptr)
	{
		m_effect->GetLights(lightManager);
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
		it->sqrDistToSphereCenter = LengthSq( it->position - m_triggerSphereOffset * m_triggerSphereScalarMulti);
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
		renderer.DrawSphere( this, TranslationMatrix( m_triggerSphereOffset * m_triggerSphereScalarMulti ) * m_worldTransform, currentRad, 12, Tr2DebugRenderer::Wireframe, 0xbbffbbff );
	}
	
	for( auto it = m_processedTransforms.begin(); it != m_processedTransforms.end(); ++it )
	{
		renderer.DrawSphereArrow(
			this,
			it->position + m_worldTransform.GetTranslation(),
			Vector3( it->rotation ),
			Length( m_effectScaling ) * 10.f,
			8,
			Tr2DebugRenderer::Lit,
			0x990088ff 
		);
	}

	if( m_effect != nullptr )
	{
		m_effect->RenderDebugInfo( renderer );
	}
}

