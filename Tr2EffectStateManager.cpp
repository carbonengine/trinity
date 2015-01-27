#include "StdAfx.h"

#include "Tr2EffectStateManager.h"

#include "TriRenderBatch.h"
#include "Tr2PerObjectData.h"
#include "Include/ITriConstants.h"
#include "ITr2ShaderState.h"

using namespace Tr2RenderContextEnum;

namespace {

	// These are shared across managers.

	typedef std::vector<std::pair<Tr2VertexDefinition, Tr2VertexLayoutAL*>>	VertexLayoutMap_t;
	VertexLayoutMap_t	s_vertexLayoutMap;

	std::vector<Tr2ShaderAL*>			s_shaders[ Tr2RenderContextEnum::SHADER_TYPE_COUNT ];

	std::vector<std::pair<Tr2SamplerDescription, Tr2SamplerStateAL*>> s_samplerSetups;

	typedef std::vector<uint32_t>		TRenderStateKeyValues;
	std::vector<TRenderStateKeyValues>	s_renderStateSetups;
}

void Tr2EffectStateManager::CurrentValues::Reset()
{
	for( int j = 0; j < SHADER_TYPE_COUNT; ++j )
	{
		for( int i = 0; i < SAMPLER_MAX_COUNT; ++i )
		{
			m_samplerTextures[j][i] = std::make_pair( nullptr, Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
			m_samplerSetupBinding[j][i] = UNKNOWN;
		}
	 
		m_shader[j] = UNKNOWN;
	}

	m_renderingMode = (Tr2EffectStateManager::RenderingMode)UNKNOWN;
	m_renderStateSetup = UNKNOWN;

	m_vertexDeclaration = UNKNOWN;
	m_indexBuffer = nullptr;
	for( int i = 0; i < VERTEX_STREAM_MAX_COUNT; ++i )
	{
		m_streams[i].m_vertexBuffer = nullptr;
		m_streams[i].m_offset = UNKNOWN;
		m_streams[i].m_stride = UNKNOWN;
	}
}

Tr2EffectStateManager::Tr2EffectStateManager( Tr2RenderContext &renderContext )
	: m_renderContext( renderContext )
	, m_fillMode( Tr2RenderContextEnum::FM_SOLID )
	, m_isCullModeInverted( false )
	, m_isManagedRendering( false )
{
}

uint32_t Tr2EffectStateManager::RegisterRenderStateSetup( const Tr2RenderStateSetup& rss )
{
	TRenderStateKeyValues kv;
	kv.reserve( rss.size() * 2 );
	for( auto it = rss.cbegin(); it != rss.cend(); ++it )
	{
		kv.push_back( it->first );
		kv.push_back( it->second );
	}

	if( rss.size() > 1 )
	{
		for( size_t i = 1; i < rss.size(); ++i )
		{
			if( kv[i*2+0] == RS_CULLMODE )
			{
				std::swap( kv[0], kv[i*2+0] );
				std::swap( kv[1], kv[i*2+1] );
				break;
			}
		}
	}
	
	for( uint32_t i = 0; i != s_renderStateSetups.size(); ++i )
	{
		if( kv == s_renderStateSetups[i] )
		{
			// We've seen this setup before
			return i;
		}
	}

	// New setup, add it
	s_renderStateSetups.push_back( kv );
	return (uint32_t)s_renderStateSetups.size()-1;
}

uint32_t Tr2EffectStateManager::RegisterShader( 
	ShaderType type, 
	const void* bytecode, 
	uint32_t bytecodeSize, 
	const void* patchedBytecode, 
	uint32_t patchedBytecodeSize, 
	const Tr2ShaderInputDefinition& inputDefinition )
{
	for( size_t i = 0; i != s_shaders[type].size(); ++i )
	{
		const Tr2ShaderAL& existing = *s_shaders[type][i];

		uint32_t bufferSize = 0;
		const void* buffer;
		if( FAILED( existing.GetBytecode( buffer, bufferSize ) ) )
		{
			continue;
		}

		if( bufferSize == bytecodeSize )
		{
			if( memcmp( buffer, bytecode, bufferSize ) == 0 )
			{
				// We've seen this setup before
				return (uint32_t)i;
			}
		}
	}

	// New shader, add it; created using the primary rendercontext.
	std::unique_ptr<Tr2ShaderAL> shader( new Tr2ShaderAL );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL(	shader->Create( renderContext, 
								type, 
								bytecode, 
								bytecodeSize, 
								patchedBytecode, 
								patchedBytecodeSize, 
								inputDefinition )
				, UNKNOWN );

	s_shaders[type].push_back( shader.release() );

	return (uint32_t)s_shaders[type].size() - 1;
}


void Tr2EffectStateManager::Initialize()
{
	m_currentValues.Reset();
	m_isManagedRendering	= false;
}

void Tr2EffectStateManager::Shutdown()
{
	for( int i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		for( auto it = s_shaders[i].begin(); it != s_shaders[i].end(); ++it )
		{
			delete *it;
		}
		s_shaders[i].clear();
	}
    for( auto it = s_vertexLayoutMap.begin(); it != s_vertexLayoutMap.end(); ++it )
    {
        delete it->second;
    }
	s_vertexLayoutMap.clear();
}

void Tr2EffectStateManager::BeginManagedRendering()
{
	D3DPERF_EVENT(L"Tr2EffectStateManager::BeginManagedRendering");

	m_currentValues.Reset();

	for( int i = 0; i < SHADER_TYPE_COUNT; ++i )
	{
		if( SHADER_TYPE_EXISTS( i ) )
		{
			m_renderContext.SetShader( nullShader[i] );
		}
	}

	m_isManagedRendering = true;

	Tr2EffectStateManager::SetInvertedCullMode( false );

	if( !m_renderContext.IsValid() )
	{
		return;
	}

	m_renderContext.SetRenderState( RS_FILLMODE, m_fillMode );

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
	for( int i = 0; i < MAX_TEXTURESTAGES; ++i )
	{
		m_renderContext.m_d3dDevice9->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
	}
#endif

	m_renderContext.SetRenderState( RS_CLIPPLANEENABLE, 0x0 );
}

void Tr2EffectStateManager::EndManagedRendering()
{
	D3DPERF_EVENT(L"Tr2EffectStateManager::EndManagedRendering");

	UnsetAllTextures();
	m_isManagedRendering = false;
}

void Tr2EffectStateManager::ApplyRenderStates( uint32_t ix )
{
	if( m_isManagedRendering )
	{
		if( ix == m_currentValues.m_renderStateSetup )
		{
			return;
		}
	}

	if( ix < s_renderStateSetups.size() )
	{
		D3DPERF_EVENT( L"ApplyRenderStates" );
		ApplyStandardStates( m_currentValues.m_renderingMode );
		auto& kv = s_renderStateSetups[ix];

		if( !kv.empty() )
		{
			uint32_t oldValue = kv[1];
			if( Tr2EffectStateManager::IsCullModeInverted() && kv[0] == RS_CULLMODE )
			{
				if( kv[1] == CULLMODE_CW )
				{
					kv[1] = CULLMODE_CCW;
				}
				else if( kv[1] == CULLMODE_CCW )
				{
					kv[1] = CULLMODE_CW;
				}
				else
				{
					kv[1] = CULLMODE_NONE;
				}
			}

			m_renderContext.SetRenderStates( &kv[0], (uint32_t)kv.size() / 2 );
			kv[1] = oldValue;
		}
	}

	m_currentValues.m_renderStateSetup = ix;
}

void Tr2EffectStateManager::ApplySamplerSetup( ShaderType inputType, uint32_t samplerIx, uint32_t ix )
{
	

	if( m_isManagedRendering )
	{
		if( ix == m_currentValues.m_samplerSetupBinding[inputType][samplerIx] )
		{
			return;
		}
		m_currentValues.m_samplerSetupBinding[inputType][samplerIx] = ix;
	}

	if( ix < s_samplerSetups.size() )
	{
		Tr2SamplerStateAL& ss = *s_samplerSetups[ix].second;
		{
			D3DPERF_EVENT( L"ApplySamplerSetup" );
			m_renderContext.SetSamplerState( ss, inputType, samplerIx );
		}
	}
}

void Tr2EffectStateManager::ForgetTexture( const Tr2TextureAL& texture )
{
	using namespace std;
	for( int type = 0; type < SHADER_TYPE_COUNT; ++type )
	{
		for( int i = 0; i < SAMPLER_MAX_COUNT; ++i )
		{
			if( m_currentValues.m_samplerTextures[type][i].first == &texture )
			{
				m_currentValues.m_samplerTextures[type][i] = std::make_pair( nullptr, Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
			}
		}
	}
}

void Tr2EffectStateManager::ApplyShaderBuffer( ShaderType inputType, uint32_t samplerIx, const Tr2GpuBufferAL& buffer )
{
	

	if( m_isManagedRendering )
	{
		auto record = std::make_pair( Tr2ObjectALOpaquePointer( &buffer ), Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
		if( record == m_currentValues.m_samplerTextures[inputType][samplerIx] )
		{
			return;
		}
		m_currentValues.m_samplerTextures[inputType][samplerIx] = record;
	}

	m_renderContext.SetShaderBuffer( inputType, samplerIx, buffer );
}

void Tr2EffectStateManager::ApplyTexture( ShaderType inputType, uint32_t samplerIx, const Tr2TextureAL& texture, ColorSpace colorSpace )
{
	

	if( m_isManagedRendering )
	{
		auto record = std::make_pair( Tr2ObjectALOpaquePointer( &texture ), colorSpace );
		if( record == m_currentValues.m_samplerTextures[inputType][samplerIx] )
		{
			return;
		}
		m_currentValues.m_samplerTextures[inputType][samplerIx] = record;
	}

	m_renderContext.SetTexture( inputType, samplerIx, texture, colorSpace );
}

void Tr2EffectStateManager::UnsetAllTextures()
{
	for( int type = 0; type < SHADER_TYPE_COUNT; ++type )
	{
		if( SHADER_TYPE_EXISTS( type ) )
		{
#if TRINITY_PLATFORM == TRINITY_DIRECTX9
			const uint32_t maxTextures = type == VERTEX_SHADER ? 4 : 16;
#else
			const uint32_t maxTextures = 16;
#endif
			for( uint32_t i = 0; i < maxTextures; ++i )
			{
				Tr2EffectStateManager::ApplyTexture( ShaderType( type ), i, nullTX );
			}
		}
	}
}

void Tr2EffectStateManager::ApplyShader( ShaderType type, uint32_t ix )
{
	

	if( m_isManagedRendering )
	{
		if( ix == m_currentValues.m_shader[type] )
		{
			return;
		}

		m_currentValues.m_shader[type] = ix;
	}

	if( ix < s_shaders[type].size() )
	{
		D3DPERF_EVENT( L"ApplyShader" );
		m_renderContext.SetShader( *s_shaders[type][ix] );
	}
	else
	{
		m_renderContext.SetShader( nullShader[type] );
	}
}

namespace {

	// when applying a renderMode, should we set the cull state?
	static const bool applyCullMode[Tr2EffectStateManager::RM_COUNT] = 
	{
		false,	// RM_ANY,
		true,	// RM_OPAQUE,
		true,	// RM_DECAL,
		true,	// RM_DECAL_NO_DEPTH,
		true,	// RM_ALPHA,
		false,	// RM_ALPHA_ADDITIVE,
		true,	// RM_DEPTH_ONLY
		true,	// RM_PICKING,
		false,	// RM_FULLSCREEN,
		true,	// RM_SPRITE2D,
		true,	// RM_CULL,
		false,	// RM_LIGHT,
        true,	// RM_ERASE,
        true,	// RM_PREPASS_COLOR,
	};


	// When changing or adding state/value pairs, or adding an entire new block,
	// keep in mind that..
	// 1. the fillmode, if any, must be the first pair.
	// 2. 0, 0  must be the last pair.

	static uint32_t opaquePairs[] = 
	{
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, FALSE,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, TRUE,
		RS_ZFUNC, CMP_LESSEQUAL,
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	static uint32_t decalPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, TRUE,
		RS_ALPHAFUNC, CMP_GREATER, 
		RS_ALPHAREF, 127,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, TRUE,
		RS_ZFUNC, CMP_LESSEQUAL,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	
	static uint32_t decalNoDepthPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, TRUE,
		RS_ALPHAFUNC, CMP_GREATER, 
		RS_ALPHAREF, 127,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_LESSEQUAL,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0 
	};

	static uint32_t alphaPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, TRUE,
		RS_SRCBLEND, BM_SRCALPHA,
		RS_DESTBLEND, BM_INVSRCALPHA,
		RS_BLENDOP, BO_ADD,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_LESSEQUAL,
		RS_ALPHATESTENABLE, FALSE,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	static uint32_t alphaAdditivePairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_CULLMODE, CULLMODE_NONE,
		RS_ALPHABLENDENABLE, TRUE,
		RS_SRCBLEND, BM_ONE,
		RS_DESTBLEND, BM_ONE,
		RS_BLENDOP, BO_ADD,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_LESSEQUAL,
		RS_ALPHATESTENABLE, FALSE,
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	static uint32_t depthOnlyPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, FALSE,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, TRUE,
		RS_ZFUNC, CMP_LESSEQUAL,		
		RS_COLORWRITEENABLE, 0,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	static uint32_t pickingPairs[] = {
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, FALSE,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, TRUE,
		RS_ZFUNC, CMP_LESSEQUAL,
		RS_FILLMODE, FM_SOLID,
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	static uint32_t fullscreenPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, FALSE,
		RS_CULLMODE, CULLMODE_NONE,
		RS_ZENABLE, FALSE,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_ALWAYS,
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		0, 0
	};

	static uint32_t sprite2dPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, TRUE,
		RS_SRCBLEND, BM_ONE,
		RS_DESTBLEND, BM_INVSRCALPHA,
		RS_BLENDOP, BO_ADD,
		RS_ALPHATESTENABLE, FALSE,
		RS_CULLMODE, CULLMODE_NONE,
		RS_ZENABLE, FALSE,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_ALWAYS,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};

	static uint32_t cullPairs[] = {
		0, 0
	};

	static uint32_t lightPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_CULLMODE, CULLMODE_NONE,
		RS_ALPHABLENDENABLE, TRUE,
		RS_SRCBLEND, BM_ONE,
		RS_DESTBLEND, BM_ONE,
		RS_BLENDOP, BO_ADD,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_LESSEQUAL,
		RS_ZENABLE, TRUE,
		RS_ALPHATESTENABLE, FALSE,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, TRUE,
		RS_BLENDOPALPHA, BO_ADD,
		RS_SRCBLENDALPHA, BM_ONE,
		RS_DESTBLENDALPHA, BM_ONE,
		0, 0
	};

	static uint32_t erasePairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, FALSE,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, TRUE,
		RS_ZFUNC, CMP_ALWAYS,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		0, 0
	};

	static uint32_t prepassColorPairs[] = {
		RS_FILLMODE, FM_SOLID,
		RS_ALPHABLENDENABLE, FALSE,
		RS_ALPHATESTENABLE, FALSE,
		RS_ZENABLE, TRUE,
		RS_ZWRITEENABLE, FALSE,
		RS_ZFUNC, CMP_EQUAL,		
		RS_COLORWRITEENABLE, 0x0f,
		RS_DEPTHBIAS, 0,
		RS_SLOPESCALEDEPTHBIAS, 0,
		RS_SEPARATEALPHABLENDENABLE, FALSE,
		0, 0
	};


	static uint32_t * const modePairs[Tr2EffectStateManager::RM_COUNT] = 
	{
		nullptr,			// RM_ANY,
		opaquePairs,		// RM_OPAQUE,
		decalPairs,			// RM_DECAL,
		decalNoDepthPairs,	// RM_DECAL_NO_DEPTH,
		alphaPairs,			// RM_ALPHA,
		alphaAdditivePairs,	// RM_ALPHA_ADDITIVE,
		depthOnlyPairs,		// RM_DEPTH_ONLY
		pickingPairs,		// RM_PICKING,
		fullscreenPairs,	// RM_FULLSCREEN,
		sprite2dPairs,		// RM_SPRITE2D,
		cullPairs,			// RM_CULL,
		lightPairs,			// RM_LIGHT,
        erasePairs,			// RM_ERASE,
        prepassColorPairs,	// RM_PREPASS_COLOR,
	};
}

void Tr2EffectStateManager::ApplyStandardStates( RenderingMode rm )
{
	if( rm > RM_ANY && rm < RM_COUNT )
	{
		if( applyCullMode[ rm ] )
		{
			m_renderContext.SetRenderState( RS_CULLMODE, Tr2Renderer::IsRightHanded() != IsCullModeInverted() ? CULLMODE_CW : CULLMODE_CCW );
		}
		auto pairs = modePairs[rm];
		if( pairs[0] == RS_FILLMODE )
		{
			pairs[1] = m_fillMode;
		}

		m_renderContext.SetRenderStates( modePairs[rm], 0 );
	}

	m_currentValues.m_renderingMode = rm;
	m_currentValues.m_renderStateSetup = UNKNOWN;
}

void Tr2EffectStateManager::SetWireframeRendering( bool b )
{
	m_fillMode = b ? Tr2RenderContextEnum::FM_WIREFRAME : Tr2RenderContextEnum::FM_SOLID;
}

void Tr2EffectStateManager::SetInvertedCullMode( bool b )
{
	m_isCullModeInverted = b;
}

bool Tr2EffectStateManager::IsCullModeInverted( void )
{
	return m_isCullModeInverted;
}

uint32_t Tr2EffectStateManager::GetVertexDeclarationHandle( const Tr2VertexDefinition& vertexDefinition )
{
	for( size_t i = 0; i != s_vertexLayoutMap.size(); ++i )
	{
		if( s_vertexLayoutMap[i].first == vertexDefinition )
		{
			return (uint32_t)i;
		}
	}

	s_vertexLayoutMap.push_back( std::make_pair( vertexDefinition, new Tr2VertexLayoutAL() ) );
	return (uint32_t)s_vertexLayoutMap.size() - 1;
}

void Tr2EffectStateManager::ApplyVertexDeclaration( uint32_t declaration )
{
	if( declaration == m_currentValues.m_vertexDeclaration )
	{
		return;
	}

	
	CCP_ASSERT( declaration != UNINITIALIZED_DECLARATION );
	CCP_ASSERT( declaration < s_vertexLayoutMap.size() );

	if( declaration < s_vertexLayoutMap.size() )
	{
		Tr2VertexLayoutAL& hvl = *s_vertexLayoutMap[declaration].second;
		if( !hvl.IsValid() )
		{
			hvl.Create( s_vertexLayoutMap[declaration].first, m_renderContext );	
		}
		m_renderContext.SetVertexLayout( hvl );
	}

	m_currentValues.m_vertexDeclaration = declaration;
}

bool Tr2EffectStateManager::GetVertexDeclarationElements( uint32_t declaration, Tr2VertexDefinition& definition )
{
	CCP_ASSERT( declaration < s_vertexLayoutMap.size() || declaration == UNINITIALIZED_DECLARATION );
	if( declaration < s_vertexLayoutMap.size() )
	{
		definition = s_vertexLayoutMap[ declaration ].first;
		return true;
	}
	return false;
}

void Tr2EffectStateManager::ApplyStreamSource( uint32_t stream, const Tr2VertexBufferAL & buffer, uint32_t offset, uint32_t stride )
{
	
	if( m_isManagedRendering )
	{
		if( &buffer == m_currentValues.m_streams[stream].m_vertexBuffer	&& 
			offset  == m_currentValues.m_streams[stream].m_offset		&& 
			stride  == m_currentValues.m_streams[stream].m_stride )
		{
			return;
		}

		m_currentValues.m_streams[stream].m_vertexBuffer = &buffer;
		m_currentValues.m_streams[stream].m_offset = offset;
		m_currentValues.m_streams[stream].m_stride = stride;
	}

	m_renderContext.SetStreamSource( stream, buffer, offset, stride );
}

void Tr2EffectStateManager::ApplyIndexBuffer( const Tr2IndexBufferAL & indices )
{
	
	if( m_isManagedRendering )
	{
		if( &indices == m_currentValues.m_indexBuffer )
		{
			return;
		}

		m_currentValues.m_indexBuffer = &indices;
	}

	m_renderContext.SetIndices( indices );
}


uint32_t Tr2EffectStateManager::RegisterSamplerSetup( const Tr2SamplerDescription& description )
{
	for( size_t i = 0; i < s_samplerSetups.size(); ++i )
	{
		const Tr2SamplerDescription& existing = s_samplerSetups[i].first;
		if( existing == description )
		{
			// We've seen this setup before
			return (uint32_t)i;
		}
	}

	// New setup, add it
	USE_MAIN_THREAD_RENDER_CONTEXT();
	Tr2SamplerStateAL* ss( new Tr2SamplerStateAL );
	if( FAILED( ss->Create( renderContext, description ) ) )
	{
		delete ss;
		return UNKNOWN;
	}
	s_samplerSetups.push_back( std::make_pair( description, ss ) );
	return (uint32_t)s_samplerSetups.size() - 1;
}

void Tr2EffectStateManager::ReleaseDeviceResources( TriStorage s )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( (s & TRISTORAGE_ALL) == TRISTORAGE_ALL )
	{
		if( renderContext.IsValid() )
		{
			for( int type = 0; type < SHADER_TYPE_COUNT; ++type )
			{
				for( uint32_t i = 0; i < SAMPLER_MAX_COUNT; ++i )
				{
					m_renderContext.SetTexture( ShaderType( type ), i, nullTX );
				}
			}
		
			for( uint32_t i = 0; i < VERTEX_STREAM_MAX_COUNT; ++i )
			{
				m_renderContext.SetStreamSource( i, nullVB, 0, 0 );
			}
			m_renderContext.SetIndices( nullIB );

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
			// ... ehm.. what about the other platforms
			m_renderContext.m_d3dDevice9->SetVertexDeclaration( nullptr );
#endif
		}
		m_currentValues.Reset();

		for( int i = 0; i < SHADER_TYPE_COUNT; ++i )
		{
			for( auto it = s_shaders[i].begin(); it != s_shaders[i].end(); ++it )
			{
				delete *it;
			}
			s_shaders[i].clear();
		}
        for( auto it = s_vertexLayoutMap.begin(); it != s_vertexLayoutMap.end(); ++it )
        {
            delete it->second;
        }
		s_vertexLayoutMap.clear();

		s_renderStateSetups.clear();
		for( auto it = s_samplerSetups.begin(); it != s_samplerSetups.end(); ++it )
		{
			delete it->second;
		}
		s_samplerSetups.clear();

		for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
		{
			m_perObjectConstantBuffers[i].Destroy();
		}
	}
}
