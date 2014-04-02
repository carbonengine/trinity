
////////////////////////////////////////////////////////////
//
// Created: June 2010
// Copyright: CCP 2010
//

#include "StdAfx.h"

#include "Tr2HighLevelShader.h"
#include "Tr2ShaderMaterial.h"
#include "Tr2ShaderPermuteTag.h"
#include "TriSettingsRegistrar.h"

//TriSetting to force release-like behaviour
bool g_strictShaderCompilation = false;
TRI_REGISTER_SETTING( "strictShaderCompilation", g_strictShaderCompilation );

// --------------------------------------------------------------------------------------
// Description:
//   Gets the ascii source code for the error shader.  This is used when compilation of
//   another shader fails.
// Arguments:
//   length - Length of the character buffer for the error shader (output parameter)
// Return Value:
//   Text buffer containing ascii source for the error shader
// --------------------------------------------------------------------------------------
static const char* GetErrorShaderSourceCode( unsigned int& length )
{
	const char* source = "/*errortag*/float4x4 WorldToClipMatrix : register(vs, c226);\
						  float4 ErrorVertexShader(float4 position:POSITION) : POSITION\
						  {if (position.w < 0.00000001) \
						  position.w = 1.0; \
						  float4 pos = position / position.w; \
						  float4 clipPos = mul(pos, WorldToClipMatrix);\
						  return(clipPos);}\
						  float4 Time;\
						  float4 ErrorPixelShader() : COLOR0{\
						  return float4(Time.y, 0,0,1);}\
						  technique error{pass error\
						  {pixelshader = compile ps_2_0 ErrorPixelShader();vertexshader = compile vs_2_0 ErrorVertexShader();}}";

	length = (unsigned int)strlen( source );

	return source;
}

Tr2HighLevelShader::Tr2HighLevelShader( IRoot* lockobj ) :
	m_name(),
	m_nameHash( 0 ),
	m_description(),
	m_shaderFilePath(),
	m_UIName(),
	PARENTLOCK( m_permuteTags ),
	PARENTLOCK( m_parameters ),
	m_shaderStorage(),
	m_bReloadShader( false ),
	m_stringTable( nullptr ),
	m_stringTableSize( 0 ),
	m_shaderFileVersion( 0 )
{
}

Tr2HighLevelShader::~Tr2HighLevelShader()
{
	delete[] m_stringTable;
}

bool Tr2HighLevelShader::OnModified( Be::Var *val )
{
	// when shader path changes, then re-build myself. ( is this a good idea to implement? )
	if( IsMatch( val, m_shaderFilePath ) )
	{
		LoadFXFile();
		m_shaderStorage.clear();
	}

	return true;
}


// --------------------------------------------------------------------------------------
// Description:
//   add a shader material to the reference list, called when a shader material is created
// or changes its HLS binding.
// See also:
//	RebuildLowLevelShadersAfterCodeChange
// --------------------------------------------------------------------------------------

void Tr2HighLevelShader::RegisterShader(Tr2ShaderMaterial * material)
{
	m_materialReferences.insert( material );
}


// --------------------------------------------------------------------------------------
// Description:
//   removes a shader material from the reference list, called when a shader material is
//   destroyed or changes its HLS binding.
// See also:
//	RebuildLowLevelShadersAfterCodeChange
// --------------------------------------------------------------------------------------

void Tr2HighLevelShader::UnregisterShader(Tr2ShaderMaterial * material)
{
	m_materialReferences.erase( material );
}


// --------------------------------------------------------------------------------------
// Description:
//   Performs basic initialization.
// Return Value:
//   always true
// --------------------------------------------------------------------------------------

bool Tr2HighLevelShader::Initialize()
{
	LoadFXFile();

	m_nameHash = CcpHashFNV1( m_name.c_str(), m_name.length() );

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the TriStorage class was TRISTORAGE_ALL.
// Arguments:
//   s - The TriStorage class of the resource set we need to release
// --------------------------------------------------------------------------------------
void Tr2HighLevelShader::ReleaseResources( TriStorage s )
{
	m_bReloadShader |= s == TRISTORAGE_ALL;
	if( m_bReloadShader )
	{
		m_compiledFile = (IResFile*)nullptr;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   If TriStorage class of the last ReleaseResources was TRISTORAGE_ALL then
//   recreates the shader.
// --------------------------------------------------------------------------------------
bool Tr2HighLevelShader::OnPrepareResources()
{
	if( m_bReloadShader )
	{
		RebuildLowLevelShadersAfterCodeChange();
		m_bReloadShader = false;
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the hash of the high-level shader name.  The hash code is computed at
//   initialization time.
// Return Value:
//   Hash code of the high-level shader name
// --------------------------------------------------------------------------------------
unsigned int Tr2HighLevelShader::GetNameHash( void ) const
{
	return m_nameHash;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets an existing low-level shader or creates a new one, based on the given situation.
//   If a low-level shader with a key corresponding to the situation already exist, then
//   it is returned.  Otherwise a new shader is compiled.
// Arguments:
//   situation - Situation used to generate the #defines and key for the low-level shader
// Return Value:
//   The low-level shader corresponding to the given situation
// --------------------------------------------------------------------------------------
Tr2LowLevelShader* Tr2HighLevelShader::GetOrCreateLowLevelShader(
	const Tr2ShaderSituation& situation )
{
	unsigned int permuteIndex = 0;

	Tr2EffectDefine* currentDefines = CreateDefinesFromSituation( situation, permuteIndex );

	// Have we already compiled this effect?
	auto foundShader = m_shaderStorage.find( permuteIndex );
	if( foundShader != m_shaderStorage.end() )
	{
		CCP_FREE( currentDefines );
		return foundShader->second;
	}

	// Check if we have an alias (identical shader with different permute
	// index) loaded
	auto compiledPermutation = m_compiledPermutations.find( permuteIndex );

	if( compiledPermutation != m_compiledPermutations.end() )
	{
		auto alias = m_shaderAliases.find( compiledPermutation->second.offset );
		if( alias != m_shaderAliases.end() )
		{
			CCP_FREE( currentDefines );
			return alias->second;
		}
	}

	Tr2ShaderCodeSource codeSource;

	Tr2LowLevelShaderPtr lowLevelShader;
	lowLevelShader.CreateInstance();

	lowLevelShader->Initialize();

	GetEffect( permuteIndex, currentDefines, codeSource, lowLevelShader->GetEffect() );

	if( lowLevelShader->GetEffect().passes.empty() )
	{
		return nullptr;
	}

	lowLevelShader->SetEffectPath( m_shaderFilePath );

	// Process the effect to extract parameters and state
	lowLevelShader->ProcessEffect();

	// Set the permute index and compiler defines used to produce the effect
	lowLevelShader->SetPermuteIndex( permuteIndex );
	lowLevelShader->SetCodeSource( codeSource );
	lowLevelShader->SetCompilerDefines( currentDefines );

	// Cache the compiled low-level shader in our list of compiled shaders
	m_shaderStorage[permuteIndex] = lowLevelShader;

	if( compiledPermutation != m_compiledPermutations.end() )
	{
		m_shaderAliases[compiledPermutation->second.offset] = lowLevelShader;
	}

	return lowLevelShader.Detach();
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the list of parameter descriptions.
// Return Value:
//   The list of parameter descriptions
// --------------------------------------------------------------------------------------
const Tr2ShaderParameterDescriptionVector&
	Tr2HighLevelShader::GetParameterDescriptions( void ) const
{
	return m_parameters;
}

// --------------------------------------------------------------------------------------
// Description:
//   Loads sources code from the .fx file for this high-level shader.
// --------------------------------------------------------------------------------------
void Tr2HighLevelShader::LoadFXFile()
{
	delete[] m_stringTable;
	m_stringTable = nullptr;
	m_stringTableSize = 0;
	m_shaderFileVersion = 0;

	if( m_shaderFilePath.empty() )
	{
		return;
    }

	// Load compiled shader file
	m_compiledPermutations.clear();

	const static std::string s_shaderResRoot = std::string("res:/graphics/shaders");
	const static std::string s_cacheRoot = std::string("res:/graphics/shaders/Compiled/" TRINITY_PLATFORM_NAME);

	std::string cacheFileName = m_shaderFilePath.c_str();

	// remove the 'root' part
	if( m_shaderFilePath.compare( 0, s_shaderResRoot.size(), s_shaderResRoot ) == 0 )
	{
		cacheFileName.erase( 0, s_shaderResRoot.size() );
	}

	// remove .fx, add .fxp
	cacheFileName += "t";

	std::wstring cacheFilePath = (const wchar_t*)CA2W( ( s_cacheRoot + cacheFileName ).c_str() );

	m_compiledFile.Unlock();
	if( !BeClasses->CreateInstanceFromName( "BlueResFile2", BlueInterfaceIID<IResFile>(), (void**)&m_compiledFile ) )
	{
		BeClasses->CreateInstanceFromName( "ResFile", BlueInterfaceIID<IResFile>(), (void**)&m_compiledFile );
	}
	if( m_compiledFile->OpenW( cacheFilePath.c_str(), true ) )
	{
		uint32_t version;
		if( m_compiledFile->Read( &version, sizeof( version ) ) != sizeof( version ) )
		{
			CCP_LOGERR( "Unexpected end of file while reading effect file \"%s\"", m_shaderFilePath.c_str() );
			return;
		}
		if( version != 2 && version != 3 )
		{
			CCP_LOGERR( "Invalid version of effect file \"%s\" (version %i)", m_shaderFilePath.c_str(), version );
			return;
		}
		m_shaderFileVersion = version;
		
		// Read file header and store it in the m_compiledPermutations map
		uint32_t count = 0;
		if( m_compiledFile->Read( &count, sizeof( count ) ) != sizeof( count ) )
		{
			CCP_LOGERR( "Unexpected end of file while reading effect file \"%s\"", m_shaderFilePath.c_str() );
			return;
		}
		uint32_t* header = new uint32_t[count * 3];
		if( m_compiledFile->Read( header, count * 3 * sizeof( uint32_t ) ) != count * 3 * sizeof( uint32_t ) )
		{
			CCP_LOGERR( "Unexpected end of file while reading effect file \"%s\"", m_shaderFilePath.c_str() );
			return;
		}
		for( unsigned i = 0; i < count; ++i )
		{
			FileRecord record;
			record.offset = header[i * 3 + 1];
			record.size = header[i * 3 + 2];
			m_compiledPermutations[header[i * 3]] = record;
		}
		delete[] header;

		uint32_t strignTableSize;
		if( m_compiledFile->Read( &strignTableSize, sizeof( strignTableSize ) ) != sizeof( strignTableSize ) )
		{
			CCP_LOGERR( "Unexpected end of file while reading effect file \"%s\"", m_shaderFilePath.c_str() );
			return;
		}
		delete[] m_stringTable;
		m_stringTable = nullptr;
		m_stringTableSize = 0;
		m_stringTable = new char[strignTableSize];
		if( m_compiledFile->Read( m_stringTable, strignTableSize ) != strignTableSize )
		{
			CCP_LOGERR( "Unexpected end of file while reading effect file \"%s\"", m_shaderFilePath.c_str() );
			return;
		}
		m_stringTableSize = strignTableSize;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Closes compiled cache file handle, so that it can be overwritten outside Jessica.
//   Useful for debugging/recompiling shaders. 
// --------------------------------------------------------------------------------------
void Tr2HighLevelShader::CloseCacheFile()
{
	m_compiledFile.Unlock();
}

// --------------------------------------------------------------------------------------
// Description:
//   Fetches an effect from the cache. If the effect is not present, will compile from
//   source iff we're not running a final build, not on a mac, and the strict compilation
//   setting is false. If this fails, returns the error effect.
// Arguments:
//   permuteIndex - The set of situation flags for a give low-level shader.
//   currentDefines - Macros for compilation in case of cache miss.
//   codeSource - (out) Where does the shader come from (cache, source or error shader)
// Return Value:
//   The compiled effect ready for use.
// --------------------------------------------------------------------------------------
void Tr2HighLevelShader::GetEffect( int permuteIndex, Tr2EffectDefine* currentDefines, Tr2ShaderCodeSource& codeSource, Tr2EffectDescription& effect ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	effect.passes.clear();
	effect.annotations.clear();

	CCP_LOG( "Fetching effect from shader cache: %s [%08x]", m_shaderFilePath.c_str(), permuteIndex );

	auto compiledPermutation = m_compiledPermutations.find( permuteIndex );
	if( compiledPermutation != m_compiledPermutations.end() && m_compiledFile )
	{
		void* data = CCP_MALLOC( "Tr2HighLevelShader::shaderData", compiledPermutation->second.size );
		if( data )
		{
			m_compiledFile->Seek( compiledPermutation->second.offset, ICcpStream::SO_BEGIN );
			m_compiledFile->Read( data, compiledPermutation->second.size );

			if( effect.Read( data, compiledPermutation->second.size, m_shaderFileVersion, m_stringTable, m_stringTableSize, m_name.c_str() ) )
			{
				CCP_FREE( data );
				codeSource = SS_FROM_CACHE;
				return;
			}

			CCP_FREE( data );
		}
	}

	if( g_strictShaderCompilation || IsTransgaming() )
	{
		CCP_LOGERR( "Effect not present in compiled form, strict compilation enforced" );
		codeSource = SS_ERROR_SHADER;
		return;
	}

	CCP_LOGERR( "Effect not present" );
	codeSource = SS_ERROR_SHADER;
}

// --------------------------------------------------------------------------------------
// Description:
//   Creates an array of #defines for the effect compiler from the given situation.  This
//   function also computes a permute index by OR-ing together the permute tag bits for
//   each situation tag.
// Arguments:
//   situation - The current situation used to generate #defines
//   permuteIndex - The corresponding permute index (output parameter)
// Return Value:
//   Array of D3DXMACROs containing the #defines for the effect compiler
// --------------------------------------------------------------------------------------
Tr2EffectDefine* Tr2HighLevelShader::CreateDefinesFromSituation(
	const Tr2ShaderSituation& situation,
	unsigned int& permuteIndex )
{
	const std::vector<unsigned int>& situationFlags = situation.GetSituation();
	Tr2ShaderPermuteTagVector::iterator walker( m_permuteTags.begin() ),
		endOfList( m_permuteTags.end() );

	// list ends with null strings.

	unsigned int size = ((unsigned int)m_permuteTags.size() + 1) * sizeof( Tr2EffectDefine );
	Tr2EffectDefine* macros =
		static_cast<Tr2EffectDefine*>( CCP_MALLOC( "ShaderMacro", size ) );

	memset( macros, 0, size );

	int x = 0;

	permuteIndex = 0;

	while( walker != endOfList )
	{
		Tr2ShaderPermuteTag* tag = *walker;

		// look in the situation hash list, and if the current tag name hash is in there,
		// then we set our value to 1
		std::vector<unsigned int>::const_iterator finder =
			std::find( situationFlags.begin(), situationFlags.end(), tag->GetPermuteNameHash() );

		macros[x].name = tag->GetPermuteDefine();

		// might want to extend this to have tags also include a #define value?
		if( finder != situationFlags.end() )
		{
			macros[x].value = "1";
			permuteIndex |= tag->GetTagBits(); // create a index for this result.
		}
		else
		{
			macros[x].value = "0";
		}

		++walker;
		++x;
	}

	return macros;
}

// --------------------------------------------------------------------------------------
// Description:
//   Reloads the .fx file and recompiles all the low-level shaders.  This will likely
//   require the caller to also rebind all the low-level shaders for the shader materials
//   using this high-level shader.
// --------------------------------------------------------------------------------------
void Tr2HighLevelShader::RebuildLowLevelShadersAfterCodeChange()
{
	// Gets the new .fx file from disk.
	LoadFXFile();

	ShaderStorageType::iterator walker( m_shaderStorage.begin() ),
		endOfShaders( m_shaderStorage.end() );

	CCP_LOG( "Rebuilding shaders for %s", m_name.c_str() );

	m_shaderAliases.clear();

	while( walker != endOfShaders )
	{
		Tr2LowLevelShader* lowLevelShader = walker->second;

		Tr2EffectDefine* defines = lowLevelShader->GetCompilerDefines();

		// compile a new d3d effect from our new source, and with the current set of defines.
		Tr2ShaderCodeSource codeSource;

		GetEffect( lowLevelShader->GetPermuteIndex(), defines, codeSource, lowLevelShader->GetEffect() );

		lowLevelShader->SetCodeSource( codeSource );
		lowLevelShader->SetCompilerDefines( defines );
		lowLevelShader->ProcessEffect();

		auto compiledPermutation = m_compiledPermutations.find( lowLevelShader->GetPermuteIndex() );
		if( compiledPermutation != m_compiledPermutations.end() )
		{
			m_shaderAliases[compiledPermutation->second.offset] = lowLevelShader;
		}


		++walker;
	}

	// Go and call Rebuild Cached Data internal on all 'child' Shadermats, this will make sure all texture and source registers are updated in the register mirror.

	std::set<Tr2ShaderMaterial*>::iterator smwalker ( m_materialReferences.begin()), endOfSMList( m_materialReferences.end());

	while( smwalker != endOfSMList )
	{
		Tr2ShaderMaterial* sm = *smwalker;

		sm->RebuildCachedDataInternal();

		++smwalker;
	}

}

// --------------------------------------------------------------------------------------
// Description:
//   Convert a permutation index back to a space-delimited string of situation flags.
//   Bits in the permutation index with no corresponding situation flag are ignored.
// Arguments:
//   permuteIndex - permutation index to interpret
// Returns:
//   Space-delimited string containing all situation flag define names.
// --------------------------------------------------------------------------------------
std::string Tr2HighLevelShader::PermuteIndexToString( const int permuteIndex ) const
{
	std::string result;
	for( Tr2ShaderPermuteTagVector::const_iterator i = m_permuteTags.begin(); i != m_permuteTags.end(); ++i )
	{
		if( (*i)->GetTagBits() & permuteIndex )
		{
			result += (*i)->GetPermuteDefine();
			result += " ";
		}
	}
	return result;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a vector of loaded low-level shaders.
// Return value:
//   vector of loaded low-level shaders
// --------------------------------------------------------------------------------------
std::vector<Tr2LowLevelShader*> Tr2HighLevelShader::GetLoadedShaders()
{
	std::vector<Tr2LowLevelShader*> shaders;
	shaders.clear();
	shaders.reserve( m_shaderStorage.size() );
	for( auto i = m_shaderStorage.begin(); i != m_shaderStorage.end(); ++i )
	{
		shaders.push_back( i->second );
	}
	return shaders;
}