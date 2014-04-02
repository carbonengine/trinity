
#pragma once

#ifndef Tr2GPUParticlePool_h
#define Tr2GPUParticlePool_h

#include "Vector3.h"
#include "Utilities/Matrix.h"
#include "Tr2DeviceResource.h"
#include "ID3DTexture.h"

#include "Tr2ShaderMaterial.h"

BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( Tr2TextureAtlas );
BLUE_DECLARE( Tr2AtlasTexture );

static bool s_gpuParticlesRender = true;
static float s_gpuParticleUpdateRate = 1.f;
static bool s_gpuParticlesEnabled = true;
static bool s_gpuParticleSpawningAllowed = true;

//Tr2ParticleBehaviour is intended to store all unique particle behaviour, in a format
// that can easily be reinterpretted as a slab of float4s and pushed to a texture.
//Has no behaviour, just a POD struct.
struct Tr2ParticleBehaviour {
	Color colour[4]; //cubic bezier colour
		
	Vector3 size; //quadratic bezier size
	float sizeVariance;

	Vector4 textureWindow;
		
	float lifespan;
	float lifespanVariance;
	float deathTransition;
	float collideTransition;

	float dragFactor; // v' = v * e^(-dragFactor * dt)
	float angularVelocity; 
	Vector2 padding;

	float turbulenceWeight;
	float gravityWeight;
	float attractorWeight;
	float vortexWeight;

	Vector4 gradientWindow;

	const static unsigned behaviourFloat4Count = 10;

	std::string texturePath;
	std::string gradientPath;

	Tr2ParticleBehaviour() : 
		size(0,0,0), sizeVariance(0),
		textureWindow(0,0,0,0),
		lifespan(0), lifespanVariance(0),
		deathTransition(-1), collideTransition(-1),
		dragFactor(0), angularVelocity(0),
		turbulenceWeight(0),
		gravityWeight(0),
		attractorWeight(0),
		vortexWeight(0)
	{
		for( int i=0; i<4; ++i )
			colour[i] = Color(0,0,0,0);
	}

	Tr2ParticleBehaviour(const float lifespan) : 
		size(0,0,0), sizeVariance(0),
		textureWindow(0,0,0,0),
		lifespan(lifespan), lifespanVariance(0),
		deathTransition(-1), collideTransition(-1),
		dragFactor(0), angularVelocity(0),
		turbulenceWeight(0),
		gravityWeight(0),
		attractorWeight(0),
		vortexWeight(0)
	{
		for( int i=0; i<4; ++i )
			colour[i] = Color(0,0,0,0);
	}
};

//A cheap and dirty queue of fixed size.
//Does not do anything upon push-full nor pop-empty.
//Intended to store pending spawn data; spawning involves
// the rendering of potentially many small quads with unique
// constant buffers, thus can get expensive. So we just clamp
// the maximum number of active spawns using this.
//This avoids any dynamic resizing or allocation, although does
// require that our spawn structs be default-constructable and 
// copy-constructable.
template< class T, unsigned size >
class FixedSizeQueue {
private:
	T m_store[size];
	int m_front;
	int m_back;
public:
	FixedSizeQueue() : m_front(0), m_back(0) {}
	void push( T &a ) {
		if( m_back >= size ) return;
		m_store[ m_back++ ] = a;
	}
	void pop() {
		m_front++;
		if( m_front >= m_back ) {
			m_front = 0;
			m_back = 0;
		}
	}
	T &front() {
		return m_store[m_front];
	}
	bool empty() const { 
		return m_front == m_back;
	}
	bool full() const {
		return m_back == (size-1);
	}
};

//GPU particles can be rendered in multiple ways.
//PreMultipliedAlpha also supports gradients to allow special colour change over
// lifespan.
enum Tr2GPUParticleRenderMode {
	GPUPRM_Additive = 0,
	GPUPRM_Transparent = 1,
	GPUPRM_PreMultipliedAlpha = 2,
	GPUPRM_Distortion = 3
};

//Particle pools are, as expected, pools of particles. Particles are stored in a pair of render targets
// (one for position and age, another for velocity and behaviour index), which we double-up so we can
// read from one whilst writing to the other, during update for example. Behaviour data such as size,
// lifespan, drag coefficient, rotation rate etc. are all stored within another texture.
//Particles are rendered using VTF to grab particle data from the textures, this can be instanced
// (initialised with multiStream = true) or using a standard large vertex and index buffer. Particles
// also have some static random data in addition to the texture coordinates, used to apply some unique
// effects to each particle; for example, we set the maximum screen space size for each particle based
// partly upon such random data, so that some particles are allowed to be larger than others.
//Textures for particles are stored in a small atlas, but we do not make good use of this and a simple 
// sprite sheet may be better.
BLUE_CLASS( Tr2GPUParticlePool ) : 
	public ID3DTexture, 
	public Tr2DeviceResource 
{
public:
    EXPOSE_TO_BLUE();
	Tr2GPUParticlePool( IRoot* lockObj = NULL );
	~Tr2GPUParticlePool();

	//resource guff
	void ReleaseResources( TriStorage s );
	bool OnPrepareResources();
	const bool IsValid() { return m_initialised; }

	//allocate textures etc.
	bool Initialise(	unsigned width, 
						unsigned height, 
						Tr2GPUParticleRenderMode renderMode, 
						Tr2PrimaryRenderContext  &renderContext, 
						bool multiStream = true );

	bool ReInitialise(	Tr2PrimaryRenderContext &renderContext );
	Tr2GPUParticleRenderMode GetRenderMode() const { return m_renderMode; }

	//tick all particles
	void Update( float deltaTime, const Vector3 &egoMotion, Tr2RenderContext &renderContext );

	//spawn particles
	void SpawnParticles( 
		const int count, 
		const unsigned behaviourIndex,
		const float inheritVelocity, const float posScale, const float velScale,
		const Vector3 &startPos, const Vector3 &endPos, const Vector3 &startVel, const Vector3 &endVel, 
		const Matrix &emitterTransform,
		TriTextureRes *spawnPosOffset = NULL, TriTextureRes *spawnVelOffset = NULL );
	
	//render all active particles
	void Render( Tr2RenderContext &renderContext );
	//ID3DTexture
	Tr2TextureAL* GetTexture();

	//particle appearance
	//this should more reasonably be dynamically generated (inc. resize to sheet size)
	bool AddTexture( const std::string &path, Vector4 &window );
	void ReleaseTexture( const std::string &path );

	bool AddGradient( const std::string &path, Vector4 &window );
	void ReleaseGradient( const std::string &path );

	bool HasBehaviour( const std::string &name ) const { return m_behaviourByName.find(name) != m_behaviourByName.end(); }
	
	//default position and velocity offset textures
	void SetDefaultSpawnTextures( TriTextureRes *position, TriTextureRes *velocity );
	void SetTurbulenceTexture( TriTextureRes *turbulence );
	void SetTurbulenceScale( float scale );
	void SetTurbulenceSpeed( float speed );

	//particle behaviour
	unsigned GetBehaviourMax() const { return m_behaviourCountMax; }
	int GetBehaviourIndex( const char *name ) const;
	bool SetBehaviour( const char *name, const Tr2ParticleBehaviour &behaviour );
	bool GetOrCreateBehaviour( const char *name, Tr2ParticleBehaviour &out );
	bool GetBehaviourByName( const char *name, Tr2ParticleBehaviour &out ) const;
	bool GetBehaviour( const unsigned index, Tr2ParticleBehaviour &out ) const;
	
	//debugging
	enum ParticleDebuggingMode {
		ParticleDebugPosition,
		ParticleDebugVelocity,
		ParticleDebugBehaviour
	};
	ParticleDebuggingMode m_debuggingMode;
	std::string m_name;
	static bool HardwareSupport() { return s_hardwareSupport; }
	
	//scaling
	float GetUsageDensityModifier() const { return m_usageDensityModifier; }
private:
	bool ReInitialise_Python();
	bool SupportLudicrousParticleCount();

	bool HasALObject( int type, size_t object );

	static bool s_hardwareSupport;
	Tr2GPUParticleRenderMode m_renderMode;
	struct SpawnData {
		const static Vector3 s_zero;
		const static Matrix s_identity;
		/*const*/ float seed, behaviour;
		/*const*/ Vector3 startPos, endPos, startVel, endVel;
		/*const*/ Matrix emitterTransform;
		/*const*/ float inheritVel, posScale, velScale;
		TriTextureRes *posTexture, *velTexture;

		SpawnData( const float seed, const float behaviour,
			const Vector3 &startPos, const Vector3 &endPos, 
			const Vector3 &startVel, const Vector3 &endVel, 
			const Matrix &emitterTransform,
			const float posScale,
			const float velScale,
			const float inheritVel = 0.f,			
			TriTextureRes *posTexture = nullptr, 
			TriTextureRes *velTexture = nullptr ) : seed(seed), behaviour(behaviour),
				startPos(startPos), endPos(endPos), startVel(startVel), endVel(endVel), 
				emitterTransform(emitterTransform), inheritVel(inheritVel),
				posScale(posScale), velScale(velScale),
				posTexture(posTexture), velTexture(velTexture) 
		{}
		SpawnData( const Vector3 &egoUpdate, const float behaviour, TriTextureRes *posTexture, TriTextureRes *velTexture ) : seed(0.f), behaviour(behaviour),
			startPos(egoUpdate), endPos(s_zero), 
			startVel(s_zero), endVel(s_zero), 
			emitterTransform(s_identity), 
			inheritVel(0.f), posScale(1.f), velScale(1.f),
			posTexture(posTexture), velTexture(velTexture)
		{}
	//private:
		SpawnData() : seed(0), behaviour(0), startPos(s_zero), endPos(s_zero), startVel(s_zero), endVel(s_zero), 
			emitterTransform(s_identity), posScale(0), velScale(0), inheritVel(0), posTexture( nullptr ), velTexture( nullptr ) {}
	};
	struct SpawnQuad {
		unsigned x, y, w, h;
		//in the shader, we want to know the relative position of
		// a particle within a block spawned on the same frame (so we can
		// distribute them evenly along a path, for example)
		float startPct, yPctScale, xPctScale;
		SpawnData spawnData;
		SpawnQuad( unsigned x, unsigned y, unsigned w, unsigned h, 
			float startPct, float yPctScale, float xPctScale,
			const SpawnData &data ) : x(x), y(y), w(w), h(h), 
			startPct(startPct), yPctScale(yPctScale), xPctScale(xPctScale),
			spawnData(data) 
		{}
		SpawnQuad() {}
	};
	//std::queue< SpawnQuad > m_pendingSpawns;
	FixedSizeQueue< SpawnQuad, 512 > m_pendingSpawns;
	void RenderSpawnQuad( const SpawnQuad &quad, Tr2RenderContext & );
#if 0
	//WIP optimisation: if many emitters use the same spawn position/velocity textures,
	// we can bung resultant spawn quads in a single vertex buffer with a fat vertex format
	struct SpawnQuadFatVertex {
		float x, y;
		Vector4 startPos_behaviour;
		Vector4 startVel_inheritance;
		Vector4 endPos_posScale;
		Vector4 endVel_velScale;
		Vector4 seeds;
		Vector4 percentages;
	};
	const static unsigned s_maxSpawnDefaultQuads = 512;
	unsigned m_spawnDefaultQuads;
	Tr2VertexBufferAL m_spawnDefaultQuadBuffer;
	Tr2IndexBufferAL m_spawnDefaultQuadIndices;
	unsigned m_spawDefaultQuadDeclIndex;
	void RenderDefaultSpawnQuads( Tr2RenderContext & );
#endif
	unsigned m_activeParticleIndex, m_particleRTWidth, m_particleRTHeight, m_particleTotal;
	bool m_liveParticles;
	Be::Time m_lastSpawnTime;
	float m_maxParticleLifespan;

	//spawn situation  - renders new particles directly into the rendertargets
	//update situation - ticks all active particles, reading from behaviour texture to do this
	//render situation - renders particle sprites
	Tr2ShaderSituation m_spawnSituation, m_updateSituation, m_renderSituation;
	Tr2ShaderMaterialPtr m_spawnParticleShader, m_updateParticleShader, m_renderParticleShader;
	Tr2ConstantBufferAL m_updateConstants, m_renderConstantsFrame, m_renderConstantsObject, m_spawnConstants, m_spawnConstantsVS;

	//behaviour texture stores such things as particle colour,
	// size, rotation rate, lifespan and so on. Start/end values for each.
	Tr2TextureAL m_behaviourTexture;
	//bung data into behaviour texture
	bool UpdateBehaviourTexture( const unsigned index, const Tr2ParticleBehaviour &behaviour );
	//current behaviour total
	unsigned m_behaviourCount;
	//max behaviours (texture height)
	const unsigned m_behaviourCountMax;
	//store in an array for easy CPU access too
	Tr2ParticleBehaviour * const m_behaviours;
	//map from named behaviours to index
	std::map< std::string, unsigned > m_behaviourByName;
	//map from behaviour index to LTU, so we can eject old behaviours
	std::map< unsigned, Be::Time > m_behaviourUsageTime;
	int FindUnusedBehaviour();

	const unsigned m_behaviourTextureWidth;
	std::set<int> m_behavioursToUpdate;
	
	//atlas management
	Tr2TextureAtlasPtr m_textureAtlas;
	std::map< std::string, TriTextureResPtr > m_loadingTextures;
	std::map< std::string, Tr2AtlasTexturePtr > m_subTexturesByPath;
	std::map< std::string, int > m_subTextureRefCounts;

	Tr2TextureAtlasPtr m_gradientAtlas;
	std::map< std::string, TriTextureResPtr > m_loadingGradients;
	std::map< std::string, Tr2AtlasTexturePtr > m_gradientsByPath;
	std::map< std::string, int > m_gradientRefCounts;

	//for spawning
	TriTextureResPtr m_positionOffsetDefault, m_velocityOffsetDefault;

	//render targets store our position and age
	Tr2RenderTargetAL m_positionAgeRT[2], m_velocityBehaviourRT[2];
	//ping-pong between render targets using this flag
	unsigned m_flipRT;

	//if multipleStreams is false, bung everything in a single huge buffer
	//otherwise, use a small geometry buffer and a large texture coordinate buffer for instances
	bool m_multipleParticleStreams;
	Tr2VertexBufferAL m_particleInstanceBuffer, m_particleQuadBuffer, m_particleSingleBuffer;
	Tr2IndexBufferAL m_particleQuadIndices, m_particleSingleIndices;

	//for update and spawn
	struct SimpleQuadVert {
		float x, y, padding;
	};
	Tr2VertexBufferAL m_simpleQuadBuffer;
	Tr2IndexBufferAL m_simpleQuadIndices;
	unsigned m_simpleQuadDeclIndex;
	unsigned m_simpleQuadVertCount, m_simpleQuadPrimCount;

	struct ParticleQuadVert {
		float x, y, z;
	};
	struct ParticleInstanceVert {
		float u, v;
		float seed0, seed1;
	};
	struct ParticleSingleVert {
		float x, y, z;
		float u, v;
		float seed0, seed1;
	};

	unsigned m_particleVertexDeclIndex;

	//space-turbulence! Use a volume texture.
	TriTextureResPtr m_turbulenceTexture;
	//tile size in space-units
	float m_turbulenceScale;
	//offset (based on ego ball)
	Vector3 m_turbulenceOffset;
	//animate the turbulence by using an additional offset
	Vector3 m_turbulenceAnim;
	//animation speed
	float m_turbulenceSpeed;
	
	//debug info
	unsigned m_debugSpawnQuadCounter;
	bool m_initialised;

	//usage-based scaling of density
	float m_usageDensityModifier;
	float m_usageDensityModifierTarget;
	Be::Time m_lastWrapTime;
	void UpdateUsageDensity();
	void UpdateUsageDensityTarget();

	CcpMutex m_spawnCS;

};

TYPEDEF_BLUECLASS( Tr2GPUParticlePool );
BLUE_DECLARE( Tr2GPUParticlePool );


#endif//Tr2GPUParticlePool_h
