#pragma once
#ifndef Tr2Renderer_H
#define Tr2Renderer_H


#include "TriDebugTextRenderer.h"
#include "Tr2Blitter.h"
#include "Shader/Tr2EffectDescription.h"

BLUE_DECLARE( Tr2Effect );

enum TR2SHADERMODEL
{
	TR2SM_1_1,
	TR2SM_2_0_LO,
	TR2SM_2_0_HI,
	TR2SM_3_0_LO,
	TR2SM_3_0_HI,
	TR2SM_3_0_DEPTH,

	TR2SM_AUTHORING,

	TR2SM_COUNT
};


class TriSettings;
class Tr2LineGraph;
class TriPoolAllocator;
class TriViewport;

BLUE_DECLARE( Tr2Material );
BLUE_DECLARE( Tr2Shader );

// See http://core/wiki/Tr2Renderer

// Tr2Renderer is the successor to TriDevice, with a cleaner, simpler and better abstracted interface
// to the underlying hardware. Tr2Renderer focuses on effects based rendering, with no support for
// fixed function rendering.
class Tr2Renderer
{
public:
	static bool Initialize();
	static void Shutdown();

	static void PrepareDeviceResources();
	static void ReleaseDeviceResources( TriStorage s );

	static void SetShaderModel( TR2SHADERMODEL sm );
	static TR2SHADERMODEL GetShaderModel();
	static const char* GetShaderModelString( TR2SHADERMODEL sm );

	// Get the default start registers for the currently set shader model
	static unsigned int GetPerFrameVSStartRegister();
	static unsigned int GetPerFramePSStartRegister();
	static unsigned int GetPerObjectVSStartRegister();
	static unsigned int GetPerObjectPSStartRegister();
	static unsigned int GetPerObjectRTVertexBufferDataRegister();
	static unsigned int GetPerObjectVSGUIStartRegister();
	static unsigned int GetPerObjectMorphTargetVertexBufferDataStartRegister();
	static unsigned int GetPerObjectMorphTargetAnimationDataStartRegister();


	template<Tr2RenderContextEnum::ShaderType shaderType>
	static unsigned int GetPerObjectStartRegister()
	{
		if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
		{
			return GetPerObjectPSStartRegister();
		}
		return GetPerObjectVSStartRegister();
	}

	static unsigned int GetPerObjectStartRegister( Tr2RenderContextEnum::ShaderType shaderType )
	{
		if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
		{
			return GetPerObjectPSStartRegister();
		}
		return GetPerObjectVSStartRegister();
	}

	static void DisableResourceLoad( bool flag )	{ m_disableGeometryLoad = m_disableTextureLoad = m_disableEffectLoad = flag; }
	static bool IsGeometryLoadDisabled()			{ return m_disableGeometryLoad; }
	static bool IsTextureLoadDisabled()				{ return m_disableTextureLoad; }
	static bool IsEffectLoadDisabled()				{ return m_disableEffectLoad; }
	static bool IsAsyncLoadDisabled()				{ return m_disableAsyncLoad; }

	static bool m_disableGeometryLoad;
	static bool m_disableTextureLoad;
	static bool m_disableEffectLoad;
	static bool m_disableAsyncLoad;

	static TriPoolAllocator* GetPoolAllocator();

	// query if currently set shadermodel supports certain features
	static bool IsLowQuality();

	// texture-resize directories
	static bool IsTextureToResize( const char* filename );
	static void AddMipLevelSkipExclusionDirectory( const char* path );

	// Register an effect. All registered effects are automatically reinitialized upon shader model change.
	// Reinit's can also be triggered from somewhere else, for example when toggling shadows
	static void RegisterEffect( Tr2Effect* f );
	static void UnregisterEffect( Tr2Effect* f );
	static void ReinitializeRegisteredEffects();
	static void RebuildEffects();

	static void ReserveQuadListIndexBuffer( uint32_t numOfQuads );
	[[nodiscard]] static Tr2SuballocatedBuffer::Allocation& GetQuadListIndexBuffer();

	static void BeginFrame();
	static void EndFrame();
	static unsigned long long GetCurrentFrameCounter();

	static HRESULT BeginRenderContext();
	static HRESULT EndRenderContext();

    static void GetBackBufferDimensions( unsigned int& w, unsigned int& h );
    
	// Sets up a perspective projection transform based on the field of view, front/back plane and aspect ratio
	static void SetPerspectiveProjection( float fov, float front, float back, float asp, const Matrix& viewportAdjustment = IdentityMatrix() );

	// Sets up a perspective projection transform based on left/right/bottom/top parameters, and front/back planes
	static void SetPerspectiveProjection( float left, float right, float bottom, float top, float front, float back, const Matrix& viewportAdjustment = IdentityMatrix() );

	// Sets up an orthogonal projection transform
	static void SetOrthoProjection( float width, float height, float front, float back, const Matrix& viewportAdjustment = IdentityMatrix() );

	// Sets up an orthogonal projection transform based on left/right/bottom/top parameters, and front/back planes
	static void SetOrthoProjection( float left, float right, float bottom, float top, float front, float back, const Matrix& viewportAdjustment = IdentityMatrix() );

	// Adjusts the existing projection by scaling and then translation, useful for
	// picking and other methods where rendering subrects from the frame
	// is required
	static void AdjustProjection( const Vector2& scaling, const Vector2& translation );

	// Explicitly sets a projection transform. Note that field-of-view, aspect ratio and front/back planes are
	// extracted from this - use only for special purpose projection, such as in picking.
	static void SetProjectionTransform( const Matrix& proj, const Matrix& viewportAdjustment = IdentityMatrix() );

    static const Matrix& GetProjectionTransform();
	static Matrix GetReversedDepthProjectionTransform();
	static const Matrix& GetProjectionRawTransform();
    static const Matrix& GetInverseProjectionTransform();
	static const PROJECTION_TYPE GetCurrentProjectionType();

	static Vector3 ProjectWorldToScreen( const Vector3& worldPos, const Tr2Viewport& vp );

	static const TriViewport& GetViewport();
		
    static float GetFrontClip();
    static float GetBackClip();
	static float GetFrustumRadius();
	static void GetFrustumPlane( size_t index, Vector4& plane );
    static float GetFieldOfView();
    static float GetAspectRatio();
	static float GetOrthoWidth();
	static float GetOrthoHeight();
	
    static void SetWorldTransform( const Matrix& m );
    static const Matrix& GetWorldTransform();

    static void SetViewTransform( const Matrix& m );
    static const Matrix& GetViewTransform();
    static const Matrix& GetInverseViewTransform();
    
    static const Vector3& GetViewPosition();
    static Vector3 GetViewLookAt();

	static float GetAnimationTime();
	static float GetAnimationTimeElapsed( float startTime );

	static uint32_t GetUpscalingContextID();
	static void SetUpscalingContextID( uint32_t upscalingContextID );

	// Text output for debugging purposes.
	// Calls to Printf gather up text - text is rendered on RenderDebugInfo.
	// This allows calls to Printf outside the rendering phase.
	static void Printf( int x, int y, uint32_t color, const char* msg, ... );
	static void Printf( TriDebugFont font, const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... );
    static void Printf( TriDebugFont font, const Vector3& pos, uint32_t color, const char* msg, ... );
    static void Printf( TriDebugFont font, int fontStyle, const Vector3& pos, Vector4 color, const char* msg, ... );

	// Text output for debugging purposes.
	// Text is rendered immediately so these functions can only be called within
	// a render context.
	static void PrintfImmediate( Tr2RenderContext& renderContext, int x, int y, uint32_t color, uint32_t format, const char* msg, ... );
	static void PrintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... );
	static void PrintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Vector3& pos, uint32_t color, const char* msg, ... );

	// Line rendering for debugging purposes.
	// Calls to the functions below build up a line set - lines are rendered on RenderDebugInfo.
	// This allows line rendering from outside the rendering phase.
	static void DrawLine( const Vector3& from, const Vector3& to, uint32_t color = 0xffffffff );
	static void DrawLine( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor );
	static void DrawSphere( const Vector3& center, float radius, int segments, uint32_t color = 0xffffffff );
	static void DrawSphere( const Vector4& sphere, int segments, uint32_t color = 0xffffffff );
	static void DrawBox( const Vector3& min, const Vector3& max, uint32_t color = 0xffffffff );
	static void DrawOrientedBox( const Matrix& boxMatrix, uint32_t color = 0xffffffff );

	static void RenderDebugInfo( Tr2RenderContext& renderContext );

	static bool DrawTexture( Tr2RenderContext& renderContext, Tr2TextureAL& texture, const Vector2& tlTexCoord = Vector2( 0.0f, 0.0f ), const Vector2& brTexCoord = Vector2( 1.0f, 1.0f ), Tr2Blitter::Filtering filter = Tr2Blitter::FILTER_POINT );
	static bool DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, Tr2TextureAL& texture );
	static bool DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord );
    static bool DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, const Vector2& tlVertexCoord, const Vector2& brVertexCoord );
	static bool DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord );

	static void DrawScreenQuad( Tr2RenderContext& renderContext, Tr2Material* effect );
	static void DrawScreenQuad( Tr2RenderContext& renderContext, Tr2Effect* effect, const Vector2 &topLeft, const Vector2 &bottomRight );
	static void DrawCameraSpaceScreenQuad( Tr2RenderContext& renderContext, Tr2Shader* shader, Tr2Material* material );
	static bool DrawFullScreenWithShader( Tr2RenderContext& renderContext, Tr2Material * material );

	static bool RunComputeShader( Tr2Material* effect, unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ, Tr2RenderContext& renderContext );
	static bool RunComputeShader( Tr2Material* effect, const BlueSharedString& technique, unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ, Tr2RenderContext& renderContext );
	static bool RunComputeShaderIndirect( Tr2Material* effect, Tr2BufferAL& indirectParams, unsigned offset, Tr2RenderContext& renderContext );

	static void PushProjection();
	static void PopProjection();
	static void PushViewTransform();
	static void PopViewTransform();

	static TriSettings& GetSettings();

	// Used to detect if the creation of resources is allowed
	static bool IsResourceCreationAllowed();

	static void EnableFallbackTextureDebugging();
	static void DisableFallbackTextureDebugging();
	static const Tr2TextureAL& GetFallbackTexture( Tr2EffectResource::Type textureType, const char* debugContext );
	static void InitializeSystemShaderOptions();

	static bool GetGeometryShaderSupport();
private:

	static void SetResourceCreationAllowed( bool isAllowed );

	friend class TriDevice;

};

#endif // Tr2Renderer_H