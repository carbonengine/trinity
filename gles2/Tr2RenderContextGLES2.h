#pragma once
#ifndef Tr2RenderContextGLES2_h_
#define Tr2RenderContextGLES2_h_

#if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#include "../Tr2RenderContextEnum.h"

class Tr2VertexBufferAL;
class Tr2IndexBufferAL;
class Tr2ConstantBufferAL;
class Tr2VertexLayoutAL;
class Tr2ShaderAL;
class Tr2SamplerStateAL;
class Tr2TextureAL;
struct ITr2RenderContextEvents;

#include "../Tr2VertexDefinition.h"
#include "../Tr2FragmentOpSettings.h"
#include "Tr2RenderTargetALGLES2.h"
#include "Tr2CapsALGLES2.h"
#if defined(TRINITY_AL_MOBILE)
#include <EGL/egl.h>
#endif

// -------------------------------------------------------------
// Description:
//   See http://carbon/wiki/Tr2RenderContext
// -------------------------------------------------------------
class Tr2RenderContextAL
{
public:
    Tr2RenderContextAL();
	~Tr2RenderContextAL();
	void Destroy();
	ALResult CreateSecondaryContext() { return E_FAIL; }

	static void SetPrimaryRenderContext( Tr2PrimaryRenderContextAL* );
	static Tr2PrimaryRenderContextAL& GetPrimaryRenderContext();
	static Tr2PrimaryRenderContextAL* GetPrimaryRenderContextPointer();

	ALResult CreateDevice(
		uint32_t Adapter, 
		Tr2WindowHandle hFocusWindow, 
		const Tr2PresentParametersAL& presentationParameters );
	ALResult SetPresentParameters( unsigned adapter, const Tr2PresentParametersAL& presentationParameters );

	const Tr2CapsAL& GetCaps() const;

	ALResult BeginScene()			{ return S_OK; }
	ALResult EndScene()				{ return S_OK; }
	ALResult FinishCommandList()	{ return E_FAIL; }
	ALResult ExecuteCommandList()	{ return E_FAIL; }
	ALResult Present();

	void	ReleaseDeviceResources();

	ALResult SetStreamSource(		
		uint32_t stream, 
		const Tr2VertexBufferAL & buffer, 
		uint32_t offset, 
		uint32_t stride );
	ALResult SetIndices( const Tr2IndexBufferAL & buffer );
	ALResult SetTopology( long topology );
	ALResult SetShader( const Tr2ShaderAL& shader );

	ALResult SetShaderBuffer(		
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer );

	ALResult SetUav(
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2GpuBufferAL& buffer,
		uint32_t initialCount = -1 ) throw()
	{ 
		return E_FAIL; 
	}

	ALResult SetUav( 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		Tr2TextureAL& texture ) throw()
	{ 
		return E_FAIL; 
	}

	ALResult ClearUav( Tr2GpuBufferAL& buffer, const float values[4] ) throw()
	{
		// For desktop OpenGL 4.3 glClearBufferData
		return E_FAIL;
	}

	ALResult ClearUav( Tr2GpuBufferAL& buffer, const uint32_t values[4] ) throw()
	{
		// For desktop OpenGL 4.3 glClearBufferData
		return E_FAIL;
	}

	ALResult SetTexture(				
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t slot, 
		const Tr2TextureAL& texture, 
		Tr2RenderContextEnum::ColorSpace colorSpace = Tr2RenderContextEnum::COLOR_SPACE_LINEAR );
	
	ALResult DrawIndexedPrimitive(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t minimumIndex = 0 );
	ALResult DrawPrimitive( uint32_t startVertex, uint32_t primitiveCount );
	ALResult DrawIndexedInstanced(	
		uint32_t numVertices, 
		uint32_t startIndex, 
		uint32_t primitiveCount, 
		uint32_t numInstances );

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint32_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride);

	ALResult DrawIndexedPrimitiveUP( 
		uint32_t numVertices, 
		uint32_t primitiveCount, 
		const uint16_t* indexData, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride);

	ALResult DrawPrimitiveUP(		
		uint32_t primitiveCount, 
		const void* vertexStreamZeroData, 
		uint32_t vertexStreamZeroStride );

	ALResult DrawIndexedInstancedIndirect( Tr2GpuBufferAL& params, uint32_t offset ) throw()
	{
		// For desktop OpenGL 4.3 something like glMultiDrawArraysIndirect
		return E_FAIL;
	}

	ALResult DrawInstancedIndirect( Tr2GpuBufferAL& params, uint32_t offset )
	{
		return E_FAIL;
	}
	
	ALResult RunComputeShader( unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ ) throw()
	{
		return E_FAIL;
	}
	ALResult RunComputeShaderIndirect( Tr2GpuBufferAL& indirectParams, unsigned offset )
	{
		return E_FAIL;
	}

	ALResult CopyBufferCounter( Tr2GpuBufferAL& dest, uint32_t destOffset, Tr2GpuBufferAL& src )
	{
		return E_FAIL;
	}

	ALResult SetVertexLayout( const Tr2VertexLayoutAL& layout );

	ALResult SetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t value );
	ALResult SetRenderStates( const uint32_t* stateValuePairs, uint32_t count );
	ALResult GetRenderState( Tr2RenderContextEnum::RenderState state, uint32_t* value );
	ALResult SetClipPlane( uint32_t planeIndex, const float* planeEq );
	ALResult SetScissorRect(			
		uint32_t left, 
		uint32_t top, 
		uint32_t right, 
		uint32_t bottom );

	
	static void DestroyMainThreadRenderContext();

	static void ShaderDeleted( int shader );

	long m_topology;	// in DX9, that's part of the DrawXyz calls, so remember this.

	ALResult SetConstants(			
		Tr2ConstantBufferAL& buffer, 
		Tr2RenderContextEnum::ShaderType constantType, 
		uint32_t registerIndex, 
		uint32_t maxRegisterCount = 0 );

	ALResult SetSamplerState(		
		const Tr2SamplerStateAL& samplerState, 
		Tr2RenderContextEnum::ShaderType inputType, 
		uint32_t registerNumber );


	// Helper function to clear the current primary backbuffer, depth and/or stencil.
	ALResult Clear(						
		uint32_t clearFlags, 
		uint32_t color, 
		float depth, 
		uint32_t stencil = 0,
		uint32_t slot = 0 );

	ALResult SetDepthStencil( const Tr2DepthStencilAL& depthStencil );
	void SetReadOnlyDepth( bool enable );
	ALResult SetRenderTarget( const Tr2RenderTargetAL& renderTarget, uint32_t slot = 0 );

	ALResult SetNumberOfLights(	uint32_t numLights );

	ALResult SetViewport( const Tr2Viewport& viewport );
	ALResult GetViewport( Tr2Viewport& viewport );
	
	static bool ConvertToGLPixelFormat( 
		Tr2RenderContextEnum::PixelFormat format, 
		GLenum& internalFormat, 
		GLenum& targetFormat, 
		GLenum& targetType );

	ALResult PushRenderTarget( uint32_t slot = 0 );
	ALResult PopRenderTarget( uint32_t slot = 0 );
	ALResult PushDepthStencil();
	ALResult PopDepthStencil();
	ALResult GetRenderTargetSize(	
		uint32_t& width, 
		uint32_t& height, 
		uint32_t slot = 0 );

	bool	IsSupportedRenderTargetFormat( 
		Tr2RenderContextEnum::PixelFormat format, 
		bool withAutoGenMipmap = false );

	Tr2RenderContextEnum::PixelFormat GetBackBufferFormat() const;

	ALResult GetAFRGroupCount( uint32_t& count );

#ifdef _WIN32
	bool	IsValid() { return m_hRC != (HGLRC)0xFFffFFff; }
#else
	bool	IsValid() { return m_hWnd != 0; }
#endif
	
	static const uint32_t SHADER_TYPE_MASK = 
		( 1 << Tr2RenderContextEnum::VERTEX_SHADER ) |
		( 1 << Tr2RenderContextEnum::PIXEL_SHADER );


	// Debug helpers
	size_t GetStackSizeRT( uint32_t RT = 0 )	const { return m_stackRT[RT].size(); }
	size_t GetStackSizeDS()						const { return m_stackDS    .size(); }

	ALResult  CopySubresourceRegion(		
		Tr2TextureAL& destination,
		const Tr2TextureSubresource& destSubresource,
		Tr2TextureAL& source, 
		const Tr2TextureSubresource& sourceSubresource );

	Tr2RenderTargetAL& GetDefaultBackBuffer()	{ return m_defaultBackBuffer; }

	ITr2RenderContextEvents* m_events;

	ALResult InternalBlitToBackBuffer( Tr2TextureAL& source );
	ALResult InternalResolveRT( Tr2RenderTargetAL& destination, const Tr2RenderTargetAL& source );
private:
	enum { MAX_RENDER_TARGET = 8 };
	const Tr2RenderTargetAL*					m_boundRenderTarget[MAX_RENDER_TARGET];
	uint32_t									m_renderTargetHighWaterMark;
	const Tr2DepthStencilAL*					m_boundDepthStencil;
	TrackableStdStack<const Tr2RenderTargetAL*>	m_stackRT[MAX_RENDER_TARGET];
	TrackableStdStack<const Tr2DepthStencilAL*>	m_stackDS;

	ALResult SetRtDsToDevice( uint32_t changedSlot );
	uint32_t	m_defaultFrameBufferObject;

	uint32_t m_offscreenFrameBuffer0;
	uint32_t m_offscreenFrameBuffer1;

	bool	m_boundIndexBufferIs16Bit;
	uint32_t ComputeVertexCount( uint32_t primitiveCount );

private:
	Tr2RenderTargetAL m_defaultBackBuffer;
	Tr2DepthStencilAL m_defaultDepthStencil;

	struct ProgramObject
	{
		int program;
		int constantBuffers[16];
		int intConstant;
		int shadowStateInt;
		int shadowStateFloat;
		int shadowStateOffsets;
		std::map<	std::pair<	Tr2VertexDefinition::UsageCode, 
								uint32_t>, 
					int> 
			attributes;
	};

	struct Blitter;
	Blitter* m_blitter;

	const Tr2ShaderAL*	m_vertexShader;
	const Tr2ShaderAL*	m_pixelShader;
	ProgramObject* m_boundProgramObject;
	Tr2ConstantBufferAL* m_boundBuffers[16];
	int m_numberOfLights;
	const Tr2VertexDefinition* m_boundLayout;
	struct VertexStream
	{
		GLuint buffer;
		uint32_t stride; 
		uint32_t offset; 
	};
	VertexStream m_boundStreams[8];

	Tr2Viewport m_currentViewport;

	Tr2TextureAL*		m_boundTextures[Tr2RenderContextEnum::SHADER_TYPE_COUNT][16];
	Tr2SamplerStateAL::StateData m_boundSamplers[Tr2RenderContextEnum::SHADER_TYPE_COUNT][16];

	unsigned m_currentActiveTexture;

	struct BlendState
	{
		bool separateAlphaBlend;
		GLenum blendMode;
		GLenum blendModeAlpha;
		GLenum srcBlend;
		GLenum destBlend;
		GLenum srcBlendAlpha;
		GLenum destBlendAlpha;
		bool dirty;
	} m_blendState;

	struct StencilOpState
	{
		bool twoSided;
		GLenum stencilFail;
		GLenum stencilZFail;
		GLenum stencilPass;
		GLenum ccwStencilFail;
		GLenum ccwStencilZFail;
		GLenum ccwStencilPass;
		bool dirty;
	} m_stencilOpState;

	struct StencilTestState
	{
		bool twoSided;
		GLenum func;
		GLint ref;
		GLuint mask;
		GLenum ccwFunc;
		GLint ccwRef;
		GLuint ccwMask;
		bool dirty;
	} m_stencilTestState;

	struct DepthOffsetState
	{
		float slopeScaleOffset;
		float bias;
		bool dirty;
	} m_depthOffsetState;

	Tr2FragmentOpSettings::TAlphaTestParameters	m_alphaTestParameters;
	Tr2FragmentOpSettings						m_fragmentOpSettings;

	// Map from vertex+fragment shader to program
	static std::map<std::pair<int, std::pair<int, bool> >, ProgramObject*> s_programs;

	uint32_t m_renderStates[Tr2RenderContextEnum::RS_MAX_STATE];

	ALResult CreateOpenGLContext( Tr2PresentParametersAL& pPresentationParameters );
	void ReleaseOpenGLContext();

	Tr2RenderContextAL( const Tr2RenderContextAL& ) /* = delete */;
	Tr2RenderContextAL& operator=( const Tr2RenderContextAL& ) /* = delete */;

	ALResult SetProgram();

	struct ShadowStateRestoreInfo;

	bool ApplyShadowRenderStates( ShadowStateRestoreInfo& info );
	bool ApplyVertexDeclaration( ShadowStateRestoreInfo& info, const void* data = nullptr, size_t stride = 0 );

	void    RestoreState( const ShadowStateRestoreInfo& info );
public:
#ifdef _WIN32
	HGLRC			m_hRC;
	HDC				m_hDC;
#elif defined(TRINITY_AL_MOBILE)
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLContext m_context;
#endif
	Tr2WindowHandle	m_hWnd;
	Tr2CapsAL m_caps;
};

#endif	// #if( TRINITY_PLATFORM==TRINITY_OPENGLES2 )

#endif //Tr2RenderContextGLES2_h_
