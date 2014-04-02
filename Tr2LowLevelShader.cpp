////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//
#include "StdAfx.h"
#include "Tr2LowLevelShader.h"

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Constructor.  Initializes members to default values.
// --------------------------------------------------------------------------------------
Tr2LowLevelShader::Tr2LowLevelShader( IRoot* lockobj ) :
	m_permuteIndex( 0xFFFFFFFF ),
	m_compilerDefines( NULL ),
	m_codeSource( SS_ERROR_SHADER ),
	m_inErrorState( false ),
	m_sortValue( 0 ),
	m_effectPath(),
	m_shaderTypeMask( 0xffFFffFFu )
{	
}

// --------------------------------------------------------------------------------------
// Description:
//   Frees the dynamically-allocated compiler defines array.
// --------------------------------------------------------------------------------------
Tr2LowLevelShader::~Tr2LowLevelShader()
{
	CCP_FREE( m_compilerDefines );
}

// --------------------------------------------------------------------------------------
// Description:
//   Called when a Tr2LowLevelShader is created.
// Return Value:
//   true, always
// --------------------------------------------------------------------------------------
bool Tr2LowLevelShader::Initialize( void )
{
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the sort value for the compiled shader.  The sort value is created when the
//   shader is compiled and analyzed.
// Return Value:
//   The shader sort value
// See Also:
//   Process Effect
// --------------------------------------------------------------------------------------
unsigned int Tr2LowLevelShader::GetSortValue( void ) const
{
	return m_sortValue;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the number of passes in the shader.
// Return Value:
//   The number of passes supported by the shader
// --------------------------------------------------------------------------------------
unsigned int Tr2LowLevelShader::GetPassCount( void ) const
{
	return static_cast<unsigned int>( m_effect.passes.size() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Searches for a constant/uniform by its name.
// Arguments:
//   name - Constant name.
// Return value:
//   Pointer to constant (temporary) or nullptr if the constant is not found
// --------------------------------------------------------------------------------------
const Tr2EffectConstant* Tr2LowLevelShader::GetConstant( const char* name ) const
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

// --------------------------------------------------------------------------------------
// Description:
//   Searches for a sampler by its texture name.
// Arguments:
//   name - Texture name.
// Return value:
//   Pointer to sampler (temporary) or nullptr if the sampler is not found
// --------------------------------------------------------------------------------------
const Tr2EffectResource* Tr2LowLevelShader::GetResource( const char* name ) const
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

// --------------------------------------------------------------------------------------
// Description:
//   Returns map of effect annotations for a given parameter or nullptr if the parameter
//   is not found or it doesn't contain any annotations.  
// Arguments:
//   parameterName - Name of the effect parameter
// Return Value:
//   Map of effect annotations or nullptr
// --------------------------------------------------------------------------------------
const Tr2EffectParameterAnnotationMap* Tr2LowLevelShader::GetParameterAnnotations( const char* parameterName ) const
{
	auto annotations = m_effect.annotations.find( parameterName );
	if( annotations == m_effect.annotations.end() )
	{
		return nullptr;
	}
	return &annotations->second;
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
bool Tr2LowLevelShader::HasShaderStage( uint32_t passIndex, 
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
//   Applies the vertex and pixel shaders, and all the sampler and render states for the
//   given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply state.
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::ApplyAllStateForPass( 
										uint32_t passIndex,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	for( unsigned i = SHADER_TYPE_FIRST; i != SHADER_TYPE_COUNT; ++i )
	{
		if( m_shaderTypeMask & ( 1 << i ) )
		{
			renderContext.m_esm.ApplyShader( ShaderType( i ), pass.stageInputs[i].m_shader );
			for( Tr2SamplerSetupMap::const_iterator it = pass.stageInputs[i].samplers.begin(); 
				it != pass.stageInputs[i].samplers.end(); ++it )
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
//   Applies all the render states for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply render state.
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::ApplyRenderStates( 
										uint32_t passIndex,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies all the sampler states for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply sampler state.
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::ApplySamplerStates( 
										uint32_t passIndex, 
										Tr2RenderContextEnum::ShaderType type,
										Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];

	for( Tr2SamplerSetupMap::const_iterator it = pass.stageInputs[type].samplers.begin(); 
		it != pass.stageInputs[type].samplers.end(); ++it )
	{
		renderContext.m_esm.ApplySamplerSetup( 
			type, 
			it->first, 
			it->second.handle );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies the shader for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply the shader.
//   type - Shader type to apply
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::ApplyShader( 
									uint32_t passIndex, 
									Tr2RenderContextEnum::ShaderType type,
									Tr2RenderContext &renderContext ) const
{
	const Tr2Pass& pass = m_effect.passes[passIndex];
	renderContext.m_esm.ApplyShader( type, pass.stageInputs[type].m_shader );
}

unsigned Tr2LowLevelShader::GetShaderTypeMask()
{
	return m_shaderTypeMask;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the permute index, which is a bitwise OR of the permute bits for this shader
//   configuration
// Arguments:
//   permuteIndex - The permuteIndex of this low-level shader
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::SetPermuteIndex( unsigned int permuteIndex )
{
	m_permuteIndex = permuteIndex;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets code source for this shader (for debugging)
// Arguments:
//   codeSource - The code source for this shader
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::SetCodeSource( Tr2ShaderCodeSource codeSource )
{
	m_codeSource = codeSource;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the compiler defines for this low-level shader.  The compiler defines are
//   generated from the situation flags when a low-level shader is bound.
// Arguments:
//   defs - Array of #defines for the effect compiler
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::SetCompilerDefines( Tr2EffectDefine* defs )
{
	if( m_compilerDefines && ( m_compilerDefines != defs ) )
	{
		CCP_FREE( m_compilerDefines );
	}

	m_compilerDefines = defs;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the compiled D3DX effect for this shader and rebuilds annotation map.
// Arguments:
//   effect - The compiled effect
// --------------------------------------------------------------------------------------
Tr2EffectDescription& Tr2LowLevelShader::GetEffect()
{
	return m_effect;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the path to the effect source file
// Arguments:
//   effectPath - The path to the effect source file
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::SetEffectPath( const std::string& effectPath )
{
	m_effectPath = effectPath;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns effect pass data.  
// Arguments:
//   passIx - Pass index
// Return Value:
//   Pass data
// --------------------------------------------------------------------------------------
const Tr2Pass& Tr2LowLevelShader::GetPass( unsigned int passIx ) const
{
	return m_effect.passes[passIx];
}

// --------------------------------------------------------------------------------------
// Description:
//   Breaks apart the effect to extract parameters and state for each pass.  Uses the
//   Tr2EffectStateManager to accomplish this.  The low-level shader is not usable in
//   the renderer until this function has been called.
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::ProcessEffect( void )
{
	m_sortValue = 0;
	if( !m_effect.passes.empty() )
	{
		// Construct sort value so that the following parameters affect it, in the order given:
		// 1) Number of passes
		// 2) Pixel shader in the first pass
		// 3) Vertex shader in the first pass
		// 4) Render states set in the first pass

		unsigned int ps = m_effect.passes[0].stageInputs[PIXEL_SHADER].m_shader & 0x3ff;
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
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the number of shader parameters and a pointer to the start of the data
//   buffer for the given pass.
// Arguments:
//   passIndex - Index of the pass for which to get handles for the shader constants
//   type - Shader (input stage) type
//   count - Number of shader float registers (output parameter)
//   values - A pointer to the start of the shader constant buffer
// --------------------------------------------------------------------------------------
void Tr2LowLevelShader::GetDefaultConstantValues( 
	unsigned int passIndex, 
	Tr2RenderContextEnum::ShaderType type,
	unsigned int& count, 
	const void*& values )
{
	count = m_effect.passes[passIndex].stageInputs[type].m_constantValueSize;
	values = m_effect.passes[passIndex].stageInputs[type].constantValues;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the shader constant table for the given pass.
// Arguments:
//   passIndex - The index of the pass for which to get the vertex shader constant table
//   type - Shader (input stage) type
// Return Value:
//   The shader constant table for the specified pass
// --------------------------------------------------------------------------------------
const Tr2EffectConstantVector& Tr2LowLevelShader::GetConstantTable( unsigned int passIndex, Tr2RenderContextEnum::ShaderType type )
{
	return m_effect.passes[passIndex].stageInputs[type].constants;
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
const Tr2EffectResourceMap& Tr2LowLevelShader::GetInputResources(unsigned int passIx, Tr2RenderContextEnum::ShaderType type )
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
const Tr2EffectResourceMap& Tr2LowLevelShader::GetInputUavs(unsigned int passIx, Tr2RenderContextEnum::ShaderType type )
{
	return m_effect.passes[passIx].stageInputs[type].uavs;
}
