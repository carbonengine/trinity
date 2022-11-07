#include "StdAfx.h"
#include "EveBoosterSet2.h"

#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"
#include "Shader/Tr2Effect.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "TriSettingsRegistrar.h"
#include "Include/TriMath.h"

#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "EveTrailsSet.h"
#include "Tr2LightManager.h"

BLUE_DECLARE_VECTOR( EveSpriteSet );

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

// constants
// how many planes per booster tree structure?
const unsigned int EVE_BOOSTER_PLANES_COUNT[EveBoosterSet2::SHAPE_COUNT] = { 4, 6 };

// externs
extern float g_eveSpaceSceneLowDetailThreshold;
extern float g_eveSpaceSceneMediumDetailThreshold;

// registered globals
// enable
bool g_eveSpaceObjectTrailsEnabled = true;
TRI_REGISTER_SETTING( "eveSpaceObjectTrailsEnabled", g_eveSpaceObjectTrailsEnabled );
// cut-off minimum length for trails
float g_eveSpaceObjectTrailsMinLength = 200.f;
TRI_REGISTER_SETTING( "eveSpaceObjectTrailsMinLength", g_eveSpaceObjectTrailsMinLength );
float g_eveSpaceObjectTrailsMinLengthFade = 1000.f;
TRI_REGISTER_SETTING( "eveSpaceObjectTrailsMinLengthFade", g_eveSpaceObjectTrailsMinLengthFade );
// cut-off maximum length for trails
float g_eveSpaceObjectTrailsMaxLength = 50000.f;
TRI_REGISTER_SETTING( "eveSpaceObjectTrailsMaxLength", g_eveSpaceObjectTrailsMaxLength );
float g_eveSpaceObjectTrailsMaxLengthFade = 20000.f;
TRI_REGISTER_SETTING( "eveSpaceObjectTrailsMaxLengthFade", g_eveSpaceObjectTrailsMaxLengthFade );

const unsigned g_lightNoiseSize = 128;
float g_lightNoise[g_lightNoiseSize];
bool g_lightNoiseInitialized = false;


EveBoosterSet2Renderable::EveBoosterSet2Renderable( IRoot* lockobj ) : 
	m_isVisible( false ),
	m_boosterLOD( 0.f ),
	m_trailsLOD( 0.f ),
	m_parentRotation( 0.f, 0.f, 0.f, 1.f ),
	m_parentSpeed( 0.f ),
	m_overallIntensity( 0.f ),
	m_lastAccFactor( 0.f ),
	m_lastValue( 0.f ),
	// Trails
	m_trailIntensity( 0.f ),
	m_trailsTotalLength( 0.f ),
	m_trailsTimeToNext( 0.f ),
	m_trailsTimeDelta( 1.f ),
	m_trailsOffsetLatest( 0 ),
	m_trailsOffsetAccu( 0.f, 0.f, 0.f ),
	m_parentTransform( IdentityMatrix() )
{
	BoundingBoxInitialize( m_trailsBoundsMin, m_trailsBoundsMax );
	// "invalidate" all trail control positions
	for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		m_trailsControlPositions[ i ] = Vector3( 0.f, 0.f, 0.f );
		m_trailsControlNormals[ i ] = Vector3( 0.f, 0.f, -1.f );
		m_trailsControlNormalsFactor[ i ] = 1.f;
		m_trailsSequenceLength[ i ] = 0.f;
	}

	m_trailsOffsets = (XMVECTOR*)CCP_ALIGNED_MALLOC( "EveBoosterSet2::m_trailsOffsets", sizeof( XMVECTOR ) * EVE_MAX_POSITION_OFFSET_COUNT, 16 );
	memset( m_trailsOffsets, 0, EVE_MAX_POSITION_OFFSET_COUNT * sizeof( XMVECTOR ) );
}

EveBoosterSet2Renderable::~EveBoosterSet2Renderable()
{
	CCP_ALIGNED_FREE( m_trailsOffsets );
}

// --------------------------------------------------------------------------------
// Description:
//   This function calculates the booster intensity (aka "gain") from the given
//   ball and the given time. How it calculates this comes from the early days
//   of EVE and is "just" some black magic...
// Arguments:
//   acceleration - parent acceleration
//   t - global time
// Return value:
//   Returns the intensity of the booster (from 0.f to something a little bit
//   higher than 1.f (because of afterburners and microwarps)
// --------------------------------------------------------------------------------
float EveBoosterSet2Renderable::CalculateIntensity( const Vector3& acceleration, Be::Time t )
{
	// if we want the boosters to be permanently visible
	if( m_boosterSet->m_alwaysOn )
	{
		return m_boosterSet->m_alwaysOnIntensity;
	}

	Vector3 backwd;
	// TODO: ok... this is a bit weird
	TriVectorRotatedBasisQuaternion( &backwd, TRITA_Z, &m_parentRotation );
	float speedRatio = m_boosterSet->m_maxVel ? ( m_parentSpeed / m_boosterSet->m_maxVel ) : 0.f;
	float accFactor = Dot( acceleration, backwd );
	accFactor *= max( 0.3f, speedRatio );
	if( accFactor < 0.f )
	{
		accFactor = 0.f;
	}
	else if( accFactor > 1.f )
	{
		accFactor = 1.f;
	}

	float speedMultiplier = 0.8f;
	float accMultiplier = 0.2f;

	// dampening (like drum noise)
	accFactor = accFactor * 0.2f + m_lastAccFactor * 0.8f;
	m_lastAccFactor = accFactor;
	float value = m_lastValue * 0.8f + ( speedMultiplier * speedRatio  + accMultiplier * accFactor ) * 0.2f;
	value = min( value, 2.0f );
	m_lastValue = value;

	return value;
}

void EveBoosterSet2Renderable::Update( float deltaT, Be::Time t, const Matrix& parentMatrix, float parentSpeed, const Vector3& parentAcceleration, const Quaternion& parentRotation )
{
	// can(!) get the speed from destiny's position ball
	if( m_boosterSet->m_destinyUpdate )
	{
		m_parentSpeed = parentSpeed;
	}
	else
	{
		// if no time has elapsed, no speed calculation is possible at all
		if( deltaT != 0.f )
		{
			// rely on actual position data
			Vector3 dir = parentMatrix.GetTranslation() - m_parentTransform.GetTranslation();
			m_parentSpeed = Length( dir ) / deltaT;
		}
	}

	// keep the transform of the parent (aka ship) around
	m_parentTransform = parentMatrix;
	m_parentRotation = parentRotation;

	// update the intensity, which is based on ship's movement
	m_overallIntensity = CalculateIntensity( parentAcceleration, t );
}

// --------------------------------------------------------------------------------
// Description:
//   No transparency.
// --------------------------------------------------------------------------------
bool EveBoosterSet2Renderable::HasTransparentBatches()
{
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Only have additive batches via a geometry provider, since we are using
//   instanced rendering.
// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE )
	{
		return;
	}
	if( !m_boosterSet->m_display )
	{
		return;
	}
	if( !m_boosterSet->m_instanceBuffer.IsValid() )
	{
		return;
	}
	if( m_boosterSet->m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	// boosters visible based on LOD?
	if( m_boosterLOD > g_eveSpaceSceneLowDetailThreshold )
	{
		TriForwardingBatch* batch = batches->Allocate<TriForwardingBatch>();
		if( batch )
		{
			batch->SetPerObjectData( perObjectData );
			batch->SetShaderMaterial( ( m_boosterLOD > g_eveSpaceSceneMediumDetailThreshold * 1.5f || !m_boosterSet->m_effectFar ) ? m_boosterSet->m_effect : m_boosterSet->m_effectFar );
			batch->SetGeometryProvider( this );
			batches->Commit( batch );
		}
	}

	if( m_trailsLOD > g_eveSpaceSceneLowDetailThreshold )
	{
		if( m_boosterSet->m_trails )
		{
			// are they enabled?
			if( g_eveSpaceObjectTrailsEnabled )
			{
				// trail length can be zero! then render nothing!
				if( m_trailsTotalLength > 0.f )
				{
					// trail intensity can be zero! then render nothing!
					if( m_trailIntensity > 0.f )
					{
						m_boosterSet->m_trails->GetBatches( batches, perObjectData );
					}
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   No sorting. Everything is NonSorted
// --------------------------------------------------------------------------------
float EveBoosterSet2Renderable::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill the per-object data. First the world matrix of the parent-ship.
// SeeAlso:
//   EveBoosterSetPerObjectData, TriRenderBatchAccumulator
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveBoosterSet2Renderable::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveBoosterSetPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}

	// column_major for shaders
	perObjectData->m_vsData.shipMatrix = Transpose( m_parentTransform );

	// vs data
	perObjectData->m_vsData.boosterIntensity = m_overallIntensity;
	perObjectData->m_vsData.shipSpeed = m_parentSpeed;
	perObjectData->m_vsData.maxBoosterSize = m_boosterSet->m_maxSize;
	// ps data
	perObjectData->m_psData.boosterIntensity = m_overallIntensity;
	perObjectData->m_psData.trailIntensity = m_trailIntensity;
	perObjectData->m_psData.warpIntensity = m_boosterSet->m_warpIntensity;

	for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		perObjectData->m_vsData.trailsControlPositions[ i ] = Vector4( m_trailsControlPositions[ i ], m_trailsSequenceLength[ i ] );
		perObjectData->m_vsData.trailsControlNormals[ i ] = Vector4( m_trailsControlNormals[ i ], m_trailsControlNormalsFactor[ i ] );
	}

	return perObjectData;
}

// --------------------------------------------------------------------------------
// Description:
//   Setup instanced reandering and call DIP
// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::SubmitGeometry( Tr2RenderContext& renderContext )
{
	// how many indiviual boosters are in this set?
	unsigned int boosterCount = (unsigned int)m_boosterSet->m_singleBoosters.size();

	auto shape = Tr2Renderer::GetShaderModel() >= TR2SM_3_0_HI ? EveBoosterSet2::BOX :EveBoosterSet2:: STAR;
	auto indexBuffer = Tr2Renderer::GetQuadListIndexBuffer( EVE_BOOSTER_PLANES_COUNT[shape] );
	if( !indexBuffer )
	{
		return;
	}

	// decl & index
	renderContext.m_esm.ApplyVertexDeclaration( m_boosterSet->m_vertexDeclHandle );
	renderContext.m_esm.ApplyIndexBuffer( *indexBuffer );
	// stream0: "indexed", the star shape	
	renderContext.m_esm.ApplyStreamSource( 0, m_boosterSet->m_vertexBuffer.GetSharedResource(), 0, sizeof( EveBoosterSet2::BoosterVertex ) );
	// stream1: "instance", the star shape' position	
	renderContext.m_esm.ApplyStreamSource( 1, m_boosterSet->m_instanceBuffer, 0, sizeof( EveBoosterSet2::InstanceVertex ) );
	// draw	
	renderContext.SetTopology( TOP_TRIANGLES );	
	renderContext.DrawIndexedInstanced( 4 * EVE_BOOSTER_PLANES_COUNT[shape], 0, 2 * EVE_BOOSTER_PLANES_COUNT[shape], boosterCount );
}

// --------------------------------------------------------------------------------
// Description:
//   Transform and modify the saved bounding sphere, so it can be used for
//   culling etc.
// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::GetBoundingSphere( Vector4& boundingSphere ) const
{
	// move bounding sphere back to catch all the glowy exhaust
	boundingSphere = m_boosterSet->m_boosterBoundingSphere + Vector4( 0.f, 0.f, -0.5f * m_boosterSet->m_boosterBoundingSphere.w, 0.f );
	// transform center into worldspace
	boundingSphere.GetXYZ() = TransformCoord( boundingSphere.GetXYZ(), m_parentTransform );
	// blow up radius so we contain all the glowy stuff coming out of a booster
	boundingSphere.w = 2.f * m_boosterSet->m_boosterBoundingSphere.w;
}

// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::UpdateVisibility( const TriFrustum& frustum )
{

	Vector4 transformedBoundingSphere;
	GetBoundingSphere( transformedBoundingSphere );

	// LOD for boosters: use the bounding sphere
	m_boosterLOD = 2.f * frustum.GetPixelSizeAccross( &transformedBoundingSphere );
	
	// LOD for trails: based on closest control point of spline with sphere around it
	unsigned int cntrPosIdx = 0;
	float sqDist = FLT_MAX;
	for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		Vector3 tmp( m_trailsControlPositions[ i ] - frustum.m_viewPos );
		float d = LengthSq( tmp );
		if( d < sqDist )
		{
			sqDist = d;
			cntrPosIdx = i;
		}
	}
	Vector4 tmp( m_trailsControlPositions[ cntrPosIdx ], transformedBoundingSphere.w );
	m_trailsLOD = 7.5f * frustum.GetPixelSizeAccross( &tmp );

	m_isVisible = frustum.IsSphereVisible( &transformedBoundingSphere ) || frustum.IsBoxVisible( m_trailsBoundsMin, m_trailsBoundsMax );
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Put this object on the list, since it
//   is an ITr2Renderable.
//   Also get the renderables from the fire effects, if we are firing.
// Arguments:
//   frustum - the current view frustum of the current frame
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable, EveStretch
// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_isVisible )
	{
		renderables.push_back( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Does a lot of calculations for trails based on the movement of the parent (aka 
//   the ship). Calling the function that calculates all the spline data.
// Arguments:
//   deltaT - time since last frame
//   t - global time
// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::UpdateTrails( float deltaT, Be::Time t )
{
	// update the spline's tangents and positions
	CalculateSplineData( deltaT );

	// calc trails intensity, which is based on speed
	if( ( m_trailsTotalLength > g_eveSpaceObjectTrailsMinLength ) && ( m_trailsTotalLength < g_eveSpaceObjectTrailsMinLength + g_eveSpaceObjectTrailsMinLengthFade ) )
	{
		// fading
		m_trailIntensity = SinSmooth( ( m_trailsTotalLength - g_eveSpaceObjectTrailsMinLength ) / g_eveSpaceObjectTrailsMinLengthFade );
	}
	else if( ( m_trailsTotalLength > g_eveSpaceObjectTrailsMaxLength - g_eveSpaceObjectTrailsMaxLengthFade ) && ( m_trailsTotalLength < g_eveSpaceObjectTrailsMaxLength ) )
	{
		// fading
		m_trailIntensity = SinSmooth( ( g_eveSpaceObjectTrailsMaxLength - m_trailsTotalLength ) / g_eveSpaceObjectTrailsMaxLengthFade );
	}
	else if( ( m_trailsTotalLength < g_eveSpaceObjectTrailsMinLength ) || ( m_trailsTotalLength > g_eveSpaceObjectTrailsMaxLength ) )
	{
		m_trailIntensity = 0.f;
	}
	else
	{
		m_trailIntensity = 1.f;
	}

	// ovverride! for jessica editing
	if( m_boosterSet->m_alwaysOn )
	{
		m_trailIntensity = 1.f;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Updates the ringbuffer based on the new offset, then based on that ringbuffer
//   we calculate splinepoints and normals.
// Arguments:
//   deltaT - time since last frame
// --------------------------------------------------------------------------------
void EveBoosterSet2Renderable::CalculateSplineData( float deltaT )
{
	// time MUST elapse
	if( deltaT <= 0.f )
	{
		return;
	}

	// where are we?
	Vector3 parentPos = TransformCoord( Vector3( 0.f, 0.f, 0.f ), m_parentTransform );

	// what dir are we moving?
	Vector3 movementDir( 0.f, 0.f, 1.f );
	TriVectorRotateQuaternion( &movementDir, &movementDir, &m_parentRotation );
	// how far did we get since last call?
	movementDir *= deltaT * m_parentSpeed;

	if( m_boosterSet->m_physicsUpdate )
	{
		// update offset ringbuffer based on real ingame movement
		m_trailsTimeToNext += deltaT;
		m_trailsOffsetAccu -= movementDir;

		// how many interations fit into elapsed time since last frame?
		unsigned int iterCount = ( unsigned int )( m_trailsTimeToNext / EVE_POSITION_OFFSET_DELTAT );

		// cumulative offset
		XMVECTOR offset = ( ( EVE_POSITION_OFFSET_DELTAT / m_trailsTimeToNext ) * iterCount ) * m_trailsOffsetAccu;

		// we shouldn't update the m_trailsOffsets unless we have at least 1 iteration
		if( iterCount > 0 )
		{
			// next two branches do the same thing, but the first one is slightly better
			// in performance for small iterCount
			if( iterCount < 20 )
			{
				// apply cumulative offset to all positions
				if( Dot( m_trailsOffsetAccu, m_trailsOffsetAccu ) > 0.00001f )
				{
					for( unsigned int i = 0; i < EVE_MAX_POSITION_OFFSET_COUNT; ++i )
					{
						m_trailsOffsets[ i ] = XMVectorAdd( m_trailsOffsets[i], offset );
					}
				}

				// treat those marginal positions
				for( unsigned int j = 0; j < iterCount; ++j )
				{
					++m_trailsOffsetLatest;
					if( m_trailsOffsetLatest >= EVE_MAX_POSITION_OFFSET_COUNT )
					{
						m_trailsOffsetLatest = 0;
					}

					m_trailsOffsets[m_trailsOffsetLatest] = ( ( iterCount - 1 - j ) * ( EVE_POSITION_OFFSET_DELTAT / m_trailsTimeToNext )  ) * m_trailsOffsetAccu;// offsets[j + 1];
				}
			}
			else
			{
				++m_trailsOffsetLatest;
				if( m_trailsOffsetLatest >= EVE_MAX_POSITION_OFFSET_COUNT )
				{
					m_trailsOffsetLatest = 0;
				}

				XMVECTOR partialOffset = ( EVE_POSITION_OFFSET_DELTAT / m_trailsTimeToNext ) * m_trailsOffsetAccu;

				// apply cumulative offset to all positions
				for( unsigned int i = 0; i < EVE_MAX_POSITION_OFFSET_COUNT; ++i )
				{
					unsigned n;
					if( i >= m_trailsOffsetLatest )
					{
						n = i - m_trailsOffsetLatest;
					}
					else
					{
						n = i + EVE_MAX_POSITION_OFFSET_COUNT - m_trailsOffsetLatest;
					}
					if( n < iterCount )
					{
						m_trailsOffsets[ i ] = XMVectorMultiply( partialOffset, XMVectorReplicate( float( iterCount - 1 - n ) ) );
					}
					else
					{
						m_trailsOffsets[ i ] = XMVectorAdd( m_trailsOffsets[i], offset );
					}
				}
				m_trailsOffsetLatest = ( m_trailsOffsetLatest + iterCount - 1 ) % EVE_MAX_POSITION_OFFSET_COUNT;
			}

			m_trailsOffsetAccu -= ( ( EVE_POSITION_OFFSET_DELTAT / m_trailsTimeToNext ) * iterCount ) * m_trailsOffsetAccu;
			m_trailsTimeToNext -= EVE_POSITION_OFFSET_DELTAT * iterCount;
		}

		// calc position for spline, always relative to current spaceship pos
		int ringIdx = m_trailsOffsetLatest;
		for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
		{
			m_trailsControlPositions[ i ] =  parentPos + Vector3( m_trailsOffsets[ ringIdx ] );

			ringIdx -= (unsigned int)( m_trailsTimeDelta / EVE_POSITION_OFFSET_DELTAT );
			if( ringIdx < 0 )
			{
				ringIdx += EVE_MAX_POSITION_OFFSET_COUNT;
			}
		}
	}
	else
	{
		// update offset ringbuffer based on static offsets
		for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
		{
			Vector3 rotatedOffset;
			TriVectorRotateMatrix( &rotatedOffset, &m_boosterSet->m_trailsStaticOffsets[ i ], &m_parentTransform );
			m_trailsControlPositions[ i ] =  parentPos + rotatedOffset;
		}
	}


	// cycle over all control pints and determine trails total length
	m_trailsTotalLength = 0.f;
	for( unsigned int i = 1; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		Vector3 sequenceDir = m_trailsControlPositions[ i ] - m_trailsControlPositions[ i - 1 ];
		m_trailsTotalLength += Length( sequenceDir );
	}


	// build aa bounding box for trail
	BoundingBoxInitialize( m_trailsBoundsMin, m_trailsBoundsMax );
	// carefull: trails are instanced with the boosters, so we add not a
	// single point, but the whole bounding sphere of the booster points
	Vector4 boosterBoundsSphere;
	GetBoundingSphere( boosterBoundsSphere );
	for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		Vector3 r = Vector3( boosterBoundsSphere.w, boosterBoundsSphere.w, boosterBoundsSphere.w );
		Vector3 boundMin( m_trailsControlPositions[ i ] - r );
		Vector3 boundMax( m_trailsControlPositions[ i ] + r );
		BoundingBoxUpdate( m_trailsBoundsMin, m_trailsBoundsMax, boundMin, boundMax );
	}


	// first tangent is always constant and points backwards from ship
	Vector3 firstTangent( 0.f, 0.f, -1.f * std::min( m_boosterSet->m_trailsSmoothing, Length( m_trailsControlPositions[1] - m_trailsControlPositions[0] ) ) );
	m_trailsControlNormals[0] = TransformNormal( firstTangent, m_parentTransform );

	// last tangent is always from before-last to last point
	Vector3 lastDir = m_trailsControlPositions[ EVE_MAX_CONTROL_POINT_COUNT - 1 ] - m_trailsControlPositions[ EVE_MAX_CONTROL_POINT_COUNT - 2 ];
	m_trailsControlNormals[ EVE_MAX_CONTROL_POINT_COUNT - 1 ] = 0.5 * lastDir;

	// the rest is calculated according to c-r (or something...)
	for( unsigned int i = 1; i < EVE_MAX_CONTROL_POINT_COUNT - 1; ++i )
	{
		// from -1 to +1
		Vector3 dir = m_trailsControlPositions[ i + 1 ] - m_trailsControlPositions[ i - 1 ];
		m_trailsControlNormals[i] = Normalize( dir );
		// adjust length!
		Vector3 t0( m_trailsControlPositions[ i + 1 ] - m_trailsControlPositions[ i ] );
		Vector3 t1( m_trailsControlPositions[ i ] - m_trailsControlPositions[ i - 1 ] );
		float len0 = Length( t0 );
		float len1 = Length( t1 );
		// keep the normal add len0 and store a factor to calc a len1-normal
		m_trailsControlNormals[ i ] *= len0;
		m_trailsControlNormalsFactor[ i ] = len1 / len0;
	}

	for( unsigned int i = 1; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		// "sequence length" value is the length of this segment normalized along the whole trail
		Vector3 segment( m_trailsControlPositions[ i ] - m_trailsControlPositions[ i - 1 ] );
		m_trailsSequenceLength[ i ] = Length( segment );
		// normalize if non-zero length
		if( m_trailsTotalLength > 0.f )
		{
			m_trailsSequenceLength[ i ] /= m_trailsTotalLength;
		}
	}
}


namespace
{
	ALResult GetBoxVB( Tr2BufferAL& vb, Tr2PrimaryRenderContext& renderContext )
	{
		const uint32_t vertexCount = 4 * 6;
		EveBoosterSet2::BoosterVertex vertices[vertexCount];
		auto p = &vertices[0];
		( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );

		( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );

		( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );

		( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );

		( p++ )->position = Vector3( -1.0f, -1.0f, 0.0f );
		( p++ )->position = Vector3( -1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, -1.0f );
		( p++ )->position = Vector3( 1.0f, -1.0f, 0.0f );

		( p++ )->position = Vector3( -1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, 0.0f );
		( p++ )->position = Vector3( 1.0f, 1.0f, -1.0f );
		( p++ )->position = Vector3( -1.0f, 1.0f, -1.0f );

		return vb.Create( sizeof( EveBoosterSet2::BoosterVertex ), vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &vertices[0], renderContext );

	}

	ALResult GetStarVB( Tr2BufferAL& vb, Tr2PrimaryRenderContext& renderContext )
	{
		const uint32_t vertexCount = 4 * 4;
		EveBoosterSet2::BoosterVertex vertices[vertexCount];
		auto p = &vertices[0];
		for( unsigned int i = 0; i < vertexCount; i += 4 )
		{
			float t = (float)i * XM_PI / 4.f / 4.f;
			float x = cos( t ) * 0.5f;
			float y = sin( t ) * 0.5f;
			p->position = Vector3( -x, -y, 0.f );
			p->texCoord = Vector2( 1.f, 1.f );
			++p;
			p->position = Vector3( -x, -y, -1.f );
			p->texCoord = Vector2( 1.f, 0.f );
			++p;
			p->position = Vector3( x, y, -1.f );
			p->texCoord = Vector2( 0.f, 0.f );
			++p;
			p->position = Vector3( x, y, 0.0f );
			p->texCoord = Vector2( 0.f, 1.f );
			++p;
		}

		return vb.Create( sizeof( EveBoosterSet2::BoosterVertex ), vertexCount, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, &vertices[0], renderContext );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, build the tree-shape geometry we will use for
//   rendering the boosters
// --------------------------------------------------------------------------------
EveBoosterSet2::EveBoosterSet2( IRoot* lockobj ) :
	PARENTLOCK( m_boosterRenderables ),
	m_glowColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_haloColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_warpGlowColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_warpHaloColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_display( true ),
	m_physicsUpdate( true ),
	m_destinyUpdate( true ),
	m_alwaysOn( false ),
	m_alwaysOnIntensity( 1.f ),
	m_staticTrailLength( 0.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_maxVel( 250.f ),
	m_warpIntensity( 0.f ),
	m_maxSize( 0.f ),
	m_glowScale( 1.f ),
	m_symHaloScale( 1.f ),
	m_haloScaleX( 1.f ),
	m_haloScaleY( 1.f ),
	m_flareLodEnabled( true ),
	m_glowsVisible( true ),
	m_trailsSmoothing( 10.f ),
	m_lightOffset( 0.f ),
	m_lightRadius( 0.f ),
	m_lightWarpRadius( 0.f ),
	m_lightFlickerAmplitude( 0.f ),
	m_lightFlickerFrequency( 0.f ),
	m_lightColor( 0.f, 0.f, 0.f, 0.f ),
	m_lightWarpColor( 0.f, 0.f, 0.f, 0.f ),
	m_vertexBuffer( BlueSharedString( "BoosterBoxVB" ), GetBoxVB )
{
	BoundingSphereInitialize( m_boosterBoundingSphere );
	
	for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		m_trailsStaticOffsets[ i ] = Vector3( 0.f, 0.f, 0.f );
	}

	if( !g_lightNoiseInitialized )
	{
		g_lightNoiseInitialized = true;
		for( unsigned i = 0; i < g_lightNoiseSize; ++i )
		{
			g_lightNoise[i] = float( rand() ) / float( RAND_MAX );
		}
	}
}

void EveBoosterSet2::SetCount( unsigned count )
{
	if( count < 1 )
	{
		count = 1;
	}
	while( count < m_boosterRenderables.size() )
	{
		m_boosterRenderables.Remove( m_boosterRenderables.size() - 1 );
	}
	while( m_boosterRenderables.size() < count )
	{
		EveBoosterSet2RenderablePtr renderable;
		renderable.CreateInstance();
		renderable->m_boosterSet = this;
		m_boosterRenderables.Append( renderable->GetRawRoot() );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveBoosterSet2::~EveBoosterSet2()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveBoosterSet2::Initialize()
{
	PrepareResources();
	return true;
}

bool EveBoosterSet2::OnModified( Be::Var* value )
{

	if( m_glows )
	{
		if( IsMatch( value, m_glowScale ) || IsMatch( value, m_haloScaleX ) || IsMatch( value, m_haloScaleY ) || IsMatch( value, m_symHaloScale ) ||
			IsMatch( value, m_glowColor ) || IsMatch( value, m_warpGlowColor ) || IsMatch( value, m_haloColor ) || IsMatch( value, m_warpHaloColor ) )
		{
			m_glows->Clear();
			for( auto it = m_singleBoosters.begin(); it != m_singleBoosters.end(); ++it )
			{
				CreateFlares( *it );
			}
			m_glows->Rebuild();
		}
		else if( IsMatch( value, m_staticTrailLength ) )
		{
			float step = m_staticTrailLength / ( EVE_MAX_CONTROL_POINT_COUNT - 1 );
			
			for( unsigned int i = 0; i <EVE_MAX_CONTROL_POINT_COUNT; ++i )
			{
				m_trailsStaticOffsets[i] = Vector3( 0.f, 0.f, -step * i );
			}
		}
	}
	return true;
}


// --------------------------------------------------------------------------------
// Description:
//   Does a lot of calculations for the boosters and trails based on the
//   movement of the parent (aka the ship). Querying some parameters
//   from the provided balls.
// Arguments:
//   deltaT - time since last frame
//   t - global time
//   parentMatrix - world matrix of the parent holding this booster (aka a ship!)
//   parentSpeed - parent object speed
//   parentAcceleration - parent object acceleration
//   parentRotation - parent object rotation
// --------------------------------------------------------------------------------
void EveBoosterSet2::Update( float deltaT, Be::Time t, const Matrix& parentMatrix, float parentSpeed, const Vector3& parentAcceleration, const Quaternion& parentRotation, int boosterInstance )
{
	if( m_boosterRenderables.size() == 0 )
	{
		// We need to have at least one
		EveBoosterSet2RenderablePtr renderable;
		renderable.CreateInstance();
		renderable->m_boosterSet = this;
		m_boosterRenderables.Append( renderable->GetRawRoot() );
	}
	if( (unsigned)boosterInstance > m_boosterRenderables.size() )
	{
		return;
	}
	m_boosterRenderables[boosterInstance]->Update( deltaT, t, parentMatrix, parentSpeed, parentAcceleration, parentRotation );
}

// --------------------------------------------------------------------------------
// Description:
//   Does a lot of calculations for trails based on the movement of the parent (aka 
//   the ship). Calling the function that calculates all the spline data.
// Arguments:
//   deltaT - time since last frame
//   t - global time
// --------------------------------------------------------------------------------
void EveBoosterSet2::UpdateTrails( float deltaT, Be::Time t )
{
	if( m_trails && g_eveSpaceObjectTrailsEnabled )
	{
		for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
		{
			(*it)->UpdateTrails( deltaT, t );
		}
		m_trails->Update( t );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Clear all the individual boosters & trails this set was holding so far.
// --------------------------------------------------------------------------------
void EveBoosterSet2::Clear()
{
	// clear everything
	m_singleBoosters.clear();
	if( m_glows )
	{
		m_glows->Clear();
	}
	if( m_trails )
	{
		m_trails->Clear();
	}

	// no bounding sphere
	BoundingSphereInitialize( m_boosterBoundingSphere );
	// TODO: BoundingBoxInitialize( m_trailsBoundsMin, m_trailsBoundsMax );

	// also release the resources
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
void EveBoosterSet2::Add( const Matrix* localMatrix, const Vector4* functionality, bool hasTrail, uint32_t atlasIndex0, uint32_t atlasIndex1, float lightScale )
{
	// keep it in our list of boosters
	SingleBoosterData sbd;
	sbd.transform = *localMatrix;
	sbd.functionality = *functionality;
	Vector3 lightOffset( 0.f, 0.f, -m_lightOffset );
	sbd.lightPosition = TransformCoord( lightOffset, *localMatrix );
	sbd.lightRadius = std::max( Length( localMatrix->GetX() ), Length( localMatrix->GetY() ) ) * lightScale;
	sbd.lightPhase = float( g_lightNoiseSize ) * float( rand() ) / float( RAND_MAX );
	sbd.atlasIndex0 = atlasIndex0;
	sbd.atlasIndex1 = atlasIndex1;
	m_singleBoosters.push_back( sbd );

	Vector3 pos( localMatrix->_41, localMatrix->_42, localMatrix->_43 );
	float scale = std::max( Length( localMatrix->GetX() ), Length( localMatrix->GetY() ) );

	if( m_glows )
	{
		CreateFlares( sbd );
	}

	// also add it to the trails
	if( m_trails )
	{
		if( hasTrail )
		{

			Matrix offset = *localMatrix;
			offset.GetTranslation() -= offset.GetZ() * 0.5f;
			m_trails->Add( &offset, scale );
		}
	}


	// add to bounding sphere (WARNING: this builds an exact bounding sphere, only
	// containing the points of the boosters, NOT their size! This will be handled
	// in ::GetRenderables()
	BoundingSphereUpdate( pos, m_boosterBoundingSphere );

	// keep the biggset one around for comparison in the shaer etc.
	if( scale > m_maxSize )
	{
		m_maxSize = scale;
	}
}

void EveBoosterSet2::CreateFlares( SingleBoosterData& boosterData )
{
	auto localMatrix = boosterData.transform;
	// grab pos/dir/scale from the local transform matrix
	Vector3 pos( localMatrix._41, localMatrix._42, localMatrix._43 );
	Vector3 dir( localMatrix._31, localMatrix._32, localMatrix._33 );
	float scale = std::max( Length( localMatrix.GetX() ), Length( localMatrix.GetY() ) );

	dir = Normalize( dir );
	if( scale < 3.f )
	{
		dir *= scale / 3.f;
	}

	float seed = float( rand() ) / float( RAND_MAX ) * 0.7f;

	Vector3 spritePos = pos - 2.5f * dir;
	m_glows->Add( spritePos, seed, seed, scale * m_glowScale, scale * m_glowScale, 0.0f,
		m_glowColor, m_warpGlowColor );

	spritePos = pos - 3.0f * dir;
	m_glows->Add( spritePos, seed, 1.0f + seed, scale * m_symHaloScale, scale * m_symHaloScale, 0.0f,
		m_haloColor, m_warpHaloColor );

	spritePos = pos - 3.01f * dir;
	m_glows->Add( spritePos, seed, 1.0f + seed, scale * m_haloScaleX, scale * m_haloScaleY, 0.0f,
		m_haloColor, m_warpHaloColor );
}

// --------------------------------------------------------------------------------
// Description:
//   Set all internal data of this set
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetData( 
	float glowScale, 
	const Color* glowColor, 
	const Color* warpGlowColor, 
	float symHaloScale, 
	float haloScaleX, 
	float haloScaleY, 
	const Color* haloColor, 
	const Color* warpHaloColor, 
	bool alwaysOn )
{
	m_glowScale = glowScale;
	m_glowColor = *glowColor;
	m_warpGlowColor = *warpGlowColor;
	m_symHaloScale = symHaloScale;
	m_haloScaleX = haloScaleX;
	m_haloScaleY = haloScaleY;
	m_haloColor = *haloColor;
	m_warpHaloColor = *warpHaloColor;
	m_alwaysOn = alwaysOn;
}

// --------------------------------------------------------------------------------
// Description:
//   Assigns dynamic lighting parameters
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetLightData( float offset, float flickerAmplitude, float flickerFrequency, float radius, const Color& color, float warpRadius, const Color& warpColor )
{
	m_lightOffset = offset;
	m_lightFlickerAmplitude = flickerAmplitude;
	m_lightFlickerFrequency = flickerFrequency;
	m_lightRadius = radius;
	m_lightColor = color;
	m_lightWarpRadius = warpRadius;
	m_lightWarpColor = warpColor;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the main effect of this set from the outside
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetEffect( Tr2Effect* effect, Tr2Effect* effectFar )
{
	m_effect = effect;
	m_effectFar = effectFar;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the glow (spriteset) of this set from the outside
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetGlow( EveSpriteSetPtr glow )
{
	m_glows = glow;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the trail (trailset) of this set from the outside
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetTrail( EveTrailsSetPtr trail )
{
	m_trails = trail;
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration and free
//   all the vertex buffer
// --------------------------------------------------------------------------------
void EveBoosterSet2::ReleaseResources( TriStorage s )
{
	m_instanceBuffer = Tr2BufferAL();
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration for the instanced
//   rendering and create and fill the vertex buffers
// --------------------------------------------------------------------------------
bool EveBoosterSet2::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	static Tr2VertexDefinition s_boosterInstancedVertex;
	if( s_boosterInstancedVertex.empty() )
	{
		Tr2VertexDefinition& vd = s_boosterInstancedVertex;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_2, vd.TEXCOORD, 0 );
		// stream 1
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 1, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 2, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 3, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 4, 1, 1 );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD, 5, 1, 1 );
		vd.Add( vd.FLOAT32_1, vd.TEXCOORD, 6, 1, 1 );
		vd.Add( vd.FLOAT32_2, vd.TEXCOORD, 7, 1, 1 );
	}

	// create vertex-declarartion
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_boosterInstancedVertex );
	if(	m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	// create star-shape geometry as "indexed" geometry
	if( Tr2Renderer::GetShaderModel() >= TR2SM_3_0_HI )
	{
		m_vertexBuffer = Tr2ProceduralBuffer( BlueSharedString( "BoosterBoxVB" ), GetBoxVB );
	}
	else
	{
		m_vertexBuffer = Tr2ProceduralBuffer( BlueSharedString( "BoosterStarVB" ), GetStarVB );
	}

	// now build the "instance" buffer, which depends on the actual number of booster, this set currently holds
	RebuildInstanceData( renderContext );

	if( m_trails )
	{
		m_trails->PrepareResources();
	}

	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Rebuild the instance vertex buffer. Must even re-create it because the
//   number of individual boosters might has changed.
// --------------------------------------------------------------------------------
void EveBoosterSet2::RebuildInstanceData( Tr2RenderContext& /*renderContext*/ )
{
	// get rid of old one
	m_instanceBuffer = Tr2BufferAL();

	// something there?
	if( m_singleBoosters.empty() )
	{
		return;
	}

	// how many indiviual boosters are in this set?
	unsigned int boosterCount = (unsigned int)m_singleBoosters.size();

	// create and fill with star-shape's position and some random-value
	std::vector<InstanceVertex> vertices( boosterCount );
	for( unsigned int i = 0; i < boosterCount ; ++i )
	{
		vertices[i].transform = m_singleBoosters[i].transform;
		vertices[i].wavePhase = (float)rand() / (float)RAND_MAX;
		vertices[i].functionality = m_singleBoosters[i].functionality;
		vertices[i].atlasIndex0 = float( m_singleBoosters[i].atlasIndex0 );
		vertices[i].atlasIndex1 = float( m_singleBoosters[i].atlasIndex1 );
	}
	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN( m_instanceBuffer.Create(	
		sizeof( InstanceVertex ),
		boosterCount, 
		Tr2GpuUsage::VERTEX_BUFFER,
		Tr2CpuUsage::NONE,
		&vertices[0], 
		renderContext ) );
}


void EveBoosterSet2::UpdateVisibility( const TriFrustum& frustum )
{
	m_glowsVisible = false;
	if( m_display )
	{
		for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
		{
			(*it)->UpdateVisibility( frustum );
		}
		if( m_glows )
		{
			for( auto& renderable : m_boosterRenderables )
			{
				if( m_glows->UpdateVisibility( frustum, renderable->m_parentTransform, nullptr, 0 ) )
				{
					m_glowsVisible = true;
					break;
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Put this object on the list, since it
//   is an ITr2Renderable.
//   Also get the renderables from the fire effects, if we are firing.
// Arguments:
//   frustum - the current view frustum of the current frame
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable, EveStretch
// --------------------------------------------------------------------------------
void EveBoosterSet2::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display )
	{
		return;
	}

	// add this object (which is a renderable), if it is visible
	if( m_effect )
	{
		for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
		{
			(*it)->GetRenderables( renderables );
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   Return the average overall intensity of this booster set. Has been calculated
//   in ::Update()
// Return value:
//   The intensity
// --------------------------------------------------------------------------------
float EveBoosterSet2::GetBoosterIntensity() const
{
	float intensity = 0.f;
	// Use average, consider using an index
	float oneOverCount = 1.f / (float)m_boosterRenderables.size();
	for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
	{
		intensity += (*it)->GetIntensity() * oneOverCount;
	}
	return intensity;
}

// --------------------------------------------------------------------------------
// Description:
//   Return the overall intensity of an instance of this booster set. Has been calculated
//   in ::Update()
// Return value:
//   The intensity
// --------------------------------------------------------------------------------
float EveBoosterSet2::GetBoosterIntensity( int index ) const
{
	float intensity = 0.f;
	if( (unsigned)index < m_boosterRenderables.size() )
	{
		intensity = m_boosterRenderables[index]->GetIntensity();
	}
	return intensity;
}


// --------------------------------------------------------------------------------
// Description:
//   Render debug info of this turret set: bounding sphere
// --------------------------------------------------------------------------------
void EveBoosterSet2::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
	{
		for( uint32_t j = 0; j < m_singleBoosters.size(); ++j )
		{
			Matrix transform = m_singleBoosters[j].transform * ( *it )->m_parentTransform;
			renderer.DrawCylinder( 
				Tr2DebugObjectReference( this, j ), 
				transform, 
				Vector3( 0, 0, 0 ), 
				Vector3( 0, 0, -1 ), 
				1.0f, 
				8, 
				ITr2DebugRenderer2::Lit, 
				Tr2DebugColor( 0x88ffff00, 0x22ffff00 ) );
		}

		// trails box
		renderer.DrawBox( this, (*it)->m_trailsBoundsMin, (*it)->m_trailsBoundsMax, ITr2DebugRenderer2::Wireframe, 0xff00ffff );

		if( m_glows )
		{
			m_glows->RenderDebugInfo( renderer, ( *it )->m_parentTransform, nullptr, 0 );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Transform and modify the saved bounding sphere, so it can be used for
//   culling etc.
// --------------------------------------------------------------------------------
void EveBoosterSet2::GetBoundingSphere( Vector4& boundingSphere ) const
{
	BoundingSphereInitialize( boundingSphere );
	for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
	{
		Vector4 bs;
		(*it)->GetBoundingSphere( bs );
		BoundingSphereUpdate( bs, boundingSphere );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Registers glow sprites with quad renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveBoosterSet2::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_glows )
	{
		m_glows->RegisterWithQuadRenderer( quadRenderer );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds glow sprites to quad renderer.
// Arguments:
//   quadRenderer - quad renderer
//   world - parent local to world transform
// --------------------------------------------------------------------------------
void EveBoosterSet2::AddToQuadRenderer( Tr2QuadRenderer& quadRenderer, const Matrix& world )
{
	if( !m_glows || !m_glowsVisible || !m_display )
	{
		return;
	}

	for( auto it = m_boosterRenderables.begin(); it != m_boosterRenderables.end(); it++ )
	{
		if( (*it)->m_boosterLOD > g_eveSpaceSceneLowDetailThreshold || !m_flareLodEnabled )
		{
			m_glows->AddBoosterGlowToQuadRenderer( quadRenderer, (*it)->m_parentTransform, (*it)->m_overallIntensity, m_warpIntensity );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds lights from boosters to light manager
// Arguments:
//   lightManager - light manager
//   parentTransform - parent local to world transform
// --------------------------------------------------------------------------------
void EveBoosterSet2::GetLights( Tr2LightManager& lightManager, const Matrix& parentTransform ) const
{
	if( ( m_lightRadius <= 0.f && m_lightWarpRadius <= 0.f ) )
	{
		return;
	}

	
	for( auto dit = m_boosterRenderables.begin(); dit != m_boosterRenderables.end(); dit++ )
	{
		if( (*dit)->m_overallIntensity <= 0 )
		{
			continue;
		}

		float warpIntensity = std::min( std::max( m_warpIntensity, 0.f ), 1.f );
		float radiusFactor = m_lightRadius * ( 1.f - warpIntensity ) + m_lightWarpRadius * warpIntensity;
		radiusFactor *= (*dit)->m_overallIntensity;
		Color color = m_lightColor * ( 1.f - warpIntensity ) + m_lightWarpColor * warpIntensity;
		XMMATRIX transform = (*dit)->m_parentTransform;
		for( auto it = std::begin( m_singleBoosters ); it != std::end( m_singleBoosters ); ++it )
		{
			float phase = ( it->lightPhase + Tr2Renderer::GetAnimationTime() ) * m_lightFlickerFrequency;
			float p0 = g_lightNoise[int( phase ) % g_lightNoiseSize];
			float p1 = g_lightNoise[( int( phase ) + 1 ) % g_lightNoiseSize];
			float t = phase - std::floor( phase );
			float flicker = 1 + m_lightFlickerAmplitude * 2.0f * ( p0 * ( 1.0f - t ) + p1 * t ) - m_lightFlickerAmplitude;
			lightManager.AddPointLight( 
				Vector3( XMVector3TransformCoord( it->lightPosition, transform ) ), 
				it->lightRadius * radiusFactor,
				color * flicker );

		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveBoosterSetPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	FillAndSetConstants( *buffers[VERTEX_SHADER], m_vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER ], m_psData, PIXEL_SHADER , Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}
