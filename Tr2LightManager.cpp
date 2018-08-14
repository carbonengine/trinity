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

CCP_STATS_DECLARE( lightsGathered, "Trinity/Tr2LightManager/lightsGathered", true, CST_COUNTER_LOW, "How many lights were pushed to GPU" );

extern float g_eveSpaceSceneLODFactor;

namespace
{

// Initial size of the light buffer (number of elements)
const uint32_t LIGHT_BUFFER_SIZE = 1024;
// Size of the light index buffer (number of indices)
const uint32_t INDEX_BUFFER_SIZE = 4 * 1024 * 1024;
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
	uint32_t msaaSamples;
	uint32_t _padding[1];
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
	:m_lightData( LIGHT_BUFFER_SIZE, "Tr2LightManager::m_lightData" )
{
	m_effect.CreateInstance();
	m_effect->SetEffectPathName( effectPath );

	m_lightBuffer.CreateInstance();
	m_indexBuffer.CreateInstance();

	m_lightBufferVariable.Register( "LightBuffer", m_lightBuffer );
	m_indexBufferVariable.Register( "LightIndexBuffer", m_indexBuffer );

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

void Tr2LightManager::Clear()
{
	m_lightData.Clear();
}

void Tr2LightManager::SetFrustum( const TriFrustum& frustum )
{
	m_frustum = frustum;
}

void Tr2LightManager::AddPointLight( const Vector3& position, float radius, const Color& color )
{
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
		data._padding = 0;
		m_lightData.Add( data, "Tr2LightManager::m_lightData" );
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
	if( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count < m_lightData.GetCount() )
	{
		CR_RETURN_HR( m_lightBuffer->Create( 
			std::max( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count + 1024, m_lightData.GetCount() ),
			sizeof( PerLightData ), 
			Tr2GpuBuffer::CPU_WRITABLE ) );
	}

	auto lightCount = std::min( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( m_lightData.GetCount() ) );

	Vector4* data;
	CR_RETURN_HR( m_lightBuffer->GetGpuBuffer( 0 )->MapForWriting( data, renderContext ) );
	memcpy( data, m_lightData.GetData(), lightCount * sizeof( PerLightData ) );
	CCP_STATS_ADD( lightsGathered, m_lightData.GetCount() );
	m_lightBuffer->GetGpuBuffer( 0 )->UnmapForWriting( renderContext );
	return S_OK;
}

ALResult Tr2LightManager::DoUpdateLists( uint32_t msaaType, Tr2RenderContext& renderContext )
{
	if( !m_lightBuffer->IsValid() || !m_indexBuffer->IsValid() )
	{
		return E_INVALIDCALL;
	}

	CR_RETURN_HR( UpdateLightBuffer( renderContext ) );

	PerFrameData perFrameData;
	perFrameData.projInverse = Transpose( Tr2Renderer::GetInverseProjectionTransform() );
	perFrameData.viewInverse = Transpose( Tr2Renderer::GetInverseViewTransform() );
	perFrameData.cameraPos = Tr2Renderer::GetViewPosition();
	perFrameData.width = uint32_t( Tr2Renderer::GetDeviceViewport().m_width );
	perFrameData.height = uint32_t( Tr2Renderer::GetDeviceViewport().m_height );
	perFrameData.tilesX = ( perFrameData.width + ( TILE_WIDTH - 1 ) ) / TILE_WIDTH;
	perFrameData.tilesY = ( perFrameData.height + ( TILE_HEIGHT - 1 ) ) / TILE_HEIGHT;
	perFrameData.lightCount = std::min( m_lightBuffer->GetGpuBuffer( 0 )->GetDesc().count, uint32_t( m_lightData.GetCount() ) );
	perFrameData.indexBufferSize = INDEX_BUFFER_SIZE;
	perFrameData.msaaSamples = msaaType;
	if( !FillAndSetConstants( m_perFrameData, perFrameData, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext ) )
	{
		return E_FAIL;
	}
	static const BlueSharedString msaaOptions[] = { 
		BlueSharedString( "DEPTH_BUFFER_NONE" ), 
		BlueSharedString( "DEPTH_BUFFER_NON_MSAA" ), 
		BlueSharedString( "DEPTH_BUFFER_MSAA" ) };
	m_effect->SetOption( BlueSharedString( "DEPTH_BUFFER_TYPE" ), msaaOptions[std::min( msaaType, 2u )]);

	if( !Tr2Renderer::RunComputeShader( m_effect, perFrameData.tilesX, perFrameData.tilesY, 1, renderContext ) )
	{
		return E_FAIL;
	}
	return S_OK;
}

ALResult Tr2LightManager::UpdateLists( uint32_t msaaType, Tr2RenderContext& renderContext )
{
	m_lightBufferVariable = m_lightBuffer;
	m_indexBufferVariable = m_indexBuffer;

	if( m_lightData.GetCount() == 0 )
	{
		return ClearLightIndices( renderContext );
	}

	auto hr = DoUpdateLists( msaaType, renderContext );
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
		m_perFrameData.Destroy();
	}
}

bool Tr2LightManager::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_lightBuffer->Create( LIGHT_BUFFER_SIZE, sizeof( PerLightData ), Tr2GpuBuffer::CPU_WRITABLE );
	m_indexBuffer->Create( INDEX_BUFFER_SIZE, sizeof( uint32_t ), Tr2GpuStructuredBuffer::GPU_WRITABLE | Tr2GpuStructuredBuffer::COUNTER );
	m_perFrameData.Create( sizeof( PerFrameData ), Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY, nullptr, renderContext );

	m_effect->SetParameter( BlueSharedString( "LightBuffer" ), m_lightBuffer );
	m_effect->SetParameter( BlueSharedString( "LightIndices" ), m_indexBuffer, 0 );

	return true;
}
