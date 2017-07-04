////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#include "StdAfx.h"
#include "Tr2GpuParticleSystem.h"
#include "Tr2GpuBuffer.h"
#include "Tr2GpuStructuredBuffer.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Tr2VariableStore.h"
#include "Tr2RenderTarget.h"
#include "Resources/TriTextureRes.h"
#include "Tr2PushPopRT.h"
#include "Tr2PushPopDS.h"

namespace
{

const uint32_t DEFAULT_MAX_PARTICLES = 512 * 512;
const float MAXIMUM_FRAME_TIME = 1.f / 15.f;
const float MAX_TURBULENCE_LENGTH = 4096.f;
const float TURBULENCE_ANIMATION_SPEED = 0.05f;
const size_t TEXTURE_METHOD_EMITS_PER_DP = 8;

// --------------------------------------------------------------------------------------
// Description:
//   Finds minim width and height of a rectangle such that width and height are power of
//   two values and rectangle area is not less than the given area.  
// --------------------------------------------------------------------------------------
void GetMinPow2Rectange( uint32_t area, uint32_t& width, uint32_t& height )
{
	uint32_t side = std::max( 1u, uint32_t( std::sqrt( double( area ) ) ) );
	width = 1;
	while( width < side )
	{
		width *= 2;
	}
	side = std::max( 1u, area / width );
	height = 1;
	while( height < side )
	{
		height *= 2;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the effect is usable.  
// --------------------------------------------------------------------------------------
bool CheckEffect( Tr2Effect* effect )
{
	return effect && effect->GetShaderStateInterface();
}

}


#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD

Tr2GpuParticleSystem::EmitterParamsGpu::EmitterParamsGpu( const EmitterParams& params )
	:minLifeTime( params.minLifeTime ),
	maxLifeTime( params.maxLifeTime ),
	sizeVariance( params.sizeVariance ),
	textureIndex( float( params.textureIndex ) + std::max( 0.001f, std::min( 0.99f, 1.f - params.colorMidpoint - std::floor( params.colorMidpoint ) ) ) ),
	sizes( params.sizes ),
	drag( params.drag ),
	turbulenceAmplitude( params.turbulenceAmplitude ),
	turbulenceFrequency( float( params.turbulenceFrequency ) / MAX_TURBULENCE_LENGTH ),
	gravity( params.gravity ),
	attractorPosition( params.attractorPosition ),
	attractorStrength( params.attractorStrength ),
	velocityStretchRotation( params.velocityStretchRotation )
{
	colors[0] = params.colors[0];
	colors[1] = params.colors[1];
	colors[2] = params.colors[2];
	colors[3] = params.colors[3];
}

#else

Tr2GpuParticleSystem::EmitterParamsGpu::EmitterParamsGpu( const EmitterParams& params )
	:minLifeTime( params.minLifeTime ),
	maxLifeTime( params.maxLifeTime ),
	sizes( params.sizes, params.sizeVariance ),
	textureIndex( float( params.textureIndex ) + std::max( 0.001f, std::min( 0.99f, 1.f - params.colorMidpoint - std::floor( params.colorMidpoint ) ) ) ),
	drag( params.drag ),
	turbulenceAmplitude( params.turbulenceAmplitude ),
	turbulenceFrequency( float( params.turbulenceFrequency ) / MAX_TURBULENCE_LENGTH ),
	gravity( params.gravity ),
	attractorPosition( params.attractorPosition ),
	attractorStrength( params.attractorStrength ),
	velocityStretchRotation( params.velocityStretchRotation )
{
	colors[0] = params.colors[0];
	colors[1] = params.colors[1];
	colors[2] = params.colors[2];
	colors[3] = params.colors[3];
}

#endif

Tr2GpuParticleSystem::Tr2GpuParticleSystem( IRoot* )
	:m_clearRequested( true ),
	m_maxParticles( DEFAULT_MAX_PARTICLES ),
	m_emitRequests( 64, "Tr2GpuParticleSystem::m_emitRequests" ),
	m_previousTime( -1 ),
	m_enableEmit( true ),
	m_enableUpdate( true ),
	m_enableSort( true ),
	m_enableRender( true ),
	m_updateVisibleCount( false ),
	m_visibleCount( 0 ),
	m_liveTime( 0.f ),
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_TEXTURE_METHOD
	m_decl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
#endif
	m_turbulenceOffset( 0.f, 0.f, 0.f ),
	m_turbulenceAnimation( 0.f, 0.f, 0.f )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_variableStore.CreateInstance();

	InitializeBuffers();
	RegisterVariables();
	SetMaxParticles( m_maxParticles );

	m_emitTimer.Create( renderContext );
	m_updateTimer.Create( renderContext );
	m_sortTimer.Create( renderContext );
	m_renderTimer.Create( renderContext );
}

Tr2GpuParticleSystem::~Tr2GpuParticleSystem()
{
	auto backup = m_variableStore;
	m_variableStore = nullptr;
	SetVariableStore( m_emit );
	SetVariableStore( m_update );
	SetVariableStore( m_render );
	SetVariableStore( m_clear );
	SetVariableStore( m_setDrawParameters );
	SetVariableStore( m_setSortParameters );
	SetVariableStore( m_sort );
	SetVariableStore( m_sortStep );
	SetVariableStore( m_sortInner );
}

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD

// --------------------------------------------------------------------------------------
// Description:
//   Creates GPU buffer variables.  
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::InitializeBuffers()
{
	m_particleData.CreateInstance();
	m_deadList.CreateInstance();
	m_visibleList.CreateInstance();
	m_emitterParamsBuffer.CreateInstance();
	m_drawParameters.CreateInstance();
	m_sortParameters.CreateInstance();
}

// --------------------------------------------------------------------------------------
// Description:
//   Registers GPU buffer variables with the local variable store.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::RegisterVariables()
{
	m_variableStore->RegisterVariable( "ParticleBuffer", m_particleData );
	m_variableStore->RegisterVariable( "DeadBuffer", m_deadList );
	m_variableStore->RegisterVariable( "VisibleBuffer", m_visibleList );
	m_variableStore->RegisterVariable( "DrawParameters", m_drawParameters );
	m_variableStore->RegisterVariable( "SortParameters", m_sortParameters );
	m_variableStore->RegisterVariable( "Emitters", m_emitterParamsBuffer );
}

#else

// --------------------------------------------------------------------------------------
// Description:
//   Creates render targets.  
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::InitializeBuffers()
{
	m_positions[0].CreateInstance();
	m_positions[1].CreateInstance();
	m_velocities[0].CreateInstance();
	m_velocities[1].CreateInstance();
	m_emitterParamsTexture.CreateInstance();
}

// --------------------------------------------------------------------------------------
// Description:
//   Registers render target variables with the local variable store.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::RegisterVariables()
{
	m_variableStore->RegisterVariable( "Positions", m_positions[0] );
	m_variableStore->RegisterVariable( "Velocities", m_velocities[0] );
	m_variableStore->RegisterVariable( "Emitters", m_emitterParamsTexture );
	m_variableStore->RegisterVariable( "PositionsSize", Vector2( 0, 0 ) );
	m_variableStore->RegisterVariable( "EmittersSize", Vector2( 0, 0 ) );
}

#endif

bool Tr2GpuParticleSystem::Initialize()
{
	SetVariableStore( m_emit );
	SetVariableStore( m_update );
	SetVariableStore( m_render );
	SetVariableStore( m_clear );
	SetVariableStore( m_setDrawParameters );
	SetVariableStore( m_setSortParameters );
	SetVariableStore( m_sort );
	SetVariableStore( m_sortStep );
	SetVariableStore( m_sortInner );
	if( m_maxParticles != DEFAULT_MAX_PARTICLES )
	{
		SetMaxParticles( m_maxParticles );
	}
	return true;
}

void Tr2GpuParticleSystem::SetMaxParticles( uint32_t maxParticles )
{
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
	m_maxParticles = maxParticles;
	m_particleData->ReleaseResources( TRISTORAGE_ALL );
	m_deadList->ReleaseResources( TRISTORAGE_ALL );
	m_visibleList->ReleaseResources( TRISTORAGE_ALL );
#else
	uint32_t width, height;
	GetMinPow2Rectange( maxParticles, width, height );
	m_maxParticles = width * height;
	m_variableStore->RegisterVariable( "PositionsSize", Vector2( float( width ), float( height ) ) );

	m_positions[0]->Destroy();
	m_positions[1]->Destroy();
	m_velocities[0]->Destroy();
	m_velocities[1]->Destroy();
#endif
	PrepareResources();
	Clear();
}

void Tr2GpuParticleSystem::ReleaseResources( TriStorage s )
{
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_TEXTURE_METHOD
	if( s & m_vb.GetMemoryClass() )
	{
		m_vb.Destroy();
	}
	if( s & m_ib.GetMemoryClass() )
	{
		m_ib.Destroy();
	}
	m_decl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
#endif
}

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
bool Tr2GpuParticleSystem::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_drawParameters->IsValid() )
	{
		m_drawParameters->Create( 4, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, Tr2GpuBuffer::GPU_WRITABLE | Tr2GpuBuffer::DRAW_INDIRECT );
	}
	if( !m_sortParameters->IsValid() )
	{
		m_sortParameters->Create( 4, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, Tr2GpuBuffer::GPU_WRITABLE | Tr2GpuBuffer::DRAW_INDIRECT );
	}
	if( !m_particleData->IsValid() )
	{
		m_particleData->Create( m_maxParticles, sizeof( ParticleData ), Tr2GpuStructuredBuffer::GPU_WRITABLE );
	}
	if( !m_deadList->IsValid() )
	{
		m_deadList->Create( m_maxParticles, sizeof( uint32_t ), Tr2GpuStructuredBuffer::COUNTER | Tr2GpuStructuredBuffer::GPU_WRITABLE );
	}
	if( !m_visibleList->IsValid() )
	{
		m_visibleList->Create( m_maxParticles, sizeof( uint32_t ) * 2, Tr2GpuStructuredBuffer::COUNTER | Tr2GpuStructuredBuffer::GPU_WRITABLE );
	}
	UpdateGpuEmitterParams( renderContext );
	return true;
}

#else

bool Tr2GpuParticleSystem::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_emitterParamsTexture->IsGood() )
	{
		UpdateGpuEmitterParams( renderContext );
	}

	uint32_t width, height;
	GetMinPow2Rectange( m_maxParticles, width, height );

	if( !m_vb.IsValid() )
	{
		std::vector<Vector4> vb;
		vb.reserve( m_maxParticles * 4 );
		for( uint32_t i = 0; i < m_maxParticles; ++i )
		{
			float seed = float( rand() ) / RAND_MAX;
			for( uint32_t j = 0; j < 4; ++j )
			{
				vb.push_back( Vector4( float( i % width ), float( i / width ), float( j ), seed ) );
			}
		}
		m_vb.Create( m_maxParticles * 4 * sizeof( Vector4 ), Tr2RenderContextEnum::USAGE_IMMUTABLE, &vb.front(), renderContext );
	}

	if( !m_ib.IsValid() )
	{
		const static unsigned quad[] = { 0, 1, 2, 1, 3, 2 };
		std::vector<uint32_t> ib;
		ib.reserve( m_maxParticles * 6 );
		for( uint32_t i = 0; i < m_maxParticles; ++i )
		{
			for( uint32_t j = 0; j < 6; ++j )
			{
				ib.push_back( i * 4 + quad[j] );
			}
		}
		m_ib.Create( m_maxParticles * 6, Tr2RenderContextEnum::USAGE_IMMUTABLE, Tr2RenderContextEnum::IB_32BIT, &ib.front(), renderContext );
	}
	if( !m_positions[0]->IsValid() )
	{
		m_positions[0]->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT );
	}
	if( !m_positions[1]->IsValid() )
	{
		m_positions[1]->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT );
	}
	if( !m_velocities[0]->IsValid() )
	{
		m_velocities[0]->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT );
	}
	if( !m_velocities[1]->IsValid() )
	{
		m_velocities[1]->Create( width, height, 1, Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT );
	}
	if( m_decl == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition decl;
		decl.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::POSITION );
		m_decl = renderContext.m_esm.GetVertexDeclarationHandle( decl );
	}
	Clear();
	return true;
}

#endif

bool Tr2GpuParticleSystem::OnModified( Be::Var* value )
{
	// assign local variable store to any effect that has been set
	if( IsMatch( value, m_emit ) )
	{
		SetVariableStore( m_emit );
	}
	else if( IsMatch( value, m_update ) )
	{
		SetVariableStore( m_update );
	}
	else if( IsMatch( value, m_render ) )
	{
		SetVariableStore( m_render );
	}
	else if( IsMatch( value, m_clear ) )
	{
		SetVariableStore( m_clear );
	}
	else if( IsMatch( value, m_setDrawParameters ) )
	{
		SetVariableStore( m_setDrawParameters );
	}
	else if( IsMatch( value, m_setSortParameters ) )
	{
		SetVariableStore( m_setSortParameters );
	}
	else if( IsMatch( value, m_sort ) )
	{
		SetVariableStore( m_sort );
	}
	else if( IsMatch( value, m_sortStep ) )
	{
		SetVariableStore( m_sortStep );
	}
	else if( IsMatch( value, m_sortInner ) )
	{
		SetVariableStore( m_sortInner );
	}
	else if( IsMatch( value, m_maxParticles ) )
	{
		SetMaxParticles( m_maxParticles );
	}
	return true;
}

void Tr2GpuParticleSystem::SetVariableStore( Tr2Effect* effect )
{
	if( effect )
	{
		effect->StartUpdate();
		effect->SetVariableStore( m_variableStore );
		effect->EndUpdate();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Public method to remove all particles. The actual clear though happens during next
//   Update call.  
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::Clear()
{
	m_clearRequested = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates particle system. Optionally clears the system, emits new particles, runs
//   physics simulation does sorting.
// Arguments:
//   time - Current time
//   originShift - world origin shif compared to previous frame
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::Update( Be::Time time, const Vector3& originShift, Tr2RenderContext& renderContext )
{
	if( !CheckEffect( m_emit ) || !CheckEffect( m_update ) || !CheckEffect( m_clear ) || !CheckEffect( m_setDrawParameters ) )
	{
		return;
	}

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_TEXTURE_METHOD
	Tr2PushPopDS pushPopDS( nullDS, renderContext );
	Tr2PushPopRT pushPopRT0( renderContext, 0 );
	Tr2PushPopRT pushPopRT1( renderContext, 1 );
#endif

	float dt = ( m_previousTime == Be::Time( -1 ) || !m_enableUpdate ) ? 0 : TimeAsFloat( time - m_previousTime );
	dt = std::min( dt, MAXIMUM_FRAME_TIME );
	m_previousTime = time;

	if( m_clearRequested )
	{
		if( !DoClear( renderContext ) )
		{
			return;
		}
		m_clearRequested = false;
		m_turbulenceOffset = Vector3( 0.f, 0.f, 0.f );
	}

	m_turbulenceOffset -= originShift;
	m_turbulenceOffset /= MAX_TURBULENCE_LENGTH;
	m_turbulenceOffset.x -= floor( m_turbulenceOffset.x );
	m_turbulenceOffset.y -= floor( m_turbulenceOffset.y );
	m_turbulenceOffset.z -= floor( m_turbulenceOffset.z );
	m_turbulenceOffset *= MAX_TURBULENCE_LENGTH;

	m_turbulenceAnimation.z += dt * TURBULENCE_ANIMATION_SPEED;

	ExpireEmitterParams( dt );
	if( m_enableEmit )
	{
		UpdateEmitterParams( renderContext );
		EmitParticles( renderContext );
	}
	m_emitRequests.Clear();

	if( m_liveTime <= 0.f )
	{
		// no particles left alive
		return;
	}
	
	RunSimulation( dt, originShift, renderContext );

	m_liveTime -= dt;

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
	renderContext.CopyBufferCounter( *m_drawParameters, 0, *m_visibleList );

	UpdateLiveCount( renderContext );

	if( m_enableSort )
	{
		Sort( renderContext );
	}

	// prepare draw arguments for Render
	renderContext.m_esm.UnsetAllTextures();
	Tr2Renderer::RunComputeShader( m_setDrawParameters, 1, 1, 1, renderContext );
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates number of live/visible particles for debugging. 
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::UpdateLiveCount( Tr2RenderContext& renderContext )
{
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
	if( m_updateVisibleCount )
	{
		uint32_t* count = nullptr;
		CR_RETURN( m_drawParameters->GetGpuBuffer( 0 )->Lock( 0, sizeof( uint32_t ), (void**)&count, Tr2RenderContextEnum::LOCK_READONLY, renderContext ) );
		m_visibleCount = *count;
		m_drawParameters->GetGpuBuffer( 0 )->Unlock( renderContext );
	}
#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes all particles from the system.
// --------------------------------------------------------------------------------------
bool Tr2GpuParticleSystem::DoClear( Tr2RenderContext& renderContext )
{
	m_liveTime = 0;
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
	return Tr2Renderer::RunComputeShader( m_clear, 1, 1, 1, renderContext );
#else
	m_targetIndex = 0;
	m_emitIndex = 0;

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	CR_RETURN_VAL( renderContext.SetRenderTarget( *m_positions[0] ), false );
	Tr2Renderer::DrawScreenQuad( m_clear );
	CR_RETURN_VAL( renderContext.SetRenderTarget( *m_positions[1] ), false );
	Tr2Renderer::DrawScreenQuad( m_clear );
	return true;
#endif
}

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD

// --------------------------------------------------------------------------------------
// Description:
//   Runs physics simulation. Basically executes a single compute shader
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::RunSimulation( float dt, const Vector3& originShift, Tr2RenderContext& renderContext )
{
	Tr2GpuTimerALContext ctx( m_updateTimer, renderContext );

	struct
	{
		uint32_t groupX;
		uint32_t groupY;
		uint32_t groupZ;
		float dt;

		Vector3 originShift;
		float padding0;

		Vector3 turbulenceOffset;
		float padding1;

		Vector3 turbulenceAnimation;
		float padding2;

		Vector4 frustumPlanes[6];
	} updateCB;

	updateCB.groupX = 32;
	updateCB.groupY = std::max( m_maxParticles / 32 / 32 / 32, 1u );
	updateCB.groupZ = std::max( m_maxParticles / 32 / 32 / 32 / 32, 1u );
	updateCB.dt = dt;
	updateCB.originShift = originShift;
	updateCB.turbulenceOffset = m_turbulenceOffset;
	updateCB.turbulenceAnimation = m_turbulenceAnimation;

	for( size_t i = 0; i < 6; ++i )
	{
		Tr2Renderer::GetFrustumPlane( i, updateCB.frustumPlanes[i] );
	}
	FillAndSetConstants( m_updateCB, updateCB, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	Tr2Renderer::RunComputeShader( m_update, updateCB.groupX, updateCB.groupY, updateCB.groupZ, renderContext );
}

#else

// --------------------------------------------------------------------------------------
// Description:
//   Runs physics simulation. Basically draws a single quad
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::RunSimulation( float dt, const Vector3& originShift, Tr2RenderContext& renderContext )
{
	Tr2GpuTimerALContext ctx( m_updateTimer, renderContext );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );
	m_variableStore->RegisterVariable( "Positions", m_positions[m_targetIndex] );
	m_variableStore->RegisterVariable( "Velocities", m_velocities[m_targetIndex] );

	m_targetIndex = 1 - m_targetIndex;

	renderContext.SetRenderTarget( *m_positions[m_targetIndex] );
	renderContext.SetRenderTarget( *m_velocities[m_targetIndex], 1 );

	struct
	{
		float dt;
		float width;
		float height;
		float padding0;
		Vector3 originShift;
		float padding1;
	} updateCB;
	updateCB.dt = dt;
	updateCB.width = float( m_positions[0]->GetWidth() );
	updateCB.height = float( m_positions[0]->GetHeight() );
	updateCB.originShift = originShift;
	updateCB.padding0 = updateCB.padding1 = 0;
	FillAndSetConstants( m_updateCB, updateCB, Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );
	Tr2Renderer::DrawScreenQuad( m_update );
}

#endif

// --------------------------------------------------------------------------------------
// Description:
//   Updates lifetime for persistent emitter parameters and moves those that are not used
//   anymore to a "available" pool.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::ExpireEmitterParams( float dt )
{
	std::vector<uintptr_t> toRemove;
	for( auto it = begin( m_emitterParamsIndex ); it != end( m_emitterParamsIndex ); ++it )
	{
		if( it->second.lifetime > 0 )
		{
			it->second.lifetime -= dt;
			if( it->second.lifetime < 0 )
			{
				m_expiredEmitters.push_back( it->second.index );
				toRemove.push_back( it->first );
			}
		}
	}
	for( auto it = begin( toRemove ); it != end( toRemove ); ++it )
	{
		m_emitterParamsIndex.erase( *it );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Goes through emit request and updates persistent emitter parameters.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::UpdateEmitterParams( Tr2RenderContext& renderContext )
{
	float maxLiveTime = 0.f;
	bool emitterBufferDirty = false;
	for( auto it = begin( m_emitRequests ); it != end( m_emitRequests ); ++it )
	{
		maxLiveTime = std::max( maxLiveTime, it->params.maxLifeTime );
		auto found = m_emitterParamsIndex.find( it->id );
		uint32_t index;
		if( found == m_emitterParamsIndex.end() )
		{
			EmitterParamsIndex paramsIndex;
			if( m_expiredEmitters.empty() )
			{
				m_emitterParams.push_back( it->params );
				paramsIndex.index = m_emitterParams.size() - 1;
			}
			else
			{
				paramsIndex.index = m_expiredEmitters.back();
				m_expiredEmitters.pop_back();
				m_emitterParams[paramsIndex.index] = it->params;
			}
			paramsIndex.hash = it->hash;
			paramsIndex.lifetime = it->params.maxLifeTime;

			m_emitterParamsIndex[it->id] = paramsIndex;
			index = uint32_t( paramsIndex.index );
			emitterBufferDirty = true;
		}
		else
		{
			emitterBufferDirty |= found->second.hash != it->hash;
			found->second.hash = it->hash;
			found->second.lifetime = it->params.maxLifeTime;
			index = uint32_t( found->second.index );
			m_emitterParams[found->second.index] = it->params;
		}
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
		it->emitter.emitterSeed |= index;
#else
		it->emitter.directionPreviousEmitter.w = float( index );
#endif
	}
	if( emitterBufferDirty )
	{
		UpdateGpuEmitterParams( renderContext );
	}
	m_liveTime = std::max( m_liveTime, maxLiveTime );
}

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD

// --------------------------------------------------------------------------------------
// Description:
//   Updates GPU buffer with persistent emitter parameters.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::UpdateGpuEmitterParams( Tr2RenderContext& renderContext )
{
	if( !m_emitterParamsBuffer->IsValid() || m_emitterParamsBuffer->GetCount() < m_emitterParams.size() )
	{
		m_emitterParamsBuffer->Create( std::max( 64u, uint32_t( m_emitterParams.size() ) ), sizeof( EmitterParamsGpu ), Tr2GpuStructuredBuffer::CPU_WRITABLE );
	}

	if( m_emitterParams.size() == 0 )
	{
		return;
	}

	m_emitterParamsBuffer->GetGpuBuffer( 0 )->UpdateBuffer( 0, uint32_t( m_emitterParams.size() * sizeof( m_emitterParams[0] ) ), &m_emitterParams[0], renderContext );
}

#else

// --------------------------------------------------------------------------------------
// Description:
//   Updates texture with persistent emitter parameters.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::UpdateGpuEmitterParams( Tr2RenderContext& renderContext )
{
	auto texture = m_emitterParamsTexture->GetTexture();
	if( !texture || texture->GetHeight() < m_emitterParams.size() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		m_emitterParamsTexture->Create( 
			sizeof( EmitterParamsGpu ) / sizeof( Vector4 ), 
			std::max( 64u, uint32_t( m_emitterParams.size() ) ), 
			1, 
			Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 
			Tr2RenderContextEnum::USAGE_CPU_WRITE, 
			renderContext );
		texture = m_emitterParamsTexture->GetTexture();
		if( !texture )
		{
			return;
		}

	}
	void* data = nullptr;
	uint32_t pitch;
	CR_RETURN( texture->Lock( 0, data, pitch, Tr2RenderContextEnum::LOCK_WRITEONLY, renderContext ) );
	for( size_t i = 0; i < m_emitterParams.size(); ++i )
	{
		*reinterpret_cast<EmitterParamsGpu*>( reinterpret_cast<uint8_t*>( data ) + i * pitch ) = m_emitterParams[i];
	}
	texture->Unlock( renderContext );

	m_variableStore->RegisterVariable( "EmittersSize", Vector2( float( m_emitterParamsTexture->GetWidth() ), float( m_emitterParamsTexture->GetHeight() ) ) );
}

#endif

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD

// --------------------------------------------------------------------------------------
// Description:
//   Emits new particles into the system. Uses constant buffer to pass emitter parameters
//   to the shader.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::EmitParticles( Tr2RenderContext& renderContext )
{
	struct EmitterCBPrefix
	{
		uint32_t count;
		float padding[3];
	};

	static const size_t maxSize = 64 * 1024;
	static const size_t emitsPerDispatch = ( maxSize - sizeof( EmitterCBPrefix ) ) / sizeof( EmitterGpu );
	struct CB
	{
		EmitterCBPrefix prefix;
		EmitterGpu emitters[emitsPerDispatch];
	} cb;

	Tr2GpuTimerALContext ctx( m_emitTimer, renderContext );
	for( size_t i = 0; i < m_emitRequests.GetCount(); i += emitsPerDispatch )
	{
		cb.prefix.count = uint32_t( std::min( m_emitRequests.GetCount() - i, emitsPerDispatch ) );
		for( size_t j = 0; j < cb.prefix.count; ++j )
		{
			cb.emitters[j] = m_emitRequests[i + j].emitter;
		}
		FillAndSetConstants( 
			m_emitCB, 
			&cb, 
			sizeof( EmitterCBPrefix ) + cb.prefix.count * sizeof( EmitterGpu ), 
			Tr2RenderContextEnum::COMPUTE_SHADER, 
			Tr2Renderer::GetPerObjectVSStartRegister(), 
			renderContext );

		Tr2Renderer::RunComputeShader( m_emit, cb.prefix.count, 1, 1, renderContext );
	}
}

#else

// --------------------------------------------------------------------------------------
// Description:
//   Emits new particles into the system. Uses constant buffer to pass emitter parameters
//   to the shader.
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::EmitParticles( Tr2RenderContext& renderContext )
{
	auto* effectResource = m_emit->GetShaderStateInterface();

	if( !effectResource || !effectResource->GetPassCount( 0 ) )
	{
		return;
	}

	struct EmitterCBPrefix
	{
		float width;
		float height;
		float count;
		float padding0;
	};

	struct PerObjectPS
	{
		EmitterCBPrefix prefix;
		EmitterGpu emitters[TEXTURE_METHOD_EMITS_PER_DP];
	} perObjectPS;

	struct PerObjectVS
	{
		EmitterCBPrefix prefix;
		Vector4 offsetCount[TEXTURE_METHOD_EMITS_PER_DP];
	} perObjectVS;

	Tr2GpuTimerALContext ctx( m_emitTimer, renderContext );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

	renderContext.m_esm.ApplyStreamSource( 0, m_vb, 0, sizeof( Vector4 ) );
	renderContext.m_esm.ApplyIndexBuffer( m_ib );
	renderContext.m_esm.ApplyVertexDeclaration( m_decl );
	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );

	renderContext.SetRenderTarget( *m_positions[m_targetIndex] );
	renderContext.SetRenderTarget( *m_velocities[m_targetIndex], 1 );

	effectResource->ApplyAllStateForPass( 0, 0, renderContext );
	m_emit->ApplyMaterialDataForPass( 0, 0, renderContext );

	for( size_t i = 0; i < m_emitRequests.GetCount(); i += TEXTURE_METHOD_EMITS_PER_DP )
	{
		uint32_t count = uint32_t( std::min( m_emitRequests.GetCount() - i, TEXTURE_METHOD_EMITS_PER_DP ) );
		perObjectPS.prefix.count = float( count );
		perObjectPS.prefix.width = float( m_positions[0]->GetWidth() );
		perObjectPS.prefix.height = float( m_positions[0]->GetHeight() );
		for( size_t j = 0; j < count; ++j )
		{
			perObjectPS.emitters[j] = m_emitRequests[i + j].emitter;
		}
		perObjectVS.prefix.count = float( count );
		perObjectVS.prefix.width = float( m_positions[0]->GetWidth() );
		perObjectVS.prefix.height = float( m_positions[0]->GetHeight() );
		for( size_t j = 0; j < count; ++j )
		{
			perObjectVS.offsetCount[j] = Vector4( 
				perObjectPS.emitters[j].offsetSeed.x,
				perObjectPS.emitters[j].offsetSeed.y,
				0.f,
				perObjectPS.emitters[j].positionCount.w );
		}
		FillAndSetConstants( m_updateCB, &perObjectVS, sizeof( EmitterCBPrefix ) + count * sizeof( EmitterGpu ), Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
		FillAndSetConstants( m_emitCB, &perObjectPS, sizeof( EmitterCBPrefix ) + count * sizeof( EmitterGpu ), Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );


		renderContext.DrawIndexedPrimitive( 4 * 4 * count, 0, 2 * 4 * count );
	}
	m_emitIndex = m_emitIndex % m_maxParticles;
}
#endif

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD

// --------------------------------------------------------------------------------------
// Description:
//   Sorts particles!
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::Sort( Tr2RenderContext& renderContext )
{
	if( !CheckEffect( m_setSortParameters ) || !CheckEffect( m_sort ) || !CheckEffect( m_sortStep ) || !CheckEffect( m_sortInner ) )
	{
		return;
	}

	Tr2GpuTimerALContext ctx( m_sortTimer, renderContext );

	renderContext.m_esm.UnsetAllTextures();
	renderContext.CopyBufferCounter( *m_sortParameters, 0, *m_visibleList );
	Tr2Renderer::RunComputeShader( m_setSortParameters, 1, 1, 1, renderContext );
	Tr2Renderer::RunComputeShaderIndirect( m_sort, *m_sortParameters, 0, renderContext );

	if( m_maxParticles > 512 )
	{
		uint32_t presorted = 512;
		bool done = false;
		while( !done )
		{
			done = SortIncremental( presorted, renderContext );
			presorted *= 2;
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Sorts more particles!
// --------------------------------------------------------------------------------------
bool Tr2GpuParticleSystem::SortIncremental( uint32_t presorted, Tr2RenderContext& renderContext )
{
	const uint32_t maxSize = m_maxParticles;
	bool done = maxSize <= presorted * 2;
	uint32_t numThreadGroups=0;
	
	if( maxSize > presorted )
	{	
		uint32_t pow2 = presorted; 
		while( pow2 < maxSize ) 
		{
			pow2 *= 2;
		}
		numThreadGroups = pow2 >> 9;
	}	

	uint32_t mergeSize = presorted * 2;
	for( uint32_t mergeSubSize = mergeSize >> 1; mergeSubSize > 256; mergeSubSize = mergeSubSize >> 1 ) 
	{
		struct SortConstants
		{
			uint32_t constants[4];
		} sortConstants;
		sortConstants.constants[0] = mergeSubSize;
		if( mergeSubSize == mergeSize / 2 )
		{
			sortConstants.constants[1] = 2 * mergeSubSize - 1;
			sortConstants.constants[2] = -1;
		}
		else
		{
			sortConstants.constants[1] = mergeSubSize;
			sortConstants.constants[2] = 1;
		}

		FillAndSetConstants( m_sortCB, sortConstants, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
		Tr2Renderer::RunComputeShader( m_sortStep, numThreadGroups, 1, 1, renderContext );
	}
	Tr2Renderer::RunComputeShader( m_sortInner, numThreadGroups, 1, 1, renderContext );
	return done;
}

#endif

// --------------------------------------------------------------------------------------
// Description:
//   Renders particles!
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::Render( Tr2RenderContext& renderContext )
{
	if( !m_render || !m_enableRender || m_liveTime <= 0.f )
	{
		return;
	}

	Tr2GpuTimerALContext ctx( m_renderTimer, renderContext );

#if GPU_PARTICLES_METHOD == GPU_PARTICLES_TEXTURE_METHOD
	m_variableStore->RegisterVariable( "Positions", m_positions[m_targetIndex] );
	m_variableStore->RegisterVariable( "Velocities", m_velocities[m_targetIndex] );
#endif

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	m_render->Render( this, renderContext );
}

void Tr2GpuParticleSystem::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
	renderContext.m_esm.ApplyVertexDeclaration( Tr2EffectStateManager::NULL_DECLARATION );
	renderContext.DrawInstancedIndirect( *m_drawParameters, 0 );
#else

	renderContext.m_esm.ApplyStreamSource( 0, m_vb, 0, sizeof( Vector4 ) );
	renderContext.m_esm.ApplyIndexBuffer( m_ib );
	renderContext.m_esm.ApplyVertexDeclaration( m_decl );

	renderContext.DrawIndexedPrimitive( m_maxParticles * 4, 0, 2 * m_maxParticles );

#endif
}

// --------------------------------------------------------------------------------------
// Description:
//   Public method to emit particles. Stores emit request to be processed during next
//   update. This method is thread safe.
// Arguments:
//   emitter - Emitter parameters
//   id - Persistent emitter parameters semi-unique id
//   hash - Persistent emitter parameters hash value
//   params - Persistent emitter parameters
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::Emit( const Emitter& emitter, uintptr_t id, uintptr_t hash, const EmitterParams& params )
{
	EmitRequest request;
	if( !m_enableEmit )
	{
		return;
	}
#if GPU_PARTICLES_METHOD == GPU_PARTICLES_BUFFER_METHOD
	request.emitter = emitter;
	request.emitter.count = std::min( request.emitter.count, m_maxParticles );
	request.emitter.emitterSeed = rand() << 16;
#else
	auto start = ( m_emitIndex += emitter.count ) - emitter.count;
	request.emitter.positionCount = Vector4( emitter.position, float( std::min( emitter.count, m_maxParticles ) ) );
	request.emitter.positionPreviousRadius = Vector4( emitter.positionPrevious, emitter.radius );
	request.emitter.directionAngle = Vector4( emitter.direction, emitter.angle );
	request.emitter.directionPreviousEmitter = Vector4( emitter.directionPrevious, float( emitter.emitterSeed & 0xffff ) );
	request.emitter.velocityMinSpeed = Vector4( emitter.velocity, emitter.minSpeed );
	request.emitter.velocityPreviousMaxSpeed = Vector4( emitter.velocityPrevious, emitter.maxSpeed );
	request.emitter.offsetSeed = Vector4( float( start % m_positions[0]->GetWidth() ), float( ( start / m_positions[0]->GetWidth() ) % m_positions[0]->GetHeight() ), float( rand() ), 0.f );
	request.emitter.innerAngleUnused.x = emitter.innerAngle;
#endif

	request.id = id;
	request.hash = hash;
	request.params = EmitterParamsGpu( params );
	m_emitRequests.Add( request, "Tr2GpuParticleSystem::m_emitRequests" );
}

float Tr2GpuParticleSystem::GetEmitTime()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_emitTimer.GetTime( renderContext ) * 1000.f;
}

float Tr2GpuParticleSystem::GetUpdateTime()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_updateTimer.GetTime( renderContext ) * 1000.f;
}

float Tr2GpuParticleSystem::GetSortTime()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_sortTimer.GetTime( renderContext ) * 1000.f;
}

float Tr2GpuParticleSystem::GetRenderTime()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return m_renderTimer.GetTime( renderContext ) * 1000.f;
}

bool Tr2GpuParticleSystem::HasParticles() const
{
	return m_liveTime > 0.f;
}