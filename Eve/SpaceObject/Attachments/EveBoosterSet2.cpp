#include "StdAfx.h"
#include "EveBoosterSet2.h"

#include "Utilities/BoundingSphere.h"
#include "Utilities/BoundingBox.h"
#include "Utilities/ViewDistanceInfo.h"
#include "Tr2Effect.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "TriSettingsRegistrar.h"
#include "Curves/TriVectorCurve.h"
#include "Include/TriMath.h"

#include "EveSpriteSet.h"
#include "EveTrailsSet.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );

// constants
// how many planes per booster tree structure?
const unsigned int EVE_BOOSTER_PLANES_COUNT = 4;

// vertex decl for multi-stream rendering

// geometry of tree-shape structure for boosters
static EveBoosterSet2::BoosterVertex s_starShapeMesh[4 * EVE_BOOSTER_PLANES_COUNT];

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


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, build the tree-shape geometry we will use for
//   rendering the boosters
// --------------------------------------------------------------------------------
EveBoosterSet2::EveBoosterSet2( IRoot* lockobj ) :
	m_glowColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_haloColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_display( true ),
	m_physicsUpdate( true ),
	m_destinyUpdate( true ),
	m_alwaysOn( false ),
	m_drawDebugInfo( false ),
	m_alwaysOnIntensity( 1.0f ),
	m_boosterLOD( 0.f ),
	m_trailsLOD( 0.f ),
	m_parentSpeed( 0.f ),
	m_parentWarp( 0.f ),
	m_parentRotation( 0.f, 0.f, 0.f, 1.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_maxVel( 250.f ),
	m_lastAccFactor( 0.f ),
	m_lastValue( 0.f ),
	m_overallIntensity( 0.f ),
	m_maxSize( 0.f ),
	m_glowScale( 1.f ),
	m_symHaloScale( 1.f ),
	m_haloScaleX( 1.f ),
	m_haloScaleY( 1.f ),
	m_trailIntensity( 0.f ),
	m_trailsSmoothing( 10.f ),
	m_trailsTotalLength( 0.f ),
	m_trailsTimeToNext( 0.f ),
	m_trailsTimeDelta( 1.f ),
	m_trailsOffsetLatest( 0 ),
	m_trailsOffsetAccu( 0.f, 0.f, 0.f )
{
	// 0
	D3DXMatrixIdentity( &m_parentTransform );
	BoundingSphereInitialize( m_boosterBoundingSphere );
	BoundingBoxInitialize( m_trailsBoundsMin, m_trailsBoundsMax );

	// pre-build star-shape geometry
	BoosterVertex* p = &s_starShapeMesh[0];
	for( unsigned int i = 0; i < EVE_BOOSTER_PLANES_COUNT ; ++i )
	{
		float t = (float)i * XM_PI / (float)EVE_BOOSTER_PLANES_COUNT;
		float x = cos( t ) * 0.5f;
		float y = sin( t ) * 0.5f;
		p->position = Vector3( -x, -y, 0.f );
		p->texCoord = Vector2( 1.f, 1.f );
		++p;							   
		p->position = Vector3( -x, -y, -1.f );
		p->texCoord = Vector2( 1.f, 0.f );
		++p;							   
		p->position = Vector3(  x,  y, -1.f );
		p->texCoord = Vector2( 0.f, 0.f );
		++p;							   
		p->position = Vector3(  x,  y, 0.0f );
		p->texCoord = Vector2( 0.f, 1.f );
		++p;
	}

	// "invalidate" all trail control positions
	for( unsigned int i = 0; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		m_trailsControlPositions[ i ] = Vector3( 0.f, 0.f, 0.f );
		m_trailsControlNormals[ i ] = Vector3( 0.f, 0.f, -1.f );
		m_trailsControlNormalsFactor[ i ] = 1.f;
		m_trailsSequenceLength[ i ] = 0.f;
		m_trailsStaticOffsets[ i ] = Vector3( 0.f, 0.f, 0.f );
	}

	m_trailsOffsets = (XMVECTOR*)CCP_ALIGNED_MALLOC( "EveBoosterSet2::m_trailsOffsets", sizeof( XMVECTOR ) * EVE_MAX_POSITION_OFFSET_COUNT, 16 );
	memset( m_trailsOffsets, 0, EVE_MAX_POSITION_OFFSET_COUNT * sizeof( XMVECTOR ) );
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveBoosterSet2::~EveBoosterSet2()
{
	CCP_ALIGNED_FREE( m_trailsOffsets );
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

// --------------------------------------------------------------------------------
// Description:
//   If someone changed some data of the boosters we must re-create
// --------------------------------------------------------------------------------
bool EveBoosterSet2::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_glowScale ) )
	{
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   This function calculates the booster intensity (aka "gain") from the given
//   ball and the given time. How it calculates this comes from the early days
//   of EVE and is "just" some black magic...
// Arguments:
//   ball - ball holding all the physical movement data of the ship
//   t - global time
// Return value:
//   Returns the intensity of the booster (from 0.f to something a little bit
//   higher than 1.f (because of afterburners and microwarps)
// --------------------------------------------------------------------------------
float EveBoosterSet2::CalculateIntensity( ITriVectorFunctionPtr ball, Be::Time t )
{
	// if we want the boosters to be permanently visible
	if ( m_alwaysOn )
	{
		return m_alwaysOnIntensity;
	}

	// no ball, no gain! Cause we don't have data to build on...
	if( !ball )
	{
		return 0.f;
	}

	Vector3 velocity = Vector3( 0.f, 0.f, 0.f );
	Vector3 acceleration = Vector3( 0.f, 0.f, 0.f );
	Quaternion rotation = Quaternion( 0.f, 0.f, 0.f, 1.f );

	ball->GetValueDotAt( &velocity, t );
	ball->GetValueDoubleDotAt( &acceleration, t );
	ITriQuaternionFunctionPtr getQ( BlueCastPtr( ball ) );
	if( getQ )
	{
		getQ->GetValueAt( &rotation, t );
	}
	Vector3 backwd;
	TriVectorRotatedBasisQuaternion( &backwd, TRITA_Z, &rotation );
	float speedRatio = m_maxVel ? ( D3DXVec3Length( &velocity ) / m_maxVel ) : 0.f;
	float accFactor = D3DXVec3Dot( &acceleration, &backwd );
	accFactor *= max( 0.3f, speedRatio );
	if( accFactor < 0.f )
	{
		accFactor = 0.f;
	}
	else if( accFactor > 1.f )
	{
		accFactor = 1.f;
	}

	// dampening (like drum noise)
	accFactor = accFactor * 0.2f + m_lastAccFactor * 0.8f;
	m_lastAccFactor = accFactor;
	float value = m_lastValue * 0.8f + ( 0.5f * speedRatio  + 0.5f * accFactor ) * 0.2f;
	value = min( value, 2.0f );
	m_lastValue = value;

	return value;
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
//   ballPosition - destiny object for translation
//   ballRotation - destiny object for rotation
// --------------------------------------------------------------------------------
void EveBoosterSet2::Update( float deltaT, Be::Time t, const Matrix* parentMatrix, ITriVectorFunctionPtr ballPosition, ITriQuaternionFunctionPtr ballRotation )
{
	// can(!) get the speed from destiny's position ball
	if( m_destinyUpdate )
	{
		// ask destiny ball for speed
		if( ballPosition )
		{
			Vector3 velocity;
			ballPosition->GetValueDotAt( &velocity, t );
			m_parentSpeed = D3DXVec3Length( &velocity );
		}
	}
	else
	{
		// if no time has elapsed, no speed calculation is possible at all
		if( deltaT != 0.f )
		{
			// rely on actual position data
			Vector3 oldPos( m_parentTransform._41, m_parentTransform._42, m_parentTransform._43 );
			Vector3 newPos( parentMatrix->_41, parentMatrix->_42, parentMatrix->_43 );
			Vector3 dir( newPos - oldPos );
			m_parentSpeed = D3DXVec3Length( &dir ) / deltaT;
		}
	}

	// keep the transform of the parent (aka ship) around
	m_parentTransform = *parentMatrix;

	// can get the rotational speed from destiny's rotation ball
	if( ballRotation )
	{
		ballRotation->GetValueAt( &m_parentRotation, t );
	}

	// update the intensity, which is based on ship's movement
	m_overallIntensity = CalculateIntensity( ballPosition, t );
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
	// do we have trails?
	if( m_trails )
	{
		// are they enabled?
		if( g_eveSpaceObjectTrailsEnabled )
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
				m_trailIntensity = SinSmooth( ( g_eveSpaceObjectTrailsMaxLength - m_trailsTotalLength ) / g_eveSpaceObjectTrailsMaxLength );
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
			if( m_alwaysOn )
			{
				m_trailIntensity = 1.f;
			}

			m_trails->Update( t );
		}
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
	BoundingBoxInitialize( m_trailsBoundsMin, m_trailsBoundsMax );

	// also release the resources
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
// Description:
//   Add a new individual booster to this at a specific position/orientation.
//   First we add it to the internal booster list and then we must update the
//   lensflares handled in the EveSpriteSet.
//   Also add a trail and keep track of the biggest size
// Arguments:
//   localMatrix - position/orientation of single turret in object-space
//   parentLocatorName - name of the ship's locator this new booster belongs to
// SeeAlso:
//   EveSpriteSet
// --------------------------------------------------------------------------------
void EveBoosterSet2::Add( const Matrix* localMatrix, const Vector4* functionality )
{
	// keep it in our list of boosters
	SingleBoosterData sbd;
	sbd.transform = *localMatrix;
	sbd.functionality = *functionality;
	m_singleBoosters.push_back( sbd );

	// grab pos/dir/scale from the local transform matrix
	Vector3 pos( localMatrix->_41, localMatrix->_42, localMatrix->_43 );
	Vector3 dir( localMatrix->_31, localMatrix->_32, localMatrix->_33 );
	float scale = D3DXVec3Length( &dir );

	// also add it to all the lensflares
	if( m_glows )
	{
		Vector3 spritePos = pos - 2.5f * dir;
		m_glows->Add( spritePos, 0.0f, 0.0f, scale * m_glowScale, scale * m_glowScale, 0.0f, m_glowColor );

		spritePos = pos - 3.0f * dir;
		m_glows->Add( spritePos, 0.0f, 1.0f, scale * m_symHaloScale, scale * m_symHaloScale, 0.0f, m_haloColor );

		spritePos = pos - 3.01f * dir;
		m_glows->Add( spritePos, 0.0f, 1.0f, scale * m_haloScaleX, scale * m_haloScaleY, 0.0f, m_haloColor );
	}

	// also add it to the trails
	if( m_trails )
	{
		m_trails->Add( localMatrix, scale );
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

// --------------------------------------------------------------------------------
// Description:
//   Set all internal data of this set
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetData( float glowScale, const Color* glowColor, float symHaloScale, float haloScaleX, float haloScaleY, const Color* haloColor, bool alwaysOn )
{
	m_glowScale = glowScale;
	m_glowColor = *glowColor;
	m_symHaloScale = symHaloScale;
	m_haloScaleX = haloScaleX;
	m_haloScaleY = haloScaleY;
	m_haloColor = *haloColor;
	m_alwaysOn = alwaysOn;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the main effect of this set from the outside
// --------------------------------------------------------------------------------
void EveBoosterSet2::SetEffect( Tr2EffectPtr effect )
{
	m_effect = effect;
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
	m_vertexBuffer.Destroy();
	m_instanceBuffer.Destroy();
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
	}

	// create vertex-declarartion
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_boosterInstancedVertex );
	if(	m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	// create star-shape geometry as "indexed" geometry
	CR_RETURN_VAL(	
		m_vertexBuffer.Create(	4 * EVE_BOOSTER_PLANES_COUNT * sizeof( BoosterVertex ), 
								USAGE_IMMUTABLE, 
								&s_starShapeMesh[0], 
								renderContext )
		, false );

	// now build the "instance" buffer, which depends on the actual number of booster, this set currently holds
	RebuildInstanceData( renderContext );

	if( m_glows )
	{
		m_glows->PrepareResources();
	}

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
	m_instanceBuffer.Destroy();

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
	}
	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN(	m_instanceBuffer.Create(	
					boosterCount * sizeof( InstanceVertex ), 
					USAGE_IMMUTABLE, 
					&vertices[0], 
					renderContext ) );
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
void EveBoosterSet2::GetRenderables( const TriFrustum& frustum, std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display )
	{
		return;
	}

	// add this object (which is a renderable), if it is visible
	if( m_effect )
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
			float d = D3DXVec3LengthSq( &tmp );
			if( d < sqDist )
			{
				sqDist = d;
				cntrPosIdx = i;
			}
		}
		Vector4 tmp( m_trailsControlPositions[ cntrPosIdx ], transformedBoundingSphere.w );
		m_trailsLOD = 7.5f * frustum.GetPixelSizeAccross( &tmp );

		if( frustum.IsSphereVisible( &transformedBoundingSphere ) || frustum.IsBoxVisible( m_trailsBoundsMin, m_trailsBoundsMax ) )
		{
			renderables.push_back( this );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   No transparency.
// --------------------------------------------------------------------------------
bool EveBoosterSet2::HasTransparentBatches()
{
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Only have additive batches via a geometry provider, since we are using
//   instanced rendering.
// --------------------------------------------------------------------------------
void EveBoosterSet2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE )
	{
		return;
	}
	if( !m_display )
	{
		return;
	}
	if( !m_instanceBuffer.IsValid() )
	{
		return;
	}
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
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
			batch->SetShaderMaterial( m_effect );
			batch->SetGeometryProvider( this );
			batches->Commit( batch );
		}

		if( m_glows )
		{
			m_glows->GetBatches( batches, perObjectData );
		}
	}

	if( m_trailsLOD > g_eveSpaceSceneLowDetailThreshold )
	{
		if( m_trails )
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
						m_trails->GetBatches( batches, perObjectData );
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
float EveBoosterSet2::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill the per-object data. First the world matrix of the parent-ship.
// SeeAlso:
//   EveBoosterSetPerObjectData, TriRenderBatchAccumulator
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveBoosterSet2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveBoosterSetPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}

	// column_major for shaders
	D3DXMatrixTranspose( &perObjectData->m_vsData.shipMatrix, &m_parentTransform );

	// vs data
	perObjectData->m_vsData.boosterIntensity = m_overallIntensity;
	perObjectData->m_vsData.shipSpeed = m_parentSpeed;
	perObjectData->m_vsData.maxBoosterSize = m_maxSize;
	// ps data
	perObjectData->m_psData.boosterIntensity = m_overallIntensity;
	perObjectData->m_psData.trailIntensity = m_trailIntensity;
	perObjectData->m_psData.shipWarp = m_parentWarp;

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
void EveBoosterSet2::SubmitGeometry( Tr2RenderContext& renderContext )
{
	// how many indiviual boosters are in this set?
	unsigned int boosterCount = (unsigned int)m_singleBoosters.size();

	Tr2IndexBufferAL* indexBuffer = Tr2Renderer::GetQuadListIndexBuffer( EVE_BOOSTER_PLANES_COUNT );
	if( !indexBuffer )
	{
		return;
	}

	// decl & index
	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyIndexBuffer( *indexBuffer );
	// stream0: "indexed", the star shape	
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( BoosterVertex ) );
	// stream1: "instance", the star shape' position	
	renderContext.m_esm.ApplyStreamSource( 1, m_instanceBuffer, 0, sizeof( InstanceVertex ) );
	// draw	
	renderContext.SetTopology( TOP_TRIANGLES );	
	renderContext.DrawIndexedInstanced( 4 * EVE_BOOSTER_PLANES_COUNT, 0, 2 * EVE_BOOSTER_PLANES_COUNT, boosterCount );
}

// --------------------------------------------------------------------------------
// Description:
//   Return the overall intensity of this booster set. Has been calculated
//   in ::Update()
// Return value:
//   The intensity
// --------------------------------------------------------------------------------
float EveBoosterSet2::GetBoosterIntensity() const
{
	return m_overallIntensity;
}

// --------------------------------------------------------------------------------
// Description:
//   Updates the ringbuffer based on the new offset, then based on that ringbuffer
//   we calculate splinepoints and normals.
// Arguments:
//   deltaT - time since last frame
// --------------------------------------------------------------------------------
void EveBoosterSet2::CalculateSplineData( float deltaT )
{
	// time MUST elapse
	if( deltaT <= 0.f )
	{
		return;
	}

	// where are we?
	Vector3 parentPos( 0.f, 0.f, 0.f );
	D3DXVec3TransformCoord( &parentPos, &parentPos, &m_parentTransform );

	// what dir are we moving?
	Vector3 movementDir( 0.f, 0.f, 1.f );
	TriVectorRotateQuaternion( &movementDir, &movementDir, &m_parentRotation );
	// how far did we get since last call?
	movementDir *= deltaT * m_parentSpeed;

	if( m_physicsUpdate )
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
				if( D3DXVec3Dot( &m_trailsOffsetAccu, &m_trailsOffsetAccu ) > 0.00001f )
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
			m_trailsControlPositions[ i ] =  parentPos + m_trailsOffsets[ ringIdx ];

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
			TriVectorRotateMatrix( &rotatedOffset, &m_trailsStaticOffsets[ i ], &m_parentTransform );
			m_trailsControlPositions[ i ] =  parentPos + rotatedOffset;
		}
	}






	// cycle over all control pints and determine trails total length
	m_trailsTotalLength = 0.f;
	for( unsigned int i = 1; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		Vector3 sequenceDir = m_trailsControlPositions[ i ] - m_trailsControlPositions[ i - 1 ];
		m_trailsTotalLength += D3DXVec3Length( &sequenceDir );
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
	Vector3 firstTangent( 0.f, 0.f, -1.f * m_trailsSmoothing );
	D3DXVec3TransformNormal( &m_trailsControlNormals[ 0 ], &firstTangent, &m_parentTransform );

	// last tangent is always from before-last to last point
	Vector3 lastDir = m_trailsControlPositions[ EVE_MAX_CONTROL_POINT_COUNT - 1 ] - m_trailsControlPositions[ EVE_MAX_CONTROL_POINT_COUNT - 2 ];
	m_trailsControlNormals[ EVE_MAX_CONTROL_POINT_COUNT - 1 ] = 0.5 * lastDir;

	// the rest is calculated according to c-r (or something...)
	for( unsigned int i = 1; i < EVE_MAX_CONTROL_POINT_COUNT - 1; ++i )
	{
		// from -1 to +1
		Vector3 dir = m_trailsControlPositions[ i + 1 ] - m_trailsControlPositions[ i - 1 ];
		D3DXVec3Normalize( &m_trailsControlNormals[ i ], &dir );
		// adjust length!
		Vector3 t0( m_trailsControlPositions[ i + 1 ] - m_trailsControlPositions[ i ] );
		Vector3 t1( m_trailsControlPositions[ i ] - m_trailsControlPositions[ i - 1 ] );
		float len0 = D3DXVec3Length( &t0 );
		float len1 = D3DXVec3Length( &t1 );
		// keep the normal add len0 and store a factor to calc a len1-normal
		m_trailsControlNormals[ i ] *= len0;
		m_trailsControlNormalsFactor[ i ] = len1 / len0;
	}

	for( unsigned int i = 1; i < EVE_MAX_CONTROL_POINT_COUNT; ++i )
	{
		// "sequence length" value is the length of this segment normalized along the whole trail
		Vector3 segment( m_trailsControlPositions[ i ] - m_trailsControlPositions[ i - 1 ] );
		m_trailsSequenceLength[ i ] = D3DXVec3Length( &segment );
		// normalize if non-zero length
		if( m_trailsTotalLength > 0.f )
		{
			m_trailsSequenceLength[ i ] /= m_trailsTotalLength;
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Render debug info of this turret set: bounding sphere
// --------------------------------------------------------------------------------
void EveBoosterSet2::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	if( m_drawDebugInfo )
	{
		// booster sphere
		Vector4 transformedBoundingSphere;
		GetBoundingSphere( transformedBoundingSphere );
		Tr2Renderer::DrawSphere( transformedBoundingSphere, 10, 0xff00ffff );

		// trails box
		Tr2Renderer::DrawBox( m_trailsBoundsMin, m_trailsBoundsMax, 0xff00ffff );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Transform and modify the saved bounding sphere, so it can be used for
//   culling etc.
// --------------------------------------------------------------------------------
void EveBoosterSet2::GetBoundingSphere( Vector4& boundingSphere ) const
{
	// move bounding sphere back to catch all the glowy exhaust
	boundingSphere = m_boosterBoundingSphere + Vector4( 0.f, 0.f, -0.5f * m_boosterBoundingSphere.w, 0.f );
	// transform center into worldspace
	D3DXVec3TransformCoord( (Vector3*)&boundingSphere, (Vector3*)&boundingSphere, &m_parentTransform );
	// blow up radius so we contain all the glowy stuff coming out of a booster
	boundingSphere.w = 2.f * m_boosterBoundingSphere.w;
}

void EveBoosterSet2::UpdateViewDistanceInfo( const TriFrustum& frustum, ViewDistanceInfo& viewDistance ) const
{
	Vector4 bs;

	BoundingSphereFromBox( bs, m_trailsBoundsMin, m_trailsBoundsMax );
	viewDistance.UpdateClipPlanes( bs, frustum );

	GetBoundingSphere( bs );
	viewDistance.UpdateClipPlanes( bs, frustum );
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