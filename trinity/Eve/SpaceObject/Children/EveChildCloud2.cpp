////////////////////////////////////////////////////////////
//
//    Created:   February 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "EveChildCloud2.h"
#include "TriFrustum.h"
#include "TriRenderBatch.h"
#include "Utilities/BoundingSphere.h"
#include "Tr2PerObjectData.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/IEveShadowCaster.h"
#include "Tr2Renderer.h"
#include "../../../Lights/Tr2Light.h"
#include "../../../Shader/Tr2Effect.h"
#include "../../../Shader/Tr2Shader.h"
#include "../../../Shader/Parameter/Tr2RuntimeTextureParameter.h"
#include "../../../Shader/Parameter/TriTextureParameter.h"
#include "../../../Shader/Parameter/Tr2TextureAnimationParameter.h"
#include "../../../Tr2TextureReference.h"
#include "../../../Tr2TextureAnimation.h"

using namespace Tr2RenderContextEnum;

namespace
{

class EveChildCloudPerObjectData : public Tr2PerObjectData
{
public:
	virtual void SetPerObjectDataToDevice( Tr2ConstantBufferAL** buffers, unsigned constantTypeMask, Tr2RenderContext& renderContext ) const
	{
		static const unsigned mask =
			( 1 << VERTEX_SHADER ) |
			SHADER_TYPE_EXISTS( COMPUTE_SHADER ) |
			SHADER_TYPE_EXISTS( GEOMETRY_SHADER ) |
			SHADER_TYPE_EXISTS( HULL_SHADER ) |
			SHADER_TYPE_EXISTS( DOMAIN_SHADER );
		if( ( constantTypeMask & mask ) != 0 )
		{
			FillAndSetConstants(
				*buffers[Tr2RenderContextEnum::VERTEX_SHADER],
				&m_data,
				sizeof( m_data ),
				mask,
				Tr2Renderer::GetPerObjectVSStartRegister(),
				renderContext );
		}
		if( ( constantTypeMask & ( 1 << PIXEL_SHADER ) ) != 0 )
		{
			FillAndSetConstants(
				*buffers[Tr2RenderContextEnum::PIXEL_SHADER],
				&m_data,
				sizeof( m_data ),
				1 << PIXEL_SHADER,
				Tr2Renderer::GetPerObjectPSStartRegister(),
				renderContext );
		}
	}

	void ApplyConstantBuffers( Tr2IndirectDrawBufferWriter& writer, Tr2RenderContext& renderContext ) const
	{
		writer.SetPerObjectData( VERTEX_SHADER, &m_data, sizeof( m_data ) );
		writer.SetPerObjectData( PIXEL_SHADER, &m_data, sizeof( m_data ) );
	}

	EveChildCloud2::PerObjectData m_data;
};

}



EveChildCloud2::EveChildCloud2( IRoot* lockobj ) :
	PARENTLOCK( m_lights ),
	m_localTransform( IdentityMatrix() ),
	m_worldTransform( IdentityMatrix() ),
	m_prevSunDirection( 0, 0, 0 ),
	m_localSunDirection( 0, 0, 0 ),
	m_depthSlices{ 0, 0, 0, 0 },
	m_noiseTextureSize( 32 ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_translation( 0.f, 0.f, 0.f ),
	m_rotation( 0.f, 0.f, 0.f, 1.0f ),
	m_reflectionMode( EntityComponents::REFLECT_NEVER ),
	m_minVisibleQuality( Tr2VolumerticQuality::Low ),
	m_currentQuality( Tr2VolumerticQuality::High ),
	m_display( true ),
	m_castShadows( true ),
	m_receiveShadows( true ),
	m_lightmapDirty( true ),
	m_renderedLastFrame( true ),
	m_declaration( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_sortingModifier( 1.0f ),
	m_minScreenSize( 0.0f ),
	m_adjustedMinScreenSize( 0.0f ),
	m_effectHash( 0 ),
	m_lightmapWidth( 0 ),
	m_lightmapHeight( 0 ),
	m_lightmapDepth( 0 ),
	m_lightmapDirtyOffset( 0 ),
	m_lightmapSizeScale( 0.5f ),
	m_shadowMapSize( 512 ),
	m_depthShadowMapHandle( NULL )
{
	m_lightMap.CreateInstance();
	m_emptyLightMap.CreateInstance();

	m_variableStore.CreateInstance();
	m_variableStore->RegisterVariable( "LightMap", m_emptyLightMap );
	m_variableStore->RegisterVariable( "LightMapRW", m_lightMap );

	std::fill( std::begin( m_mapTiling ), std::end( m_mapTiling ), Vector3( 1, 1, 1 ) );
	std::fill( std::begin( m_mapOffsets ), std::end( m_mapOffsets ), Vector3( 0, 0, 0 ) );
	PrepareResources();

	m_depthShadowMapHandle = GlobalStore().RegisterVariable( "DepthShadowMap", static_cast<ITr2TextureProvider*>( nullptr ) );
}

EveChildCloud2::~EveChildCloud2()
{
}

void EveChildCloud2::RegisterComponents()
{
	auto registry = GetComponentRegistry();
	if( registry )
	{
		registry->RegisterComponent<ITr2VolumetricRenderable>( this );
		if( EntityComponents::ShouldReflect( m_reflectionMode ) && m_display && m_reflectionEffect )
		{
			registry->RegisterComponent<ITr2Renderable>( this );
		}
	}
}

bool EveChildCloud2::Initialize()
{
	if( m_effect )
	{
		m_effect->SetVariableStore( m_variableStore );
		m_effect->RebuildCachedData();
	}
	if( m_reflectionEffect )
	{
		m_reflectionEffect->SetVariableStore( m_variableStore );
		m_reflectionEffect->RebuildCachedData();
	}
	m_vertexBuffer = Tr2BufferAL();
	PrepareResources();
	return true;
}

bool EveChildCloud2::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_reflectionMode ) || IsMatch( value, m_display ) || IsMatch( value, m_reflectionEffect ) )
	{
		ReRegister();
	}
	if( IsMatch( value, m_effect ) )
	{
		if( m_effect )
		{
			m_effect->SetVariableStore( m_variableStore );
			m_effect->RebuildCachedData();
		}
		MarkLightmapDirty( true );
	}
	if( IsMatch( value, m_reflectionEffect ) )
	{
		if( m_reflectionEffect )
		{
			m_reflectionEffect->SetVariableStore( m_variableStore );
			m_reflectionEffect->RebuildCachedData();
		}
	}
	return true;
}

const char* EveChildCloud2::GetName() const
{
	return m_name.c_str();
}

void EveChildCloud2::SetName( const char* name )
{
	m_name = name;
}

void EveChildCloud2::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
}

void EveChildCloud2::GetRenderables( std::vector<ITr2Renderable*>& )
{
}

bool EveChildCloud2::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery ) const
{
	sphere = Vector4( m_boundingSphere.center, m_boundingSphere.radius );
	return true;
}

void EveChildCloud2::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

float EveChildCloud2::GetSortValue( const TriFrustum& frustum )
{
	Vector3 d = frustum.m_viewPos - m_worldTransform.GetTranslation();
	float distance = Length( d ) - Length( m_scaling ) * m_sortingModifier;
	return distance;
}

void EveChildCloud2::GetVolumetricBatches( const TriFrustum& frustum, ITriRenderBatchAccumulator* batches )
{
	if( m_currentQuality < m_minVisibleQuality )
	{
		return;
	}
	auto isVisible = m_display && frustum.IsSphereVisible( m_boundingSphere.center, m_boundingSphere.radius );
	if( !isVisible )
	{
		return;
	}

	auto screenSize = frustum.GetPixelSizeAccross( m_boundingSphere );
	if( screenSize < m_adjustedMinScreenSize )
	{
		return;
	}

	if( !HasValidTransform() )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() || m_declaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}
	if( !m_effect || !m_effect->GetShaderStateInterface() )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_effect );
	batch.SetPerObjectData( GetPerObjectData( batches, screenSize ) );
	batch.SetGeometry( m_declaration, m_vertexBuffer, sizeof( Vector3 ), m_indexBuffer, m_indexBuffer.GetDesc().stride );
	batch.SetDrawIndexedInstanced( 12 * 3, 1, 0, 0, 0 );
	batches->Commit( batch );

	m_renderedLastFrame = true;
}

void EveChildCloud2::CreateEmptyLightMap()
{
	if( !m_emptyLightMap->GetTexture()->IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		uint8_t gray[4] = { 127, 127, 127, 127 };
		Tr2SubresourceData initialData;
		initialData.m_sysMem = gray;
		initialData.m_sysMemPitch = 4;
		initialData.m_sysMemSlicePitch = 4;

		m_emptyLightMap->GetTexture()->Create(
			Tr2BitmapDimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM, 1, 1, 1, 1 ),
			Tr2GpuUsage::SHADER_RESOURCE,
			Tr2CpuUsage::NONE,
			&initialData,
			renderContext );
		m_emptyLightMap->GetTexture()->SetName( "EveChildCloud2 Empty Lightmap" );
		m_emptyLightMap->OnTextureChange().Broadcast();
	}
}

bool EveChildCloud2::HasValidTransform() const
{
	auto det = Determinant( m_worldTransform );
	if( det == 0 || isinf( det ) || isnan( det ) )
	{
		return false;
	}
	return true;
}

bool EveChildCloud2::UpdateVolumetricLightmap( Tr2RenderContext& renderContext )
{
	if( m_currentQuality < m_minVisibleQuality )
	{
		return false;
	}
	if( m_lightmapDirty && m_effect && m_lightmapWidth > 0 && HasValidTransform() )
	{
		CreateEmptyLightMap();

		uint32_t lightmapWidth = std::max( 1u, uint32_t( m_lightmapWidth * m_lightmapSizeScale ) );
		uint32_t lightmapHeight = std::max( 1u, uint32_t( m_lightmapHeight * m_lightmapSizeScale ) );
		uint32_t lightmapDepth = std::max( 1u, uint32_t( m_lightmapDepth * m_lightmapSizeScale ) );

		auto& lightMap = *m_lightMap->GetTexture();
		if( lightMap.GetWidth() != lightmapWidth || lightMap.GetHeight() != lightmapHeight || lightMap.GetDesc().GetDepth() != lightmapDepth )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL( lightMap.Create(
							   Tr2BitmapDimensions( Tr2RenderContextEnum::TEX_TYPE_3D, Tr2RenderContextEnum::PIXEL_FORMAT_R8G8_UNORM, lightmapWidth, lightmapHeight, lightmapDepth, 1 ),
							   Tr2GpuUsage::SHADER_RESOURCE | Tr2GpuUsage::UNORDERED_ACCESS,
							   Tr2CpuUsage::NONE,
							   renderContext ),
						   false );
			lightMap.SetName( "EveChildCloud2 Lightmap" );
			m_lightMap->OnTextureChange().Broadcast();
			m_lightmapDirtyOffset = 0;
			m_variableStore->RegisterVariable( "LightMap", m_emptyLightMap );
		}

		EveChildCloudPerObjectData perObjectData;
		PopulatePerObjectData( perObjectData.m_data, 1 );
		Tr2ConstantBufferAL* cbs[Tr2RenderContextEnum::CBUFFER_COUNT] = { renderContext.GetConstantBuffer( 0 ) };
		perObjectData.SetPerObjectDataToDevice( cbs, 1 << COMPUTE_SHADER, renderContext );

		const uint32_t VOXELS_PER_UPDATE = uint32_t( 6400000 * m_lightmapSizeScale * m_lightmapSizeScale * m_lightmapSizeScale );
		uint32_t slices = std::max( VOXELS_PER_UPDATE / ( lightmapHeight * lightmapDepth ), 1u );
		auto success = Tr2Renderer::RunComputeShader(
			m_effect,
			BlueSharedString( "GenerateLightmap" ),
			slices,
			( m_lightmapHeight + 7 ) / 8,
			( m_lightmapDepth + 7 ) / 8,
			renderContext );
		if( success )
		{
			m_lightmapDirtyOffset += slices;
			if( m_lightmapDirtyOffset >= lightmapWidth )
			{
				m_lightmapDirty = false;
				m_lightmapDirtyOffset = 0;
				m_variableStore->RegisterVariable( "LightMap", m_lightMap );
				*m_emptyLightMap->GetTexture() = Tr2TextureAL();
				m_emptyLightMap->OnTextureChange().Broadcast();
			}
			return true;
		}
		else
		{
			m_lightmapDirtyOffset = 0;
		}
	}
	return false;
}

void EveChildCloud2::SetSceneInformation( const SceneInformation& sceneInformation )
{
	float lightmapSizeScale;
	switch( sceneInformation.quality )
	{
	case Tr2VolumerticQuality::Low:
		lightmapSizeScale = 0.1f;
		break;
	case Tr2VolumerticQuality::Medium:
		lightmapSizeScale = 0.15f;
		break;
	default:
		lightmapSizeScale = 0.25f;
	}
	if( lightmapSizeScale != m_lightmapSizeScale )
	{
		m_lightmapSizeScale = lightmapSizeScale;
		m_lightmapDirty = true;
	}
	m_currentQuality = sceneInformation.quality;

	std::copy_n( sceneInformation.depthSlices, SceneInformation::depthSliceCount, m_depthSlices );

	m_localSunDirection = Normalize( TransformNormal( sceneInformation.sunDirection, Inverse( m_worldTransform ) ) );
	if( Dot( m_prevSunDirection, m_localSunDirection ) < cosf( 5.0f / 180.0f ) )
	{
		m_prevSunDirection = m_localSunDirection;
		MarkLightmapDirty( true );
	}

	m_targetWidth = sceneInformation.targetWidth;
	m_targetHeight = sceneInformation.targetHeight;

	m_effect->SetOption(
		BlueSharedString( "CLOUD_SHADOWS" ),
		sceneInformation.receiveShadows && m_receiveShadows ? BlueSharedString( "CLOUD_SHADOWS_RECEIVE" ) : BlueSharedString( "CLOUD_SHADOWS_NONE" ) );

	m_effect->SetOption(
		BlueSharedString( "CLOUD_SHADOW_ALGORITHM" ),
		sceneInformation.receiveShadows && m_receiveShadows && sceneInformation.raytracedShadows ? BlueSharedString( "CLOUD_SHADOWS_RAYTRACED" ) : BlueSharedString( "CLOUD_SHADOWS_CASCADED" ) );
}

void EveChildCloud2::ReleaseResources( TriStorage s )
{
	m_declaration = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;

	m_lightmapDirty = true;
	m_lightmapDirtyOffset = 0;
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		m_lightmapWidth = 0;
	}

	if( m_depthShadowMapHandle )
	{
		m_depthShadowMapHandle->Clear();
	}

	m_shadowMapDS = Tr2DepthStencilPtr();
}

void EveChildCloud2::ClearVariableStore()
{
	if( m_depthShadowMapHandle )
	{
		m_depthShadowMapHandle->Clear();
	}
}

bool EveChildCloud2::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_vertexBuffer.IsValid() )
	{

		Vector3 data[] = {
			{ -0.5f, -0.5f, 0.5f },
			{ 0.5f, -0.5f, 0.5f },
			{ 0.5f, 0.5f, 0.5f },
			{ -0.5f, 0.5f, 0.5f },
			{ -0.5f, -0.5f, -0.5f },
			{ 0.5f, -0.5f, -0.5f },
			{ 0.5f, 0.5f, -0.5f },
			{ -0.5f, 0.5f, -0.5f },
		};

		uint16_t indices[] = {
			// front
			0,
			1,
			2,
			2,
			3,
			0,
			// right
			1,
			5,
			6,
			6,
			2,
			1,
			// back
			7,
			6,
			5,
			5,
			4,
			7,
			// left
			4,
			0,
			3,
			3,
			7,
			4,
			// bottom
			4,
			5,
			1,
			1,
			0,
			4,
			// top
			3,
			2,
			6,
			6,
			7,
			3
		};

		m_vertexBuffer.Create( sizeof( Vector3 ), 8, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, data, renderContext );
		m_indexBuffer.Create( 2, 36, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, indices, renderContext );
	}

	if( m_declaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		static Tr2VertexDefinition s_decl;
		if( s_decl.empty() )
		{
			s_decl.Add( s_decl.FLOAT32_3, s_decl.POSITION );
		}

		m_declaration = renderContext.m_esm.GetVertexDeclarationHandle( s_decl );
	}

	return true;
}

Tr2PerObjectData* EveChildCloud2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator, float screenSize )
{
	auto data = accumulator->Allocate<EveChildCloudPerObjectData>();
	if( data )
	{
		PopulatePerObjectData( data->m_data, screenSize );
	}
	return data;
}

void EveChildCloud2::PopulatePerObjectData( PerObjectData& data, float screenSize ) const
{
	data.world = Transpose( m_worldTransform );
	data.projectionInv = Transpose( Inverse( Tr2Renderer::GetReversedDepthProjectionTransform() ) );
	auto worldViewInv = Inverse( m_worldTransform * Tr2Renderer::GetViewTransform() );
	data.worldViewInv = Transpose( worldViewInv );
	data.viewPosition = TransformCoord( Tr2Renderer::GetViewPosition(), Inverse( m_worldTransform ) );
	data.lightViewProj = Transpose( m_lightViewProj );

	uint32_t lightmapWidth = std::max( 1u, uint32_t( m_lightmapWidth * m_lightmapSizeScale ) );
	uint32_t lightmapHeight = std::max( 1u, uint32_t( m_lightmapHeight * m_lightmapSizeScale ) );
	uint32_t lightmapDepth = std::max( 1u, uint32_t( m_lightmapDepth * m_lightmapSizeScale ) );

	data.lightmapDimensions[0] = lightmapWidth;
	data.lightmapDimensions[1] = lightmapHeight;
	data.lightmapDimensions[2] = lightmapDepth;
	data.lightmapDimensions[3] = m_lightmapDirtyOffset;

	data.noiseConfig[0] = rand() % m_noiseTextureSize;
	data.noiseConfig[1] = rand() % m_noiseTextureSize;
	data.noiseConfig[2] = m_noiseTextureSize;
	data.noiseConfig[3] = m_noiseTextureSize;

	auto worldViewT = Transpose( m_worldTransform * Tr2Renderer::GetViewTransform() );
	data.viewDirection = TransformNormal( Vector3( 0, 0, -1 ), worldViewT );
	data.depthSlice0 = Transform( Vector4{ 0, 0, -1, -m_depthSlices[0] }, worldViewT ).w;
	data.depthSlice1 = Transform( Vector4{ 0, 0, -1, -m_depthSlices[1] }, worldViewT ).w;
	data.depthSlice2 = Transform( Vector4{ 0, 0, -1, -m_depthSlices[2] }, worldViewT ).w;

	data.sunDirection = -m_localSunDirection;

	Vector3 scale;
	Quaternion rotation;
	Vector3 translation;
	Decompose( scale, rotation, translation, m_worldTransform );
	scale *= 1.f / std::max( std::max( scale.x, scale.y ), scale.z );
	data.relativeScaling = scale;
	data.lodFactor = std::max( 0.f, screenSize / std::max( 1.f, m_minScreenSize ) - 1 );

	data.targetInvSize = Vector2( 2.f / m_targetWidth, 2.f / m_targetHeight );

	size_t i = 0;
	const auto lightCount = sizeof( data.lights ) / sizeof( data.lights[0] );
	for( auto& light : m_lights )
	{
		Vector3 position;
		float radius;
		Color color;
		light->GetLight( position, radius, color );
		data.lights[i].position = position;
		data.lights[i].radius = radius;
		if( radius > 0 )
		{
			data.lights[i].innerRadius = std::max( std::min( light->GetLightData().innerRadius / radius, 1.f ), 0.f );
			data.lights[i].color = ( Vector4( color ) * light->GetBrightnessMultiplier() ).GetXYZ() * pow( data.lights[i].innerRadius * 2 + 1, 3 );
		}
		else
		{
			data.lights[i].innerRadius = 0;
			data.lights[i].color = Vector3( 0, 0, 0 );
		}
		++i;
		if( i >= lightCount )
		{
			break;
		}
	}
	if( i < lightCount )
	{
		std::fill( std::begin( data.lights ) + i, std::end( data.lights ), LightData{} );
	}

	data.mapOffsets[0] = Vector4( m_mapOffsets[0], 0.f );
	data.mapOffsets[1] = Vector4( m_mapOffsets[1], 0.f );
	data.mapOffsets[2] = Vector4( m_mapOffsets[2], 0.f );
}

void EveChildCloud2::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
	if( m_effect )
	{
		auto hash = m_effect->GetHashValue();
		if( m_effectHash != hash )
		{
			m_effectHash = hash;
			m_lightmapDirty = true;
			m_lightmapWidth = 0;
			m_lightmapHeight = 0;
			m_lightmapDepth = 0;
		}
		if( m_lightmapDirty && m_lightmapWidth == 0 && m_effect->GetShaderStateInterface() )
		{
			if( m_effect->GetShaderStateInterface()->GetResource( "LightMap" ) )
			{
				Tr2TextureAL densityMap;
				if( auto param = m_effect->GetResourceByName( "DensityMap" ) )
				{
					if( TriTextureParameterPtr textureParam = BlueCastPtr( param ) )
					{
						if( auto resource = textureParam->GetResource() )
						{
							if( auto texture = resource->GetTexture() )
							{
								densityMap = *texture;
							}
						}
					}
					else if( Tr2TextureAnimationParameterPtr animParam = BlueCastPtr( param ) )
					{
						densityMap = animParam->GetTexture();
					}
				}
				if( densityMap.IsValid() )
				{
					auto& desc = densityMap.GetDesc();
					m_lightmapWidth = desc.GetWidth();
					m_lightmapHeight = desc.GetHeight();
					m_lightmapDepth = desc.GetDepth();
					m_lightmapDirty = true;
					m_lightmapDirtyOffset = 0;
				}
			}
			else
			{
				CreateEmptyLightMap();
				m_variableStore->RegisterVariable( "LightMap", m_emptyLightMap );
				*m_lightMap->GetTexture() = Tr2TextureAL();
				m_lightMap->OnTextureChange().Broadcast();
				m_lightmapDirty = false;
			}
		}
	}

	if( m_animation )
	{
		if( !m_animation->UpdateOnlyWhenRendered() || m_renderedLastFrame )
		{
			m_animation->AdvanceTime( updateContext.GetDeltaT() );
		}
	}
	m_renderedLastFrame = false;
}

void EveChildCloud2::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );

	Matrix parent;
	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( parent );
	}
	else
	{
		params.spaceObjectParent->GetLocalToWorldTransform( parent );
	}
	auto prevLocation = m_worldTransform.GetTranslation();
	m_worldTransform = m_localTransform * parent;

	m_boundingSphere = CcpMath::Sphere( CcpMath::AxisAlignedBox( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ) ), m_worldTransform );

	auto shift = -updateContext.GetOriginShift() + m_worldTransform.GetTranslation() - prevLocation;
	shift = TransformNormal( shift, Inverse( m_worldTransform ) );
	for( size_t i = 0; i < 3; ++i )
	{
		m_mapOffsets[i].x = fmodf( m_mapOffsets[i].x + shift.x * m_mapTiling[i].x, 1.f );
		m_mapOffsets[i].y = fmodf( m_mapOffsets[i].y + shift.y * m_mapTiling[i].y, 1.f );
		m_mapOffsets[i].z = fmodf( m_mapOffsets[i].z + shift.z * m_mapTiling[i].y, 1.f );
	}

	m_adjustedMinScreenSize = m_minScreenSize * updateContext.GetLodFactor();
}

void EveChildCloud2::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Bounding Box" );
	options.insert( "Bounding Sphere" );
}

void EveChildCloud2::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Bounding Box" ) )
	{
		Vector3 minBounds( -0.5f, -0.5f, -0.5f );
		Vector3 maxBounds( 0.5f, 0.5f, 0.5f );
		uint32_t color = 0xff00ff00;

		renderer.DrawBox( this, m_worldTransform, minBounds, maxBounds, Tr2DebugRenderer::Wireframe, color );
	}
	if( renderer.HasOption( GetRawRoot(), "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, m_boundingSphere.center, m_boundingSphere.radius, 18, Tr2DebugRenderer::Wireframe, 0xff00ff00 );
	}
}


void EveChildCloud2::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}
	XMMATRIX worldTransform = m_worldTransform;
	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetX() ),
											   XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetY() ), XMVector3LengthEst( m_worldTransform.GetZ() ) ) ) ) /
		3.f;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, worldTransform, scaling );
	}
}

bool EveChildCloud2::IsLightmapDirty() const
{
	return m_lightmapDirty;
}

void EveChildCloud2::MarkLightmapDirty( bool dirty )
{
	m_lightmapDirty = dirty;
	m_lightmapDirtyOffset = 0;
}

void EveChildCloud2::GetVolumetricShadowBatches( ITriRenderBatchAccumulator* batches )
{
	if( !m_display || !m_effect || !m_castShadows )
	{
		return;
	}

	auto shader = m_effect->GetShaderStateInterface();

	if( !shader )
	{
		return;
	}
	uint32_t technique;
	if( !shader->GetTechniqueIndex( BlueSharedString( "Shadow" ), technique ) )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_effect );
	batch.SetPerObjectData( GetPerObjectData( batches, 1 ) );
	batch.SetRenderingMode( Tr2EffectStateManager::RM_ALPHA );
	batch.SetVertexDeclaration( Tr2EffectStateManager::NULL_DECLARATION );
	batch.SetDrawInstanced( 3, 1, 0, 0 );
	batches->Commit( batch );
}

void EveChildCloud2::GetVolumetricShadowInfo( ShadowInfo& shadowInfo, Vector3 sunDir )
{
	SetupShadowFrustum( shadowInfo, sunDir );
}

bool EveChildCloud2::PrepareCloudShadowMap( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_receiveShadows )
	{
		return false;
	}

	if( !m_shadowMapDS )
	{
		m_shadowMapDS.CreateInstance();
	}

	if( !m_shadowMapDS || !m_shadowMapDS->IsValid() )
	{
		m_shadowMapDS->Create( m_shadowMapSize, m_shadowMapSize, Tr2RenderContextEnum::DSFMT_D32F, 1, 0, Tr2RenderContextEnum::EX_NONE );
	}

	// Using depth stencil as shadow map
	renderContext.m_esm.PushViewport();
	renderContext.m_esm.PushRenderTarget( Tr2TextureAL() ); //empty texture
	renderContext.m_esm.PushDepthStencilBuffer( *m_shadowMapDS->GetTexture() );

	renderContext.m_esm.UpdateRenderTargetViewport( m_shadowMapDS->GetWidth(), m_shadowMapDS->GetHeight() );

	// we want a clean depth buffer for this
	CR( renderContext.Clear( Tr2RenderContextEnum::CLEARFLAGS_ZBUFFER, 0xffffffff, 1, 0 ) );

	renderContext.SetReadOnlyDepth( false );

	renderContext.m_esm.SetViewport( m_shadowMapDS->GetWidth(), m_shadowMapDS->GetHeight(), 0, 0, 0, 1 );

	return true;
}


void EveChildCloud2::SetCloudShadowMapHandle()
{
	if( m_shadowMapDS->IsValid() )
	{
		m_depthShadowMapHandle->SetValue( m_shadowMapDS );
	}
}

bool EveChildCloud2::IsVisible( const EveUpdateContext& updateContext ) const
{
	auto& frustum = updateContext.GetFrustum();
	auto isVisible = m_display && frustum.IsSphereVisible( m_boundingSphere.center, m_boundingSphere.radius );
	if( !isVisible )
	{
		return false;
	}

	auto screenSize = frustum.GetPixelSizeAccross( m_boundingSphere );
	if( screenSize < m_minScreenSize * updateContext.GetLodFactor() )
	{
		return false;
	}
	return true;
}

void EveChildCloud2::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( m_currentQuality < m_minVisibleQuality )
	{
		return;
	}
	if( reason == TR2RENDERREASON_REFLECTION && batchType == TRIBATCHTYPE_TRANSPARENT )
	{
		if( !HasValidTransform() )
		{
			return;
		}

		if( !m_vertexBuffer.IsValid() || m_declaration == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return;
		}
		if( !m_reflectionEffect || !m_reflectionEffect->GetShaderStateInterface() )
		{
			return;
		}

		Tr2RenderBatch batch;
		batch.SetMaterial( m_reflectionEffect );
		batch.SetPerObjectData( GetPerObjectData( batches, 10000 ) );
		batch.SetGeometry( m_declaration, m_vertexBuffer, sizeof( Vector3 ), m_indexBuffer, m_indexBuffer.GetDesc().stride );
		batch.SetDrawIndexedInstanced( 12 * 3, 1, 0, 0, 0 );
		batch.SetRenderingMode( Tr2EffectStateManager::RM_ALPHA );
		batches->Commit( batch );
	}
}

void EveChildCloud2::SetupShadowFrustum( ShadowInfo& shadowInfo, Vector3 sunDir )
{
	// Find light view
	Matrix lightView = Inverse( OrthoNormalBasisZ( -sunDir ) );

	AxisAlignedBoundingBox aabb = CcpMath::AxisAlignedBox( Vector3( -0.5f, -0.5f, -0.5f ), Vector3( 0.5f, 0.5f, 0.5f ) );

	aabb.Transform( m_worldTransform * lightView );

	aabb.m_max.z += 2500000.f;

	m_lightViewProj = lightView * OrthoOffCenterMatrix( aabb.m_max.x, aabb.m_min.x, aabb.m_max.y, aabb.m_min.y, -aabb.m_max.z, -aabb.m_min.z );
	
	// create shadow frustum out from lightView, aabb.min, aabb.max
	TriFrustumOrtho shadowFrustum;
	shadowFrustum.DeriveFrustum( lightView, aabb.m_min, aabb.m_max );
	
	shadowInfo.aabbMax = aabb.m_max;
	shadowInfo.lightViewProj = m_lightViewProj;
	shadowInfo.shadowFrustum = shadowFrustum;
	shadowInfo.shadowMapSize = m_shadowMapSize;
}

bool EveChildCloud2::HasTransparentBatches()
{
	return true;
}

float EveChildCloud2::GetSortValue()
{
	return std::numeric_limits<float>::max();
}

Tr2PerObjectData* EveChildCloud2::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	auto data = accumulator->Allocate<EveChildCloudPerObjectData>();
	if( data )
	{
		PopulatePerObjectData( data->m_data, 1 );
	}
	return data;
}
