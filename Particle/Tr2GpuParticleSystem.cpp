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

const uint32_t DEFAULT_MAX_PARTICLES = 1024 * 1024;

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
	m_turbulenceOffset( 0.f, 0.f, 0.f ),
	m_turbulenceAnimation( 0.f, 0.f, 0.f )
{
	m_variableStore.CreateInstance();

	InitializeBuffers();
	RegisterVariables();
	SetMaxParticles( m_maxParticles );

	m_emitTimer.SetCaptureGpuTime( true );
	m_updateTimer.SetCaptureGpuTime( true );
	m_sortTimer.SetCaptureGpuTime( true );
	m_renderTimer.SetCaptureGpuTime( true );
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
	m_counters.CreateInstance();
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
	m_variableStore->RegisterVariable( "ParticleCounters", m_counters );
}

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
	m_maxParticles = maxParticles;
	m_particleData->ReleaseResources( TRISTORAGE_ALL );
	m_deadList->ReleaseResources( TRISTORAGE_ALL );
	m_visibleList->ReleaseResources( TRISTORAGE_ALL );
	m_counters->ReleaseResources( TRISTORAGE_ALL );
	PrepareResources();
	Clear();
}

void Tr2GpuParticleSystem::ReleaseResources( TriStorage s )
{
	if( s & m_emitCB.GetMemoryClass() )
	{
		m_emitCB = Tr2ConstantBufferAL();
	}
	if( s & m_updateCB.GetMemoryClass() )
	{
		m_updateCB = Tr2ConstantBufferAL();
	}
	if( s & m_sortCB.GetMemoryClass() )
	{
		m_sortCB = Tr2ConstantBufferAL();
	}
	if( ( s & TRISTORAGE_MANAGEDMEMORY ) != 0 )
	{
		Clear();
	}
}

bool Tr2GpuParticleSystem::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_drawParameters->IsValid() )
	{
		m_drawParameters->Create( 4, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, Tr2GpuBuffer::GPU_WRITABLE | Tr2GpuBuffer::DRAW_INDIRECT );
		m_drawParameters->SetName( "GPU Particle Draw Params" );
		m_clearRequested = true;
	}
	if( !m_sortParameters->IsValid() )
	{
		m_sortParameters->Create( 4, Tr2RenderContextEnum::PIXEL_FORMAT_R32_UINT, Tr2GpuBuffer::GPU_WRITABLE | Tr2GpuBuffer::DRAW_INDIRECT );
		m_sortParameters->SetName( "GPU Particle Sort Params" );
		m_clearRequested = true;
	}
	if( !m_particleData->IsValid() )
	{
		m_particleData->Create( m_maxParticles, sizeof( ParticleData ), Tr2GpuStructuredBuffer::GPU_WRITABLE );
		m_particleData->SetName( "GPU Particles" );
		m_clearRequested = true;
	}
	if( !m_deadList->IsValid() )
	{
		m_deadList->Create( m_maxParticles, sizeof( uint32_t ), Tr2GpuStructuredBuffer::GPU_WRITABLE );
		m_deadList->SetName( "GPU Particle Dead List" );
		m_clearRequested = true;
	}
	if( !m_visibleList->IsValid() )
	{
		m_visibleList->Create( m_maxParticles, sizeof( uint32_t ) * 2, Tr2GpuStructuredBuffer::GPU_WRITABLE );
		m_visibleList->SetName( "GPU Particle Visible List" );
		m_clearRequested = true;
	}
	if( !m_counters->IsValid() )
	{
		m_counters->Create( 2, Tr2RenderContextEnum::PIXEL_FORMAT_R32_SINT, Tr2GpuBuffer::GPU_WRITABLE );
		m_counters->SetName( "GPU Particle Counters" );
		m_clearRequested = true;
	}
	UpdateGpuEmitterParams( renderContext );
	return true;
}

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

	if( m_enableSort )
	{
		Sort( renderContext );
	}

	// prepare draw arguments for Render
	Tr2Renderer::RunComputeShader( m_setDrawParameters, 1, 1, 1, renderContext );

	UpdateLiveCount( renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates number of live/visible particles for debugging. 
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::UpdateLiveCount( Tr2RenderContext& renderContext )
{
	if( m_updateVisibleCount )
	{
		const uint32_t* count = nullptr;
		CR_RETURN( m_drawParameters->GetGpuBuffer( 0 )->MapForReading( count, renderContext ) );
		m_visibleCount = *count / 6;
		m_drawParameters->GetGpuBuffer( 0 )->UnmapForReading( renderContext );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes all particles from the system.
// --------------------------------------------------------------------------------------
bool Tr2GpuParticleSystem::DoClear( Tr2RenderContext& renderContext )
{
	m_liveTime = 0;
	uint32_t zeroes[] = { 0, 0, 0, 0 };
	renderContext.ClearUav( *m_counters->GetGpuBuffer( 0 ), zeroes );

	uint32_t perObjectData[4] = { m_particleData->GetCount(), 0, 0, 0 };
	FillAndSetConstants( m_updateCB, perObjectData, sizeof( perObjectData ), Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	return Tr2Renderer::RunComputeShader( m_clear, 1, 1, 1, renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Runs physics simulation. Basically executes a single compute shader
// --------------------------------------------------------------------------------------
void Tr2GpuParticleSystem::RunSimulation( float dt, const Vector3& originShift, Tr2RenderContext& renderContext )
{
	m_updateTimer.Begin( renderContext );
	ON_BLOCK_EXIT( [&] { m_updateTimer.End( renderContext ); } );

	const uint32_t numthreadsX = 16;
	const uint32_t numthreadsY = 16;

	struct
	{
		uint32_t groupX;
		uint32_t groupY;
		uint32_t groupZ;
		float dt;

		Vector3 originShift;
		uint32_t bufferSize;

		Vector3 turbulenceOffset;
		float padding1;

		Vector3 turbulenceAnimation;
		float padding2;

		Vector4 frustumPlanes[6];
	} updateCB;

	updateCB.groupX = 32;
	updateCB.groupY = std::max( m_maxParticles / 32 / numthreadsX / numthreadsY, 1u );
	updateCB.groupZ = std::max( m_maxParticles / 32 / 32 / numthreadsX / numthreadsY, 1u );
	updateCB.dt = dt;
	updateCB.originShift = originShift;
	updateCB.bufferSize = m_particleData->GetCount();
	updateCB.turbulenceOffset = m_turbulenceOffset;
	updateCB.turbulenceAnimation = m_turbulenceAnimation;

	for( size_t i = 0; i < 6; ++i )
	{
		Tr2Renderer::GetFrustumPlane( i, updateCB.frustumPlanes[i] );
	}
	FillAndSetConstants( m_updateCB, updateCB, Tr2RenderContextEnum::COMPUTE_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	Tr2Renderer::RunComputeShader( m_update, BlueSharedString( "ClearCounters" ), 1, 1, 1, renderContext );
	Tr2Renderer::RunComputeShader( m_update, updateCB.groupX, updateCB.groupY, updateCB.groupZ, renderContext );
}

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
		auto lifeTime = std::max( it->params.minLifeTime, it->params.maxLifeTime ) + 1;
		maxLiveTime = std::max( maxLiveTime, lifeTime );
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
			paramsIndex.lifetime = lifeTime;

			m_emitterParamsIndex[it->id] = paramsIndex;
			index = uint32_t( paramsIndex.index );
			emitterBufferDirty = true;
		}
		else
		{
			emitterBufferDirty |= found->second.hash != it->hash;
			found->second.hash = it->hash;
			found->second.lifetime = std::max( found->second.lifetime, lifeTime );
			index = uint32_t( found->second.index );
			m_emitterParams[found->second.index] = it->params;
		}
		it->emitter.emitterSeed |= index;
	}
	if( emitterBufferDirty )
	{
		UpdateGpuEmitterParams( renderContext );
	}
	m_liveTime = std::max( m_liveTime, maxLiveTime );
}

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

	if( !m_emitterParamsBuffer->IsValid() )
	{
		return;
	}
	m_emitterParamsBuffer->GetGpuBuffer( 0 )->UpdateBuffer( 0, uint32_t( m_emitterParams.size() * sizeof( m_emitterParams[0] ) ), &m_emitterParams[0], renderContext );
}

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

	static const size_t maxSize = TRINITY_PLATFORM_MAX_CONSTANT_BUFFER_SIZE;
	static const size_t emitsPerDispatch = ( maxSize - sizeof( EmitterCBPrefix ) ) / sizeof( EmitterGpu );
	struct CB
	{
		EmitterCBPrefix prefix;
		EmitterGpu emitters[emitsPerDispatch];
	} cb;

	m_emitTimer.Begin( renderContext );
	ON_BLOCK_EXIT( [&] { m_emitTimer.End( renderContext ); } );

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

	m_sortTimer.Begin( renderContext );
	ON_BLOCK_EXIT( [&] { m_sortTimer.End( renderContext ); } );

	Tr2Renderer::RunComputeShader( m_setSortParameters, 1, 1, 1, renderContext );
	if( m_sortParameters && m_sortParameters->IsValid() )
	{
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

	m_renderTimer.Begin( renderContext );
	ON_BLOCK_EXIT( [&] { m_renderTimer.End( renderContext ); } );

	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_ALPHA_ADDITIVE );
	m_render->Render( this, renderContext );
}

void Tr2GpuParticleSystem::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( m_drawParameters && m_drawParameters->IsValid() )
	{
		renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
		renderContext.m_esm.ApplyVertexDeclaration( Tr2EffectStateManager::NULL_DECLARATION );
		renderContext.DrawInstancedIndirect( *m_drawParameters, 0 );
	}
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
	request.emitter = emitter;
	request.emitter.count = std::min( request.emitter.count, m_maxParticles );
	request.emitter.emitterSeed = rand() << 16;
	request.id = id;
	request.hash = hash;
	request.params = EmitterParamsGpu( params );
	m_emitRequests.Add( request, "Tr2GpuParticleSystem::m_emitRequests" );
}

float Tr2GpuParticleSystem::GetEmitTime()
{
	return m_emitTimer.GpuTime();
}

float Tr2GpuParticleSystem::GetUpdateTime()
{
	return m_updateTimer.GpuTime();
}

float Tr2GpuParticleSystem::GetSortTime()
{
	return m_sortTimer.GpuTime();
}

float Tr2GpuParticleSystem::GetRenderTime()
{
	return m_renderTimer.GpuTime();
}

bool Tr2GpuParticleSystem::HasParticles() const
{
	return m_liveTime > 0.f;
}
