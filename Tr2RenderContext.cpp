////////////////////////////////////////////////////////////
//
//    Created:   July 2013
//    Copyright: CCP 2013
//

#include "StdAfx.h"
#include "Tr2RenderContext.h"
#include "Tr2VariableStore.h"
#include "Tr2RenderTarget.h"
#include "Tr2PerObjectData.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Shader.h"

CCP_STATS_DECLARE( batchCount, "Trinity/batchCount", true, CST_COUNTER_HIGH, "Batches rendered per frame");

Tr2RenderContextBase::Tr2RenderContextBase( Tr2RenderContext& renderContext )
	:m_esm( renderContext )
{
	m_backBuffer.CreateInstance();
	m_backBuffer->m_name = "backbuffer";
	m_objectIdVariable		= GlobalStore().RegisterVariable( "objectId",	0.0f );
	m_areaIdVariable		= GlobalStore().RegisterVariable( "areaId",		0.0f );
}

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Called by Tr2RenderContextAL when a primary or secondary context is created. 
//   Initializes Tr2EffectStateManager instance.
// Arguments:
//   renderContext - AL render context created
// --------------------------------------------------------------------------------------
void Tr2RenderContextBase::OnContextCreated( Tr2RenderContextAL& renderContext )
{
	m_backBuffer->Attach( &renderContext.GetDefaultBackBuffer(), this );
	m_esm.Initialize();
}

// --------------------------------------------------------------------------------------
// Description:
//   Called by DX11 Tr2RenderContextAL when a texture is set as a render target. In this 
//   case DX11 runtime will unset this texture from texture sampling registers (if it's
//   bound for read). Informs Tr2EffectStateManager instance of this so that it can 
//   update its state accordingly.
// Arguments:
//   texture - Texture that is unset from reading register
//   renderContext - Unused
// --------------------------------------------------------------------------------------
void Tr2RenderContextBase::OnTextureUnset( const Tr2TextureAL& texture, Tr2RenderContextAL& )
{
	m_esm.ForgetTexture( texture );
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns back buffer render target as Blue-exposed Tr2RenderTarget. Exposed to 
//   script.
// Return Value:
//   Back buffer render target 
// --------------------------------------------------------------------------------------
Tr2RenderTargetPtr Tr2RenderContextBase::GetBackBuffer()
{
	return m_backBuffer;
}


Tr2RenderContext::Tr2RenderContext()
	:Tr2RenderContextBase( *this )
{
	m_events = this;
}

#if TRINITY_PLATFORM == TRINITY_DIRECTX11
Tr2PrimaryRenderContext::Tr2PrimaryRenderContext()
	:Tr2RenderContextBase( *reinterpret_cast<Tr2RenderContext*>( this ) )
{
	m_events = this;
}

Tr2PrimaryRenderContext::operator Tr2RenderContext&()
{
	return *reinterpret_cast<Tr2RenderContext*>( this );
}
#endif


namespace {
#if( TRINITY_PLATFORM==TRINITY_DIRECTX11 )
	Tr2PrimaryRenderContextPtr	s_mainThreadRenderContext;
#else
	Tr2RenderContextPtr s_mainThreadRenderContext;
#endif
}
	
void Tr2RenderContext::DestroyMainThreadRenderContext()
{
	if( s_mainThreadRenderContext )
	{
		s_mainThreadRenderContext->Destroy();
		s_mainThreadRenderContext.Unlock();
		Tr2RenderContextAL::SetPrimaryRenderContext( nullptr );
	}
}

Tr2PrimaryRenderContext& Tr2RenderContext_GetMainThreadRenderContext()
{
	if( !s_mainThreadRenderContext )
	{
		s_mainThreadRenderContext.CreateInstance();
		Tr2RenderContextAL::SetPrimaryRenderContext( s_mainThreadRenderContext );
	}

	return *s_mainThreadRenderContext;
}

TriVariable* Tr2RenderContextBase::GetObjectIdVariable( void )
{
	return m_objectIdVariable;
}

void Tr2RenderContextBase::RenderBatchesInOrder( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT( L"Tr2RenderContext::RenderBatchesInOrder" );


	const Tr2PerObjectData* curPerObjectData = nullptr;

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	for( TriRenderBatch* it = batches->GetFirstBatch(); it != nullptr; it = it->GetNext() )
	{
		auto material = it->GetShaderMaterialInterface();

		if( !material )
		{
			continue;
		}

		auto shader = it->GetShaderStateInterface();

		if( shader == 0 )
		{
			continue;
		}

		uint32_t technique;
		if( !shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}

		const Tr2PerObjectData* perObjectData = it->GetPerObjectData();
		Tr2RenderContext* renderContext = reinterpret_cast<Tr2RenderContext*>( this );

		if( perObjectData && ( perObjectData != curPerObjectData ) )
		{
			D3DPERF_EVENT(L"RenderBatchesInOrder::Set per-object data to device");
			
			perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, shader->GetShaderTypeMask( technique ), *renderContext );
			curPerObjectData = perObjectData;
		}

		uint32_t passCount = shader->GetPassCount( technique );

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			D3DPERF_EVENT1( L"Pass %i", passIx );
			shader->ApplyAllStateForPass( technique, passIx, *renderContext );
			material->ApplyMaterialDataForPass( technique, passIx, *renderContext );
			it->SubmitGeometry( *renderContext );

			CCP_STATS_INC( batchCount );
		}
	}
}

void Tr2RenderContextBase::RenderBatchesSortedByEffect( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName, BatchesRenderHints hints )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT(L"Tr2EffectStateManager::RenderBatchesSortedByEffect");


	Tr2EffectStateManager::RenderingMode currentMode = Tr2EffectStateManager::RM_ANY;
	const Tr2PerObjectData* curPerObjectData = nullptr;
	unsigned int curShaderTypeMask = 0;

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	const bool hasPerEffectData = ( hints & HINT_NO_PER_EFFECT_DATA ) == 0;
	Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	TriRenderBatch* batch = batches->GetFirstBatch();
	while( batch != nullptr )
	{
		auto material = batch->GetShaderMaterialInterface();


		// If the batch doesn't have an effect, it is a view-modifier batch,
		// so we need to call SubmitGeometry (to make the view parameter change)
		// and move to the next batch
		if( !material )
		{
			D3DPERF_EVENT( CA2W(batch->GetBatchTypeName().c_str()) );
			batch->SubmitGeometry( *renderContext );
			batch = batch->GetNext();
			continue;
		}

		auto currentShader = batch->GetShaderStateInterface();

		if ( currentShader == 0)
		{
			batch = batch->GetNext();
			continue;
		}

		uint32_t technique;
		if( !currentShader->GetTechniqueIndex( techniqueName, technique ) )
		{
			batch = batch->GetNext();
			continue;
		}

		const auto currentShaderMask = currentShader->GetShaderTypeMask( technique );

		// Get the number of passes (must be at least 1 to SubmitGeometry)
		const uint32_t passCount = currentShader->GetPassCount( technique );
		if( passCount == 0)
		{
			batch = batch->GetNext();
			continue;
		}

		// Change the rendering mode , if needed
		const Tr2EffectStateManager::RenderingMode mode = batch->GetRenderingMode();
		if( mode != Tr2EffectStateManager::RM_ANY )
		{
			m_esm.ApplyStandardStates( mode );
			currentMode = mode;
		}

		TriRenderBatch* const startOfPass = batch;
		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			{
				D3DPERF_EVENT1( L"Begin Pass %i", passIx );

				currentShader->ApplyAllStateForPass( technique, passIx, *renderContext );
			}

			batch = startOfPass;

			// Figure out in advance, before we actually start applying state and submitting geometry, how many
			// batches we will be able to batch based on rendering mode and shader sharing.
			TriRenderBatch *lastBatch = batch;
			while( lastBatch )
			{
				const Tr2EffectStateManager::RenderingMode mode = lastBatch->GetRenderingMode();
				if( mode != Tr2EffectStateManager::RM_ANY && mode != currentMode )
				{
					break;
				}

				material = lastBatch->GetShaderMaterialInterface();
				if( material )
				{
					if( lastBatch->GetShaderStateInterface() != currentShader )
					{
						break;
					}
				}
				else
				{
					break;
				}

				lastBatch = lastBatch->GetNext();
			}

			uint32_t samplersDirtyMask = Tr2RenderContext::SHADER_TYPE_MASK;

			while( batch && batch != lastBatch )
			{
				D3DPERF_EVENT( CA2W( batch->GetBatchTypeName().c_str() ) );

				if( hasPerEffectData || batch == startOfPass )
				{
					if( hasPerEffectData && samplersDirtyMask )
					{
						// We have to apply sampler states in case any of the batches override them
						const uint32_t shaderMask = currentShaderMask & Tr2RenderContext::SHADER_TYPE_MASK & samplersDirtyMask;
						for( uint32_t shaderType = SHADER_TYPE_FIRST; shaderType < SHADER_TYPE_COUNT; ++shaderType )
						{
							if( shaderMask & ( 1 << shaderType ) )
							{
								currentShader->ApplySamplerStates( technique, passIx, ShaderType( shaderType ), *renderContext );
							}
						}
					}
					// Set the data from the material, i.e constants and samplers for this pass
					samplersDirtyMask = batch->GetShaderMaterialInterface()->ApplyMaterialDataForPass( technique, passIx, *renderContext );
				}

				// If the batch has per-object data, set it to the device
				const Tr2PerObjectData* const perObjectData = batch->GetPerObjectData();
				if( perObjectData && ( ( perObjectData != curPerObjectData ) || ( currentShaderMask != curShaderTypeMask ) ) )
				{
					D3DPERF_EVENT( L"Object - RenderBatchesSortedByEffect::SetPerObjectDataToDevice" );
					curPerObjectData = perObjectData;
					curShaderTypeMask = currentShaderMask;
					perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, currentShaderMask, *renderContext );
				}

				CCP_STATS_INC( batchCount );

				// Submit the geometry for this batch
				batch->SubmitGeometry( *renderContext );

				batch = batch->GetNext();
			}
		}
	}
}

void Tr2RenderContextBase::RenderBatches( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName )
{
	if( batches->IsChainedByEffect() )
	{
		RenderBatchesSortedByEffect( batches, techniqueName );
	}
	else
	{
		RenderBatchesInOrder( batches, techniqueName );
	}
}

void Tr2RenderContextBase::RenderBatchesWithOverride( ITriRenderBatchAccumulator* batches, Tr2Material* overrideEffect, OverrideMode overrideMode, const BlueSharedString& techniqueName )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !overrideEffect )
	{
		RenderBatches( batches, techniqueName );
		return;
	}

	D3DPERF_EVENT( L"Tr2RenderContextBase::RenderBatchesWithOverride" );

	auto overrideShader = overrideEffect->GetShaderStateInterface();
	if( !overrideShader )
	{
		return;
	}

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	auto overrideMaterial =  overrideEffect;
	for( TriRenderBatch* it = batches->GetFirstBatch(); it != nullptr; it = it->GetNext() )
	{
		auto mode = it->RenderWithOverride();
		if( mode == TriRenderBatch::DO_NOT_RENDER_WITH_OVERRIDE )
		{
			continue;
		}

		auto materialForThisBatch = it->GetShaderMaterialInterface();
		auto shaderForThisBatch = it->GetShaderStateInterface();

		if( !shaderForThisBatch )
		{
			continue;
		}

		uint32_t technique;
		if( !shaderForThisBatch->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}
		if( shaderForThisBatch->GetPassCount( technique ) == 0 )
		{
			continue;
		}

		// Setting render states appropriate to this batch (e.g. wireframe)
		m_esm.ApplyStandardStates( it->GetRenderingMode() );

		Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

		// Get the per-object data
		const Tr2PerObjectData* perObjectData = it->GetPerObjectData();

		uint32_t passCount = overrideShader->GetPassCount( 0 );

		for( uint32_t passIx = 0; passIx < passCount; ++passIx )
		{
			D3DPERF_EVENT1(L"Pass %i", passIx);

			const uint32_t shaderMaskAS = mode == TriRenderBatch::DO_NOT_USE_OVERRIDE_SHADERS ? Tr2RenderContext::SHADER_TYPE_MASK : ( Tr2RenderContext::SHADER_TYPE_MASK & ~( 1 << PIXEL_SHADER ) );
			const uint32_t shaderMask = shaderMaskAS & shaderForThisBatch->GetShaderTypeMask( technique );
			for( uint32_t shaderType = SHADER_TYPE_FIRST; shaderType < SHADER_TYPE_COUNT; ++shaderType )
			{
				if( shaderMaskAS & ( 1 << shaderType ) )
				{
					shaderForThisBatch->ApplyShader( technique, 0, ShaderType( shaderType ), *renderContext );
				}
				if( shaderMask & ( 1 << shaderType ) )
				{
					shaderForThisBatch->ApplySamplerStates( technique, 0, ShaderType( shaderType ), *renderContext );
					materialForThisBatch->ApplyShaderInputs( technique, 0, ShaderType( shaderType ), *renderContext );
				}
			}
			if( overrideMode != OM_DO_NOT_SET_ORIGINAL_PS && mode != TriRenderBatch::DO_NOT_USE_OVERRIDE_SHADERS )
			{
				materialForThisBatch->ApplyShaderInputs( technique, 0, PIXEL_SHADER, *renderContext );
			}

			if( mode != TriRenderBatch::DO_NOT_USE_OVERRIDE_SHADERS )
			{
				overrideShader->ApplyShader( 0, passIx, PIXEL_SHADER, *renderContext );
				overrideShader->ApplyRenderStates( 0, passIx, *renderContext );
				overrideShader->ApplySamplerStates( 0, passIx, PIXEL_SHADER, *renderContext );

				// The override may need specific data. 
				// This isn't guaranteed to play well together.
				overrideMaterial->ApplyShaderInputs( 0, 0, PIXEL_SHADER, *renderContext );
			}
			else
			{
				shaderForThisBatch->ApplyRenderStates( technique, passIx, *renderContext );
			}


			// Apply per-object data
			//------------------------
			// NOTE: it's necessary to set per-object data *after* applying constants
			// and samplers from the effect, because the effect can stomp on the per-object
			// data in some cases.
			//
			// TODO: ensure that the per-object data can be set outside of the pass loop (optimization)
			// without the effect interfering with per-object parameters and data
			// <delder> 18-11-2009
			//
			if( perObjectData )
			{
				D3DPERF_EVENT(L"Tr2RenderContextBase::RenderBatchesWithOverride - SetPerObjectDataToDevice");
				perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, overrideShader->GetShaderTypeMask( 0 ), *renderContext );
			}

			if( mode != TriRenderBatch::DO_NOT_USE_OVERRIDE_SHADERS && overrideEffect && overrideMode == OM_APPLY_PS )
			{
				overrideEffect->ApplyShaderInputs( 0, passIx, PIXEL_SHADER, *renderContext );
			}

			it->SubmitGeometry( *renderContext );

			CCP_STATS_INC( batchCount );
		}
	}
}

void Tr2RenderContextBase::RenderBatchesForPicking( Tr2Material* effect, TriRenderBatch* &p, const BlueSharedString& techniqueName, int &objectNum )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_ASSERT( effect );


	auto shader = effect->GetShaderStateInterface();

	if( !shader )
	{
		// Might still be loading or load could have failed
		return;
	}

	uint32_t passCount = shader->GetPassCount( 0 );
	CCP_ASSERT( passCount == 1 );
	if( passCount == 0 )
	{
		return;
	}

	Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	shader->ApplyShader( 0, 0, PIXEL_SHADER, *renderContext );
	shader->ApplyRenderStates( 0, 0, *renderContext );
	shader->ApplySamplerStates( 0, 0, PIXEL_SHADER, *renderContext );

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	while( p )
	{
		const Tr2PerObjectData* perObjectData = p->GetPerObjectData();

		if( perObjectData )
		{
			perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, shader->GetShaderTypeMask( 0 ), *renderContext );
			uint32_t id = perObjectData->GetUserData();
			if( id != objectNum )
			{
				objectNum = id;
				if( m_objectIdVariable )
				{
					m_objectIdVariable->SetValue( (float)id );
				}
			}

			uint32_t areaID = p->GetPickingData();
			if( m_areaIdVariable )
			{
				m_areaIdVariable->SetValue( (float)areaID );
			}

			// Apply vertex shader inputs from the effect associated with the batch
			auto originalShader = p->GetShaderStateInterface();
			auto originalMaterial = p->GetShaderMaterialInterface();

			// FIXME, allow for picking
			if( originalShader == nullptr )
			{
				p = p->GetNext();
				continue;
			}

			uint32_t technique;
			if( !originalShader->GetTechniqueIndex( techniqueName, technique ) )
			{
				p = p->GetNext();
				continue;
			}

			if( originalShader && originalShader->GetPassCount( technique ) > 0 )
			{
				for( uint32_t shaderType = SHADER_TYPE_FIRST; shaderType < SHADER_TYPE_COUNT; ++shaderType )
				{
					if( shaderType != PIXEL_SHADER && SHADER_TYPE_EXISTS( shaderType ) )
					{
						originalShader->ApplyShader( technique, 0, ShaderType( shaderType ), *renderContext );
						originalShader->ApplySamplerStates( technique, 0, ShaderType( shaderType ), *renderContext );
						originalMaterial->ApplyShaderInputs( technique, 0, ShaderType( shaderType ), *renderContext );
					}
				}

				// Apply pixel shader inputs from the pick effect
				effect->ApplyShaderInputs( 0, 0, PIXEL_SHADER, *renderContext );

				p->SubmitGeometry( *renderContext );

				CCP_STATS_INC( batchCount );
			}
		}

		p = p->GetNext();
	}

}

void Tr2RenderContextBase::RenderBatchesForPickingWithoutOverride( ITriRenderBatchAccumulator* batches, const BlueSharedString& techniqueName, int &objectNum )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT(L"Tr2EffectStateManager::RenderBatchesForPickingWithoutOverride");


	const Tr2PerObjectData* curPerObjectData = nullptr;

	Tr2ConstantBufferAL*	perObjectConstantBuffers[CBUFFER_COUNT];
	for( uint32_t i = 0; i != CBUFFER_COUNT; ++i )
	{
		perObjectConstantBuffers[i] = &m_perObjectConstantBuffers[i];
	}

	Tr2RenderContext *renderContext = reinterpret_cast<Tr2RenderContext*>( this );

	for( TriRenderBatch* it = batches->GetFirstBatch(); it != nullptr; it = it->GetNext() )
	{
		auto material = it->GetShaderMaterialInterface();

		if( !material )
		{
			continue;
		}

		auto shader = it->GetShaderStateInterface();

		if( !shader )
		{
			continue;
		}

		const Tr2PerObjectData* perObjectData = it->GetPerObjectData();

		uint32_t technique;
		if( !shader->GetTechniqueIndex( techniqueName, technique ) )
		{
			continue;
		}

		if( perObjectData )
		{
			if( perObjectData != curPerObjectData )
			{
				D3DPERF_EVENT(L"Set per-object data to device");
				perObjectData->SetPerObjectDataToDevice( perObjectConstantBuffers, shader->GetShaderTypeMask( technique ), *renderContext );
				curPerObjectData = perObjectData;
			}

			uint32_t id = perObjectData->GetUserData();
			if( id != objectNum )
			{
				objectNum = id;
				if( m_objectIdVariable )
				{
					m_objectIdVariable->SetValue( (float)id );
				}
			}

			uint32_t areaID = it->GetPickingData();
			if( m_areaIdVariable )
			{
				m_areaIdVariable->SetValue( (float)areaID );
			}
		}

		uint32_t passCount = shader->GetPassCount( technique );
		CCP_ASSERT( passCount == 1 );

		shader->ApplyAllStateForPass( technique, 0, *renderContext );
		material->ApplyMaterialDataForPass( technique, 0, *renderContext );
		it->SubmitGeometry( *renderContext );
		CCP_STATS_INC( batchCount );

	}
}