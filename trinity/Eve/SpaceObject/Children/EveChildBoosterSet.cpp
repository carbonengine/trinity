// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveChildBoosterSet.h"

#include "Utilities/BoundingSphere.h"
#include "Shader/Tr2Effect.h"
#include "TriRenderBatch.h"
#include "TriFrustum.h"
#include "Include/TriMath.h"
#include "Eve/SpaceObject/Utils/EveBoosterUtilities.h"
#include "Eve/SpaceObject/Attachments/Sets/EveSpriteSet.h"
#include "Tr2LightManager.h"
#include "Tr2Renderer.h"

using namespace Tr2RenderContextEnum;


// --------------------------------------------------------------------------------
// Description:
//   Initialize data members, build the box-shape geometry we will use for
//   rendering the boosters
// --------------------------------------------------------------------------------
EveChildBoosterSet::EveChildBoosterSet( IRoot* lockobj ) :
	m_glowColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_haloColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_warpGlowColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_warpHaloColor( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_display( true ),
	m_thrust( 0.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_warpIntensity( 0.f ),
	m_maxSize( 0.f ),
	m_glowScale( 1.f ),
	m_symHaloScale( 1.f ),
	m_haloScaleX( 1.f ),
	m_haloScaleY( 1.f ),
	m_flareLodEnabled( true ),
	m_glowsVisible( true ),
	m_lightOffset( 0.f ),
	m_lightRadius( 0.f ),
	m_lightWarpRadius( 0.f ),
	m_lightFlickerAmplitude( 0.f ),
	m_lightFlickerFrequency( 0.f ),
	m_lightColor( 0.f, 0.f, 0.f, 0.f ),
	m_lightWarpColor( 0.f, 0.f, 0.f, 0.f ),
	m_vertexBuffer( MakeChildBoosterBoxBuffer() ),
	m_isVisible( false ),
	m_boostersVisible( false ),
	m_boosterHighLod( false ),
	m_parentTransform( IdentityMatrix() ),
	m_driveName( DEFAULT_DRIVE_NAME ),
	m_parentScale( 1.f )
{
	Tr2Renderer::ReserveQuadListIndexBuffer( 6 );
	BoundingSphereInitialize( m_boosterBoundingSphere );
}

// --------------------------------------------------------------------------------
// Description:
//   Cleanup
// --------------------------------------------------------------------------------
EveChildBoosterSet::~EveChildBoosterSet()
{
}

// --------------------------------------------------------------------------------
// Description:
//   If loading from a .red file, we now can start creating resources
// --------------------------------------------------------------------------------
bool EveChildBoosterSet::Initialize()
{
	PrepareResources();
	return true;
}

bool EveChildBoosterSet::OnModified( Be::Var* value )
{
	if( m_glows )
	{
		if( IsMatch( value, m_glowScale ) || IsMatch( value, m_haloScaleX ) || IsMatch( value, m_haloScaleY ) || IsMatch( value, m_symHaloScale ) ||
			IsMatch( value, m_glowColor ) || IsMatch( value, m_warpGlowColor ) || IsMatch( value, m_haloColor ) || IsMatch( value, m_warpHaloColor ) )
		{
			m_glows->Clear();
			for( const auto& booster : m_singleBoosters )
			{
				CreateBoosterFlares( *m_glows, booster.transform, GetFlareParams() );
			}
			m_glows->Rebuild();
		}
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//
// Arguments:
//   updateContext -
// --------------------------------------------------------------------------------
void EveChildBoosterSet::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_ringBufferOffsets.AdvanceFrame();

	if( params.isVisible )
	{
		m_ringBufferData.clear();
		for( const auto& booster : m_singleBoosters )
		{
			Tr2ChildBoosterInstanceData data;
			data.atlasIndex0 = booster.atlasIndex0;
			data.atlasIndex1 = booster.atlasIndex1;
			data.intensity = m_thrust;
			data.transform = Float4x3( booster.transform );
			data.wavePhase = booster.wavePhase;
			m_ringBufferData.push_back( data );
		}
		m_ringBufferOffsets.UploadTransforms( Tr2RingBuffer::GetInstance<Tr2ChildBoosterInstanceData>(), m_ringBufferData.data(), uint32_t( m_ringBufferData.size() ) );
	}

	m_parentTransform = params.localToWorldTransform;

	// scale with highest scale factor
	float scaleXSq = LengthSq( m_parentTransform.GetX() );
	float scaleYSq = LengthSq( m_parentTransform.GetY() );
	float scaleZSq = LengthSq( m_parentTransform.GetZ() );
	float scale = std::max( std::max( scaleXSq, scaleYSq ), scaleZSq );
	m_parentScale = sqrt( scale );

	m_hasUpdated = true;
}

EveBoosterFlareParams EveChildBoosterSet::GetFlareParams() const
{
	return { m_warpGlowColor, m_glowScale, m_glowColor, m_haloScaleX, m_haloScaleY, m_symHaloScale, m_haloColor, m_warpHaloColor };
}

// --------------------------------------------------------------------------------
// Description:
//   Clear all the individual boosters this set was holding so far.
// --------------------------------------------------------------------------------
void EveChildBoosterSet::Clear()
{
	// clear everything
	m_singleBoosters.clear();
	if( m_glows )
	{
		m_glows->Clear();
	}
	m_maxSize = 0.f;

	// no bounding sphere
	BoundingSphereInitialize( m_boosterBoundingSphere );

	// also release the resources
	ReleaseResources( TRISTORAGE_ALL );
}

// --------------------------------------------------------------------------------
void EveChildBoosterSet::Add( const Matrix& localMatrix, uint32_t atlasIndex0, uint32_t atlasIndex1, float lightScale )
{
	Vector3 pos( localMatrix._41, localMatrix._42, localMatrix._43 );
	float scale = std::max( Length( localMatrix.GetX() ), Length( localMatrix.GetY() ) );

	// keep it in our list of boosters
	SingleBoosterData sbd;
	sbd.transform = localMatrix;
	Vector3 lightOffset( 0.f, 0.f, -m_lightOffset );
	sbd.lightPosition = TransformCoord( lightOffset, localMatrix );
	sbd.lightRadius = scale * lightScale;
	sbd.lightPhase = GenerateBoosterLightPhase();
	sbd.atlasIndex0 = atlasIndex0;
	sbd.atlasIndex1 = atlasIndex1;
	sbd.wavePhase = (float)rand() / (float)RAND_MAX;
	m_singleBoosters.push_back( sbd );

	if( m_glows )
	{
		CreateBoosterFlares( *m_glows, sbd.transform, GetFlareParams() );
	}

	// add to bounding sphere (WARNING: this builds an exact bounding sphere, only
	// containing the points of the boosters, NOT their size! This will be handled
	// in ::GetBoundingSphere()
	BoundingSphereUpdate( pos, m_boosterBoundingSphere );

	// keep the biggest one around for comparison in the shader etc.
	if( scale > m_maxSize )
	{
		m_maxSize = scale;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Set all internal data of this set
// --------------------------------------------------------------------------------
void EveChildBoosterSet::SetData(
	float glowScale,
	const Color& glowColor,
	const Color& warpGlowColor,
	float symHaloScale,
	float haloScaleX,
	float haloScaleY,
	const Color& haloColor,
	const Color& warpHaloColor )
{
	m_glowScale = glowScale;
	m_glowColor = glowColor;
	m_warpGlowColor = warpGlowColor;
	m_symHaloScale = symHaloScale;
	m_haloScaleX = haloScaleX;
	m_haloScaleY = haloScaleY;
	m_haloColor = haloColor;
	m_warpHaloColor = warpHaloColor;
}

// --------------------------------------------------------------------------------
// Description:
//   Assigns dynamic lighting parameters
// --------------------------------------------------------------------------------
void EveChildBoosterSet::SetLightData( float offset, float flickerAmplitude, float flickerFrequency, float radius, const Color& color, float warpRadius, const Color& warpColor )
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
void EveChildBoosterSet::SetEffect( Tr2Effect* effect, Tr2Effect* effectFar )
{
	m_effect = effect;
	m_effectFar = effectFar;
}

// --------------------------------------------------------------------------------
// Description:
//   Set the glow (spriteset) of this set from the outside
// --------------------------------------------------------------------------------
void EveChildBoosterSet::SetGlow( EveSpriteSetPtr glow )
{
	m_glows = glow;
}

// --------------------------------------------------------------------------------
// Description:
//   We have to free all device stuff, so release vertex declaration
// --------------------------------------------------------------------------------
void EveChildBoosterSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
}

// --------------------------------------------------------------------------------
// Description:
//   (Re)-allocate all device stuff: create a vertex declaration
// --------------------------------------------------------------------------------
bool EveChildBoosterSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	static Tr2VertexDefinition s_boosterVertex;
	if( s_boosterVertex.empty() )
	{
		Tr2VertexDefinition& vd = s_boosterVertex;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
	}

	// create vertex-declarartion
	m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( s_boosterVertex );
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return false;
	}

	return true;
}

void EveChildBoosterSet::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	m_glowsVisible = false;
	m_isVisible = false;
	m_boostersVisible = false;

	if( !m_hasUpdated )
	{
		return;
	}

	if( m_display )
	{
		Vector4 transformedBoundingSphere;
		GetBoundingSphere( transformedBoundingSphere, BoundingSphereQuery::EVE_BOUNDS_NORMAL );

		auto& frustum = updateContext.GetFrustum();

		// LOD for boosters: use the bounding sphere
		float boosterLOD = 2.f * frustum.GetPixelSizeAccross( &transformedBoundingSphere );
		m_boosterHighLod = boosterLOD > updateContext.GetMediumDetailThreshold() * 1.5f;
		m_boostersVisible = boosterLOD > updateContext.GetLowDetailThreshold();

		m_isVisible = frustum.IsSphereVisible( &transformedBoundingSphere );

		if( m_glows )
		{
			if( m_glows->UpdateVisibility( updateContext, m_parentTransform, nullptr, 0 ) )
			{
				m_glowsVisible = true;
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Standard way of rendering in Trinity. Put this object on the list, since it
//   is an ITr2Renderable.
// Arguments:
//   renderables - a vector for all the renderable we want to render
// SeeAlso:
//   ITr2Renderable
// --------------------------------------------------------------------------------
void EveChildBoosterSet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	// display?
	if( !m_display )
	{
		return;
	}

	// add this object (which is a renderable), if it is visible
	if( m_effect && m_isVisible )
	{
		renderables.push_back( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Transform and modify the saved bounding sphere, so it can be used for
//   culling etc.
// --------------------------------------------------------------------------------
bool EveChildBoosterSet::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( !m_hasUpdated )
	{
		return false;
	}

	sphere = PadBoosterBoundingSphere( m_boosterBoundingSphere, m_parentTransform );
	sphere.w *= m_parentScale;
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//   Registers glow sprites with quad renderer.
// Arguments:
//   quadRenderer - quad renderer
// --------------------------------------------------------------------------------
void EveChildBoosterSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
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
void EveChildBoosterSet::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_glows || !m_glowsVisible || !m_display )
	{
		return;
	}

	if( m_boostersVisible || !m_flareLodEnabled )
	{
		m_glows->AddBoosterGlowToQuadRenderer( quadRenderer, m_parentTransform, m_thrust, m_warpIntensity );
	}
}

void EveChildBoosterSet::SetDriveName( const std::string& driveName )
{
	m_driveName = driveName;
}

void EveChildBoosterSet::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		registry->RegisterComponent<ITr2LightOwner>( this );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds lights from boosters to light manager
// Arguments:
//   lightManager - light manager
// --------------------------------------------------------------------------------
void EveChildBoosterSet::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_hasUpdated )
	{
		return;
	}

	if( m_lightRadius <= 0.f && m_lightWarpRadius <= 0.f )
	{
		return;
	}

	if( m_thrust <= 0 )
	{
		return;
	}

	EveBoosterLightParams params{ m_lightWarpRadius * m_parentScale,
								  m_lightWarpColor,
								  m_lightRadius * m_parentScale,
								  m_lightColor,
								  m_lightFlickerAmplitude,
								  m_lightFlickerFrequency };
	AddBoosterLights( lightManager, m_singleBoosters, m_parentTransform, m_thrust, m_warpIntensity, params );
}

void EveChildBoosterSet::SetControllerVariable( const char* name, float value )
{
	if( name == m_driveName )
	{
		m_thrust = value;
	}
	else if( strcmp( name, EveChildBoosterSet::WARP_DRIVE_NAME ) == 0 )
	{
		m_warpIntensity = value;
	}
}

// --------------------------------------------------------------------------------
// Description:
//   No transparency.
// --------------------------------------------------------------------------------
bool EveChildBoosterSet::HasTransparentBatches()
{
	return false;
}

// --------------------------------------------------------------------------------
// Description:
//   Only have additive batches via a geometry provider, since we are using
//   instanced rendering.
// --------------------------------------------------------------------------------
void EveChildBoosterSet::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( batchType != TRIBATCHTYPE_ADDITIVE )
	{
		return;
	}
	if( !m_display )
	{
		return;
	}
	if( m_ringBufferOffsets.GetCurrentFrameOffset() == Tr2RingBufferOffsets::INVALID_OFFSET )
	{
		return;
	}
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}
	if( m_singleBoosters.empty() )
	{
		return;
	}

	// boosters visible based on LOD?
	if( m_boostersVisible )
	{
		auto& indexBuffer = Tr2Renderer::GetQuadListIndexBuffer();
		if( !indexBuffer.IsValid() )
		{
			return;
		}

		Tr2RenderBatch batch;
		batch.SetMaterial( ( m_boosterHighLod || !m_effectFar ) ? m_effect : m_effectFar );
		batch.SetPerObjectData( perObjectData );
		batch.SetVertexDeclaration( m_vertexDeclHandle );
		auto& vb = m_vertexBuffer.GetSharedResource();
		batch.SetStreamSource( 0, vb.GetBuffer(), vb.GetStride() );
		batch.SetInidices( indexBuffer );

		batch.SetDrawIndexedInstanced(
			3 * 2 * 6, // 3 vertices, 2 triangles, 6 faces
			uint32_t( m_singleBoosters.size() ),
			indexBuffer.GetStartIndex(),
			vb.GetOffset() / vb.GetStride(),
			0 );
		batches->Commit( batch );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   No sorting. Everything is NonSorted
// --------------------------------------------------------------------------------
float EveChildBoosterSet::GetSortValue()
{
	return 1.f;
}

// --------------------------------------------------------------------------------
// Description:
//   Fill the per-object data. First the world matrix of the parent-ship.
// SeeAlso:
//   EveChildBoosterSetPerObjectData, TriRenderBatchAccumulator
// --------------------------------------------------------------------------------
Tr2PerObjectData* EveChildBoosterSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	// allocate only once
	auto perObjectData = accumulator->Allocate<EveChildBoosterSetPerObjectData>();
	if( !perObjectData )
	{
		return NULL;
	}

	// column_major for shaders
	perObjectData->m_vsData.worldMatrix = Transpose( m_parentTransform );

	// vs data
	perObjectData->m_vsData.padding0 = 0.f;
	perObjectData->m_vsData.padding1 = 0.f;
	perObjectData->m_vsData.maxBoosterSize = m_maxSize;
	perObjectData->m_vsData.instanceOffset = m_ringBufferOffsets.GetCurrentFrameOffset();
	// ps data
	perObjectData->m_psData.padding0 = 0.f;
	perObjectData->m_psData.padding1 = 0.f;
	perObjectData->m_psData.warpIntensity = m_warpIntensity;
	perObjectData->m_psData.padding2 = 0.f;

	return perObjectData;
}

// --------------------------------------------------------------------------------
// Description:
//   Get debug options of this booster set
// --------------------------------------------------------------------------------
void EveChildBoosterSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Boosters" );
}

// --------------------------------------------------------------------------------
// Description:
//   Render debug info of this booster set
// --------------------------------------------------------------------------------
void EveChildBoosterSet::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_hasUpdated )
	{
		return;
	}

	if( !renderer.HasOption( this, "Boosters" ) )
	{
		return;
	}

	for( uint32_t j = 0; j < m_singleBoosters.size(); ++j )
	{
		Matrix transform = m_singleBoosters[j].transform * m_parentTransform;
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

	if( m_glows )
	{
		m_glows->RenderDebugInfo( renderer, m_parentTransform, nullptr, 0 );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all the matrices to HW
// --------------------------------------------------------------------------------
void EveChildBoosterSetPerObjectData::SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
{
	FillAndSetConstants( *buffers[VERTEX_SHADER], m_vsData, VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( *buffers[PIXEL_SHADER], m_psData, PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
}

void EveChildBoosterSetPerObjectData::ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const
{
	writer.SetPerObjectData( VERTEX_SHADER, &m_vsData, sizeof( m_vsData ) );
	writer.SetPerObjectData( PIXEL_SHADER, &m_psData, sizeof( m_psData ) );
}
