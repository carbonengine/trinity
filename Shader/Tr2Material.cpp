////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#include "StdAfx.h"
#include "Tr2Material.h"
#include "ITriReroutable.h"
#include "Tr2Shader.h"
#include "ITr2EffectValue.h"
#include "Tr2VariableStore.h"

CCP_STATS_DECLARE( effectCBLocks, "Trinity/effectCBLocks", true, CST_COUNTER_LOW, "number of CB locks for effect parameters" );

namespace
{

CcpAtomic<uint32_t> s_materialId = 0;

}

Tr2EffectPassParameters::Tr2EffectPassParameters()
{
}

Tr2EffectPassParameters::~Tr2EffectPassParameters()
{
	for( auto it = m_reroutedParameters.begin(); it != m_reroutedParameters.end(); ++it )
	{
		( *it )->SetDestination( NULL, 0 );
		( *it )->Unlock();
	}
}

void Tr2EffectPassParameters::AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size )
{
	if( size )
	{
		// fix the size to be multiple of Vector4s
		if( size % sizeof( Vector4 ) )
		{
			size += sizeof( Vector4 ) - size % sizeof( Vector4 );
		}

		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( !m_stageInput[type].m_constantBuffer )
		{
			m_stageInput[type].m_constantBuffer.reset( CCP_NEW( "Tr2EffectPassParameters::m_stageInput::m_constantBuffer" ) Tr2ConstantBufferAL );
		}
		m_stageInput[type].m_constantBuffer->Create(
			size,
			Tr2RenderContextEnum::USAGE_CPU_WRITE | Tr2RenderContextEnum::USAGE_LOCK_FREQUENTLY,
			nullptr,
			renderContext );
	}
}

Tr2Material::Tr2Material( IRoot* lockobj )
{
	m_sortValue = s_materialId++;
}

Tr2Material::~Tr2Material()
{
}

uint32_t Tr2Material::ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext ) const
{
	if( !m_shader )
	{
		return 0;
	}
	unsigned mask = m_shader->GetShaderTypeMask();
	uint32_t samplersChangedMask = 0;
	for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT && mask; ++i )
	{
		if( mask & ( 1 << i ) )
		{
			bool samplersChanged = false;
			ApplyShaderInputs( passIndex, Tr2RenderContextEnum::ShaderType( i ), samplersChanged, renderContext );
			mask &= ~( 1 << i );
			if( samplersChanged )
			{
				samplersChangedMask |= 1 << i;
			}
		}
	}
	return samplersChangedMask;
}

void Tr2Material::ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const
{
	bool samplersChanged;
	ApplyShaderInputs( passIndex, shaderType, samplersChanged, renderContext );
}

void Tr2Material::ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, bool& samplersChanged, Tr2RenderContext& renderContext ) const
{
	auto& pp = *m_parametersForPasses[passIndex];
	auto& input = pp.m_stageInput[shaderType];

	auto cb = input.m_constantBuffer.get();
	if( cb && cb->GetSize() )
	{
		CCP_STATS_INC( effectCBLocks );
		uint8_t* const mirror = reinterpret_cast<uint8_t*>( cb->GetBufferMirror( renderContext ) );
		if( cb->IsValid() && mirror )
		{
			const auto endVS = input.m_shaderParameters.cend();
			for( auto it = input.m_shaderParameters.cbegin(); it != endVS; ++it )
			{
				size_t size = it->m_registerCount;
				uint8_t* const dst = mirror + it->m_registerIndex;
				it->m_sourceValue->CopyValueToEffect( shaderType, dst, size, renderContext );
			}
			cb->UpdateFromMirror( renderContext );

			renderContext.SetConstants( *cb, shaderType, Tr2RenderContextEnum::CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS );
		}
	}

	unsigned char destHandle[8] = { 0 };

	samplersChanged = false;
	for( auto it = input.m_textures.cbegin(); it != input.m_textures.cend(); ++it )
	{
		destHandle[0] = static_cast<unsigned char>( it->m_registerIndex );
		it->m_sourceValue->CopyValueToEffect( shaderType, destHandle, it->m_registerCount, renderContext );
		if( destHandle[1] )
		{
			samplersChanged = true;
		}
	}
	samplersChanged |= !input.m_samplers.empty();
	for( auto it = input.m_samplers.begin(); it != input.m_samplers.end(); ++it )
	{
		renderContext.m_esm.ApplySamplerSetup( shaderType, it->registerIndex, it->handle );
	}

	for( auto it = input.m_uavs.cbegin(); it != input.m_uavs.cend(); ++it )
	{
		destHandle[0] = static_cast<unsigned char>( it->m_registerIndex );
		reinterpret_cast<uint32_t*>( destHandle )[1] = it->m_initialCount;
		it->m_sourceValue->CopyValueToEffect( shaderType, destHandle, it->m_registerCount, renderContext );
	}
}
unsigned int Tr2Material::GetSortValue() const
{
	if( m_shader )
	{
		return m_shader->GetSortValue();
	}
	else
	{
		return m_sortValue;
	}
}

Tr2Shader* Tr2Material::GetShaderStateInterface() const
{
	return m_shader;
}

void Tr2Material::UnloadResources()
{
}

bool Tr2Material::LoadResources()
{
	return true;
}
