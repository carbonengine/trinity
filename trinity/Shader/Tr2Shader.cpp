// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Shader.h"
#include "Tr2Renderer.h"


using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Constructor.  Initializes members to default values.
// --------------------------------------------------------------------------------------
Tr2Shader::Tr2Shader( IRoot* lockobj ) :
	m_sortValue( 0 ),
	m_hasVertexBufferAccessInRtShadow( false )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Frees the dynamically-allocated compiler defines array.
// --------------------------------------------------------------------------------------
Tr2Shader::~Tr2Shader()
{
}

bool Tr2Shader::GetTechniqueIndex( const BlueSharedString& name, uint32_t& index ) const
{
	if( m_effect.techniques.empty() )
	{
		return false;
	}
	if( name == ANY_TECHNIQUE )
	{
		index = 0;
		return true;
	}
	for( size_t i = 0; i < m_effect.techniques.size(); ++i )
	{
		if( m_effect.techniques[i].name == name )
		{
			index = uint32_t( i );
			return true;
		}
	}
	return false;
}

// --------------------------------------------------------------------------------------
unsigned int Tr2Shader::GetPassCount( uint32_t techniqueIndex ) const
{
	if( m_effect.techniques.empty() )
	{
		return 0;
	}
	return static_cast<unsigned int>( m_effect.techniques[techniqueIndex].passes.size() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Searches for a constant/uniform by its name.
// Arguments:
//   name - Constant name.
// Return value:
//   Pointer to constant (temporary) or nullptr if the constant is not found
// --------------------------------------------------------------------------------------
const Tr2EffectConstant* Tr2Shader::GetConstant( const char* name ) const
{
	for( auto t = m_effect.techniques.begin(); t != m_effect.techniques.end(); ++t )
	{
		for( auto pass = t->passes.begin(); pass != t->passes.end(); ++pass )
		{
			for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
			{
				for( auto constant = pass->stageInputs[i].constants.begin(); constant != pass->stageInputs[i].constants.end(); ++constant )
				{
					if( strcmp( constant->name.c_str(), name ) == 0 )
					{
						return &*constant;
					}
				}
			}
		}
	}
	return nullptr;
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
unsigned int Tr2Shader::GetSortValue( void ) const
{
	return m_sortValue;
}

// --------------------------------------------------------------------------------------
// Description:
//   Searches for a sampler by its texture name.
// Arguments:
//   name - Texture name.
// Return value:
//   Pointer to sampler (temporary) or nullptr if the sampler is not found
// --------------------------------------------------------------------------------------
const Tr2EffectResource* Tr2Shader::GetResource( const char* name ) const
{
	for( auto t = m_effect.techniques.begin(); t != m_effect.techniques.end(); ++t )
	{
		for( auto pass = t->passes.begin(); pass != t->passes.end(); ++pass )
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
const Tr2EffectParameterAnnotationMap* Tr2Shader::GetParameterAnnotations( const char* parameterName ) const
{
	auto it = std::find_if( m_effect.annotations.begin(), m_effect.annotations.end(), [&]( Tr2EffectAnnotationMap::const_reference key ) { return strcmp( key.first, parameterName ) == 0; } );
	return it == m_effect.annotations.end() ? nullptr : &it->second;
}


const Tr2EffectDescription& Tr2Shader::GetEffectDescription() const
{
	return m_effect;
}

// --------------------------------------------------------------------------------------
void Tr2Shader::ApplyAllStateForPass( uint32_t techniqueIndex, uint32_t passIndex, Tr2RenderContext& renderContext ) const
{
	auto& technique = m_effect.techniques[techniqueIndex];

	const Tr2Pass& pass = technique.passes[passIndex];

	renderContext.m_esm.ApplyShaderProgram( pass.shaderProgram );

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

// --------------------------------------------------------------------------------------
uint32_t Tr2Shader::ApplyShaderOverride( uint32_t techniqueIndex, uint32_t passIndex, const Tr2Shader& overrideShader, uint32_t overridePassIndex, Tr2RenderContext& renderContext ) const
{
	auto program = m_effect.techniques[techniqueIndex].passes[passIndex].shaderProgram;
	auto overrideProgram = overrideShader.m_effect.techniques[0].passes[overridePassIndex].shaderProgram;

	auto combined = Tr2EffectStateManager::RegisterShaderProgramOverride( program, overrideProgram );
	renderContext.m_esm.ApplyShaderProgram( combined );
	return combined;
}

// --------------------------------------------------------------------------------------
void Tr2Shader::ApplyRenderStates( uint32_t techniqueIndex, uint32_t passIndex, Tr2RenderContext& renderContext ) const
{
	auto& technique = m_effect.techniques[techniqueIndex];
	auto& pass = technique.passes[passIndex];

	renderContext.m_esm.ApplyRenderStates( pass.renderStates );
}

// --------------------------------------------------------------------------------------
unsigned Tr2Shader::GetShaderTypeMask( uint32_t techniqueIndex ) const
{
	if( m_effect.techniques.empty() )
	{
		return 0;
	}
	return m_effect.techniques[techniqueIndex].shaderTypeMask;
}

// --------------------------------------------------------------------------------------
Tr2EffectDescription& Tr2Shader::GetEffect()
{
	return m_effect;
}

// --------------------------------------------------------------------------------------
bool Tr2Shader::HasVertexBufferAccessInRtShadow()
{
	return m_hasVertexBufferAccessInRtShadow;
}

// --------------------------------------------------------------------------------------
// Description:
//   Refreshes effect-dependent variables.
// --------------------------------------------------------------------------------------
void Tr2Shader::ProcessEffect()
{
	m_sortValue = 0;
	if( m_effect.techniques.empty() )
	{
		return;
	}
	auto& technique = m_effect.techniques[0];

	if( !technique.passes.empty() )
	{
		// Construct sort value so that the following parameters affect it, in the order given:
		// 1) Number of passes
		// 2) Pixel shader in the first pass
		// 3) Vertex shader in the first pass
		// 4) Render states set in the first pass

		unsigned int ps = technique.passes[0].stageInputs[PIXEL_SHADER].m_shader & 0x3ff;
		unsigned int vs = technique.passes[0].stageInputs[VERTEX_SHADER].m_shader & 0x3ff;
		unsigned int states = technique.passes[0].renderStates & 0x3ff;
		unsigned int numPasses = technique.passes.size() & 0x3;

		m_sortValue = ( numPasses << 30 ) | ( ps << 20 ) | ( vs << 10 ) | states;
	}

#if TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_METAL
	USE_MAIN_THREAD_RENDER_CONTEXT();

	for( auto& t : m_effect.techniques )
	{
		for( auto& pass : t.passes )
		{
			if( auto program = renderContext.m_esm.GetShaderProgram( pass.shaderProgram ) )
			{
				pass.indirectLayout = Tr2IndirectDrawBufferLayout( *program, renderContext );
			}
		}
	}

	m_hasVertexBufferAccessInRtShadow = false;
	const BlueSharedString techniqueName = BlueSharedString( "RtShadow" );
	auto effect = GetEffect();
	uint32_t techniqueIndex;
	if( GetTechniqueIndex( techniqueName, techniqueIndex ) )
	{
		for( const auto& library : effect.techniques[techniqueIndex].libraries )
		{
			for( const auto& localRegister : library.localInput.signature.registers )
			{
				if( localRegister.registerType == Tr2ShaderRegisterAL::CONSTANT_BUFFER &&
					localRegister.registerIndex == Tr2Renderer::GetPerObjectRTVertexBufferDataRegister() )
				{
					m_hasVertexBufferAccessInRtShadow = true;
				}
			}
		}
	}
#endif
}
