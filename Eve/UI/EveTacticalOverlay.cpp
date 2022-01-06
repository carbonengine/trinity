////////////////////////////////////////////////////////////
//
//    Created:   2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"
#include "EveTacticalOverlay.h"
#include "Shader/Tr2Effect.h"
#include "Tr2QuadRenderer.h"
#include "Eve/EveUpdateContext.h"
#include "include/TriMath.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "Tr2VariableStore.h"

extern float g_eveSpaceSceneLowDetailThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;
extern float g_eveSpaceSceneHighDetailThreshold;
extern float g_eveSpaceSceneVisibilityThreshold;

EveTacticalOverlayTrackObject::EveTacticalOverlayTrackObject( IRoot* lockobj ) :
	m_position( 0, 0, 0 ),
	m_velocity( 0, 0, 0 ),
	m_radius( 0 ),
	m_aggressive( false ),
	m_showVelocity( true )
{
}

void EveTacticalOverlayTrackObject::UpdatePosition( EveUpdateContext& updateContext )
{
	if( m_positionCurve )
	{
		m_positionCurve->GetValueDotAt( &m_velocity, updateContext.GetTime() );
		m_positionCurve->GetValueAt( &m_position, updateContext.GetTime() );
	}
}

const Tr2VertexDefinition& EveTacticalOverlay::AnchorVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );
	}
	return s_definition;
}

const Tr2VertexDefinition& EveTacticalOverlay::SphereConnectorVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 1, 1, 1 );
	}
	return s_definition;
}

const Tr2VertexDefinition& EveTacticalOverlay::VelocityConnectorVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1, 1, 1 );
	}
	return s_definition;
}

EveTacticalOverlay::EveTacticalOverlay( IRoot* lockobj ) :
	PARENTLOCK( m_trackObjects ),
	m_ranges( 200000.f, 50000.f, 1.f, 50.f ),
	m_connectorEffectHash( 0 ),
	m_anchorEffectHash( 0 ),
	m_velocityEffectHash( 0 ),
	m_connectorSegmentsLow( 2 ),
	m_connectorSegmentsMedium( 5 ),
	m_connectorSegmentsHigh( 9 ),
	m_rootPosition( 0, 0, 0 ),
	m_targetSegmentCount( 25000.f ),
	m_totalSegmentsLast( 0.f ),
	m_requestedSegmentsLast( 0.f ),
	m_arcSegmentMultiplier( 1.f ),
	m_segmentCountMultiplier( 2.f ),
	m_minRadiusForRange( 150.f ),
	m_interestRange( 0.f ),
	m_outsideInterestIntensity( 0.35f )
{
	m_variableStore.CreateInstance();
	RegisterVariables();
}

EveTacticalOverlay::~EveTacticalOverlay()
{
	auto backup = m_variableStore;
	m_variableStore = nullptr;
	SetVariableStore( m_anchorEffect );
	SetVariableStore( m_connectorEffect );
	SetVariableStore( m_velocityEffect );
}

bool EveTacticalOverlay::Initialize()
{
	SetVariableStore( m_anchorEffect );
	SetVariableStore( m_connectorEffect );
	SetVariableStore( m_velocityEffect );
	return true;
}

bool EveTacticalOverlay::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_anchorEffect ) )
	{
		SetVariableStore( m_anchorEffect );
	}
	else if( IsMatch( value, m_connectorEffect ) )
	{
		SetVariableStore( m_connectorEffect );
	}
	else if( IsMatch( value, m_velocityEffect ) )
	{
		SetVariableStore( m_velocityEffect );
	}
	return true;
}

void EveTacticalOverlay::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_connectorEffect )
	{
		m_connectorEffectHash = m_connectorEffect->GetHashValue() + ( reinterpret_cast<uint64_t>( m_connectorEffect.p ) << 32 );
		quadRenderer.RegisterEffect( m_connectorEffectHash, TRIBATCHTYPE_ADDITIVE, sizeof( SphereConnectorVertex ), 1, SphereConnectorVertex::GetDefinition(), m_connectorEffect );
	}
	if( m_anchorEffect )
	{
		m_anchorEffectHash = m_anchorEffect->GetHashValue() + ( reinterpret_cast<uint64_t>( m_anchorEffect.p ) << 32 );
		quadRenderer.RegisterEffect( m_anchorEffectHash, TRIBATCHTYPE_ADDITIVE, sizeof( AnchorVertex ), 1, AnchorVertex::GetDefinition(), m_anchorEffect );
	}
	if( m_velocityEffect )
	{
		m_velocityEffectHash = m_velocityEffect->GetHashValue() + ( reinterpret_cast<uint64_t>( m_velocityEffect.p ) << 32 );
		quadRenderer.RegisterEffect( m_velocityEffectHash, TRIBATCHTYPE_ADDITIVE, sizeof( VelocityConnectorVertex ), 1, VelocityConnectorVertex::GetDefinition(), m_velocityEffect );
	}
}

void EveTacticalOverlay::UpdateSyncronous( EveUpdateContext& updateContext ) 
{
	if( m_positionCurve )
	{
		m_positionCurve->GetValueAt( &m_rootPosition, updateContext.GetTime() );
		m_positionCurve->GetValueDotAt( &m_rootVelocity, updateContext.GetTime() );
	}

	for( auto it = m_trackObjects.begin(); it != m_trackObjects.end(); it++ )
	{
		(*it)->UpdatePosition( updateContext );
	}
	RegisterVariables();
}

static inline float GetSubdivisionCount( float pixelSize, float low, float mid, float high )
{
	if( pixelSize < g_eveSpaceSceneVisibilityThreshold )
	{
		return 0;
	}
	float lowCount;
	float highCount;
	float lowStep;
	float highStep;

	if( pixelSize <= g_eveSpaceSceneLowDetailThreshold )
	{
		lowCount = 1;
		highCount = low;
		lowStep = g_eveSpaceSceneVisibilityThreshold;
		highStep = g_eveSpaceSceneLowDetailThreshold;
	}
	else if( pixelSize <= g_eveSpaceSceneMediumDetailThreshold )
	{
		lowCount = low;
		highCount = mid;
		lowStep = g_eveSpaceSceneLowDetailThreshold;
		highStep = g_eveSpaceSceneMediumDetailThreshold;
	}
	else
	{
		lowCount = mid;
		highCount = high;
		lowStep = g_eveSpaceSceneMediumDetailThreshold;
		highStep = g_eveSpaceSceneHighDetailThreshold;
	}

	float linstep = TriLinearize( lowStep, highStep, pixelSize );
	return floor( Lerp( lowCount, highCount, linstep ) );
}

// --------------------------------------------------------------------------------------
// Description:
//   Registers GPU buffer variables with the local variable store.
// --------------------------------------------------------------------------------------
void EveTacticalOverlay::RegisterVariables()
{
	m_variableStore->RegisterVariable( "PlanePosition", m_rootPosition );
	m_variableStore->RegisterVariable( "Fadeout", m_ranges );
	m_variableStore->RegisterVariable( "RootVelocity", m_rootVelocity );
}

void EveTacticalOverlay::SetVariableStore( Tr2Effect* effect )
{
	if( effect )
	{
		effect->StartUpdate();
		effect->SetVariableStore( m_variableStore );
		effect->EndUpdate();
	}
}

void EveTacticalOverlay::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform )
{
	m_connectorBuffer.clear();
	m_anchorBuffer.clear();
	m_velocityBuffer.clear();
	
	Vector3 up( 0, 1, 0 );
	float distanceThreshold = ( m_ranges.x + m_ranges.y ) * m_ranges.z;
	float requestedSegments = 0.f;
	for( auto it = m_trackObjects.begin(); it != m_trackObjects.end(); it++ )
	{
		Vector3 position = (*it)->GetPosition();
		float radius = (*it)->GetRadius();
		Vector3 direction = position - m_rootPosition;
		float distance = Length( direction );
		if( distance > distanceThreshold )
		{
			continue;
		}

		direction.y = 0;
		direction = Normalize( direction );
		if( !(direction.x || direction.z) )
		{
			direction.x = 0.01;
		}
		Vector3 positionPlane = m_rootPosition + direction * distance;
		Vector4 bs;
		BoundingSphereFromPoints( bs, position, positionPlane );
		if( !frustum.IsSphereVisible( &bs ) )
		{
			continue;
		}
		
		float pixelDiameter = frustum.GetPixelSizeAccross( &bs );
		float segments = GetSubdivisionCount( pixelDiameter, m_connectorSegmentsLow, m_connectorSegmentsMedium, m_connectorSegmentsHigh );
		if( segments != 0 )
		{
			Vector2 planarDiff( positionPlane.x - position.x, positionPlane.z - position.z );
			float length = Length( planarDiff );
			float height = std::abs( position.y - positionPlane.y );
			// Wide arches need more segments to look good
			segments *= 1.f + m_arcSegmentMultiplier * length / height;
			requestedSegments += segments * m_segmentCountMultiplier;
			if( m_requestedSegmentsLast && m_requestedSegmentsLast > m_targetSegmentCount )
			{
				segments *= m_targetSegmentCount / m_requestedSegmentsLast;
				segments = max( segments, 1.f );
			}
			segments = m_segmentCountMultiplier * floor( segments + 0.5f );
		}
		float counter = radius > m_minRadiusForRange ? segments + 1 : segments;
		float interestReducedIntensity = 0.0;
		if( m_interestRange > 0.0001f && ( distance - radius - m_ranges.w ) > m_interestRange )
		{
			interestReducedIntensity = 1 - m_outsideInterestIntensity;
		}
		AnchorVertex avtx;
		avtx.instanceData = Vector4( position.x, position.y, position.z, interestReducedIntensity );
		m_anchorBuffer.push_back( avtx );
		for( int j = 0; j < counter; j++ )
		{
			SphereConnectorVertex vtx;
			float segmentInfo = segments * 256.f + j;
			vtx.instanceData = Vector4( position, segmentInfo );
			vtx.instanceData2 = floor( radius ) + interestReducedIntensity;
			m_connectorBuffer.push_back(vtx);
		}
		// Object properties(radius and movement)
		for( float i = 0; i < 3; i++ )
		{
			if( i == 1.f && !((*it)->IsAggressive()) )
			{
				continue;
			}
			if( i == 0.f && !((*it)->ShowVelocity()))
			{
				continue;
			}
			VelocityConnectorVertex vtx;
			vtx.instanceData = Vector4( position, i );
			float blink = i == 1 ? 0.9f : 0.f;
			vtx.instanceData2 = Vector4( (*it)->GetVelocity(), floor( radius ) + blink );
			m_velocityBuffer.push_back( vtx );
		}
	}
	
	VelocityConnectorVertex vtx;
	// Add velocity for your ship
	vtx.instanceData = Vector4( m_rootPosition, 0.f );
	vtx.instanceData2 = Vector4( m_rootVelocity, floor( m_ranges.w ) );
	m_velocityBuffer.push_back( vtx );
	// And blinking velocities for selected target
	if( m_interestObject && m_interestObject->ShowVelocity() )
	{
		// Your velocity at the target
		vtx.instanceData = Vector4( m_interestObject->GetPosition(), 0.f );
		vtx.instanceData2 = Vector4( m_rootVelocity, floor( m_interestObject->GetRadius() ) + 0.9f );
		m_velocityBuffer.push_back( vtx );
		// And target at your ship
		vtx.instanceData = Vector4( m_rootPosition, 0 );
		vtx.instanceData2 = Vector4( m_interestObject->GetVelocity(), floor( m_ranges.w ) + 0.9f );
		m_velocityBuffer.push_back( vtx );
	}

	m_totalSegmentsLast = (float)m_connectorBuffer.size();
	m_requestedSegmentsLast = requestedSegments;
}


void EveTacticalOverlay::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if( !m_connectorEffectHash || !m_anchorEffectHash || !m_velocityEffectHash )
	{
		return;
	}

	quadRenderer.AddQuads( m_connectorEffectHash, &m_connectorBuffer[0], m_connectorBuffer.size() );
	quadRenderer.AddQuads( m_anchorEffectHash, &m_anchorBuffer[0], m_anchorBuffer.size() );
	quadRenderer.AddQuads( m_velocityEffectHash, &m_velocityBuffer[0], m_velocityBuffer.size() );
}

