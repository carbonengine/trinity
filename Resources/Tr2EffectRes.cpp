#include "StdAfx.h"
#include "Tr2EffectRes.h"
#include "Tr2Renderer.h"

static unsigned int s_effectResId = 0;

using namespace Tr2RenderContextEnum;

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
	m_stringTable( nullptr ),
	m_data( nullptr ),
	m_shaderTypeMask( 0xffFFffFFu )
{	
	m_sortValue = s_effectResId++;
}

Tr2EffectRes::~Tr2EffectRes()
{
	delete[] m_stringTable;
}

const Tr2EffectConstant* Tr2EffectRes::GetConstant( const char* name ) const
{
	for( auto pass = m_effect.passes.begin(); pass != m_effect.passes.end(); ++pass )
	{
		for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			for( auto constant = pass->stageInputs[i].constants.begin(); constant != pass->stageInputs[i].constants.end(); ++constant )
			{
				if( strcmp( constant->name, name ) == 0 )
				{
					return &*constant;
				}
			}
		}
	}
	return nullptr;
}

const Tr2EffectResource* Tr2EffectRes::GetResource( const char* name ) const
{
	for( auto pass = m_effect.passes.begin(); pass != m_effect.passes.end(); ++pass )
	{
		for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			for( auto constant = pass->stageInputs[i].resources.begin(); constant != pass->stageInputs[i].resources.end(); ++constant )
			{
				if( strcmp( constant->second.name, name ) == 0 )
				{
					return &constant->second;
				}
			}
			for( auto constant = pass->stageInputs[i].uavs.begin(); constant != pass->stageInputs[i].uavs.end(); ++constant )
			{
				if( strcmp( constant->second.name, name ) == 0 )
				{
					return &constant->second;
				}
			}
		}
	}
	return nullptr;
}

bool Tr2EffectRes::DoOpenStream()
{
	BePaths->GetStreamFromPathW( GetPath(), &m_dataStream );

	if( !m_dataStream )
	{
		return false;
	}

	return true;
}

BlueAsyncRes::LoadingResult Tr2EffectRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_dataStream->LockData( (void**)&m_data, 0 ) )
	{
		return LR_FAILED;
	}

	m_dataSize = (unsigned int)m_dataStream->GetSize();

	return LR_SUCCESS;
}

bool Tr2EffectRes::DoPrepare()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	delete[] m_stringTable;
	m_stringTable = nullptr;
	m_effect.passes.clear();
	m_effect.annotations.clear();

	if( m_data == nullptr )
	{
		return false;
	}

	const uint8_t* buffer = reinterpret_cast<const uint8_t*>( m_data );
	const uint8_t* bufferEnd = buffer + m_dataSize;

#define READ( storeType, valueType, value )													\
	if( buffer + sizeof( storeType ) > bufferEnd )											\
	{																						\
		CCP_LOGERR( "Unexpected end of file while reading effect \"%S\"", GetPath() );		\
		return false;																		\
	}																						\
	value = valueType( *reinterpret_cast<const storeType*>( buffer ) );						\
	buffer += sizeof( storeType );

	uint32_t version;
	READ( uint32_t, uint32_t, version );

	if( version != 2 && version != 3 )
	{
		CCP_LOGERR( "Invalid version of effect file \"%S\" (version %i)", GetPath(), version );
		return false;
	}

	uint32_t headerSize;
	READ( uint32_t, uint32_t, headerSize );

	if( headerSize == 0 )
	{
		CCP_LOGERR( "File \"%s\" contains no compiled effects", GetPath() );
		return false;
	}

	if( 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t ) > m_dataSize )
	{
		CCP_LOGERR( "Invalid file header size in effect \"%S\". Corrupt file?", GetPath() );
		return false;
	}

	// Get the first permutation from the file
	uint32_t permutation;
	READ( uint32_t, uint32_t, permutation );

	uint32_t offset;
	READ( uint32_t, uint32_t, offset );
	if( offset > m_dataSize )
	{
		CCP_LOGERR( "Invalid offset in effect \"%S\". Corrupt file?", GetPath() );
		return false;
	}

	uint32_t tableSize = *reinterpret_cast<const uint32_t*>( reinterpret_cast<const char*>( m_data ) + 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) );
	if( 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t ) + tableSize > m_dataSize )
	{
		CCP_LOGERR( "Invalid string table size in effect \"%S\". Corrupt file?", GetPath() );
		return false;
	}
	m_stringTable = new char[tableSize];
	memcpy( m_stringTable, reinterpret_cast<const char*>( m_data ) + 2 * sizeof( uint32_t ) + headerSize * 3 * sizeof( uint32_t ) + sizeof( uint32_t ), tableSize );

	buffer = reinterpret_cast<const uint8_t*>( m_data ) + offset;

	if( !m_effect.Read( buffer, m_dataSize - offset, version, m_stringTable, tableSize, CW2A( GetPath() ) ) )
	{
		return false;
	}

	m_sortValue = 0;
	if( !m_effect.passes.empty() )
	{
		// Construct sort value so that the following parameters affect it, in the order given:
		// 1) Number of passes
		// 2) Pixel shader in the first pass
		// 3) Vertex shader in the first pass
		// 4) Render states set in the first pass

		unsigned int ps = m_effect.passes[0].stageInputs[PIXEL_SHADER ].m_shader & 0x3ff;
		unsigned int vs = m_effect.passes[0].stageInputs[VERTEX_SHADER].m_shader & 0x3ff;
		unsigned int states = m_effect.passes[0].renderStates & 0x3ff;
		unsigned int numPasses = m_effect.passes.size() & 0x3;

		m_sortValue = (numPasses << 30) | (ps << 20) | (vs << 10) | states;
	}

	m_shaderTypeMask = 0;
	for( auto pass = m_effect.passes.cbegin(); pass != m_effect.passes.cend(); ++pass )
	{
		for( unsigned i = SHADER_TYPE_FIRST; i != SHADER_TYPE_COUNT; ++i )
		{
			if( pass->stageInputs[i].m_shader != Tr2EffectStageInput::INVALID )
			{
				m_shaderTypeMask |= 1u << i;
			}
		}
	}


	return true;
}

void Tr2EffectRes::DoCloseStream()
{
	if( m_dataStream )
	{
		m_dataStream->UnlockData();
		m_data = NULL;
		m_dataSize = 0;
		m_dataStream = 0;
	}
}

void Tr2EffectRes::ReleaseResources( TriStorage s )
{
	if( (s & TRISTORAGE_ALL) == TRISTORAGE_ALL )
	{
		SetPrepared( false );
		CancelPendingLoad();

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
	if ( !renderContext.IsValid() )
	{
		// It's possible to get a PrepareResources when there's no valid render context
		return false;
	}

	if( IsPrepared() || IsLoading() )
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

void Tr2EffectRes::ApplyRenderStates(	uint32_t passIx, 
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIx];

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

void Tr2EffectRes::ApplySamplerStates(	uint32_t passIx, 
										Tr2RenderContextEnum::ShaderType type,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIx];

	for(	auto it = pass.stageInputs[type].samplers.cbegin(); 
			it != pass.stageInputs[type].samplers.cend(); 
			++it )
	{
		renderContext.m_esm.ApplySamplerSetup( 
			type, 
			it->first, 
			it->second.handle );
	}
}

void Tr2EffectRes::ApplyAllStateForPass(uint32_t passIndex, 
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
	{
		if( SHADER_TYPE_EXISTS( i ) )
		{
			renderContext.m_esm.ApplyShader( ShaderType( i ), pass.stageInputs[i].m_shader );
			for(	auto it = pass.stageInputs[i].samplers.cbegin(); 
					it != pass.stageInputs[i].samplers.end(); 
					++it )
			{
				renderContext.m_esm.ApplySamplerSetup( 
					Tr2RenderContextEnum::ShaderType( i ), 
					it->first, 
					it->second.handle );
			}
		}
	}

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns map of effect annotations for a given parameter or nullptr if the parameter
//   is not found or it doesn't contain any annotations.  
// Arguments:
//   parameterName - Name of the effect parameter
// Return Value:
//   Map of effect annotations or nullptr
// --------------------------------------------------------------------------------------
const Tr2EffectParameterAnnotationMap* Tr2EffectRes::GetParameterAnnotations( const char* parameterName ) const
{
	auto it = std::find_if( m_effect.annotations.begin(), m_effect.annotations.end(), [&]( Tr2EffectAnnotationMap::const_reference key ) { return strcmp( key.first, parameterName ) == 0; } );
	return it == m_effect.annotations.end() ? nullptr : &it->second;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns effect pass data.  
// Arguments:
//   passIx - Pass index
// Return Value:
//   Pass data
// --------------------------------------------------------------------------------------
const Tr2Pass& Tr2EffectRes::GetPass( unsigned int passIx ) const
{
	return m_effect.passes[passIx];
}

unsigned int Tr2EffectRes::GetPassCount() const
{
	if( !IsGood() || !IsPrepared() )
	{
		return 0;
	}
	return (unsigned int)m_effect.passes.size();
}

void Tr2EffectRes::ApplyShader( uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type,
								Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIx];
	renderContext.m_esm.ApplyShader( type, pass.stageInputs[type].m_shader );
}

unsigned Tr2EffectRes::GetShaderTypeMask()
{
	return m_shaderTypeMask;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns constant table for the given shader.  
// Arguments:
//   passIx - Pass index
//   type - Shader (input stage) type
// Return Value:
//   Shader constant table
// --------------------------------------------------------------------------------------
const Tr2EffectConstantVector& Tr2EffectRes::GetConstantTable( 
								uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type )
{
	return m_effect.passes[passIx].stageInputs[type].constants;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns texture inputs for the given shader.  
// Arguments:
//   passIx - Pass index
//   type - Shader (input stage) type
// Return Value:
//   Texture input sampler map
// --------------------------------------------------------------------------------------
const Tr2EffectResourceMap& Tr2EffectRes::GetInputResources(
								uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type )
{
	return m_effect.passes[passIx].stageInputs[type].resources;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns UAV inputs for the given shader.  
// Arguments:
//   passIx - Pass index
//   type - Shader (input stage) type
// Return Value:
//   UAV input map
// --------------------------------------------------------------------------------------
const Tr2EffectResourceMap& Tr2EffectRes::GetInputUavs(
								uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type )
{
	return m_effect.passes[passIx].stageInputs[type].uavs;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns input definition for requested shader.  
// Arguments:
//   passIx - Pass index
//   type - Shader (input stage) type
// Return Value:
//   Pointer to shader input definition
// --------------------------------------------------------------------------------------
const Tr2ShaderInputDefinition* Tr2EffectRes::GetShaderInputDefinition( 
								uint32_t passIx, 
								Tr2RenderContextEnum::ShaderType type )
{
	if( passIx >= m_effect.passes.size() || type >= SHADER_TYPE_COUNT )
	{
		return nullptr;
	}
	return &m_effect.passes[passIx].stageInputs[type].inputDefinition;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns if the specified pass has specified shader stage.
// Arguments:
//   passIx - Pass index
//   type - Shader (input stage) type
// Return Value:
//   true If the effect contains specified stage type in the specified pass
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2EffectRes::HasShaderStage( uint32_t passIndex, 
								   Tr2RenderContextEnum::ShaderType type ) const
{
	if( passIndex >= m_effect.passes.size() || type >= SHADER_TYPE_COUNT )
	{
		return false;
	}
	return m_effect.passes[passIndex].stageInputs[type].m_exists;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns default constant values for the given shader.  
// Arguments:
//   passIx - Pass index
//   type - Shader (input stage) type
//   count - (out) Number of Vector4 values
//   values - (out) Default constant values
// Return Value:
//   Default constant values for the given shader
// --------------------------------------------------------------------------------------
void Tr2EffectRes::GetDefaultConstantValues(unsigned int passIx, Tr2RenderContextEnum::ShaderType type, unsigned int& count, const void*& values )
{
	count = m_effect.passes[passIx].stageInputs[type].m_constantValueSize;
	values = m_effect.passes[passIx].stageInputs[type].constantValues;
}

bool Tr2EffectRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2EffectRes::GetMemoryUsage()
{
	// memory usage here is kind of nebulous - pixel/vertex shaders are
	// registered with the effect state manager, won't ever be freed.
	return 1024;
}
