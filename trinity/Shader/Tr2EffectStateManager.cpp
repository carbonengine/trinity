// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2EffectStateManager.h"

#include "TriRenderBatch.h"
#include "Tr2PerObjectData.h"

#include <ITriConstants.h>

using namespace Tr2RenderContextEnum;

namespace
{

// These are shared across managers.

typedef std::vector<std::pair<Tr2VertexDefinition, Tr2VertexLayoutAL*>> VertexLayoutMap_t;
VertexLayoutMap_t s_vertexLayoutMap;
std::mutex s_vertexLayoutMutex;

std::vector<Tr2ShaderAL*> s_shaders;
std::vector<Tr2ShaderBytecodeAL*> s_shaderLibraries;
std::vector<std::pair<Tr2ShaderProgramAL*, std::vector<uint32_t>>> s_shaderPrograms;

typedef std::vector<uint32_t> TRenderStateKeyValues;

static uint32_t opaquePairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	TRUE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t decalPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	TRUE,
	RS_ALPHAFUNC,
	CMP_GREATER,
	RS_ALPHAREF,
	127,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	TRUE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};


static uint32_t decalNoDepthPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	TRUE,
	RS_ALPHAFUNC,
	CMP_GREATER,
	RS_ALPHAREF,
	127,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t alphaPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	TRUE,
	RS_SRCBLEND,
	BM_SRCALPHA,
	RS_DESTBLEND,
	BM_INVSRCALPHA,
	RS_BLENDOP,
	BO_ADD,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t alphaAdditivePairs[] = {
	RS_FILLMODE,
	FM_SOLID,
	RS_CULLMODE,
	CULLMODE_NONE,
	RS_ALPHABLENDENABLE,
	TRUE,
	RS_SRCBLEND,
	BM_ONE,
	RS_DESTBLEND,
	BM_ONE,
	RS_BLENDOP,
	BO_ADD,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_COLORWRITEENABLE,
	0x7,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t depthOnlyPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	TRUE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_COLORWRITEENABLE,
	0,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t pickingPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	TRUE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_FILLMODE,
	FM_SOLID,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t fullscreenPairs[] = {
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_CULLMODE,
	CULLMODE_NONE,
	RS_ZENABLE,
	FALSE,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_ALWAYS,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
};

static uint32_t sprite2dPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	TRUE,
	RS_SRCBLEND,
	BM_ONE,
	RS_DESTBLEND,
	BM_INVSRCALPHA,
	RS_BLENDOP,
	BO_ADD,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_CULLMODE,
	CULLMODE_NONE,
	RS_ZENABLE,
	FALSE,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_ALWAYS,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

static uint32_t cullPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
};

static uint32_t lightPairs[] = {
	RS_FILLMODE,
	FM_SOLID,
	RS_CULLMODE,
	CULLMODE_NONE,
	RS_ALPHABLENDENABLE,
	TRUE,
	RS_SRCBLEND,
	BM_ONE,
	RS_DESTBLEND,
	BM_ONE,
	RS_BLENDOP,
	BO_ADD,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_LESSEQUAL,
	RS_ZENABLE,
	TRUE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	TRUE,
	RS_BLENDOPALPHA,
	BO_ADD,
	RS_SRCBLENDALPHA,
	BM_ONE,
	RS_DESTBLENDALPHA,
	BM_ONE,
};

static uint32_t erasePairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	TRUE,
	RS_ZFUNC,
	CMP_ALWAYS,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
};

static uint32_t prepassColorPairs[] = {
	RS_CULLMODE,
	CULLMODE_CW,
	RS_FILLMODE,
	FM_SOLID,
	RS_ALPHABLENDENABLE,
	FALSE,
	RS_ALPHATESTENABLE,
	FALSE,
	RS_ZENABLE,
	TRUE,
	RS_ZWRITEENABLE,
	FALSE,
	RS_ZFUNC,
	CMP_EQUAL,
	RS_COLORWRITEENABLE,
	0x0f,
	RS_DEPTHBIAS,
	0,
	RS_SLOPESCALEDEPTHBIAS,
	0,
	RS_SEPARATEALPHABLENDENABLE,
	FALSE,
};

struct ModeList
{
	uint32_t* list;
	uint32_t count;
};

static ModeList const modePairs[Tr2EffectStateManager::RM_COUNT] = {
	{ nullptr, 0 },
	{ opaquePairs, sizeof( opaquePairs ) / sizeof( uint32_t ) },
	{ decalPairs, sizeof( decalPairs ) / sizeof( uint32_t ) },
	{ decalNoDepthPairs, sizeof( decalNoDepthPairs ) / sizeof( uint32_t ) },
	{ alphaPairs, sizeof( alphaPairs ) / sizeof( uint32_t ) },
	{ alphaAdditivePairs, sizeof( alphaAdditivePairs ) / sizeof( uint32_t ) },
	{ depthOnlyPairs, sizeof( depthOnlyPairs ) / sizeof( uint32_t ) },
	{ pickingPairs, sizeof( pickingPairs ) / sizeof( uint32_t ) },
	{ fullscreenPairs, sizeof( fullscreenPairs ) / sizeof( uint32_t ) },
	{ sprite2dPairs, sizeof( sprite2dPairs ) / sizeof( uint32_t ) },
	{ cullPairs, sizeof( cullPairs ) / sizeof( uint32_t ) },
	{ lightPairs, sizeof( lightPairs ) / sizeof( uint32_t ) },
	{ erasePairs, sizeof( erasePairs ) / sizeof( uint32_t ) },
	{ prepassColorPairs, sizeof( prepassColorPairs ) / sizeof( uint32_t ) },
};


struct RenderStateSetups : public std::vector<TRenderStateKeyValues>
{
public:
	RenderStateSetups()
	{
		resize( Tr2EffectStateManager::RM_COUNT );
		for( int i = 0; i < Tr2EffectStateManager::RM_COUNT; ++i )
		{
			at( i ).assign( modePairs[i].list, modePairs[i].list + modePairs[i].count );
		}
	}
};

RenderStateSetups s_renderStateSetups;


}

void Tr2EffectStateManager::CurrentValues::Reset()
{
	m_shaderProgram = UNKNOWN;

	m_renderingMode = (Tr2EffectStateManager::RenderingMode)UNKNOWN;
	m_renderStateSetup = UNKNOWN;

	m_vertexDeclaration = UNKNOWN;
	m_indexBuffer = Tr2BufferAL();
	m_indexStride = 0;
	for( int i = 0; i < VERTEX_STREAM_MAX_COUNT; ++i )
	{
		m_streams[i].m_vertexBuffer = Tr2BufferAL();
		m_streams[i].m_offset = UNKNOWN;
		m_streams[i].m_stride = UNKNOWN;
	}
}

Tr2EffectStateManager::Tr2EffectStateManager( Tr2RenderContext& renderContext ) :
	m_renderContext( renderContext ),
	m_isManagedRendering( false ),
	m_renderTargetWidth( 0 ),
	m_renderTargetHeight( 0 )
{
	std::fill( std::begin( m_renderStateOverrides ), std::end( m_renderStateOverrides ), nullptr );
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
	return (uint32_t)s_renderStateSetups.size() - 1;
}

uint32_t Tr2EffectStateManager::RegisterShader(
	ShaderType type,
	const Tr2ShaderBytecodeAL& bytecode,
	const Tr2ShaderSignatureAL& signature,
	const char* shaderPath )
{
	for( size_t i = 0; i != s_shaders.size(); ++i )
	{
		const Tr2ShaderAL& existing = *s_shaders[i];
		if( existing.GetType() != type )
		{
			continue;
		}

		Tr2ShaderBytecodeAL existingBytecode;
		if( FAILED( existing.GetBytecode( existingBytecode ) ) )
		{
			continue;
		}

		if( existingBytecode.size == bytecode.size )
		{
			if( memcmp( existingBytecode.bytecode, bytecode.bytecode, bytecode.size ) == 0 )
			{
				if( existing.GetSignature().samplers == signature.samplers )
				{
					// We've seen this setup before
					return (uint32_t)i;
				}
			}
		}
	}

	// New shader, add it; created using the primary rendercontext.
	std::unique_ptr<Tr2ShaderAL> shader( new Tr2ShaderAL );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( shader->Create(
					   type,
					   bytecode,
					   signature,
					   shaderPath,
					   renderContext ),
				   UNKNOWN );

	s_shaders.push_back( shader.release() );

	return (uint32_t)s_shaders.size() - 1;
}

uint32_t Tr2EffectStateManager::RegisterShaderLibrary( const Tr2ShaderBytecodeAL& bytecode )
{
	for( size_t i = 0; i != s_shaderLibraries.size(); ++i )
	{
		auto& existing = *s_shaderLibraries[i];
		if( existing.size == bytecode.size )
		{
			if( memcmp( existing.bytecode, bytecode.bytecode, bytecode.size ) == 0 )
			{
				return (uint32_t)i;
			}
		}
	}

	void* code = new uint8_t[bytecode.size];
	memcpy( code, bytecode.bytecode, bytecode.size );
	Tr2ShaderBytecodeAL* newBytecode = new Tr2ShaderBytecodeAL( code, bytecode.size );
	s_shaderLibraries.push_back( newBytecode );
	return uint32_t( s_shaderLibraries.size() - 1 );
}

const Tr2ShaderBytecodeAL* Tr2EffectStateManager::GetShaderLibraryCode( uint32_t handle )
{
	if( handle >= s_shaderLibraries.size() )
	{
		return nullptr;
	}
	return s_shaderLibraries[handle];
}

uint32_t Tr2EffectStateManager::RegisterShaderProgram( uint32_t* shaders, size_t count )
{
	if( !count )
	{
		return UNKNOWN;
	}
	std::vector<uint32_t> shaderHandles;
	shaderHandles.resize( count );
	std::vector<Tr2ShaderAL> args;
	args.resize( count );
	for( size_t i = 0; i < count; ++i )
	{
		if( shaders[i] >= s_shaders.size() )
		{
			return UNKNOWN;
		}
		shaderHandles[i] = shaders[i];
		args[i] = *s_shaders[shaders[i]];
	}
	for( size_t i = 0; i < s_shaderPrograms.size(); ++i )
	{
		if( s_shaderPrograms[i].second == shaderHandles )
		{
			return uint32_t( i );
		}
	}

	std::unique_ptr<Tr2ShaderProgramAL> shader( new Tr2ShaderProgramAL );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	CR_RETURN_VAL( shader->Create( &args[0], count, renderContext ), UNKNOWN );

	s_shaderPrograms.push_back( std::make_pair( shader.release(), shaderHandles ) );

	return (uint32_t)s_shaderPrograms.size() - 1;
}

uint32_t Tr2EffectStateManager::RegisterShaderProgramOverride( uint32_t originalProgram, uint32_t overrideProgram )
{
	if( originalProgram >= s_shaderPrograms.size() )
	{
		return UNKNOWN;
	}
	if( overrideProgram >= s_shaderPrograms.size() )
	{
		return UNKNOWN;
	}

	uint32_t pixelShader = UNKNOWN;
	auto& op = s_shaderPrograms[overrideProgram].second;
	for( auto it = op.begin(); it != op.end(); ++it )
	{
		if( *it >= s_shaders.size() )
		{
			return UNKNOWN;
		}
		if( s_shaders[*it]->GetType() == PIXEL_SHADER )
		{
			pixelShader = *it;
		}
	}
	if( pixelShader == UNKNOWN )
	{
		return UNKNOWN;
	}

	std::vector<uint32_t> args = s_shaderPrograms[originalProgram].second;
	for( auto it = args.begin(); it != args.end(); ++it )
	{
		if( *it >= s_shaders.size() )
		{
			return UNKNOWN;
		}
		if( s_shaders[*it]->GetType() == PIXEL_SHADER )
		{
			*it = pixelShader;
			pixelShader = -1;
		}
	}
	if( args.empty() )
	{
		return UNKNOWN;
	}
	return RegisterShaderProgram( &args[0], args.size() );
}


void Tr2EffectStateManager::Initialize()
{
	m_viewportSizeVar.Register( "ViewportSize", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );

	m_currentValues.Reset();
	m_isManagedRendering = false;
}

void Tr2EffectStateManager::SetRenderStateOverride( Tr2RenderContextEnum::RenderState state, const uint32_t* overrides )
{
	if( m_renderStateOverrides[state] == overrides )
	{
		return;
	}
	m_renderStateOverrides[state] = overrides;
	for( auto it = m_renderStates.begin(); it != m_renderStates.end(); ++it )
	{
		it->dirty = true;
	}
}

void Tr2EffectStateManager::Shutdown()
{
	for( auto it = s_shaders.begin(); it != s_shaders.end(); ++it )
	{
		delete *it;
	}
	s_shaders.clear();

	for( auto it = s_shaderLibraries.begin(); it != s_shaderLibraries.end(); ++it )
	{
		//delete[]( *it )->bytecode;
		delete *it;
	}
	s_shaderLibraries.clear();
}

void Tr2EffectStateManager::BeginManagedRendering( Tr2RenderContextEnum::CullMode cullmode )
{
	D3DPERF_EVENT( L"Tr2EffectStateManager::BeginManagedRendering" );

	m_currentValues.Reset();

	m_renderContext.SetShaderProgram( Tr2ShaderProgramAL() );

	m_isManagedRendering = true;

	switch( cullmode )
	{
	case Tr2RenderContextEnum::CULLMODE_CW:
		Tr2EffectStateManager::SetInvertedCullMode( false );
		break;
	case Tr2RenderContextEnum::CULLMODE_CCW:
		Tr2EffectStateManager::SetInvertedCullMode( true );
		break;
	case Tr2RenderContextEnum::CULLMODE_NONE:
		break;
	}

	if( !m_renderContext.IsValid() )
	{
		return;
	}
}

void Tr2EffectStateManager::EndManagedRendering()
{
	D3DPERF_EVENT( L"Tr2EffectStateManager::EndManagedRendering" );

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
		DoApplyRenderStates( ix );
	}
	m_currentValues.m_renderStateSetup = ix;
}

void Tr2EffectStateManager::DoApplyRenderStates( uint32_t ix )
{
	if( ix >= m_renderStates.size() )
	{
		m_renderStates.resize( s_renderStateSetups.size() );
	}

	if( m_renderStates[ix].dirty )
	{
		const auto& source = s_renderStateSetups[ix];
		auto& dest = m_renderStates[ix].states;
		dest.resize( source.size() );
		for( size_t i = 0; i < dest.size(); i += 2 )
		{
			auto state = source[i];
			dest[i] = state;
			if( m_renderStateOverrides[state] )
			{
				dest[i + 1] = m_renderStateOverrides[state][source[i + 1]];
			}
			else
			{
				dest[i + 1] = source[i + 1];
			}
		}
		m_renderStates[ix].dirty = false;
	}

	auto& kv = m_renderStates[ix].states;

	if( !kv.empty() )
	{
		m_renderContext.SetRenderStates( &kv[0], (uint32_t)kv.size() / 2 );
	}
}

void Tr2EffectStateManager::ApplyShaderProgram( uint32_t ix )
{
	if( m_isManagedRendering )
	{
		if( ix == m_currentValues.m_shaderProgram )
		{
			return;
		}

		m_currentValues.m_shaderProgram = ix;
	}

	if( ix < s_shaderPrograms.size() )
	{
		m_renderContext.SetShaderProgram( *s_shaderPrograms[ix].first );
	}
	else
	{
		m_renderContext.SetShaderProgram( Tr2ShaderProgramAL() );
	}
}

Tr2ShaderProgramAL* Tr2EffectStateManager::GetShaderProgram( uint32_t ix )
{
	if( ix < s_shaderPrograms.size() )
	{
		return s_shaderPrograms[ix].first;
	}
	return nullptr;
}


void Tr2EffectStateManager::ApplyStandardStates( RenderingMode rm )
{
	if( rm > RM_ANY && rm < RM_COUNT )
	{
		DoApplyRenderStates( uint32_t( rm ) );
	}

	m_currentValues.m_renderingMode = rm;
	m_currentValues.m_renderStateSetup = UNKNOWN;
}

void Tr2EffectStateManager::SetWireframeRendering( bool b )
{
	static const uint32_t overrides[] = { 0, FM_POINT, FM_WIREFRAME, FM_WIREFRAME };

	if( b )
	{
		SetRenderStateOverride( RS_FILLMODE, overrides );
	}
	else
	{
		SetRenderStateOverride( RS_FILLMODE, nullptr );
	}
}

void Tr2EffectStateManager::SetInvertedCullMode( bool b )
{
	static const uint32_t overrides[] = { 0, CULLMODE_NONE, CULLMODE_CCW, CULLMODE_CW };

	if( b )
	{
		SetRenderStateOverride( RS_CULLMODE, overrides );
	}
	else
	{
		SetRenderStateOverride( RS_CULLMODE, nullptr );
	}
}

bool Tr2EffectStateManager::IsCullModeInverted( void )
{
	return m_renderStateOverrides[RS_CULLMODE] != nullptr;
}

void Tr2EffectStateManager::SetInvertedDepthTest( bool b )
{
	static const uint32_t overrides[] = {
		0,
		CMP_NEVER,
		CMP_GREATER,
		CMP_EQUAL,
		CMP_GREATEREQUAL,
		CMP_LESS,
		CMP_NOTEQUAL,
		CMP_LESSEQUAL,
		CMP_ALWAYS
	};

	if( b )
	{
		SetRenderStateOverride( RS_ZFUNC, overrides );
	}
	else
	{
		SetRenderStateOverride( RS_ZFUNC, nullptr );
	}
}

bool Tr2EffectStateManager::IsDepthTestInverted( void ) const
{
	return m_renderStateOverrides[RS_ZFUNC] != nullptr;
}

uint32_t Tr2EffectStateManager::GetVertexDeclarationHandle( const Tr2VertexDefinition& vertexDefinition )
{
	std::scoped_lock lock( s_vertexLayoutMutex );

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
	if( m_isManagedRendering && declaration == m_currentValues.m_vertexDeclaration )
	{
		return;
	}


	CCP_ASSERT( declaration != UNINITIALIZED_DECLARATION );

	if( declaration == NULL_DECLARATION )
	{
		m_renderContext.SetVertexLayout( Tr2VertexLayoutAL() );
	}
	else
	{
		CCP_ASSERT( declaration < s_vertexLayoutMap.size() );

		if( declaration < s_vertexLayoutMap.size() )
		{
			Tr2VertexLayoutAL& hvl = *s_vertexLayoutMap[declaration].second;
			if( !hvl.IsValid() )
			{
				USE_MAIN_THREAD_RENDER_CONTEXT();

				hvl.Create( s_vertexLayoutMap[declaration].first, renderContext );
			}
			m_renderContext.SetVertexLayout( hvl );
		}
	}
	m_currentValues.m_vertexDeclaration = declaration;
}

bool Tr2EffectStateManager::GetVertexDeclarationElements( uint32_t declaration, Tr2VertexDefinition& definition )
{
	std::scoped_lock lock( s_vertexLayoutMutex );

	CCP_ASSERT( declaration < s_vertexLayoutMap.size() || declaration == UNINITIALIZED_DECLARATION );
	if( declaration < s_vertexLayoutMap.size() )
	{
		definition = s_vertexLayoutMap[declaration].first;
		return true;
	}
	return false;
}

void Tr2EffectStateManager::ApplyStreamSource( uint32_t stream, const Tr2SuballocatedBuffer::Allocation& vertices )
{
	ApplyStreamSource( stream, vertices.GetBuffer(), vertices.GetOffset(), vertices.GetStride() );
}

void Tr2EffectStateManager::ApplyStreamSource( uint32_t stream, const Tr2BufferAL& buffer, uint32_t offset, uint32_t stride )
{

	if( m_isManagedRendering )
	{
		if( buffer == m_currentValues.m_streams[stream].m_vertexBuffer &&
			offset == m_currentValues.m_streams[stream].m_offset &&
			stride == m_currentValues.m_streams[stream].m_stride )
		{
			return;
		}

		m_currentValues.m_streams[stream].m_vertexBuffer = buffer;
		m_currentValues.m_streams[stream].m_offset = offset;
		m_currentValues.m_streams[stream].m_stride = stride;
	}

	m_renderContext.SetStreamSource( stream, buffer, offset, stride );
}


void Tr2EffectStateManager::ApplyIndexBuffer( const Tr2BufferAL& indices )
{
	ApplyIndexBuffer( indices, indices.GetDesc().stride );
}

void Tr2EffectStateManager::ApplyIndexBuffer( const Tr2SuballocatedBuffer::Allocation& indices )
{
	ApplyIndexBuffer( indices.GetBuffer(), indices.GetStride() );
}

void Tr2EffectStateManager::ApplyIndexBuffer( const Tr2BufferAL& indices, const uint32_t stride )
{
	if( stride == 1 )
	{
		CCP_LOGWARN( "Oh no! This is a big bug!" );
	}

	if( m_isManagedRendering )
	{
		if( indices == m_currentValues.m_indexBuffer && stride == m_currentValues.m_indexStride )
		{
			return;
		}
		m_currentValues.m_indexBuffer = indices;
		m_currentValues.m_indexStride = stride;
	}

	m_renderContext.SetIndices( indices, stride );
}


void Tr2EffectStateManager::ReleaseDeviceResources( TriStorage s )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		if( renderContext.IsValid() )
		{
			for( uint32_t i = 0; i < VERTEX_STREAM_MAX_COUNT; ++i )
			{
				m_renderContext.SetStreamSource( i, Tr2BufferAL(), 0, 0 );
			}
			m_renderContext.SetIndices( Tr2BufferAL() );
		}
		m_currentValues.Reset();

		for( auto it = s_shaders.begin(); it != s_shaders.end(); ++it )
		{
			delete *it;
		}
		s_shaders.clear();
		for( auto it = s_shaderPrograms.begin(); it != s_shaderPrograms.end(); ++it )
		{
			delete it->first;
		}
		s_shaderPrograms.clear();
		for( auto it = s_shaderLibraries.begin(); it != s_shaderLibraries.end(); ++it )
		{
			//delete[]( *it )->bytecode;
			delete *it;
		}
		s_shaderLibraries.clear();

		s_renderStateSetups.erase( s_renderStateSetups.begin() + RM_COUNT, s_renderStateSetups.end() );
		m_renderStates.clear();

		for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
		{
			m_perObjectConstantBuffers[i] = Tr2ConstantBufferAL();
		}
	}
}


bool Tr2EffectStateManager::PushDepthStencilBuffer()
{
	return SUCCEEDED( m_renderContext.PushDepthStencil() );
}

bool Tr2EffectStateManager::PushDepthStencilBuffer( const Tr2TextureAL& ds )
{
	m_renderContext.PushDepthStencil();

	return SetDepthStencilBuffer( ds );
}

void Tr2EffectStateManager::PopDepthStencilBuffer()
{
	m_renderContext.PopDepthStencil();
}

void Tr2EffectStateManager::PushRenderTarget( unsigned slot )
{
	m_renderContext.PushRenderTarget( slot );
}

void Tr2EffectStateManager::PushRenderTarget( const Tr2TextureAL& rt, unsigned slot )
{
	m_renderContext.PushRenderTarget( slot );
	SetRenderTarget( slot, rt );
}

void Tr2EffectStateManager::PopRenderTarget( unsigned slot )
{
	m_renderContext.PopRenderTarget( slot );

	unsigned width, height;
	if( slot == 0 && SUCCEEDED( m_renderContext.GetRenderTargetSize( width, height ) ) )
	{
		UpdateRenderTargetViewport( width, height );
	}
}

void Tr2EffectStateManager::PushViewport()
{
	m_viewportStack.push_front( m_viewport );
}

void Tr2EffectStateManager::PopViewport()
{
	CCP_ASSERT( !m_viewportStack.empty() );

	auto item = m_viewportStack.front();
	m_viewportStack.pop_front();

	SetViewport( item );
}


unsigned int Tr2EffectStateManager::GetRenderTargetWidth()
{
	return m_renderTargetWidth;
}

unsigned int Tr2EffectStateManager::GetRenderTargetHeight()
{
	return m_renderTargetHeight;
}

void Tr2EffectStateManager::SetFullScreenViewport()
{
	m_viewport.width = m_renderTargetWidth;
	m_viewport.height = m_renderTargetHeight;
	m_viewport.x = 0;
	m_viewport.y = 0;
	m_viewport.minZ = 0.0f;
	m_viewport.maxZ = 1.0f;

	SetupViewport();
}

void Tr2EffectStateManager::SetViewport( const TriViewport& vp )
{
	// As s_viewport is a CTriViewport we do a "member copy" on the original
	// viewport we got passed in
	SetViewport( vp.width, vp.height, vp.x, vp.y, vp.minZ, vp.maxZ );
}

void Tr2EffectStateManager::SetViewport( int width, int height, int x, int y, float minZ, float maxZ )
{
	// As s_viewport is a CTriViewport we fill this struct
	m_viewport.width = width;
	m_viewport.height = height;
	m_viewport.x = x;
	m_viewport.y = y;
	m_viewport.minZ = minZ;
	m_viewport.maxZ = maxZ;

	SetupViewport();
}

const TriViewport& Tr2EffectStateManager::GetViewport()
{
	return m_viewport;
}

const Tr2Viewport& Tr2EffectStateManager::GetDeviceViewport()
{
	return m_viewportOnDevice;
}

bool Tr2EffectStateManager::SetRenderTarget( unsigned int index, const Tr2TextureAL& rt, bool updateViewport, uint32_t slice )
{
	if( FAILED( m_renderContext.SetRenderTarget( rt, index, slice ) ) )
	{
		CCP_LOGERR( "Failed to set renderTarget to slot %d", index );
		return false;
	}

	if( !index && updateViewport )
	{
		unsigned width, height;
		if( SUCCEEDED( m_renderContext.GetRenderTargetSize( width, height ) ) )
		{
			// don't use rt.GetWidth/Height, rt may be nullRT
			UpdateRenderTargetViewport( width, height );
		}
	}

	return true;
}

bool Tr2EffectStateManager::SetDepthStencilBuffer( const Tr2TextureAL& ds )
{
	if( !SUCCEEDED( m_renderContext.SetDepthStencil( ds ) ) )
	{
		CCP_LOGERR( "Failed to set depth/stencil buffer" );
		return false;
	}

	return true;
}

void Tr2EffectStateManager::SetupContextResources()
{
	// Create all HW vertex layouts here to avoid creating those during multithreaded rendering
	for( auto& it : s_vertexLayoutMap )
	{
		Tr2VertexLayoutAL& hvl = *it.second;
		if( !hvl.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			hvl.Create( it.first, renderContext );
		}
	}
}

void Tr2EffectStateManager::AssignFrom( const Tr2EffectStateManager& other )
{
	BeginManagedRendering();
	if( !other.m_isManagedRendering )
	{
		EndManagedRendering();
	}

	m_renderStates = other.m_renderStates;
	std::copy( std::begin( other.m_renderStateOverrides ), std::end( other.m_renderStateOverrides ), m_renderStateOverrides );

	ApplyStandardStates( other.m_currentValues.m_renderingMode );
}

void Tr2EffectStateManager::UpdateRenderTargetViewport( unsigned width, unsigned height )
{
	CCP_ASSERT( width > 0 );
	CCP_ASSERT( height > 0 );

	// Keep record of the render target's width and height. This is used when
	// checking whether the viewport extends beyond the render target or not.
	m_renderTargetWidth = width;
	m_renderTargetHeight = height;

	// Reset the viewport
	m_viewportOnDevice.m_x = 0;
	m_viewportOnDevice.m_y = 0;
	m_viewportOnDevice.m_height = (float)height;
	m_viewportOnDevice.m_width = (float)width;
	m_viewport.height = height;
	m_viewport.width = width;
	m_viewport.x = 0;
	m_viewport.y = 0;
	m_viewport.minZ = m_viewportOnDevice.m_minZ = 0.0f;
	m_viewport.maxZ = m_viewportOnDevice.m_maxZ = 1.0f;

	m_viewportSizeVar = Vector4( m_viewportOnDevice.m_width, m_viewportOnDevice.m_height, (float)m_renderTargetWidth, (float)m_renderTargetHeight );

	// Reset the projection adjustment
	//m_viewport2projectionAdjustment = IdentityMatrix();
}

void Tr2EffectStateManager::SetupViewport()
{
	// See whether this viewport extends beyond the render target. If so,
	// we need to clip it.

	int x0 = m_viewport.x;
	int y0 = m_viewport.y;
	int x1 = m_viewport.x + m_viewport.width;
	int y1 = m_viewport.y + m_viewport.height;

	m_viewportOnDevice.m_x = std::max( (float)x0, 0.0f );
	m_viewportOnDevice.m_y = std::max( (float)y0, 0.0f );

	// Viewport width and height must be greater than zero. Using zero edge
	// length causes dx error.
	m_viewportOnDevice.m_width = (float)std::max( std::min( x1, m_renderTargetWidth ) - int( m_viewportOnDevice.m_x ), 1 );
	m_viewportOnDevice.m_height = (float)std::max( std::min( y1, m_renderTargetHeight ) - int( m_viewportOnDevice.m_y ), 1 );

	m_viewportOnDevice.m_minZ = m_viewport.minZ;
	m_viewportOnDevice.m_maxZ = m_viewport.maxZ;

	m_viewportSizeVar = Vector4( (float)m_viewport.width, (float)m_viewport.height, (float)m_renderTargetWidth, (float)m_renderTargetHeight );

	m_renderContext.SetViewport( m_viewportOnDevice );
}
