// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2EffectRes.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Shader.h"

static unsigned int s_effectResId = 0;

using namespace Tr2RenderContextEnum;

namespace
{
std::vector<Tr2ShaderOption> s_globalOptions;
}


IBlueResource* CreateTr2EffectRes( const wchar_t* name )
{
	Tr2EffectResPtr p;
	p.CreateInstance();
	return p.Detach();
}

BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_1_1", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_2_0_LO", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_2_0_HI", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_3_0_LO", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_3_0_HI", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_3_0_DEPTH", CreateTr2EffectRes );

BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_LO", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_HI", CreateTr2EffectRes );
BLUE_REGISTER_RESOURCE_EXTENSION( L"SM_DEPTH", CreateTr2EffectRes );

Tr2EffectRes::Tr2EffectRes( IRoot* lockobj ) :
	m_version( 0 ),
	m_stringTable( nullptr ),
	m_stringTableSize( 0 ),
	m_offsetCount( 0 ),
	m_offsets( nullptr ),
	m_permutations( "Tr2EffectRes::m_permutations" ),
	m_shaders( "Tr2EffectRes::m_shaders" )
{
}

Tr2EffectRes::~Tr2EffectRes()
{
}

Tr2ShaderPtr Tr2EffectRes::GetShader( const Tr2ShaderOption* options, size_t count )
{
	if( !IsGood() )
	{
		return nullptr;
	}

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return nullptr;
	}

	uint32_t multiplier = 1;
	uint32_t index = 0;

	for( size_t i = 0; i < m_permutations.size(); ++i )
	{
		auto value = m_permutations[i].defaultOption;

		bool foundGlobal = false;
		for( auto& opt : GetGlobalEffectOptions() )
		{
			if( opt.name == m_permutations[i].name )
			{
				auto found = std::find( begin( m_permutations[i].options ), end( m_permutations[i].options ), opt.value );
				if( found != end( m_permutations[i].options ) )
				{
					value = found - begin( m_permutations[i].options );
				}
				foundGlobal = true;
				break;
			}
		}

		if( !foundGlobal )
		{
			for( size_t k = 0; k < count; ++k )
			{
				if( options[k].name == m_permutations[i].name )
				{
					size_t val = -1;
					for( size_t j = 0; j < m_permutations[i].options.size(); ++j )
					{
						if( m_permutations[i].options[j] == options[k].value )
						{
							val = j;
							break;
						}
					}
					if( val == -1 )
					{
						CCP_LOGWARN( "Invalid situation value %s for permutation %s in effect %S", options[k].value.c_str(), m_permutations[i].name.c_str(), GetPath() );
					}
					else
					{
						value = val;
					}
				}
			}
		}
		index += uint32_t( value * multiplier );
		multiplier *= uint32_t( m_permutations[i].options.size() );
	}

	auto found = m_shaders.find( index );
	if( found != m_shaders.end() )
	{
		return found->second;
	}

	CCP_ASSERT( index < m_offsetCount );

	Tr2ShaderPtr shader;
	shader.CreateInstance();

	auto offset = m_offsets[index];
	auto buffer = reinterpret_cast<const uint8_t*>( m_data.get() ) + offset.offset;
	if( !shader->GetEffect().Read( buffer, offset.size, m_version, m_stringTable, m_stringTableSize, CW2A( GetPath() ) ) )
	{
		return nullptr;
	}
	shader->ProcessEffect();

	m_shaders[index] = shader;
	return shader;
}

BlueAsyncRes::LoadingResult Tr2EffectRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_version = 0;
	m_data.clear();
	m_stringTable = nullptr;
	m_stringTableSize = 0;
	m_offsets = nullptr;
	m_offsetCount = 0;
	m_permutations.clear();

	uint8_t* data = nullptr;
	if( !m_dataStream->LockData( reinterpret_cast<void**>( &data ), 0 ) )
	{
		return LR_FAILED;
	}

	if( data == nullptr )
	{
		return LR_FAILED;
	}

	auto dataSize = m_dataStream->GetSize();


	m_data.resize( "Tr2EffectRes::m_data", size_t( dataSize ) );

	if( m_data.empty() )
	{
		m_dataStream->UnlockData();
		return LR_FAILED;
	}

	memcpy( m_data.get(), data, size_t( dataSize ) );
	m_dataStream->UnlockData();

	const uint8_t* buffer = reinterpret_cast<const uint8_t*>( m_data.get() );
	const uint8_t* bufferEnd = buffer + dataSize;

#define SKIP( storeType )                                                              \
	if( buffer + sizeof( storeType ) > bufferEnd )                                     \
	{                                                                                  \
		CCP_LOGERR( "Unexpected end of file while reading effect \"%S\"", GetPath() ); \
		return LR_FAILED;                                                              \
	}                                                                                  \
	buffer += sizeof( storeType );

#define READ( storeType, valueType, value )                                            \
	if( buffer + sizeof( storeType ) > bufferEnd )                                     \
	{                                                                                  \
		CCP_LOGERR( "Unexpected end of file while reading effect \"%S\"", GetPath() ); \
		return LR_FAILED;                                                              \
	}                                                                                  \
	value = valueType( *reinterpret_cast<const storeType*>( buffer ) );                \
	buffer += sizeof( storeType );

#define READ_STRING( value )                                                   \
	{                                                                          \
		uint32_t offset;                                                       \
		READ( uint32_t, uint32_t, offset );                                    \
		if( offset >= m_stringTableSize )                                      \
		{                                                                      \
			CCP_LOGERR( "Invalid string offset in effect \"%S\"", GetPath() ); \
			return LR_FAILED;                                                  \
		}                                                                      \
		value = m_stringTable + offset;                                        \
	}

	READ( uint32_t, uint32_t, m_version );

	if( m_version < 2 || m_version > 15 )
	{
		CCP_LOGERR( "Invalid version of effect file \"%S\" (version %i)", GetPath(), m_version );
		return LR_FAILED;
	}

	if( m_version < 5 )
	{
		uint32_t headerSize;
		READ( uint32_t, uint32_t, headerSize );

		if( headerSize == 0 )
		{
			CCP_LOGERR( "File \"%ls\" contains no compiled effects", GetPath() );
			return LR_FAILED;
		}

		if( 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t ) > size_t( dataSize ) )
		{
			CCP_LOGERR( "Invalid file header size in effect \"%S\". Corrupt file?", GetPath() );
			return LR_FAILED;
		}

		// Get the first permutation from the file
		m_offsetCount = 1;
		m_offsets = reinterpret_cast<const FileRecord*>( buffer );

		m_stringTableSize = *reinterpret_cast<const uint32_t*>( reinterpret_cast<const char*>( m_data.get() ) + 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) );
		if( 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t ) + m_stringTableSize > size_t( dataSize ) )
		{
			CCP_LOGERR( "Invalid string table size in effect \"%S\". Corrupt file?", GetPath() );
			return LR_FAILED;
		}
		m_stringTable = reinterpret_cast<const char*>( m_data.get() ) + 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t );
	}
	else
	{
		if( m_version >= 15 )
		{
			uint32_t compilerVersion;
			SKIP( compilerVersion );
			uint8_t hash[32];
			SKIP( hash );
		}

		READ( uint32_t, uint32_t, m_stringTableSize );
		if( m_stringTableSize > size_t( dataSize ) )
		{
			CCP_LOGERR( "Invalid string table size in effect \"%S\". Corrupt file?", GetPath() );
			return LR_FAILED;
		}
		m_stringTable = reinterpret_cast<const char*>( buffer );
		buffer += m_stringTableSize;

		uint32_t permutationCount;
		READ( uint8_t, uint32_t, permutationCount );
		for( uint32_t i = 0; i < permutationCount; ++i )
		{
			Tr2ShaderPermutation permutation;
			std::string name;
			READ_STRING( name );
			permutation.name = BlueSharedString( name );
			READ( uint8_t, size_t, permutation.defaultOption );
			READ_STRING( permutation.description );

			if( m_version > 5 )
			{
				READ( uint8_t, uint8_t, permutation.type );
			}
			else
			{
				permutation.type = 0;
			}

			uint32_t count;
			READ( uint8_t, uint32_t, count );
			for( uint32_t j = 0; j < count; ++j )
			{
				std::string name;
				READ_STRING( name );
				permutation.options.push_back( BlueSharedString( name ) );
			}
			m_permutations.push_back( permutation );
		}

		uint32_t headerSize;
		READ( uint32_t, uint32_t, headerSize );

		if( headerSize == 0 )
		{
			CCP_LOGERR( "File \"%ls\" contains no compiled effects", GetPath() );
			return LR_FAILED;
		}

		if( 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t ) > size_t( dataSize ) )
		{
			CCP_LOGERR( "Invalid file header size in effect \"%S\". Corrupt file?", GetPath() );
			return LR_FAILED;
		}

		m_offsetCount = headerSize;
		m_offsets = reinterpret_cast<const FileRecord*>( buffer );
	}

	return LR_SUCCESS;
}

bool Tr2EffectRes::DoPrepare()
{
	m_shaders.clear();

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	return true;
}

void Tr2EffectRes::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		SetGood( false );
		SetPrepared( false );
		CancelPendingLoad();

		m_shaders.clear();

		NotifyReleaseCachedData();
	}
}

bool Tr2EffectRes::OnPrepareResources()
{
	if( Tr2Renderer::IsEffectLoadDisabled() )
	{
		return true;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !renderContext.IsValid() )
	{
		// It's possible to get a PrepareResources when there's no valid render context
		return false;
	}

	if( IsGood() || IsLoading() )
	{
		return true;
	}

	Initialize( m_path.c_str(), m_ext.c_str() );
	return true;
}

#if TRINITYDEV
void Tr2EffectRes::GetDescription( std::string& desc )
{
	desc = "Tr2EffectRes: '";
	desc += CW2A( m_path.c_str() );
	desc += "'";
}
#endif

bool Tr2EffectRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2EffectRes::GetMemoryUsage()
{
	// memory usage here is kind of nebulous - pixel/vertex shaders are
	// registered with the effect state manager, won't ever be freed.
	return m_data.size();
}


void ModifyGlobalEffectOptions( const std::vector<Tr2ShaderOption>& changes )
{
	if( changes.empty() )
	{
		return;
	}
	auto& options = s_globalOptions;
	for( auto& change : changes )
	{
		auto SameOption = [&change]( auto& x ) { return change.name == x.name; };
		if( change.value == BlueSharedString() )
		{
			options.erase( std::remove_if( begin( options ), end( options ), SameOption ) );
		}
		else
		{
			auto found = std::find_if( begin( options ), end( options ), SameOption );
			if( found != end( options ) )
			{
				found->value = change.value;
			}
			else
			{
				options.push_back( change );
			}
		}
	}
	Tr2Renderer::RebuildEffects();
}

const std::vector<Tr2ShaderOption>& GetGlobalEffectOptions()
{
	return s_globalOptions;
}