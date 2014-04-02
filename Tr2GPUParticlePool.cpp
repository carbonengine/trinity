#include "StdAfx.h"

#include "Tr2GPUParticlePool.h"


#include "Include/TriMath.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

#include "TriSettingsRegistrar.h"
#include "TriDevice.h"
#include "Resources/TriTextureRes.h"

template<typename LambdaSignature>
struct OnBlockExit 
{
private:
	bool callOnDestruction;
	LambdaSignature lambda;
public:
	std::string status;
	HRESULT hresult;
	bool Complete() 
	{ 
		callOnDestruction = false; 
		return true; 
	}

	void SetLastHR( HRESULT hr )
	{ 
		hresult = hr; 
	}

	OnBlockExit( LambdaSignature lambda ) : lambda(lambda), callOnDestruction(true), hresult(0) 
	{
	}

	~OnBlockExit()
	{ 
		if( callOnDestruction)
		{
			lambda();
			if( !status.empty() )
			{
				CCP_LOGERR( status.c_str() );
			}
			if( FAILED(hresult) ) 
			{
				CCP_LOGERR( "Last HRESULT: %08x", hresult );
			}
		} 
	}
};

template<typename L>
OnBlockExit<L> createScopeGuard( L lambda )
{
	return OnBlockExit<L>( lambda );
}


const static Tr2ParticleBehaviour s_defaultBehaviour(0.01f);
bool Tr2GPUParticlePool::s_hardwareSupport = true;

extern bool s_gpuParticleSpawningAllowed;
TRI_REGISTER_SETTING( "gpuParticleSpawn", s_gpuParticleSpawningAllowed );


const Vector3 Tr2GPUParticlePool::SpawnData::s_zero(0,0,0);
const Matrix Tr2GPUParticlePool::SpawnData::s_identity(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);

//=======================
//Tr2GPUParticlePool
Tr2GPUParticlePool::Tr2GPUParticlePool( IRoot* lockObj ) : m_activeParticleIndex(0),
	m_particleRTWidth(64), 
	m_particleRTHeight(64), 
	m_particleTotal(m_particleRTWidth * m_particleRTHeight),
	m_behaviourCountMax(256),
	m_behaviourCount(1),
	m_behaviours( CCP_NEW( "Tr2GPUParticlePool/Behaviours" ) Tr2ParticleBehaviour[m_behaviourCountMax] ),
	m_flipRT(0),
	m_debuggingMode(ParticleDebugPosition),
	m_behaviourTextureWidth( Tr2ParticleBehaviour::behaviourFloat4Count ),
	m_turbulenceOffset(0,0,0),
	m_turbulenceScale(4096.f),
	m_turbulenceAnim(0,0,0),
	m_turbulenceSpeed(0.05f),
	m_debugSpawnQuadCounter(0),
	m_initialised(false),
	m_renderMode( GPUPRM_Additive ),
	m_maxParticleLifespan(10.f),
	m_usageDensityModifier(1.f),
	m_usageDensityModifierTarget(1.f),
	m_lastWrapTime(0),
	m_spawnCS( "Tr2GpuParticlePool", "m_spawnCS" )
{
	CCP_LOG( "Constructed Tr2GPUParticlePool" );
}

Tr2GPUParticlePool::~Tr2GPUParticlePool()
{
	ReleaseResources( 0x3 );

	CCP_DELETE[] m_behaviours;
}


#if defined(_DEBUG) || defined(TRINITYDEV)
	#define CR_PCL_RETURN_VAL( x ) \
	{ \
		HRESULT _hr = x; \
		if( FAILED(_hr) ) \
		{ \
			ReportHresultError( __FILE__, __LINE__, #x, _hr ); \
			BreakInDebugger(); \
			guard.SetLastHR(_hr); \
			return false; \
		} \
	}
#else
	#define CR_PCL_RETURN_VAL( x ) { \
		HRESULT hr_ = x; \
		if( FAILED( hr_ ) ) { \
			guard.SetLastHR(hr_); \
			return false; \
		} \
	}
#endif

// -------------------------------------------------------------
// Description:
//   Initialise the pool, creating all required GPU resources.
// Arguments:
//   Width and height of the rendertargets, which naturally also controls
//    the maximum number of live particles. Rendering mode (additive, transparent etc.).
//   Multi-stream indicates that the particles should used instanced rendering, which may
//    have different performance characterstics to large flat buffers.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::Initialise( unsigned int width, unsigned int height, Tr2GPUParticleRenderMode renderMode, Tr2PrimaryRenderContext &renderContext, bool multiStream )
{
	m_multipleParticleStreams = multiStream;
	m_renderMode = renderMode;
	m_liveParticles = false;

	if( !Tr2Renderer::IsResourceCreationAllowed() || !HardwareSupport() )
	{
		return false;
	}

	CCP_LOG( "Initialising Tr2GPUParticlePool" );
	auto guard = createScopeGuard( [=]()
	{ 
		ReleaseResources( TRISTORAGE_ALL );
		CCP_LOGERR("Tr2GPUParticlePool::Initialise exited early! Something exploded!");
	} );

	if( m_positionAgeRT[0].IsValid() ) 
	{
		ReleaseResources( 0x3 );
	}
	
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
	CCP_LOG( "Tr2GPUParticlePool::Initialise - checking for vertex texture format support" );
	Tr2DisplayModeInfo mode;
		
	CR_PCL_RETURN_VAL( Tr2VideoAdapterInfo::GetAdapterDisplayMode( gTriDev->GetAdapter(), mode ) );
	
	HRESULT result = Tr2VideoAdapterInfo::SupportsVertexTextureFormat( 
		Tr2VideoAdapterInfo::DEFAULT_ADAPTER,
		mode.format,
		Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT );
	if( FAILED( result ) ) 
	{
		CCP_LOGERR( "Sorry, this device does not appear to support 128-bit wide floating point texture access in vertex shaders, GPU particles are not available" );
		s_hardwareSupport = false;
		return false;
	}
#endif //TRINITY_DIRECTX9

	//make our render targets a multiple of 64 pixels, for neatness
	width  = std::max( 64u, std::min(width, 1024u) );
	height = std::max( 64u, std::min(height,1024u) );
	m_particleRTWidth = (width+63) & ~63;
	m_particleRTHeight = (height+63) & ~63;
	m_particleTotal = m_particleRTWidth * m_particleRTHeight;

	//update time and reset spawn index (location to write
	// new particles to within the render targets)
	m_lastWrapTime = BeOS->GetCurrentFrameTime();
	m_activeParticleIndex = 0;

	//create render targets
	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating render targets (%d x %d)", m_particleRTWidth, m_particleRTHeight );

	const auto rtFormat = Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT;
	for( int i=0; i<2; ++i ) 
	{
		CR_PCL_RETURN_VAL( 
				m_positionAgeRT[i].Create(	m_particleRTWidth, 
											m_particleRTHeight, 
											1, 
											rtFormat, 
											renderContext ) );
		CR_PCL_RETURN_VAL( m_velocityBehaviourRT[i].Create( m_particleRTWidth, m_particleRTHeight, 1, rtFormat, renderContext ) );
	}
	
	//would rather use a pre-baked atlas for this with fixed contents
	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating texture atlas" );
	if( m_textureAtlas.CreateInstance() ) 
	{
		m_textureAtlas->Initialize( 
			Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM,
			512, 512, 
			Tr2RenderContextEnum::USAGE_CPU_READ,
			true );
		m_textureAtlas->SetMargin(0);
		m_textureAtlas->SetCreateOutsiders( false );
		
		m_subTexturesByPath.clear();
		m_subTextureRefCounts.clear();
	} 
	else 
	{
		return false;
	}
	
	//gradients are only used by a single render mode, because
	// they add considerable complexity to the shaders and aren't
	// actually used yet. May burn with fire.
	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating gradient atlas" );
	if( m_renderMode == GPUPRM_PreMultipliedAlpha )
	{
		if( m_gradientAtlas.CreateInstance() ) 
		{
			m_gradientAtlas->Initialize(
				Tr2RenderContextEnum::PIXEL_FORMAT_B8G8R8A8_UNORM,
				256, 256, 
				0,
				false );
			m_gradientAtlas->SetMargin(1);
			m_gradientAtlas->SetCreateOutsiders(false);

			m_gradientsByPath.clear();
			m_gradientRefCounts.clear();
		} 
		else 
		{
			return false;
		}
	} 
	else 
	{
		m_gradientAtlas = nullptr;
	}

	//the behaviour texture stores particle data (colour, size, lifespan,
	// rotation rate, turbulence/drag factors etc.) in a GPU-friendly format
	//one row of texture data completely defines a particle.
	//it is read in both the vertex and pixel shaders, when spawning, rendering and updating.
	//could be split into multiple textures for different purposes
	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating behaviour texture" );
	CR_PCL_RETURN_VAL( 
		m_behaviourTexture.Create2D( m_behaviourTextureWidth, m_behaviourCountMax, 1, 
			Tr2RenderContextEnum::PIXEL_FORMAT_R32G32B32A32_FLOAT, 
			0, nullptr, renderContext ) );

	//make sure the first row in the texture is an invisible default with short lifespan,
	// so we can easily clear live particles by setting the behaviour of a dead particle
	// to zero.
	m_behaviourByName["default"] = 0;
	m_behaviours[0] = s_defaultBehaviour;

	//Ugh, as AddTexture etc early-out if not initialised (generally with good reason), gently
	// subvert our state.
	m_initialised = true;
	for( unsigned i=0; i<m_behaviourCount; ++i ) 
	{
		AddGradient( m_behaviours[i].gradientPath, m_behaviours[i].gradientWindow );
		AddTexture( m_behaviours[i].texturePath, m_behaviours[i].textureWindow );
		m_behavioursToUpdate.insert(i);
		m_maxParticleLifespan = std::max( m_maxParticleLifespan, m_behaviours[i].lifespan * (1.f + m_behaviours[i].lifespanVariance) );
	}
	//And restore our uninitialised state.
	m_initialised = false;

	//Make sure any old pending spawns are removed.
	{
		CcpAutoMutex lock( m_spawnCS );
		while( !m_pendingSpawns.empty() )
			m_pendingSpawns.pop();
	}
	
	//declaration used for rendering the particles
	Tr2VertexDefinition particleElements;
	particleElements.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION, 0, 0, 0 );
	particleElements.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 0, m_multipleParticleStreams ? 1 : 0, 0 );

	//declaration used for update/spawn
	Tr2VertexDefinition simpleQuadElements;
	simpleQuadElements.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION, 0, 0, 0 );

	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating vertex declarations" );
	m_simpleQuadDeclIndex = Tr2EffectStateManager::GetVertexDeclarationHandle( simpleQuadElements );
	m_particleVertexDeclIndex = Tr2EffectStateManager::GetVertexDeclarationHandle( particleElements );

	
	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating buffers (%s)", m_multipleParticleStreams ? "multi-stream" : "single-stream" );

	//create simple quad - used for spawn and update
	//could possibly subdivide this for minor increase to texture cache coherency on update, but...
	m_simpleQuadVertCount = 4;
	m_simpleQuadPrimCount = 2;
	const SimpleQuadVert simpleQuadVerts[] = 
	{
		{0.f, 0.f, 0 },
		{1.f, 0.f, 0 },
		{0.f, 1.f, 0 },
		{1.f, 1.f, 0 }
	};
	CR_PCL_RETURN_VAL( m_simpleQuadBuffer.Create( sizeof(SimpleQuadVert) * 4, Tr2RenderContextEnum::USAGE_IMMUTABLE, simpleQuadVerts, renderContext ) );

	const static unsigned quadIndexData[] = { 0, 1, 2, 1, 3, 2 };
	CR_PCL_RETURN_VAL( m_simpleQuadIndices.Create( 6, Tr2RenderContextEnum::USAGE_CPU_WRITE, Tr2RenderContextEnum::IB_32BIT, quadIndexData, renderContext ) );

	//instanced rendering seems to be slightly slower than regular fat VB rendering, unfortunately.
	if( m_multipleParticleStreams ) 
	{		
		const ParticleQuadVert pclQuadVerts[] = 
		{
			{0.f, 0.f, 0.f},
			{1.f, 0.f, 1.f},
			{0.f, 1.f, 2.f},
			{1.f, 1.f, 3.f}
		};
		CCP_LOG( "Tr2GPUParticlePool::Initialise - creating vertex instanced quad buffer" );
		CR_PCL_RETURN_VAL( m_particleQuadBuffer.Create( 4 * sizeof(ParticleQuadVert), 
															Tr2RenderContextEnum::USAGE_IMMUTABLE, 
															pclQuadVerts,
															renderContext ) );
	
		const static unsigned quadIndexData[] = { 0, 1, 2, 1, 3, 2 };
		CCP_LOG( "Tr2GPUParticlePool::Initialise - creating index buffer" );
		CR_PCL_RETURN_VAL( m_particleQuadIndices.Create( 
			6, 
			Tr2RenderContextEnum::USAGE_IMMUTABLE, 
			Tr2RenderContextEnum::IB_32BIT,
			quadIndexData,
			renderContext ) );
	
		std::vector<ParticleInstanceVert> instances( m_particleRTHeight * m_particleRTWidth );
		auto pclInstance = instances.begin();
		for( unsigned y=0; y<m_particleRTHeight; ++y ) 
		{
			for( unsigned x=0; x<m_particleRTWidth; ++x ) 
			{
				const float fx = x / float(m_particleRTWidth);
				const float fy = y / float(m_particleRTHeight);
				const ParticleInstanceVert v = { fx, fy, TriFloatRandom01(), TriFloatRandom01() };
				*pclInstance++ = v;
			}
		}
		CCP_LOG( "Tr2GPUParticlePool::Initialise - creating vertex instance data buffer" );
		CR_PCL_RETURN_VAL( m_particleInstanceBuffer.Create( m_particleTotal * sizeof(ParticleInstanceVert), 
															Tr2RenderContextEnum::USAGE_IMMUTABLE,
															&instances[0],
															renderContext ) );
	} 
	else //single stream
	{
		CCP_LOG( "Tr2GPUParticlePool::Initialise - creating single stream index buffer" );
		std::vector<ParticleSingleVert> instances( m_particleTotal * 4 );
		auto pclInstance = instances.begin();
		for( unsigned y=0; y<m_particleRTHeight; ++y ) 
		{
			for( unsigned x=0; x<m_particleRTWidth; ++x ) 
			{
				const float fx = x / float(m_particleRTWidth);
				const float fy = y / float(m_particleRTHeight);
				const float seed0 = TriFloatRandom01();
				const float seed1 = TriFloatRandom01();
				const ParticleSingleVert v[] = 
				{ 
					{ 0, 0, 0, fx, fy, seed0, seed1 },
					{ 1, 0, 1, fx, fy, seed0, seed1 },
					{ 0, 1, 2, fx, fy, seed0, seed1 },
					{ 1, 1, 3, fx, fy, seed0, seed1 }
				};
				for( int i=0; i<4; ++i )
				{
					*pclInstance++ = v[i];
				}
			}
		}
		CR_PCL_RETURN_VAL( m_particleSingleBuffer.Create( m_particleTotal * sizeof(ParticleSingleVert) * 4, 
															Tr2RenderContextEnum::USAGE_IMMUTABLE, 
															&instances[0],
															renderContext ) );

		
		CCP_LOG( "Tr2GPUParticlePool::Initialise - creating single stream index buffer" );
		std::vector<unsigned> indices( m_particleTotal * 6 );
		auto quadIndex = indices.begin();
		const static unsigned quadIndexData[] = { 0, 1, 2, 1, 3, 2 };
		for( unsigned i = 0; i<m_particleTotal; ++i ) 
		{
			for( unsigned j=0; j<6; ++j ) 
			{
				*quadIndex++ = i*4 + quadIndexData[j];
			}
		}
		CR_PCL_RETURN_VAL( m_particleSingleIndices.Create( m_particleTotal * 6, 
														   Tr2RenderContextEnum::USAGE_IMMUTABLE, 
														   Tr2RenderContextEnum::IB_32BIT, 
														   &indices[0],
														   renderContext ) );
	}

	CCP_LOG( "Tr2GPUParticlePool::Initialise - summoning shaders" );
	m_spawnSituation.AddSituationString("Spawn");
	m_updateSituation.AddSituationString("Update");
	m_renderSituation.AddSituationString("Render");
	switch( renderMode ) 
	{
	case GPUPRM_Additive: 
		m_renderSituation.AddSituationString("Additive");
		break;
	case GPUPRM_Transparent: 
		m_renderSituation.AddSituationString("Transparent");
		break;
	case GPUPRM_PreMultipliedAlpha: 
		m_renderSituation.AddSituationString("PreMulAlpha");
		break;
	case GPUPRM_Distortion: 
		m_renderSituation.AddSituationString("Distortion");
		break;
	default:
		CCP_LOGERR( "Tr2GPUParticlePool::Initialise - GPU particle render mode insanity: %d", int(renderMode) );
		return false;
	};
	
	m_spawnParticleShader.CreateInstance();
	m_spawnParticleShader->SetHighLevelShaderName( "StatefulParticles" );
	m_spawnParticleShader->BindLowLevelShaderMaterialOnly( m_spawnSituation.GetSituation() );

	m_updateParticleShader.CreateInstance();
	m_updateParticleShader->SetHighLevelShaderName( "StatefulParticles" );
	m_updateParticleShader->BindLowLevelShaderMaterialOnly( m_updateSituation.GetSituation() );

	m_renderParticleShader.CreateInstance();
	m_renderParticleShader->SetHighLevelShaderName( "StatefulParticles" );
	m_renderParticleShader->BindLowLevelShaderMaterialOnly( m_renderSituation.GetSituation() );


	CCP_LOG( "Tr2GPUParticlePool::Initialise - creating default textures" );
	TriTextureResPtr turbulenceTex , noiseTex;
	BeResMan->GetResource( "res:/Texture/Global/GPUParticles/noise_sphere.dds", "", BlueInterfaceIID<TriTextureRes>(), (void**)&noiseTex );
	BeResMan->GetResource( "res:/Texture/Global/GPUParticles/perlin_vol.dds",   "", BlueInterfaceIID<TriTextureRes>(), (void**)&turbulenceTex );
	SetTurbulenceTexture( turbulenceTex );
	SetDefaultSpawnTextures( noiseTex, noiseTex );
	
	
	CCP_LOGNOTICE( "Tr2GPUParticlePool::Initialise - done" );
	m_initialised = true;
	return guard.Complete();
}

#undef CR_PCL_RETURN_VAL


// -------------------------------------------------------------
// Description:
//   Initialises the pool using the last used arguments to Intialise (e.g. after device reset)
// -------------------------------------------------------------
bool Tr2GPUParticlePool::ReInitialise( Tr2PrimaryRenderContext  &renderContext ) 
{
	return Initialise(	m_particleRTWidth, 
						m_particleRTHeight, 
						m_renderMode, 
						renderContext, 
						m_multipleParticleStreams );
}

// -------------------------------------------------------------
// Description:
//   Calls ReInitialise using the generic render renderContext, for python friendliness
// -------------------------------------------------------------
bool Tr2GPUParticlePool::ReInitialise_Python() 
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return ReInitialise( renderContext );
}

// -------------------------------------------------------------
// Description:
//   Eats a lot of memory, gives a lot of particles. For marketing purposes only!
// -------------------------------------------------------------
bool Tr2GPUParticlePool::SupportLudicrousParticleCount() 
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return Initialise( 1024, 1024, m_renderMode, renderContext, m_multipleParticleStreams );
}

// -------------------------------------------------------------
// Description:
//   Updates the current particles.
// Arguments:
//   Delta-time since last update, and relative motion of the 'egoball' (scene origin).
//   As EVE space scenes are all defined relative to the player ship, we need this translation
//     to keep GPU particle behaviour appear to be in some global reference frame.
// -------------------------------------------------------------
void Tr2GPUParticlePool::Update( float deltaTime, const Vector3 &egoMotion, Tr2RenderContext& renderContext )
{
	using namespace Tr2RenderContextEnum;

	if( !m_initialised ) 
	{
		return;
	}
	
	//update the behaviour texture if needed
	for( auto it = m_behavioursToUpdate.begin(); it != m_behavioursToUpdate.end(); ++it )
	{
		const int index = *it;
		Tr2ParticleBehaviour &behaviour = m_behaviours[index];
		m_behaviourTexture.UpdateSubresource( 0, index, m_behaviourTextureWidth, index+1, (void*)&behaviour, 
			Tr2ParticleBehaviour::behaviourFloat4Count * sizeof(float) * 4, renderContext );
	}
	m_behavioursToUpdate.clear();
	
	//early out if we've not got any particles...
	if( !m_liveParticles )
	{
		return;
	}
	//... or they're all dead
	const Be::Time currentTime = BeOS->GetCurrentFrameTime();
	if( TimeAsDouble( currentTime - m_lastSpawnTime ) > m_maxParticleLifespan )  
	{
		m_liveParticles = false;
		return;
	}

	CCP_STATS_ZONE( __FUNCTION__ );
	if( !m_updateParticleShader || !m_updateParticleShader->GetShaderStateInterface() )
	{
		CCP_LOGERR( "Tr2GPUParticlePool::Update error. Update shader: %p | %p", 
			m_updateParticleShader.p, 
			m_updateParticleShader ? m_updateParticleShader->GetShaderStateInterface() : nullptr );
		return;
	}
	
	auto guard = createScopeGuard( [=]()
	{ 
		CCP_LOGERR("Tr2GPUParticlePool::Update() early-out"); 
	} );
	
	Tr2PushPopDS pushPop( nullDS );
	Tr2Renderer::PushViewport();

	//now do the actual update
	//swap which is our front and back buffer
	m_flipRT ^= 1;
	
	//need to do this, or the RT width/height static data will
	// be left in a bad state.
	Tr2PushPopRT pushPopRT0( m_positionAgeRT[m_flipRT], renderContext, 0 );
	Tr2PushPopRT pushPopRT1( m_velocityBehaviourRT[m_flipRT], renderContext, 1 );
	
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 0, m_positionAgeRT[m_flipRT^1].GetTexture() );
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 1, m_velocityBehaviourRT[m_flipRT^1].GetTexture() );
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 2, m_behaviourTexture );
	if( m_turbulenceTexture && m_turbulenceTexture->GetTexture() )
	{
		renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 3, *m_turbulenceTexture->GetTexture() );
	}
	
	struct UpdatePclData 
	{
		Vector3 egoChange;
		float deltaTime;

		Vector3 spaceWind;
		float pad1;

		Vector3 turbulenceOffset;
		float turbulenceStrength;

		Vector3 turbulenceAnim;
		float pad2;

		float halfTexelX, halfTexelY;
		float pad3, pad4;
	};

	//update turbulence
	m_turbulenceOffset -= egoMotion;
	m_turbulenceOffset /= m_turbulenceScale;
	m_turbulenceOffset = Vector3( 
		m_turbulenceOffset.x - floor(m_turbulenceOffset.x),
		m_turbulenceOffset.y - floor(m_turbulenceOffset.y),
		m_turbulenceOffset.z - floor(m_turbulenceOffset.z) );
	m_turbulenceOffset *= m_turbulenceScale;

	m_turbulenceAnim.z += deltaTime * m_turbulenceSpeed;
	m_turbulenceAnim.z = m_turbulenceAnim.z - int(m_turbulenceAnim.z);

	UpdatePclData updatePclData = 
	{ 
		egoMotion, 
		deltaTime,
		
		Vector3(0,0,0), //space wind!
		0.f,

		m_turbulenceOffset,
		m_turbulenceScale,

		m_turbulenceAnim,
		0.f,
#if (TRINITY_PLATFORM == TRINITY_DIRECTX9)
		0.5f / float(m_particleRTWidth),
		0.5f / float(m_particleRTHeight),
#else
		0.f, 0.f,
#endif
		0.f, 0.f
	};
	
	FillAndSetConstants( m_updateConstants, updatePclData, PIXEL_SHADER, Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	
	renderContext.m_esm.ApplyVertexDeclaration( m_simpleQuadDeclIndex );
	const unsigned quadStride = sizeof(SimpleQuadVert);
	renderContext.m_esm.ApplyStreamSource( 0, m_simpleQuadBuffer, 0, quadStride );
	renderContext.m_esm.ApplyIndexBuffer( m_simpleQuadIndices );
	
	const auto passCount = m_updateParticleShader->GetShaderStateInterface()->GetPassCount();
	for( auto i = 0u; i < passCount; ++i )
	{
		m_updateParticleShader->GetShaderStateInterface()->ApplyAllStateForPass( i, renderContext );
		CR_RETURN( renderContext.SetTopology( TOP_TRIANGLES ) );
		CR_RETURN( renderContext.DrawIndexedPrimitive( m_simpleQuadVertCount, 0, m_simpleQuadPrimCount, 0 ) );
	}

	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 0, nullTX );
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 1, nullTX );
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 2, nullTX );
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 3, nullTX );

	//spawn all pending particles
	{
		CcpAutoMutex lock( m_spawnCS );
		while( !m_pendingSpawns.empty() )
		{
			const SpawnQuad &spawn = m_pendingSpawns.front();
			RenderSpawnQuad( spawn, renderContext );
			m_pendingSpawns.pop();
		}
		UpdateUsageDensity();
	}

	
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 0, nullTX );
	renderContext.m_esm.ApplyTexture( PIXEL_SHADER, 1, nullTX );
	Tr2Renderer::PopViewport();
	guard.Complete();
}


// -------------------------------------------------------------
// Description:
//   Writes a new chunk of particles to the render targets (spawning them, effectively).
//   Private, only called from Update. All data required to initialise the new particles,
//    as well as location to write them within the RTs, is in a simple POD struct.
// -------------------------------------------------------------
void Tr2GPUParticlePool::RenderSpawnQuad( const SpawnQuad &quad, Tr2RenderContext &renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	
	if( quad.w == 0 || quad.h == 0 ) 
	{		
		return;
	}
	
	auto guard = createScopeGuard( [=](){ CCP_LOGERR("Tr2GPUParticlePool::RenderSpawnQuad() early-out"); } );

	if( !m_spawnParticleShader || !m_spawnParticleShader->GetShaderStateInterface() )
	{
		return;
	}
		
	float uvScalePosX = 1.f, uvScalePosY = 1.f;
	float uvScaleVelX = 1.f, uvScaleVelY = 1.f;
	if( quad.spawnData.posTexture && quad.spawnData.posTexture->GetTexture() ) 
	{
		renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, *quad.spawnData.posTexture->GetTexture() );
		uvScalePosX = m_particleRTWidth / float(quad.spawnData.posTexture->GetWidth());
		uvScalePosY = m_particleRTHeight / float(quad.spawnData.posTexture->GetHeight());
	} 
	else if( m_positionOffsetDefault && m_positionOffsetDefault->GetTexture() ) 
	{
		renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, *m_positionOffsetDefault->GetTexture() );
		uvScalePosX = m_particleRTWidth / float(m_positionOffsetDefault->GetWidth());
		uvScalePosY = m_particleRTHeight / float(m_positionOffsetDefault->GetHeight());
	}

	if( quad.spawnData.velTexture && quad.spawnData.velTexture->GetTexture() ) 
	{
		renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 1, *quad.spawnData.velTexture->GetTexture() );
		uvScaleVelX = m_particleRTWidth / float(quad.spawnData.velTexture->GetWidth());
		uvScaleVelY = m_particleRTHeight / float(quad.spawnData.velTexture->GetHeight());
	} 
	else if( m_velocityOffsetDefault && m_velocityOffsetDefault->GetTexture() ) 
	{
		renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 1, *m_velocityOffsetDefault->GetTexture() );
		uvScaleVelX = m_particleRTWidth / float(m_velocityOffsetDefault->GetWidth());
		uvScaleVelY = m_particleRTHeight / float(m_velocityOffsetDefault->GetHeight());
	}

	struct SpawnPSData 
	{
		Matrix emitterTransform;
		Vector4 startPosition, startVelocity, endPosition, endVelocity;
		Vector4 positionTextureWindow, velocityTextureWindow;
		Vector4 seeds;
		Vector4 percentages;
	};

	struct SpawnVSData 
	{
		Vector4 window;
	};

	const auto rwidth = 1.f / float(m_particleRTWidth), rheight = 1.f / float(m_particleRTHeight);
	const auto fx = quad.x * rwidth, fy = quad.y * rheight, fw = quad.w * rwidth, fh = quad.h * rheight;
	const auto seed0 = quad.spawnData.seed;
	const auto seed1 = TriFloatRandom01();

	SpawnPSData spawnPSData = 
	{ 
		quad.spawnData.emitterTransform,
		Vector4( quad.spawnData.startPos, quad.spawnData.behaviour ), 
		Vector4( quad.spawnData.startVel, quad.spawnData.inheritVel ),
		Vector4( quad.spawnData.endPos, quad.spawnData.posScale ), 
		Vector4( quad.spawnData.endVel, quad.spawnData.velScale ),
		Vector4( fx * uvScalePosX, fy * uvScalePosY, fw * uvScalePosX, fh * uvScalePosY ),
		Vector4( fx * uvScaleVelX, fy * uvScaleVelY, fw * uvScaleVelX, fh * uvScaleVelY ),
		Vector4( seed0, seed1, 0, 0 ),
		Vector4( quad.startPct, quad.yPctScale, quad.xPctScale, 0 ),
	};
	SpawnVSData spawnVSData = 
	{
		Vector4( fx, fy, fw, fh )
	};
	
	FillAndSetConstants( m_spawnConstantsVS, spawnVSData, Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );
	FillAndSetConstants( m_spawnConstants, spawnPSData, Tr2RenderContextEnum::PIXEL_SHADER, Tr2Renderer::GetPerObjectPSStartRegister(), renderContext );

	renderContext.m_esm.ApplyIndexBuffer( m_simpleQuadIndices );
	renderContext.m_esm.ApplyStreamSource( 0, m_simpleQuadBuffer, 0, sizeof(SimpleQuadVert) );
	renderContext.m_esm.ApplyVertexDeclaration( m_simpleQuadDeclIndex );
	
	const auto passCount = m_spawnParticleShader->GetShaderStateInterface()->GetPassCount();
	for( auto i = 0u; i < passCount; ++i )
	{
		m_spawnParticleShader->GetShaderStateInterface()->ApplyAllStateForPass( i, renderContext );
		CR_RETURN( renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		CR_RETURN( renderContext.DrawIndexedPrimitive( m_simpleQuadVertCount, 0, m_simpleQuadPrimCount, 0 ) );
	}
	
	guard.Complete();
	++m_debugSpawnQuadCounter;
}


// -------------------------------------------------------------
// Description:
//   Spawns particles. This defers the actual spawning until the next Update.
// Arguments:
//   inCount - Number of particles requested, this may be clamped if too large.
//   behaviourIndex - Behaviour of these new particles (see GetBehaviourIndex)
//   inheritVelocity - percentage of the emitter's velocity these particles should inherit
//   posScale - spawn position offset from emitter is determined by a texture, and scaled by this value
//   velScale - spawn velocity (before inheritance) is determined by a texture, and scaled by this value
//   startPos, endPos - position of the emitter at the start and end of this frame or update period
//   startVel, endVel - velocity of the emitter at the start and end of this update period, for higher-order interpolation
//   emitterTransform - non-translation transformation matrix for this emitter
//   spawnPosOffset - texture definining particle spawn position around the emitter
//   spawnVelOffset - texture definining particle spawn velocity around the emitter
// -------------------------------------------------------------
void Tr2GPUParticlePool::SpawnParticles( 
	const int inCount, 
	const unsigned behaviourIndex,
	const float inheritVelocity, const float posScale, const float velScale,
	const Vector3 &startPos, 
	const Vector3 &endPos, 
	const Vector3 &startVel, 
	const Vector3 &endVel, 
	const Matrix &emitterTransform,
	TriTextureRes *spawnPosOffset, 
	TriTextureRes *spawnVelOffset )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	if( !m_initialised || !s_gpuParticleSpawningAllowed ) 
	{
		return;
	}
	
	if( inCount <= 0 || !m_positionAgeRT[0].IsValid() )
	{
		return;
	}

	m_behaviourUsageTime[behaviourIndex] =  BeOS->GetActualTime();
	m_liveParticles = true;
	m_lastSpawnTime = BeOS->GetCurrentFrameTime();

	if( m_pendingSpawns.full() )
	{
		return;
	}

	CCP_ASSERT( m_spawnParticleShader );
	const auto count = std::min( (unsigned)inCount, m_particleTotal >> 2 );
	//work out the extents we're writing to, we basically consume the render target
	// one line at a time and wrap back to the start when we hit the bottom.
	//as a result, we may need to render *up to* four quads:
	// one to finish the current partially-filled line
	// one to fill complete lines up to the bottom of the RT
	// one to fill complete lines at the top of the RT
	// one to partially fill the final line.
	//more commonly, we use one or two quads to end one line and start a new one.
	const auto startY = (m_activeParticleIndex / m_particleRTWidth) % m_particleRTHeight;
	const auto startX = m_activeParticleIndex % m_particleRTWidth;

	const auto newActiveParticleIndex = m_activeParticleIndex + count;
	const auto endY = (newActiveParticleIndex / m_particleRTWidth) % m_particleRTHeight;
	const auto endX = newActiveParticleIndex % m_particleRTWidth;

	const int fullStrips = endY < startY ? 
		(m_particleRTHeight - startY) + endY :
	    endY - startY - 1;

	const auto behaviour = behaviourIndex / float(m_behaviourCountMax-1);
	const SpawnData spawn( TriFloatRandom01(), behaviour, 
		startPos, endPos, 
		startVel, endVel, 
		emitterTransform, 
		posScale, velScale, inheritVelocity,
		spawnPosOffset,
		spawnVelOffset );

	const float rcpCount = 1.f / float(count);
	const float rowCountPct = m_particleRTWidth * rcpCount;
	
	{
		CcpAutoMutex lock( m_spawnCS );
		//note we don't care if the queue became full because
		// the queue class is obliged to not produce errors on pushing
		// even if full - we just early out above to save some time.
		//if the queue is replaced with a different fixed-size structure,
		// may need to check again here as many threads may be pushing
		// to it (via CPU particles having emitOverLifespan, mostly).
		if( endY == startY )
		{
			//draw a single-pixel strip from startX to endX
			SpawnQuad sq( startX, startY, endX - startX, 1, 0.f, 1.f, 1.f, spawn );
			m_pendingSpawns.push( sq );
		} 
		else if( endY < startY )
		{ 
			//wrap around from bottom of texture to top:
			// start strip, zero or more full strips at both top and bottom of texture, 
			// and one end strip at the top
			const auto bottomStrips = m_particleRTHeight - startY;
			const auto topStrips = fullStrips - bottomStrips;
			const auto beginWidth = m_particleRTWidth - startX;
			SpawnQuad sq1( startX, startY, beginWidth, 1, 0.f, 0.f, beginWidth * rcpCount,  spawn );
			m_pendingSpawns.push( sq1 );
			SpawnQuad sq2( 0, startY+1,	m_particleRTWidth, bottomStrips, beginWidth * rcpCount, bottomStrips * m_particleRTWidth * rcpCount, rowCountPct, spawn );
			m_pendingSpawns.push( sq2 );
			SpawnQuad sq3( 0, 0, m_particleRTWidth,	topStrips, (beginWidth + bottomStrips * m_particleRTWidth) * rcpCount, topStrips * m_particleRTWidth * rcpCount, rowCountPct, spawn );
			m_pendingSpawns.push( sq3 );
			SpawnQuad sq4( 0, topStrips, endX, 1, 1.f - endX * rcpCount, 0.f, endX * rcpCount, spawn );
			m_pendingSpawns.push( sq4 );

			//update our global density modifier, now we've wrapped around
			UpdateUsageDensityTarget();
		} 
		else 
		{ 
			//draw a start strip, zero or more full strips, and one end strip
			SpawnQuad sq1( startX, startY, m_particleRTWidth - startX, 1, 0.f, 0.f, (m_particleRTWidth - startX) * rcpCount, spawn );
			m_pendingSpawns.push( sq1 );
			if( fullStrips > 0 )
			{
				SpawnQuad sq( 0, startY+1, m_particleRTWidth, fullStrips, (m_particleRTWidth - startX) * rcpCount, m_particleRTWidth * fullStrips * rcpCount, m_particleRTWidth * rcpCount, spawn );
				m_pendingSpawns.push( sq );
			}
			SpawnQuad sq2( 0, startY+1+fullStrips, endX, 1, (m_particleRTWidth - startX + m_particleRTWidth * fullStrips) * rcpCount, 0.f, endX * rcpCount, spawn );
			m_pendingSpawns.push( sq2 );
		}
	}
	
	
	//update our position in the texture
	m_activeParticleIndex += count;
	m_activeParticleIndex %= m_particleTotal;
}


// -------------------------------------------------------------
// Description:
//   Render the particles! Assumes blending states etc. are set up
//    as appropriate for the render mode.
// -------------------------------------------------------------
void Tr2GPUParticlePool::Render( Tr2RenderContext &renderContext )
{
	if( !m_initialised || !m_liveParticles ) 
	{
		return;
	}
	CCP_STATS_ZONE( __FUNCTION__ );
	
	if( !m_renderParticleShader || !m_renderParticleShader->GetShaderStateInterface() ) 
	{
		CCP_LOGERR("Tr2GPUParticlePool::Render() early-out due to conditions not being met, did Initialise fail?"); 
		return;
	}

	auto guard = createScopeGuard( [=]()
	{ 
		CCP_LOGERR("Tr2GPUParticlePool::Render() early-out"); 
	} );
	
	struct PclPerFrameVS 
	{
		XMMATRIX viewProj;
		XMMATRIX view;
		XMMATRIX proj;
		XMMATRIX viewInverseTranspose;
		Vector4 halfTexel;
	};
	PclPerFrameVS perFrameVS;

	perFrameVS.view = Tr2Renderer::GetViewTransform();
    perFrameVS.proj = XMMatrixTranspose( Tr2Renderer::GetProjectionTransform() );
	XMVECTOR det;
	perFrameVS.viewInverseTranspose = XMMatrixTranspose( XMMatrixInverse( &det, perFrameVS.view ) );

    perFrameVS.viewProj = XMMatrixTranspose(
        XMMatrixMultiply(
        Tr2Renderer::GetViewTransform(),
        Tr2Renderer::GetProjectionTransform() ) );

	
#if (TRINITY_PLATFORM == TRINITY_DIRECTX9)
	perFrameVS.halfTexel = Vector4( 0.5f / float(m_particleRTWidth),
									0.5f / float(m_particleRTHeight), 
									0, 
									0 );
#else
	perFrameVS.halfTexel = Vector4( 0, 0, 0, 0 );
#endif

	Matrix world;
	
	struct PclPerObjectVS
	{
		Matrix world;
		Vector4 textureXY;
	};
	PclPerObjectVS perObjectVS;

	D3DXMatrixIdentity( &perObjectVS.world );

	//vertex constant gubbins
	FillAndSetConstants( m_renderConstantsFrame, perFrameVS, Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_renderConstantsObject, perObjectVS, Tr2RenderContextEnum::VERTEX_SHADER, Tr2Renderer::GetPerObjectVSStartRegister(), renderContext );	
	renderContext.m_esm.ApplyVertexDeclaration( m_particleVertexDeclIndex );

	if( m_multipleParticleStreams ) 
	{
		//shared particle data (quad vertex position/internal uv)
		renderContext.m_esm.ApplyStreamSource( 0, m_particleQuadBuffer, 0, sizeof(ParticleQuadVert) );
		//per-particle data (source uv + 2 random seeds)
		renderContext.m_esm.ApplyStreamSource( 1, m_particleInstanceBuffer, 0, sizeof(ParticleInstanceVert) );
		//quad indices
		renderContext.m_esm.ApplyIndexBuffer( m_particleQuadIndices );
	} 
	else 
	{
		//big lumpen vertex buffer
		renderContext.m_esm.ApplyStreamSource( 0, m_particleSingleBuffer, 0, sizeof(ParticleSingleVert) );
		//big lumpen index buffer
		renderContext.m_esm.ApplyIndexBuffer( m_particleSingleIndices );
	}
	
	//for sampling in the vertex shader
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::VERTEX_SHADER, 0, m_positionAgeRT[m_flipRT^1].GetTexture() );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::VERTEX_SHADER, 1, m_velocityBehaviourRT[m_flipRT^1].GetTexture() );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::VERTEX_SHADER, 2, m_behaviourTexture );

	//and just in case the pixel shader wants in on the action...
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, m_positionAgeRT[m_flipRT^1].GetTexture() );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 1, m_velocityBehaviourRT[m_flipRT^1].GetTexture() );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 2, m_behaviourTexture ); 

	//atlases
	if( m_textureAtlas && m_textureAtlas->GetTexture() )
	{
		renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 3, *m_textureAtlas->GetTexture() );
	}
	if( m_gradientAtlas && m_gradientAtlas->GetTexture() )
	{
		renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 4, *m_gradientAtlas->GetTexture() );
	}
	
	//draw the particles!
	CR_RETURN( renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES ) );
		
	const auto passCount = m_renderParticleShader->GetShaderStateInterface()->GetPassCount();
	for( auto i = 0u; i < passCount; ++i )
	{
		m_renderParticleShader->GetShaderStateInterface()->ApplyAllStateForPass( i, renderContext );

		HRESULT result;
		if( m_multipleParticleStreams )
		{
			result = renderContext.DrawIndexedInstanced( 4, 0, 2, m_particleTotal );
		} 
		else 
		{
			result = renderContext.DrawIndexedPrimitive( m_particleTotal * 4, 0, m_particleTotal * 2, 0 );
		}

		if( !SUCCEEDED(result) ) 
		{
			CCP_LOGERR( "Tr2GPUParticlePool::Render() failed with %08x", result );
		}
	}

	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 0, nullTX );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 1, nullTX );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 2, nullTX );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 3, nullTX );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::PIXEL_SHADER, 4, nullTX );

	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::VERTEX_SHADER, 0, nullTX );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::VERTEX_SHADER, 1, nullTX );
	renderContext.m_esm.ApplyTexture( Tr2RenderContextEnum::VERTEX_SHADER, 2, nullTX );

	guard.Complete();
}


// -------------------------------------------------------------
// Description:
//   Destroy GPU resources etc.
// -------------------------------------------------------------
void Tr2GPUParticlePool::ReleaseResources( TriStorage s )
{
	m_initialised = false; 

	m_positionAgeRT[0].Destroy();
	m_positionAgeRT[1].Destroy();
	m_velocityBehaviourRT[0].Destroy();
	m_velocityBehaviourRT[1].Destroy();
	m_behaviourTexture.Destroy();

	m_particleInstanceBuffer.Destroy();
	m_particleQuadBuffer.Destroy();
	m_particleSingleBuffer.Destroy();
	m_particleQuadIndices.Destroy();
	m_particleSingleIndices.Destroy();
	m_particleVertexDeclIndex = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_simpleQuadDeclIndex = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;

	m_spawnParticleShader = nullptr;
	m_updateParticleShader = nullptr;
	m_renderParticleShader = nullptr;

	m_turbulenceTexture = nullptr;
	m_positionOffsetDefault = nullptr;
	m_velocityOffsetDefault = nullptr;

	m_textureAtlas = nullptr;
	m_gradientAtlas = nullptr;
	m_subTexturesByPath.clear();
	m_subTextureRefCounts.clear();
	m_gradientsByPath.clear();
	m_gradientRefCounts.clear();

	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		m_updateConstants.Destroy();
		m_renderConstantsFrame.Destroy();
		m_renderConstantsObject.Destroy();
		m_spawnConstants.Destroy();
		m_spawnConstantsVS.Destroy();
	}

	if( m_simpleQuadBuffer.GetMemoryClass() & s )
	{
		m_simpleQuadBuffer.Destroy();
	}
	if( m_simpleQuadIndices.GetMemoryClass() & s )
	{
		m_simpleQuadIndices.Destroy();
	}
}

bool Tr2GPUParticlePool::OnPrepareResources()
{
	if( !m_positionAgeRT[0].IsValid() ) 
	{
		m_initialised = false;
		return false;
	}
	return true;
}

// -------------------------------------------------------------
// Description:
//   Fetch one of the render targets so we can view them in Jessica.
//   m_debuggingMode determines which texture we view (position, velocity, behaviour)
// -------------------------------------------------------------
Tr2TextureAL *Tr2GPUParticlePool::GetTexture() 
{
	if( !m_initialised ) 
	{
		return nullptr;
	}

	switch( m_debuggingMode ) 
	{
	case ParticleDebugPosition:
		return &m_positionAgeRT[m_flipRT^1].GetTexture();
	case ParticleDebugVelocity:
		return &m_velocityBehaviourRT[m_flipRT^1].GetTexture();
	case ParticleDebugBehaviour:
		return &m_behaviourTexture;
	default:
		return nullptr;
	}
}


// -------------------------------------------------------------
// Description:
//   Tries to add a sprite texture to the atlas.
// Arguments:
//   Texture resource path, defining the texture we wish to add.
//   window is a Vector4 defining the position and scale of the texture within
//     the atlas, if it was added successfully.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::AddTexture( const std::string &path, Vector4 &window ) {
	if( !m_textureAtlas || !m_initialised )
	{
		return false;
	}
	if( path.empty() )
	{
		window = Vector4(0,0,0,0);
		return true;
	}
	
	auto found = m_subTexturesByPath.find( path );
	if( found != m_subTexturesByPath.end() ) 
	{
		if( !found->second->IsGood() )
		{
			//still not finished loading..
			return false;
		}
		found->second->GetTextureWindow( window );
		USE_MAIN_THREAD_RENDER_CONTEXT();
		m_textureAtlas->UpdateMipMaps( renderContext );
		auto refs = m_subTextureRefCounts.find( path );
		if( refs != m_subTextureRefCounts.end() ) ++m_subTextureRefCounts[path];
		else m_subTextureRefCounts[path] = 1;
		return true;
	}

	Tr2AtlasTexturePtr texture;
	if( texture.CreateInstance() ) 
	{
		texture->SetTargetAtlasBeforeLoad( m_textureAtlas );
		texture->Initialize( CA2W( path.c_str() ), L"ddsatlas" );
				
		m_subTexturesByPath[path] = texture;

		if( texture->IsGood() ) 
		{
			m_subTextureRefCounts[path] = 0;
			texture->GetTextureWindow( window );
			return true;
		} 
		else 
		{
			m_subTextureRefCounts[path] = 0;
			// this is OK, texture is prepared asynchronously
		}
	} 
	else
	{
		// this is not OK
		CCP_LOGWARN( "Tr2GPUParticlePool::AddTexture - Failed to create Tr2AtlasTexture!" );
	}

	return false;
}


// -------------------------------------------------------------
// Description:
//   'Removes' a texture from the atlas. Actually decreases a ref count, and removes only if zero.
// Arguments:
//   Texture resource path.
// -------------------------------------------------------------
void Tr2GPUParticlePool::ReleaseTexture( const std::string &path ) 
{
	if( path.empty() || !m_initialised ) 
	{
		return;
	}

	CCP_LOG( "Tr2GPUParticlePool::ReleaseTexture releasing %s", path.c_str() );
	auto refs = m_subTextureRefCounts.find(path);
	if( refs != m_subTextureRefCounts.end() ) 
	{
		--m_subTextureRefCounts[path];
		auto count = m_subTextureRefCounts[path];
		if( count == 0 ) 
		{
			m_subTexturesByPath.erase( path );
			m_subTextureRefCounts.erase( path );
		}
	}

	CCP_LOGWARN( "Tr2GPUParticlePool::ReleaseTexture tried to release texture not in atlas (%s)", path.c_str() );
}



// -------------------------------------------------------------
// Description:
//   See AddTexture, but for textures containing gradients. Most render modes do not use these.
// Arguments:
//   See AddTexture.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::AddGradient( const std::string &path, Vector4 &window )
{
	if( m_renderMode != GPUPRM_PreMultipliedAlpha )
	{
		return true;
	}

	if( !m_gradientAtlas || !m_initialised )
	{
		return false;
	}
	if( path.empty() )
	{
		window = Vector4(0,0,0,0);
		return true;
	}

	auto found = m_gradientsByPath.find( path );
	if( found != m_gradientsByPath.end() )
	{
		if( !found->second->IsGood() ) 
		{
			return false;
		}
		found->second->GetTextureWindow( window );
		auto refs = m_gradientRefCounts.find( path );
		if( refs != m_gradientRefCounts.end() ) ++m_gradientRefCounts[path];
		else m_gradientRefCounts[path] = 1;
		return true;
	}
	
	Tr2AtlasTexturePtr texture;
	if( texture.CreateInstance() )
	{
		texture->SetTargetAtlasBeforeLoad( m_gradientAtlas );
		texture->Initialize( CA2W( path.c_str() ), L"ddsatlas" );		
		
		m_gradientsByPath[path] = texture;

		if( texture->IsGood() ) 
		{
			m_gradientRefCounts[path] = 1;
			texture->GetTextureWindow( window );
			return true;
		} 
		else 
		{
			// this is OK, texture is prepared asynchronously
			// callers just call AddTexture repeatedly until it's done loading
			m_gradientRefCounts[path] = 0;
		}
	} 
	else 
	{
		// this is not OK
		CCP_LOGWARN( "Tr2GPUParticlePool::AddGradient - Failed to create Tr2AtlasTexture!" );
	}

	return false;
}


// -------------------------------------------------------------
// Description:
//   See RemoveTexture.
// Arguments:
//   See RemoveTexture.
// -------------------------------------------------------------
void Tr2GPUParticlePool::ReleaseGradient( const std::string &path )
{
	if( m_renderMode != GPUPRM_PreMultipliedAlpha )
	{
		return;
	}

	if( path.empty() || !m_initialised ) 
	{
		return;
	}
	CCP_LOG( "Tr2GPUParticlePool::ReleaseGradient releasing %s", path.c_str() );
	auto refs = m_gradientRefCounts.find(path);
	if( refs != m_gradientRefCounts.end() ) 
	{
		--m_gradientRefCounts[path];
		auto count = m_gradientRefCounts[path];
		if( count == 0 ) 
		{
			m_gradientsByPath.erase( path );
			m_gradientRefCounts.erase( path );
		}
	}

	CCP_LOGWARN( "Tr2GPUParticlePool::ReleaseGradient tried to release texture not in atlas (%s)", path.c_str() );
}


// -------------------------------------------------------------
// Description:
//   Sets the default spawn textures. The default-defaults are for a spherical shell of unit radius.
// Arguments:
//   Position and velocity textures.
// -------------------------------------------------------------
void Tr2GPUParticlePool::SetDefaultSpawnTextures( TriTextureRes *position, TriTextureRes *velocity )
{
	 m_positionOffsetDefault = position;
	 m_velocityOffsetDefault = velocity;
}


// -------------------------------------------------------------
// Description:
//   Space is terribly viscous, and is packed with turbulent eddies. This volume
//    texture is used to define these eddies.
// Arguments:
//   A volume texture resource.
// -------------------------------------------------------------
void Tr2GPUParticlePool::SetTurbulenceTexture( TriTextureRes *turbulence )
{
	m_turbulenceTexture = turbulence;
}


// -------------------------------------------------------------
// Description:
//   Space-turbulence actually tiles on some large cubes of this size. Default is around 4km.
//   This also affects the scale of detail in the highest octaves of noise.
// Arguments:
//   The edge length of the new tiling unit.
// -------------------------------------------------------------
void Tr2GPUParticlePool::SetTurbulenceScale( float scale )
{
	m_turbulenceScale = scale;
	m_turbulenceOffset = Vector3(0.f, 0.f, 0.f);
}


// -------------------------------------------------------------
// Description:
//   Turbulence is animated, this sets the speed at which it progresses.
// Arguments:
//  Speed at which to animate the most coarse octave of turbulent noise.
// -------------------------------------------------------------
void Tr2GPUParticlePool::SetTurbulenceSpeed( float speed )
{
	m_turbulenceSpeed = speed;
}


// -------------------------------------------------------------
// Description:
//   Returns the index of a named behaviour. Returns -1 if behaviour not present.
// Arguments:
//   String giving the name of the behaviour.
// -------------------------------------------------------------
int Tr2GPUParticlePool::GetBehaviourIndex( const char *name ) const
{
	auto byName = m_behaviourByName.find( name );
	if( byName != m_behaviourByName.end() ) 
		return byName->second;
	return -1;
}


// -------------------------------------------------------------
// Description:
//   Associates the given name with a behaviour structure. 
//   Overwrites the behaviour data if already present, otherwise adds it.
// Arguments:
//   String name, and a behaviour POD struct.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::SetBehaviour( const char *name, const Tr2ParticleBehaviour &behaviour )
{
	auto byName = m_behaviourByName.find( name );
	if( byName != m_behaviourByName.end() ) 
	{
		auto index = byName->second;
		return UpdateBehaviourTexture( index, behaviour );
	}

	if( m_behaviourCount < m_behaviourCountMax )
	{
		auto index = m_behaviourCount;
		m_behaviourByName[name] = index;
		++m_behaviourCount;
		CCP_LOG( "Tr2GPUParticlePool::SetBehaviour - associated behaviour name [%s] with index [%d]", name, index );
		return UpdateBehaviourTexture( index, behaviour );
	}

	auto oldBehaviour = FindUnusedBehaviour();
	if( oldBehaviour > 0 )
	{
		ReleaseTexture( m_behaviours[oldBehaviour].texturePath );
		unsigned index = (unsigned)oldBehaviour;
		m_behaviourByName[name] = index;
		m_behaviourUsageTime[index] =  BeOS->GetActualTime();
		CCP_LOG( "Tr2GPUParticlePool::SetBehaviour - associated behaviour name [%s] with old, unusued index [%d]", name, index );
		return UpdateBehaviourTexture( index, behaviour );
	}

	return false;
}


// -------------------------------------------------------------
// Description:
//   Attempts to fetch the named behaviour, writing it to *out*. If it is not present, creates
//    a new behaviour with default values.
// Arguments:
//   Name string and a reference to a behaviour structure to fill with data.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::GetOrCreateBehaviour( const char *name, Tr2ParticleBehaviour &out )
{
	auto byName = m_behaviourByName.find( name );
	if( byName != m_behaviourByName.end() )
	{
		auto index = byName->second;
		return GetBehaviour(index, out);
	}
	Tr2ParticleBehaviour defaultBehaviour;
	if( !SetBehaviour( name, defaultBehaviour ) ) 
	{
		return false;
	}
	out = defaultBehaviour;
	return true;
}


// -------------------------------------------------------------
// Description:
//   Attempts to fetch the named behaviour, writing it to *out*. If it is not present,
//     returns false and does not change *out*.
// Arguments:
//   Name string and a reference to a behaviour structure to fill with data.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::GetBehaviourByName( const char *name, Tr2ParticleBehaviour &out ) const 
{
	auto byName = m_behaviourByName.find( name );
	if( byName != m_behaviourByName.end() )
	{
		auto index = byName->second;
		return GetBehaviour(index, out);
	}
	CCP_LOGERR( "Tr2GPUParticlePool::GetBehaviourByName tried to fetch unknown particle behaviour (%s)", name );
	return false;
}

// -------------------------------------------------------------
// Description:
//   Attempts to fetch the indexed behaviour, writing it to *out*. If it is not present,
//     returns false and does not change *out*.
// Arguments:
//   Behaviour index and a reference to a behaviour structure to fill with data.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::GetBehaviour( const unsigned index, Tr2ParticleBehaviour &out ) const 
{
	if( m_behaviourCount > index )
	{
		out = m_behaviours[index];
		return true;
	}
	CCP_LOGERR( "Tr2GPUParticlePool::GetBehaviour tried to fetch particle behaviour out of bounds (%d/%d)", index, m_behaviourCount );
	return false;
}

// -------------------------------------------------------------
// Description:
//   Stores any changes to behaviour data, these are actually written to the texture
//    on Update.
// Arguments:
//   Index to modify, and a behaviour struct carrying desired changes.
// -------------------------------------------------------------
bool Tr2GPUParticlePool::UpdateBehaviourTexture( const unsigned index, const Tr2ParticleBehaviour &behaviour )
{
	if( !m_initialised ) 
	{
		return false;
	}
	m_behaviours[index] = behaviour;
	m_behavioursToUpdate.insert(index);
	m_maxParticleLifespan = std::max( m_maxParticleLifespan, behaviour.lifespan * (1.f + behaviour.lifespanVariance) );
	return true;
}


// -------------------------------------------------------------
// Description:
//   Finds an unused behaviour slot. If all slots are in use, ejects the behaviour
//    that was least recently used and returns that index.
// -------------------------------------------------------------
int Tr2GPUParticlePool::FindUnusedBehaviour() {
	if( !m_initialised )
	{
		return -1;
	}
	//always skip index 0, it stores the default behaviour (which is never rendered)
	const auto first = m_behaviourUsageTime.find(1);
	if( first == m_behaviourUsageTime.end() ) 
	{
		return -1;
	}

	unsigned elderIndex = first->first;
	Be::Time elderTime = first->second;

	for( auto it = m_behaviourUsageTime.begin(); it != m_behaviourUsageTime.end(); ++it ) 
	{
		if( it->second < elderTime && it->first > 0 ) 
		{
			elderIndex = it->first;
			elderTime = it->second;
		}
	}
	
	return (int)elderIndex;
}


// -------------------------------------------------------------
// Description:
//   Our usage-based density modifier should approach its 'target' value
//    over a number of frames, to help hide discontinuities in emission.
// -------------------------------------------------------------
void Tr2GPUParticlePool::UpdateUsageDensity() 
{
	const float deltaDensity = m_usageDensityModifierTarget - m_usageDensityModifier;
	if( deltaDensity == 0.f )
	{
		return;
	}

	const float maxIncrement = 0.01f;
	if( fabs(deltaDensity) < maxIncrement )
	{
		m_usageDensityModifier = m_usageDensityModifierTarget;
	}
	else
	{
		if( deltaDensity > 0.f )
		{
			m_usageDensityModifier += maxIncrement;
		}
		else
		{
			m_usageDensityModifier -= maxIncrement;
		}
	}
}

// -------------------------------------------------------------
// Description:
//   When we wrap around the pool and start spawning particles at the beginning again,
//    check how long this took and tweak spawn density appropriately. 
//   This hopefully leads to a stable spawning density where we never overwrite particles
//    that are still alive, or at least minimises such overwriting.
// -------------------------------------------------------------
void Tr2GPUParticlePool::UpdateUsageDensityTarget() 
{
	Be::Time now( BeOS->GetCurrentFrameTime() );

	const double wrapTime = TimeAsDouble( now - m_lastWrapTime );	
	if( wrapTime > 0.f )
	{
		const float minDensity = 0.1f;
		const float maxDensity = 1.f;	
		const float d = float(wrapTime) / m_maxParticleLifespan;
		m_usageDensityModifierTarget = std::min( maxDensity, std::max( minDensity, d ) );
	}
	m_lastWrapTime = now;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if the object contains a reference to given AL object. This method is exposed
//   to Python and is used for debugging.
// Arguments:
//   type - Tr2RenderContextEnum::ObjectType, type of AL object
//   object - pointer to an AL object (passed as a number)
// Return Value:
//   true If object contains a reference to the given AL object
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2GPUParticlePool::HasALObject( int type, size_t object )
{
	switch( type )
	{
	case Tr2RenderContextEnum::OT_TEXTURE:
		return m_behaviourTexture == *reinterpret_cast<Tr2TextureAL*>( object );
	case Tr2RenderContextEnum::OT_RENDER_TARGET:
		return m_positionAgeRT[0] == *reinterpret_cast<Tr2RenderTargetAL*>( object ) ||
			m_positionAgeRT[1] == *reinterpret_cast<Tr2RenderTargetAL*>( object ) ||
			m_velocityBehaviourRT[0] == *reinterpret_cast<Tr2RenderTargetAL*>( object ) ||
			m_velocityBehaviourRT[1] == *reinterpret_cast<Tr2RenderTargetAL*>( object );
	case Tr2RenderContextEnum::OT_VERTEX_BUFFER:
		return m_particleInstanceBuffer == *reinterpret_cast<Tr2VertexBufferAL*>( object ) ||
			m_particleQuadBuffer == *reinterpret_cast<Tr2VertexBufferAL*>( object ) ||
			m_particleSingleBuffer == *reinterpret_cast<Tr2VertexBufferAL*>( object ) ||
			m_simpleQuadBuffer == *reinterpret_cast<Tr2VertexBufferAL*>( object );
	case Tr2RenderContextEnum::OT_INDEX_BUFFER:
		return m_particleQuadIndices == *reinterpret_cast<Tr2IndexBufferAL*>( object ) ||
			m_particleSingleIndices == *reinterpret_cast<Tr2IndexBufferAL*>( object ) ||
			m_simpleQuadIndices == *reinterpret_cast<Tr2IndexBufferAL*>( object );
	case Tr2RenderContextEnum::OT_CONSTANT_BUFFER:
		return m_updateConstants == *reinterpret_cast<Tr2ConstantBufferAL*>( object ) ||
			m_renderConstantsFrame == *reinterpret_cast<Tr2ConstantBufferAL*>( object ) ||
			m_renderConstantsObject == *reinterpret_cast<Tr2ConstantBufferAL*>( object ) ||
			m_spawnConstants == *reinterpret_cast<Tr2ConstantBufferAL*>( object ) ||
			m_spawnConstantsVS == *reinterpret_cast<Tr2ConstantBufferAL*>( object );
	}
	return false;
}
