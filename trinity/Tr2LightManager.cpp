////////////////////////////////////////////////////////////
//
//    Created:   October 2014
//    Copyright: CCP 2014
//

#include "StdAfx.h"
#include "Tr2LightManager.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Tr2GpuBuffer.h"
#include "Tr2GpuStructuredBuffer.h"
#include "Tr2TextureArray.h"

CCP_STATS_DECLARE( lightsGathered, "Trinity/Tr2LightManager/lightsGathered", true, CST_COUNTER_LOW, "How many lights were pushed to GPU" );

extern float g_eveSpaceSceneLODFactor;

namespace
{

// Initial size of the light buffer (number of elements)
const uint32_t LIGHT_BUFFER_SIZE = 1024;
// Size of the light index buffer (number of indices)
const uint32_t INDEX_BUFFER_SIZE = 8 * 1024 * 1024;
// Minimal size of a point light sphere in pixels before it is culled out
const float CUTOFF_PIXEL_SIZE = 7.f;
// Size (in pixels) for the light to stat dimming out before disappeating
const float FADE_SIZE = 5.f;

// Tile sizes in pixels, should match thread count in a compute shader
const uint32_t TILE_WIDTH = 16;
const uint32_t TILE_HEIGHT = 16;

struct PerFrameData
{
	Matrix viewInverse;
	Matrix projInverse;
	Vector3 cameraPos;
	// cppcheck-suppress unusedStructMember
	float _padding0;
	uint32_t width;
	uint32_t height;
	uint32_t tilesX;
	uint32_t tilesY;
	uint32_t lightCount;
	uint32_t indexBufferSize;
	// cppcheck-suppress unusedStructMember 
	uint32_t _padding[2];
};

bool ExtractAnnotationValue( const Tr2EffectParameterAnnotation& annotation,
							 const char* name,
							 uint32_t& value )
{
	if( strcmp( annotation.name, name ) == 0 )
	{
		if( annotation.type == Tr2EffectParameterAnnotation::INT )
		{
			value = uint32_t( annotation.intValue );
			return true;
		}
		else
		{
			CCP_LOGERR( "Tr2LightManager: unexpected annotation \"%s\" type", name );
		}
	}
	return false;
}

Tr2LightManager*& Singleton()
{
	static Tr2LightManager* manager = nullptr;
	return manager;
}

}

Tr2LightManager::Tr2LightManager( const char* effectPath )
{
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( effectPath );

	m_lightBuffer.CreateInstance();
	m_indexBuffer.CreateInstance();

	m_indexBufferCounter.CreateInstance();

	m_lightBufferVariable.Register( "LightBuffer", m_lightBuffer );
	m_indexBufferVariable.Register( "LightIndexBuffer", m_indexBuffer );
	GlobalStore().RegisterVariable( "LightProfileArray", &GetLightProfileArray() );

	PrepareResources();
}

Tr2LightManager::~Tr2LightManager()
{
	ResetVariableStore();
}

void Tr2LightManager::ResetVariableStore()
{
	Tr2GpuStructuredBufferPtr empty;
	empty.CreateInstance();

	GlobalStore().RegisterVariable( "LightBuffer", empty );
	GlobalStore().RegisterVariable( "LightIndexBuffer", empty );
	GlobalStore().RegisterVariable( "LightProfileArray", &GetLightProfileArray() );
}

Tr2LightManager* Tr2LightManager::GetOrCreateInstance( const char* effectPath )
{
	auto& singleton = Singleton();
	if( !singleton )
	{
		singleton = CCP_NEW( "Tr2LightManager::Singleton" ) Tr2LightManager( effectPath );
	}
	return singleton;
}

Tr2LightManager* Tr2LightManager::GetInstance()
{
	return Singleton();
}

void Tr2LightManager::DeleteInstance()
{
	auto& singleton = Singleton();
	CCP_DELETE singleton;
	singleton = nullptr;
}

void Tr2LightManager::SetVariableStore()
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
}

void Tr2LightManager::Clear( Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;
	ClearLightIndices( renderContext );

	for( auto& data : m_tlsLightData )
	{
		data.clear();
	}
	m_lightData.clear();
}

void Tr2LightManager::SetFrustum( const TriFrustum& frustum )
{
	m_frustum = frustum;
}

void Tr2LightManager::AddPointLight( const Vector3& position, float radius, const Color& color, Float_16 innerRadius, uint16_t flags )
{
	if( !AreLightFlagsValid( flags ) )
	{
		return;
	}
	PerLightData data;
	data.position = position;
	data.radius = radius;
	float brightness = std::max( std::max( color.r, color.g ), color.b );
	if( brightness <= 0 )
	{
		return;
	}
	if( !m_frustum.IsSphereVisible( reinterpret_cast<Vector4*>( &data.position ) ) )
	{
		return;
	}

	float size = m_frustum.GetPixelSizeAccross( reinterpret_cast<Vector4*>( &data.position ) );
	float cutoff = CUTOFF_PIXEL_SIZE * g_eveSpaceSceneLODFactor;
	if( size > cutoff )
	{
		float dimming = std::min( ( size - cutoff ) / FADE_SIZE, 1.f );
		data.color = reinterpret_cast<const Vector3&>( color );
		data.color.x *= radius * dimming;
		data.color.y *= radius * dimming;
		data.color.z *= radius * dimming;
		data.innerRadius = innerRadius;
		data.flags = flags;
		data.direction = Vector3_16( Vector3( 1.f, 0.f, 0.f ) );
		data.outerAngle = Float_16( 0.f );
		data.innerAngle = Float_16( 0.f );
		m_tlsLightData.local().push_back( data );
	}
}

void Tr2LightManager::AddLight( PerLightData& data )
{
	if( !AreLightFlagsValid( data.flags ) )
	{
		return;
	}
	float brightness = std::max( std::max( data.color.x, data.color.y ), data.color.z );
	if( brightness <= 0 || data.radius <= 0 )
	{
		return;
	}
	if( !m_frustum.IsSphereVisible( reinterpret_cast<Vector4*>( &data.position ) ) )
	{
		return;
	}

	float size = m_frustum.GetPixelSizeAccross( reinterpret_cast<Vector4*>( &data.position ) );
	float cutoff = CUTOFF_PIXEL_SIZE * g_eveSpaceSceneLODFactor;
	if( size > cutoff )
	{
		float dimming = std::min( ( size - cutoff ) / FADE_SIZE, 1.f );
		data.color.x *= data.radius * dimming;
		data.color.y *= data.radius * dimming;
		data.color.z *= data.radius * dimming;
		m_tlsLightData.local().push_back( data );
	}
}


ALResult Tr2LightManager::ClearLightIndices( Tr2RenderContext& renderContext )
{
	if( !m_indexBuffer || !m_indexBuffer->GetGpuBuffer( 0 ) )
	{
		return S_OK;
	}
	const uint32_t clear[] = { 0, 0, 0, 0 };
	return renderContext.ClearUav( *m_indexBuffer->GetGpuBuffer( 0 ), clear );
}

ALResult Tr2LightManager::UpdateLightBuffer( Tr2RenderContext& renderContext )
{
	if( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count < uint32_t( m_lightData.size() ) )
	{
		CR_RETURN_HR( m_lightBuffer->Create(
			std::max( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count + 1024, uint32_t( m_lightData.size() ) ),
			sizeof( PerLightData ),
			Tr2GpuBuffer::CPU_WRITABLE ) );
		m_lightBuffer->SetName( "Light buffer" );
	}

	auto lightCount = std::min( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( m_lightData.size() ) );

	Vector4* data;
	CR_RETURN_HR( m_lightBuffer->GetGpuBuffer( 0 )->MapForWriting( data, renderContext ) );
	memcpy( data, m_lightData.data(), lightCount * sizeof( PerLightData ) );
	CCP_STATS_ADD( lightsGathered, m_lightData.size() );
	m_lightBuffer->GetGpuBuffer( 0 )->UnmapForWriting( renderContext );
	return S_OK;
}

ALResult Tr2LightManager::DoUpdateLists(Tr2RenderContext& renderContext )
{
	if( !m_lightBuffer->IsValid() || !m_indexBuffer->IsValid() || !m_indexBufferCounter->IsValid() )
	{
		return E_INVALIDCALL;
	}

	CR_RETURN_HR( UpdateLightBuffer( renderContext ) );

	PerFrameData perFrameData;
	perFrameData.projInverse = Transpose( Tr2Renderer::GetInverseProjectionTransform() );
	perFrameData.viewInverse = Transpose( Tr2Renderer::GetInverseViewTransform() );
	perFrameData.cameraPos = Tr2Renderer::GetViewPosition();
	perFrameData.width = uint32_t( renderContext.m_esm.GetRenderTargetWidth() );
	perFrameData.height = uint32_t( renderContext.m_esm.GetRenderTargetHeight() );
	perFrameData.tilesX = ( perFrameData.width + ( TILE_WIDTH - 1 ) ) / TILE_WIDTH;
	perFrameData.tilesY = ( perFrameData.height + ( TILE_HEIGHT - 1 ) ) / TILE_HEIGHT;
	perFrameData.lightCount = std::min( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( m_lightData.size() ) );
	perFrameData.indexBufferSize = INDEX_BUFFER_SIZE;
	if( !FillAndSetConstants( m_perFrameData, perFrameData, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext ) )
	{
		return E_FAIL;
	}

	if( !Tr2Renderer::RunComputeShader( m_effect, BlueSharedString( "Clear" ), 1, 1, 1, renderContext ) )
	{
		return E_FAIL;
	}

	if( !Tr2Renderer::RunComputeShader( m_effect, perFrameData.tilesX, perFrameData.tilesY, 1, renderContext ) )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2LightManager::UpdateLists( Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;

	m_lightData.clear();
	for( auto& data : m_tlsLightData )
	{
		if( !data.empty() )
		{
			m_lightData.insert( end( m_lightData ), begin( data ), end( data ) );
		}
		data.clear();
	}

	if( m_lightData.empty() )
	{
		return ClearLightIndices( renderContext );
	}

	auto hr = DoUpdateLists( renderContext );
	if( FAILED( hr ) )
	{
		ClearLightIndices( renderContext );
	}

	return hr;
}

void Tr2LightManager::ReleaseResources( TriStorage s )
{
	if( s & m_perFrameData.GetMemoryClass() )
	{
		m_perFrameData = Tr2ConstantBufferAL();
	}
}

bool Tr2LightManager::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_lightBuffer->Create( LIGHT_BUFFER_SIZE, sizeof( PerLightData ), Tr2GpuBuffer::CPU_WRITABLE );
	m_lightBuffer->SetName( "Light buffer" );
	m_indexBuffer->Create( INDEX_BUFFER_SIZE, sizeof( uint32_t ), Tr2GpuStructuredBuffer::GPU_WRITABLE );
	m_indexBuffer->SetName( "Light indices" );
	m_indexBufferCounter->Create( 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT, Tr2GpuBuffer::GPU_WRITABLE );
	ClearLightIndices( renderContext );
	m_perFrameData.Create( sizeof( PerFrameData ), Tr2ConstantUsageAL::ONE_SHOT, nullptr, renderContext );

	m_effect->SetParameter( BlueSharedString( "LightBuffer" ), m_lightBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndices" ), m_indexBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndexCount" ), m_indexBufferCounter );

	return true;
}

bool Tr2LightManager::AreLightFlagsValid( uint16_t flags )
{
	return ( flags & ( FLAG_AFFECTS_SURFACES | FLAG_AFFECTS_PARTICLES ) ) != 0;
}

Tr2TextureArray& Tr2LightManager::GetLightProfileArray()
{
	static CTr2TextureArray lightProfiles;
	return lightProfiles;
}