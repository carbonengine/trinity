// Copyright © 2017 CCP ehf.

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

Tr2MaterialStageInput::Tr2MaterialStageInput() :
	m_constantBufferDirty( false )
{
}

Tr2MaterialStageInput::~Tr2MaterialStageInput()
{
	g_sharedConstantBuffers.ReleaseBuffer( m_sharedBufferKey );
}

Tr2EffectLibraryParameters::Tr2EffectLibraryParameters() :
	m_globalResourceSetDirty( true )
{
}

//Tr2EffectLibraryParameters::~Tr2EffectLibraryParameters()
//{
//}

void Tr2EffectLibraryParameters::AddUsedResource( ITr2EffectValuePtr resource )
{
	m_usedResources.push_back( resource );
	m_usedTexturesDirty = true;
}

void Tr2EffectLibraryParameters::AddReroutable( ITriReroutable* reroutable )
{
	m_reroutedParameters.push_back( reroutable );
}


Tr2EffectPassParameters::Tr2EffectPassParameters() :
	m_resourceSetDirty( true ),
	m_compatibleWithGdr( true ),
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

void Tr2EffectPassParameters::AddUsedResource( ITr2EffectValuePtr resource )
{
	m_usedResources.push_back( resource );
	m_usedTexturesDirty = true;
}

void Tr2EffectPassParameters::AddReroutable( ITriReroutable* reroutable )
{
	m_reroutedParameters.push_back( reroutable );
}

void Tr2EffectPassParameters::AllocateConstantMirror( Tr2RenderContextEnum::ShaderType type, unsigned int size )
{
	m_stageInput[type].AllocateConstants( size );
}

void Tr2EffectPassParameters::GetSharedConstantBuffer( Tr2RenderContextEnum::ShaderType type, const void* contents, unsigned int size )
{
	m_stageInput[type].GetSharedConstantBuffer( contents, size );
}

void Tr2MaterialStageInput::AllocateConstants( uint32_t size )
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

void Tr2MaterialStageInput::GetSharedConstantBuffer( const void* contents, uint32_t size )
{
	g_sharedConstantBuffers.ReleaseBuffer( m_sharedBufferKey );

	auto cb = g_sharedConstantBuffers.GetBuffer( contents, size );
	m_constantBuffer = cb.second;
	m_sharedBufferKey = cb.first;

	m_constantMirror.clear();
	m_constantBufferDirty = false;
}


Tr2Material::Tr2Material( IRoot* lockobj ) :
	m_resourceSetHash( 0 ),
	m_compatibleWithGdr( false )
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
	auto& pp = *m_parametersForPasses[techniqueIndex].passes[passIndex];
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
			for( auto& params : technique.passes )
			{
				m_resourceSetHash = CcpHashFNV1( &params->m_resourceSetHash, sizeof( params->m_resourceSetHash ), m_resourceSetHash );
			}
		}
	}

	renderContext.SetResourceSet( pp.m_resourceSet );
}

void Tr2Material::ApplyMaterialDataForPassWithOverride( uint32_t techniqueIndex, unsigned int passIndex, uint32_t overrideProgram, Tr2RenderContext& renderContext ) const
{
	if( !m_shader )
	{
		return;
	}
	auto sp = renderContext.m_esm.GetShaderProgram( overrideProgram );
	if( !sp )
	{
		return;
	}

	unsigned mask = m_shader->GetShaderTypeMask( techniqueIndex );
	auto& pp = *m_parametersForPasses[techniqueIndex].passes[passIndex];
	auto resourceSetDesc = Tr2ResourceSetDescriptionAL( *sp );
	for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT && mask; ++i )
	{
		if( mask & ( 1 << i ) )
		{
			auto& input = pp.m_stageInput[i];
			ApplyConstants( Tr2RenderContextEnum::ShaderType( i ), input, !pp.m_reroutedParameters.empty(), renderContext );
			UpdateResourceSetDesc( Tr2RenderContextEnum::ShaderType( i ), input, resourceSetDesc );
			mask &= ~( 1 << i );
		}
	}

	CCP_STATS_INC( effectResourceSetCreated );

	Tr2ResourceSetAL resourceSet;
	resourceSet.Create( resourceSetDesc, *sp, renderContext.GetPrimaryRenderContext() );
	renderContext.SetResourceSet( resourceSet );

	pp.m_resourceSetDirty = true;
}

bool Tr2Material::ApplyShaderInputs( uint32_t techniqueIndex, unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const
{
	auto& pp = *m_parametersForPasses[techniqueIndex].passes[passIndex];
	return ApplyShaderInputs( pp, shaderType, renderContext );
}

bool Tr2Material::ApplyShaderInputs( Tr2EffectPassParameters& pp, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) const
{
	auto& input = pp.m_stageInput[shaderType];

	ApplyConstants( shaderType, input, !pp.m_reroutedParameters.empty(), renderContext );

	return UpdateResourceSetDesc( shaderType, input, pp.m_resourceSetDesc );
}

void Tr2Material::ApplyConstants( Tr2RenderContextEnum::ShaderType shaderType, Tr2MaterialStageInput& input, bool hasReroutables, Tr2RenderContext& renderContext ) const
{
	auto& cb = input.m_constantBuffer;
	if( cb.GetSize() )
	{
		UpdateConstants( shaderType, input, hasReroutables, renderContext );
		renderContext.SetConstants( cb, shaderType, Tr2RenderContextEnum::CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS );
	}
}

void Tr2Material::UpdateConstants( Tr2RenderContextEnum::ShaderType shaderType, Tr2MaterialStageInput& input, bool hasReroutables, Tr2RenderContext& renderContext ) const
{
	auto& cb = input.m_constantBuffer;
	if( cb.GetSize() )
	{
		if( input.m_constantBufferDirty || hasReroutables || !input.m_shaderParameters.empty() )
		{
			uint8_t* const mirror = reinterpret_cast<uint8_t*>( input.m_constantMirror.get() );
			if( mirror )
			{
				const auto end = input.m_shaderParameters.cend();
				for( auto it = input.m_shaderParameters.cbegin(); it != end; ++it )
				{
					size_t size = it->m_registerCount;
					uint8_t* const dst = mirror + it->m_registerIndex;
					it->m_sourceValue->CopyValueToEffect( shaderType, dst, size, renderContext );
				}
				for( auto& it : input.m_shaderParametersWithNotification )
				{
					size_t size = it.m_registerCount;
					uint8_t* const dst = mirror + it.m_registerIndex;
					it.m_sourceValue->CopyValueToEffect( shaderType, dst, size, renderContext );
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
	}
}

bool Tr2Material::UpdateResourceSetDesc( Tr2RenderContextEnum::ShaderType shaderType, Tr2MaterialStageInput& input, Tr2ResourceSetDescriptionAL& desc ) const
{
	bool descChanged = false;
	for( auto it = input.m_textures.cbegin(); it != input.m_textures.cend(); ++it )
	{
		descChanged |= it->m_sourceValue->CopyToResourceSet( desc, shaderType, it->m_registerIndex, ITr2EffectValue::ResourceFlags( it->m_registerCount ) );
	}
	for( auto it = input.m_uavs.cbegin(); it != input.m_uavs.cend(); ++it )
	{
		descChanged |= it->m_sourceValue->ApplyUav( desc, shaderType, it->m_registerIndex );
	}
	return descChanged;
}

uint64_t Tr2Material::GetSortValue() const
{
	return m_resourceSetHash;
}

Tr2Shader* Tr2Material::GetShaderStateInterface() const
{
	return m_shader;
}

Tr2EffectPassParameters* Tr2Material::GetPassDescription( uint32_t techniqueIndex, uint32_t passIndex )
{
	return m_parametersForPasses[techniqueIndex].passes[passIndex].get();
}

void Tr2Material::InvalidateResourceSets()
{
	for( auto tit = begin( m_parametersForPasses ); tit != end( m_parametersForPasses ); ++tit )
	{
		for( auto pit = begin( tit->passes ); pit != end( tit->passes ); ++pit )
		{
			auto params = pit->get();
			params->m_resourceSet = Tr2ResourceSetAL();
			params->m_resourceSetDesc.ClearResources();
			params->m_resourceSetHash = 0;
			params->m_resourceSetDirty = true;

			params->m_usedTexturesDirty = true;
		}
		for( auto pit = begin( tit->libraries ); pit != end( tit->libraries ); ++pit )
		{
			auto params = pit->get();

			params->m_usedTexturesDirty = true;
		}
	}
	m_resourceSetHash = 0;
}

void Tr2Material::ResourceChanged()
{
	for( auto& technique : m_parametersForPasses )
	{
		for( auto& pass : technique.passes )
		{
			pass->m_resourceSetHash = 0;
			pass->m_resourceSetDirty = true;
			pass->m_usedTexturesDirty = true;
		}
		for( auto& pass : technique.libraries )
		{
			pass->m_usedTexturesDirty = true;
		}
	}
	m_resourceSetHash = 0;
}

void Tr2Material::MarkConstantBuffersDirty()
{
	for( auto& technique : m_parametersForPasses )
	{
		for( auto& pass : technique.passes )
		{
			for( auto& stage : pass->m_stageInput )
			{
				if( !stage.m_shaderParametersWithNotification.empty() )
				{
					stage.m_constantBufferDirty = true;
				}
			}
		}
		for( auto& library : technique.libraries )
		{
			if( !library->m_globalInput.m_shaderParametersWithNotification.empty() )
			{
				library->m_globalInput.m_constantBufferDirty = true;
			}
			if( !library->m_localInput.m_shaderParametersWithNotification.empty() )
			{
				library->m_localInput.m_constantBufferDirty = true;
			}
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

void Tr2Material::GetUsedBindlessTextures( uint32_t techniqueIndex, Tr2BindlessResourcesAL& usedTextures )
{
	if( !m_shader )
	{
		return;
	}
	for( auto& pass : m_parametersForPasses[techniqueIndex].passes )
	{
		if( pass->m_usedTexturesDirty )
		{
			pass->m_usedTexturesDirty = false;
			pass->m_usedTextures.Clear();
			for( auto& res : pass->m_usedResources )
			{
				res->AddUsedTexture( pass->m_usedTextures );
			}
		}
		usedTextures.Add( pass->m_usedTextures );
	}
	for( auto& library : m_parametersForPasses[techniqueIndex].libraries )
	{
		if( library->m_usedTexturesDirty )
		{
			library->m_usedTexturesDirty = false;
			library->m_usedTextures.Clear();
			for( auto& res : library->m_usedResources )
			{
				res->AddUsedTexture( library->m_usedTextures );
			}
		}
		usedTextures.Add( library->m_usedTextures );
	}
}
bool Tr2Material::CompatibleWithGdr() const
{
	return m_compatibleWithGdr;
}

bool Tr2Material::CompatibleWithGdr( const BlueSharedString& techniqueName ) const
{
	if( !m_shader )
	{
		return false;
	}
	uint32_t technique;
	if( !m_shader->GetTechniqueIndex( techniqueName, technique ) )
	{
		return false;
	}
	for( auto& pass : m_parametersForPasses[technique].passes )
	{
		if( !pass->m_compatibleWithGdr )
		{
			return false;
		}
	}
	return true;
}

void Tr2Material::ApplyConstantBuffers( uint32_t techniqueIndex, unsigned int passIndex, Tr2IndirectDrawBufferWriter& indirectBuffer, Tr2RenderContext& renderContext )
{
	unsigned mask = m_shader->GetShaderTypeMask( techniqueIndex );
	auto& passDesc = m_shader->GetEffectDescription().techniques[techniqueIndex].passes[passIndex];
	auto& pp = *m_parametersForPasses[techniqueIndex].passes[passIndex];
	for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT && mask; ++i )
	{
		auto& input = pp.m_stageInput[i];
		if( ( mask & ( 1 << i ) ) && indirectBuffer.HasMaterialConstants( Tr2RenderContextEnum::ShaderType( i ) ) )
		{
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
							it->m_sourceValue->CopyValueToEffect( Tr2RenderContextEnum::ShaderType( i ), dst, size, renderContext );
						}
						for( auto& it : input.m_shaderParametersWithNotification )
						{
							size_t size = it.m_registerCount;
							uint8_t* const dst = mirror + it.m_registerIndex;
							it.m_sourceValue->CopyValueToEffect( Tr2RenderContextEnum::ShaderType( i ), dst, size, renderContext );
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

				indirectBuffer.SetMaterialConstants( Tr2RenderContextEnum::ShaderType( i ), cb );
			}
			mask &= ~( 1 << i );
		}
	}
}

void Tr2Material::ApplyMaterialDataForRtState( uint32_t techniqueIndex, const Tr2RtPipelineStateAL& rtPipelineState, Tr2RenderContext& renderContext ) const
{
	if( !m_shader )
	{
		return;
	}
	auto& pp = *m_parametersForPasses[techniqueIndex].libraries[0];

	ApplyConstants( Tr2RenderContextEnum::COMPUTE_SHADER, pp.m_globalInput, !pp.m_reroutedParameters.empty(), renderContext );

	bool descChanged = pp.m_globalResourceSetDirty;
	descChanged |= UpdateResourceSetDesc( Tr2RenderContextEnum::COMPUTE_SHADER, pp.m_globalInput, pp.m_globalResourceSetDesc );

	if( descChanged || !pp.m_globalResourceSet.IsValid() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		pp.m_globalResourceSet.Create( pp.m_globalResourceSetDesc, rtPipelineState, renderContext );
		pp.m_globalResourceSetDirty = false;
	}

	renderContext.SetResourceSet( pp.m_globalResourceSet );
}

void Tr2Material::ApplyMaterialDataForRtMaterial( uint32_t techniqueIndex, Tr2RtLocalMaterialDescriptionAL& localMaterial, Tr2RenderContext& renderContext ) const
{
	if( !m_shader )
	{
		return;
	}
	auto& pp = *m_parametersForPasses[techniqueIndex].libraries[0];
	auto& cb = pp.m_localInput.m_constantBuffer;
	if( cb.GetSize() )
	{
		UpdateConstants( Tr2RenderContextEnum::COMPUTE_SHADER, pp.m_localInput, !pp.m_reroutedParameters.empty(), renderContext );
		localMaterial.SetConstants( Tr2RenderContextEnum::CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS, cb );
	}
}
