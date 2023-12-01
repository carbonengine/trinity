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
#include "Include/ITriEffectParameter.h"

CCP_STATS_DECLARE( effectCBLocks, "Trinity/effectCBLocks", true, CST_COUNTER_LOW, "number of CB locks for effect parameters" );
CCP_STATS_DECLARE( effectResourceSetCreated, "Trinity/effectResourceSetCreated", true, CST_COUNTER_LOW, "number of resource sets created" );

Tr2SharedConstantBuffers g_sharedConstantBuffers;


std::pair<Tr2SharedConstantBuffers::Key, Tr2ConstantBufferAL> Tr2SharedConstantBuffers::GetBuffer( const void* contents, uint32_t size )
{
	if( !size || !contents )
	{
		return std::make_pair( Key(), Tr2ConstantBufferAL() );
	}
	Key key;
	key.size = size;
	key.hash = CcpHashFNV1( contents, size );
	key.contents = contents;

	auto found = m_buffers.find( key );
	if( found != m_buffers.end() )
	{
		auto& value = found->second;
		if( !value.buffer.IsValid() )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			if( FAILED( value.buffer.Create( size, Tr2ConstantUsageAL::ONE_SHOT, contents, renderContext ) ) )
			{
				return std::make_pair( Key(), Tr2ConstantBufferAL() );
			}
		}
		++value.refCount;
		return std::make_pair( found->first, value.buffer );
	}

	if( size % sizeof( Vector4 ) )
	{
		size += sizeof( Vector4 ) - size % sizeof( Vector4 );
	}

	Value value;
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		if( FAILED( value.buffer.Create( size, Tr2ConstantUsageAL::ONE_SHOT, contents, renderContext ) ) )
		{
			return std::make_pair( Key(), Tr2ConstantBufferAL() );
		}
	}
	
	value.refCount = 1;

	auto copy = new uint8_t[size];
	memcpy( copy, contents, size );
	key.contents = copy;

	m_buffers[key] = value;
	return std::make_pair( key, value.buffer );
}

void Tr2SharedConstantBuffers::ReleaseBuffer( const Key& key )
{
	if( !key.contents )
	{
		return;
	}

	auto found = m_buffers.find( key );
	if( found == m_buffers.end() )
	{
		CCP_ASSERT( false );
		return;
	}
	if( --found->second.refCount == 0 )
	{
		auto contents = static_cast<const uint8_t*>( found->first.contents );
		m_buffers.erase( found );
		delete[] contents;
	}
}


Tr2EffectPassParameters::Tr2EffectPassParameters()
	:m_resourceSetDirty( true ),
	m_resourceSetHash( 0 )
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
	m_stageInput[type].AllocateConstants( size );
}

void Tr2EffectPassParameters::GetSharedConstantBuffer( Tr2RenderContextEnum::ShaderType type, const void* contents, unsigned int size )
{
	m_stageInput[type].GetSharedConstantBuffer( contents, size );
}


Tr2EffectPassParameters::StageInput::StageInput() :
	m_constantBufferDirty( false )
{
}

Tr2EffectPassParameters::StageInput::~StageInput()
{
	g_sharedConstantBuffers.ReleaseBuffer( m_sharedBufferKey );
}

void Tr2EffectPassParameters::StageInput::AllocateConstants( uint32_t size )
{
	g_sharedConstantBuffers.ReleaseBuffer( m_sharedBufferKey );
	m_sharedBufferKey = Tr2SharedConstantBuffers::Key();

	if( size )
	{
		// fix the size to be multiple of Vector4s
		if( size % sizeof( Vector4 ) )
		{
			size += sizeof( Vector4 ) - size % sizeof( Vector4 );
		}

		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_constantBuffer.Create(
			size,
			Tr2ConstantUsageAL::ONE_SHOT,
			nullptr,
			renderContext );
		m_constantMirror.resize( "StageInput::m_constantMirror", size );
		m_constantBufferDirty = true;
	}
	else
	{
		m_constantBuffer = Tr2ConstantBufferAL();
		m_constantBufferDirty = false;
	}
}

void Tr2EffectPassParameters::StageInput::GetSharedConstantBuffer( const void* contents, uint32_t size )
{
	g_sharedConstantBuffers.ReleaseBuffer( m_sharedBufferKey );

	auto cb = g_sharedConstantBuffers.GetBuffer( contents, size );
	m_constantBuffer = cb.second;
	m_sharedBufferKey = cb.first;

	m_constantMirror.clear();
	m_constantBufferDirty = false;
}



Tr2Material::Tr2Material( IRoot* lockobj ) :
	m_resourceSetHash( 0 )
{
}

Tr2Material::~Tr2Material()
{
}

void Tr2Material::ApplyMaterialDataForPass( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContext& renderContext ) const
{
	if( !m_shader )
	{
		return;
	}
	unsigned mask = m_shader->GetShaderTypeMask( techniqueIndex );
	auto& pp = *m_parametersForPasses[techniqueIndex][passIndex];
	bool descChanged = pp.m_resourceSetDirty;
	for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT && mask; ++i )
	{
		if( mask & ( 1 << i ) )
		{
			descChanged |= ApplyShaderInputs( techniqueIndex, passIndex, Tr2RenderContextEnum::ShaderType( i ), renderContext );
			mask &= ~( 1 << i );
		}
	}

	if( descChanged || !pp.m_resourceSet.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		CCP_STATS_INC( effectResourceSetCreated );

		auto sp = renderContext.m_esm.GetShaderProgram( m_shader->GetEffect().techniques[techniqueIndex].passes[passIndex].shaderProgram );
		if( !sp )
		{
			return;
		}
		pp.m_resourceSet.Create( pp.m_resourceSetDesc, *sp, renderContext );
		pp.m_resourceSetHash = pp.m_resourceSetDesc.ComputeHash();
		pp.m_resourceSetDirty = false;

		m_resourceSetHash = 0;
		for( auto& technique : m_parametersForPasses )
		{
			for( auto& params : technique )
			{
				m_resourceSetHash = CcpHashFNV1( &params->m_resourceSetHash, sizeof( params->m_resourceSetHash ), m_resourceSetHash );
			}
		}
	}

	renderContext.SetResourceSet( pp.m_resourceSet );
}

bool Tr2Material::ApplyShaderInputs( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const
{
	auto& pp = *m_parametersForPasses[techniqueIndex][passIndex];
	auto& input = pp.m_stageInput[shaderType];

	auto& cb = input.m_constantBuffer;
	if( cb.GetSize() )
	{
		if( input.m_constantBufferDirty || !pp.m_reroutedParameters.empty() || !input.m_shaderParameters.empty() )
		{
			uint8_t* const mirror = reinterpret_cast<uint8_t*>( input.m_constantMirror.get() );
			if( mirror )
			{
				const auto endVS = input.m_shaderParameters.cend();
				for( auto it = input.m_shaderParameters.cbegin(); it != endVS; ++it )
				{
					size_t size = it->m_registerCount;
					uint8_t* const dst = mirror + it->m_registerIndex;
					it->m_sourceValue->CopyValueToEffect( shaderType, dst, size, renderContext );
				}

				void* cbData = nullptr;
				if( SUCCEEDED( cb.Lock( &cbData, renderContext ) ) && cbData )
				{
					memcpy( cbData, mirror, cb.GetSize() );
					cb.Unlock( renderContext );
				}
			}
			input.m_constantBufferDirty = false;
		}
		renderContext.SetConstants( cb, shaderType, Tr2RenderContextEnum::CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS );
	}


	bool descChanged = false;

	for( auto it = input.m_textures.cbegin(); it != input.m_textures.cend(); ++it )
	{
		descChanged |= it->m_sourceValue->CopyToResourceSet( pp.m_resourceSetDesc, shaderType, it->m_registerIndex, ITr2EffectValue::ResourceFlags( it->m_registerCount ) );
	}

	for( auto it = input.m_uavs.cbegin(); it != input.m_uavs.cend(); ++it )
	{
		descChanged |= it->m_sourceValue->ApplyUav( pp.m_resourceSetDesc, shaderType, it->m_registerIndex );
	}

	return descChanged;
}

uint64_t Tr2Material::GetSortValue() const
{
	if( m_shader )
	{
		return ( uint64_t( m_shader->GetSortValue() ) << 32 ) | m_resourceSetHash;
	}
	else
	{
		return 0;
	}
}

Tr2Shader* Tr2Material::GetShaderStateInterface() const
{
	return m_shader;
}

Tr2EffectPassParameters* Tr2Material::GetPassDescription( uint32_t techniqueIndex, uint32_t passIndex )
{
	return m_parametersForPasses[techniqueIndex][passIndex].get();
}

void Tr2Material::InvalidateResourceSets()
{
	for( auto tit = begin( m_parametersForPasses ); tit != end( m_parametersForPasses ); ++tit )
	{
		for( auto pit = begin( *tit ); pit != end( *tit ); ++pit )
		{
			auto params = pit->get();
			params->m_resourceSet = Tr2ResourceSetAL();
			params->m_resourceSetDesc.ClearResources();
			params->m_resourceSetHash = 0;
			params->m_resourceSetDirty = true;
		}
	}
	m_resourceSetHash = 0;
}

void Tr2Material::UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities )
{
	for( auto& value : m_lodTextureParameters )
	{
		value->UsedWithScreenSize( screenSize, worldRadius, uvDensities );
	}
}
