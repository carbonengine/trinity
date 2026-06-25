// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2EffectStateManager_h
#define Tr2EffectStateManager_h

#include "Tr2DeviceResource.h"
#include "TriViewport.h"
#include "Tr2Variable.h"

#include "Tr2SuballocatedBuffer.h"


BLUE_DECLARE( Tr2EffectRes );

class ITriRenderBatchAccumulator;
class Tr2Effect;
class Tr2LowLevelShader;
class TriVariable;
class Tr2VertexDefinition;
struct Tr2ShaderInputDefinition;

BLUE_DECLARE( Tr2Material );


enum PROJECTION_TYPE
{
	PT_PERSPECTIVE = 0,
	PT_ORTHOGONAL,

	PT_UNKNOWN,
};

//
// See http://core/wiki/Tr2EffectStateManager
//
class Tr2EffectStateManager
{
public:
	Tr2EffectStateManager( Tr2RenderContext& renderContext );

	void Initialize();
	void Shutdown();

	// This is called when device is reset
	void ReleaseDeviceResources( TriStorage s );

	void BeginManagedRendering( Tr2RenderContextEnum::CullMode cullmode = Tr2RenderContextEnum::CULLMODE_CW );
	void EndManagedRendering();

	enum
	{
		// 16 textures/samplers per shader
		SAMPLER_MAX_COUNT = 16,
		VERTEX_STREAM_MAX_COUNT = 4,
		UNKNOWN = 0XFFFFFFFFu
	};

	enum RenderingMode
	{
		RM_ANY, // Don't care about the render state
		RM_OPAQUE,
		RM_DECAL,
		RM_DECAL_NO_DEPTH,
		RM_ALPHA,
		RM_ALPHA_ADDITIVE,
		RM_DEPTH_ONLY,
		RM_PICKING,
		RM_FULLSCREEN,
		RM_SPRITE2D,
		RM_CULL,
		RM_LIGHT,
		RM_ERASE,
		RM_PREPASS_COLOR,

		RM_COUNT
	};

	enum
	{
		UNINITIALIZED_DECLARATION = ~0u,
		NULL_DECLARATION = ~0u - 1
	};

	void ApplyStandardStates( RenderingMode rm );

	void ApplyRenderStates( uint32_t ix );
	void ApplyShaderProgram( uint32_t ix );
	static Tr2ShaderProgramAL* GetShaderProgram( uint32_t ix );


	void ApplyStreamSource( uint32_t stream, const Tr2SuballocatedBuffer::Allocation& vertices );

	void ApplyStreamSource( uint32_t stream, const Tr2BufferAL& buffer, uint32_t offset, uint32_t stride );
	void ApplyIndexBuffer( const Tr2BufferAL& indices );

	void ApplyIndexBuffer( const Tr2SuballocatedBuffer::Allocation& indices );
	void ApplyIndexBuffer( const Tr2BufferAL& indices, uint32_t stride );

	// --------------------------------------------------------------------------------------
	// Description:
	//   Set of render states.
	// --------------------------------------------------------------------------------------
	typedef std::map<Tr2RenderContextEnum::RenderState, uint32_t> Tr2RenderStateSetup;


	static uint32_t RegisterShader(
		Tr2RenderContextEnum::ShaderType type,
		const Tr2ShaderBytecodeAL& bytecode,
		const Tr2ShaderSignatureAL& signature,
		const char* shaderPath );
	static uint32_t RegisterShaderProgram( uint32_t* shaders, size_t count );
	static uint32_t RegisterShaderProgramOverride( uint32_t originalProgram, uint32_t overrideProgram );

	static uint32_t RegisterShaderLibrary( const Tr2ShaderBytecodeAL& bytecode );
	static const Tr2ShaderBytecodeAL* GetShaderLibraryCode( uint32_t handle );

	static uint32_t RegisterRenderStateSetup(
		const Tr2RenderStateSetup& rss );

	void SetWireframeRendering( bool b );
	void SetInvertedCullMode( bool b );
	bool IsCullModeInverted();

	void SetInvertedDepthTest( bool invertedDepthTest );
	bool IsDepthTestInverted() const;

	static uint32_t GetVertexDeclarationHandle( const Tr2VertexDefinition& vertexDefinition );
	void ApplyVertexDeclaration( uint32_t declaration );
	static bool GetVertexDeclarationElements( uint32_t declaration, Tr2VertexDefinition& definition );


	void SetFullScreenViewport();
	void SetViewport( const TriViewport& vp );
	void SetViewport( int width, int height, int x, int y, float minZ, float maxZ );
	const TriViewport& GetViewport();
	const Tr2Viewport& GetDeviceViewport();
	void PushViewport();
	void PopViewport();

	unsigned int GetRenderTargetWidth();
	unsigned int GetRenderTargetHeight();

	void PushRenderTarget( unsigned slot = 0 ); // does not set any RT, just stores the current one so it can be safely changed later
	void PushRenderTarget( const Tr2TextureAL& rt, unsigned slot = 0 );
	void PopRenderTarget( unsigned slot = 0 );
	bool SetRenderTarget( unsigned int index, const Tr2TextureAL& rt, bool updateViewport = true, uint32_t slice = 0 );

	bool PushDepthStencilBuffer(); // does not set a DS, just stores it so it can be safely changed later
	bool PushDepthStencilBuffer( const Tr2TextureAL& ds );
	void PopDepthStencilBuffer();
	bool SetDepthStencilBuffer( const Tr2TextureAL& ds );

	void SetupContextResources();
	void AssignFrom( const Tr2EffectStateManager& other );

	void UpdateRenderTargetViewport( unsigned width, unsigned height );

private:
	friend class Tr2EffectRes;
	friend class Tr2LowLevelShader;

	void SetRenderStateOverride( Tr2RenderContextEnum::RenderState state, const uint32_t* overrides );
	void DoApplyRenderStates( uint32_t ix );

	Tr2RenderContext& m_renderContext;

	// used by SetPerObjectDataToDevice
	Tr2ConstantBufferAL m_perObjectConstantBuffers[Tr2RenderContextEnum::CBUFFER_COUNT];

	struct CurrentValues
	{
		void Reset();

		uint32_t m_shaderProgram;

		uint32_t m_vertexDeclaration;

		struct HalStream
		{
			Tr2BufferAL m_vertexBuffer;
			uint32_t m_offset;
			uint32_t m_stride;
		};
		HalStream m_streams[VERTEX_STREAM_MAX_COUNT];

		Tr2BufferAL m_indexBuffer;
		uint32_t m_indexStride;
		Tr2EffectStateManager::RenderingMode m_renderingMode;
		uint32_t m_renderStateSetup;
	};

	CurrentValues m_currentValues;

	bool m_isManagedRendering;

	struct RenderStates
	{
		RenderStates() :
			dirty( true )
		{
		}

		std::vector<uint32_t> states;
		bool dirty;
	};

	std::vector<RenderStates> m_renderStates;
	const uint32_t* m_renderStateOverrides[Tr2RenderContextEnum::RS_MAX_STATE];


	int m_renderTargetWidth;
	int m_renderTargetHeight;


	CTriViewport m_viewport;
	Tr2Viewport m_viewportOnDevice;

	std::list<CTriViewport> m_viewportStack;

	Tr2Variable m_viewportSizeVar;

	void SetupViewport();


	Tr2EffectStateManager( const Tr2EffectStateManager& );
	Tr2EffectStateManager& operator=( const Tr2EffectStateManager& );
};

#endif //Tr2EffectStateManager_h
