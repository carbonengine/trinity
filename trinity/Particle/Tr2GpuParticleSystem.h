////////////////////////////////////////////////////////////
//
//    Created:   October 2015
//    Copyright: CCP 2015
//

#pragma once
#ifndef Tr2GpuParticleSystem_H
#define Tr2GpuParticleSystem_H

#include "IRenderCallback.h"
#include "Tr2AddSafeGrowableBuffer.h"
#include "Tr2ProfileTimer.h"

BLUE_DECLARE( Tr2GpuBuffer );
BLUE_DECLARE( Tr2GpuStructuredBuffer );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2VariableStore );
BLUE_DECLARE( Tr2RenderTarget );


// --------------------------------------------------------------------------------------
// Description:
//   GPU-managed particle system. 
// See Also:
//   Tr2GpuSharedEmitter, Tr2GpuUniqueEmitter
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GpuParticleSystem ): 
	public IInitialize, 
	public INotify, 
	public IRenderCallback,
	public Tr2DeviceResource
{
public:
	Tr2GpuParticleSystem( IRoot* lockObj = nullptr );
	~Tr2GpuParticleSystem();

	EXPOSE_TO_BLUE();

	// ----------------------------------------------------------------------------------
	// Description:
	//   CPU-side (public) emitter parameters. A mirror of these parameters is stored in
	//   GPU memory. These parameters govern particle behavior and appearence after it
	//   has been spawned.
	// See Also:
	//   Tr2GpuParticleSystem::EmitterParamsGpu
	// ----------------------------------------------------------------------------------
	struct EmitterParams
	{
		float minLifeTime;
		float maxLifeTime;

		uint32_t textureIndex;
		float colorMidpoint;
		Color colors[4];
		Vector3 sizes;
		float sizeVariance;

		float drag;
		float turbulenceAmplitude;
		uint32_t turbulenceFrequency;
		float gravity;
		Vector3 attractorPosition;
		float attractorStrength;
		float velocityStretchRotation;
	};

	// ----------------------------------------------------------------------------------
	// Description:
	//   CPU-side (public) emitter. This structure describes initial values for 
	//   particle attributes.
	// See Also:
	//   Tr2GpuParticleSystem::EmitterGpu
	// ----------------------------------------------------------------------------------
	struct Emitter
	{
		Vector3 position;
		uint32_t count;

		Vector3 positionPrevious;
		float radius;

		Vector3 direction;
		float angle;

		Vector3 directionPrevious;
		uint32_t emitterSeed;

		Vector3 velocity;
		float minSpeed;

		Vector3 velocityPrevious;
		float maxSpeed;

		float innerAngle;
		Vector3 unused;
	};

	virtual bool Initialize();

	virtual bool OnModified( Be::Var* value );

	void Update( Be::Time time, const Vector3& originShift, Tr2RenderContext& renderContext );
	void Render( Tr2RenderContext& renderContext );
	void Clear();
	void Emit( const Emitter& emitter, uintptr_t id, uintptr_t hash, const EmitterParams& params );
	bool HasParticles() const;

	virtual void SubmitGeometry( Tr2RenderContext& renderContext );
	virtual void ReleaseResources( TriStorage s );
private:
	virtual bool OnPrepareResources();

	void InitializeBuffers();
	void RegisterVariables();
	void SetVariableStore( Tr2Effect* effect );
	bool DoClear( Tr2RenderContext& renderContext );
	void RunSimulation( float dt, const Vector3& originShift, Tr2RenderContext& renderContext );
	void UpdateGpuEmitterParams( Tr2RenderContext& renderContext );

	void Sort( Tr2RenderContext& renderContext );
	bool SortIncremental( uint32_t presorted, Tr2RenderContext& renderContext );

	void SetMaxParticles( uint32_t maxParticles );
	void ExpireEmitterParams( float dt );
	void UpdateEmitterParams( Tr2RenderContext& renderContext );
	void EmitParticles( Tr2RenderContext& renderContext );
	void UpdateLiveCount( Tr2RenderContext& renderContext );

	float GetEmitTime();
	float GetUpdateTime();
	float GetSortTime();
	float GetRenderTime();

	// ----------------------------------------------------------------------------------
	// Description:
	//   Particle data as stored on GPU
	// ----------------------------------------------------------------------------------
	struct ParticleData
	{
		Vector3 particle;
		float age;
		Vector3 velocity;
		uint32_t emitterSeed;
	};

	// buffer to store per-particle data (see ParticleData)
	Tr2GpuStructuredBufferPtr m_particleData;
	// list of indicies to dead particles in m_particleData buffer (re-populated during
	// Update and Clear)
	Tr2GpuStructuredBufferPtr m_deadList;
	// list of visible (and alive!) particle indices
	Tr2GpuStructuredBufferPtr m_visibleList;
	// buffer for ...Indirect call parameters
	Tr2GpuBufferPtr m_drawParameters;
	Tr2GpuBufferPtr m_sortParameters;
	// buffer with persistent emitter data (see EmitterParams)
	Tr2GpuStructuredBufferPtr m_emitterParamsBuffer;
	Tr2GpuBufferPtr m_counters;

	// shader that is supposed to emit new particles
	Tr2EffectPtr m_emit;
	// shader that is supposed to update particle states and build dead/visible lists
	Tr2EffectPtr m_update;
	// shader that renders particles!
	Tr2EffectPtr m_render;
	// shader that clears particle system
	Tr2EffectPtr m_clear;
	// shader that sets ...Indirect call parameters for rendering
	Tr2EffectPtr m_setDrawParameters;
	// shader that sets ...Indirect call parameters for sorting
	Tr2EffectPtr m_setSortParameters;
	// shader that does pre-sorting
	Tr2EffectPtr m_sort;
	// shader that does incremental sort
	Tr2EffectPtr m_sortStep;
	// shader that does incremental sort (merge phase)
	Tr2EffectPtr m_sortInner;

	typedef Emitter EmitterGpu;
	struct EmitterParamsGpu
	{
		EmitterParamsGpu() {}
		explicit EmitterParamsGpu( const EmitterParams& params );

		float minLifeTime;
		float maxLifeTime;

		float textureIndex;
		Color colors[4];
		Vector3 sizes;
		float sizeVariance;

		float drag;
		float turbulenceAmplitude;
		float turbulenceFrequency;
		float gravity;
		Vector3 attractorPosition;
		float attractorStrength;
		float velocityStretchRotation;
	};

	// ----------------------------------------------------------------------------------
	// Description:
	//   Stored data coming from ::Emit method
	// ----------------------------------------------------------------------------------
	struct EmitRequest
	{
		EmitterGpu emitter;
		EmitterParamsGpu params;
		// params unique (or not so unique for shared emitters) id
		uintptr_t id;
		// params data hash value (used to determine if the data needs to be re-uploaded to GPU)
		uintptr_t hash;
	};

	// ----------------------------------------------------------------------------------
	// Description:
	//   Index into m_emitterParams for lifetime and dirty state management
	// ----------------------------------------------------------------------------------
	struct EmitterParamsIndex
	{
		// index into m_emitterParams
		size_t index;
		// params data hash value (used to determine if the data needs to be re-uploaded to GPU)
		uintptr_t hash;
		// time in seconds until the last particle using this data dies
		float lifetime;
	};

	// index from emitter params id to emitter params in GPU buffer
	std::map<uintptr_t, EmitterParamsIndex> m_emitterParamsIndex;
	// pool of indexes into m_emitterParams for unused spots
	std::vector<size_t> m_expiredEmitters;
	// mirror of GPU buffer of emitter params
	std::vector<EmitterParamsGpu> m_emitterParams;
	// emit requests accumulated during the frame
	Tr2AddSafeGrowableBuffer<EmitRequest> m_emitRequests;

	// constant buffer for emit dispatch (can become quite large)
	Tr2ConstantBufferAL m_emitCB;
	// constant buffer for update dispatch
	Tr2ConstantBufferAL m_updateCB;
	// constant buffer for sort dispatch
	Tr2ConstantBufferAL m_sortCB;

	// maximum number of particles in the system
	uint32_t m_maxParticles;
	// time until the last particle in the system dies
	float m_liveTime;

	// offset of turbulence origin in world space (due to world origin shifts)
	Vector3 m_turbulenceOffset;
	// turbulence animation (in local turbulence space)
	Vector3 m_turbulenceAnimation;

	// true if the system needs to be cleared during next update
	bool m_clearRequested;
	// enable/disable emitting new particle (debugging)
	bool m_enableEmit;
	// enable/disable physics simulation (debugging)
	bool m_enableUpdate;
	// enable/disable sorting on DX11 (debugging)
	bool m_enableSort;
	// enable/disable rendering (debugging)
	bool m_enableRender;

	Be::Time m_previousTime;

	// local variable store for passing parameters to effects
	Tr2VariableStorePtr m_variableStore;

	Tr2ProfileTimer m_emitTimer;
	Tr2ProfileTimer m_updateTimer;
	Tr2ProfileTimer m_sortTimer;
	Tr2ProfileTimer m_renderTimer;
	bool m_updateVisibleCount;
	uint32_t m_visibleCount;
};

TYPEDEF_BLUECLASS( Tr2GpuParticleSystem );

#endif