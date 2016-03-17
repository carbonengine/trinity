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
	m_position( 0, 0, 0 )
{
}

void EveTacticalOverlayTrackObject::UpdatePosition( EveUpdateContext& updateContext )
{
	if( m_positionCurve )
	{
		m_positionCurve->GetValueAt( &m_position, updateContext.GetTime() );
	}
}

const Tr2VertexDefinition& EveTacticalOverlay::AnchorVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );  // Corner

		vd.Add( vd.FLOAT32_3, vd.POSITION, 0, 1, 1 );
	}
	return s_definition;
}

const Tr2VertexDefinition& EveTacticalOverlay::SphereConnectorVertex::GetDefinition()
{
	static Tr2VertexDefinition s_definition;
	if( s_definition.empty() )
	{
		Tr2VertexDefinition& vd = s_definition;
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 5 );  // Corner
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 0, 1, 1 );  // xyz - position; w - 
		//vd.Add( vd.FLOAT32_3, vd.POSITION, 0, 1, 1 );
		//vd.Add( vd.FLOAT32_2, vd.TEXCOORD, 0, 1, 1 );
		//vd.Add( vd.FLOAT32_3, vd.TEXCOORD, 1, 1, 1 );
	}
	return s_definition;
}

EveTacticalOverlay::EveTacticalOverlay( IRoot* lockobj ) :
	PARENTLOCK( m_trackObjects ),
	m_ranges( 200000.f, 50000.f, 1.f ),
	m_connectorEffectHash( 0 ),
	m_anchorEffectHash( 0 ),
	m_connectorSegmentsLow( 2 ),
	m_connectorSegmentsMedium( 5 ),
	m_connectorSegmentsHigh( 9 ),
	m_rootPosition( 0, 0, 0 ),
	m_targetSegmentCount( 25000.f ),
	m_totalSegmentsLast( 0.f ),
	m_requestedSegmentsLast( 0.f ),
	m_arcSegmentMultiplier( 1.f ),
	m_segmentCountMultiplier( 2.f )
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
}

bool EveTacticalOverlay::Initialize()
{
	SetVariableStore( m_anchorEffect );
	SetVariableStore( m_connectorEffect );
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
	return true;
}

void EveTacticalOverlay::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_connectorEffect )
	{
		m_connectorEffectHash = m_connectorEffect->GetHashValue();
		quadRenderer.RegisterEffect( m_connectorEffectHash, sizeof( SphereConnectorVertex ), 1, SphereConnectorVertex::GetDefinition(), m_connectorEffect );
	}
	if( m_anchorEffect )
	{
		m_anchorEffectHash = m_anchorEffect->GetHashValue();
		quadRenderer.RegisterEffect( m_anchorEffectHash, sizeof( AnchorVertex ), 1, AnchorVertex::GetDefinition(), m_anchorEffect );
	}
}

void EveTacticalOverlay::UpdateSyncronous( EveUpdateContext& updateContext ) 
{
	if( m_positionCurve )
	{
		m_positionCurve->GetValueAt( &m_rootPosition, updateContext.GetTime() );
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

void EveTacticalOverlay::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables, const Matrix& parentTransform )
{
	m_connectorBuffer.clear();
	m_anchorBuffer.clear();
	Vector3 up( 0, 1, 0 );
	float distanceThreshold = ( m_ranges.x + m_ranges.y ) * m_ranges.z;
	float requestedSegments = 0.f;
	for( auto it = m_trackObjects.begin(); it != m_trackObjects.end(); it++ )
	{
		Vector3 position = (*it)->GetPosition();
		Vector3 direction = position - m_rootPosition;
		float distance = D3DXVec3Length( &direction );
		if( distance > distanceThreshold )
		{
			continue;
		}

		direction.y = 0;
		D3DXVec3Normalize( &direction, &direction );
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
			// Wide arches need more segments and relatively fewer are needed for narrow ones
			Vector2 planarDiff( positionPlane.x - position.x, positionPlane.z - position.z );
			float length = D3DXVec2Length( &planarDiff );
			float height = abs( position.y - positionPlane.y );
			segments *= 1.f + m_arcSegmentMultiplier * length / height;
			requestedSegments += segments * m_segmentCountMultiplier;
			if( m_requestedSegmentsLast && m_requestedSegmentsLast > m_targetSegmentCount )
			{
				segments *= m_targetSegmentCount / m_requestedSegmentsLast;
				segments = max( segments, 1.f );
			}
			segments = m_segmentCountMultiplier * floor( segments + 0.5f );
		}
		m_anchorBuffer.push_back( position );
		for( int j = 0; j < segments; j++ )
		{
			SphereConnectorVertex vtx;
			float segmentInfo = segments * 256.f + j;
			vtx.instanceData = Vector4( position, segmentInfo );
			m_connectorBuffer.push_back(vtx);
		}
	}
	m_totalSegmentsLast = (float)m_connectorBuffer.size();
	m_requestedSegmentsLast = requestedSegments;
}

void EveTacticalOverlay::AddQuadsToQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( !m_connectorEffectHash || !m_anchorEffectHash )
	{
		return;
	}

	quadRenderer.AddQuads( m_connectorEffectHash, &m_connectorBuffer[0], m_connectorBuffer.size() );
	quadRenderer.AddQuads( m_anchorEffectHash, &m_anchorBuffer[0], m_anchorBuffer.size() );
}

