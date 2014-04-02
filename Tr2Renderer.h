#pragma once
#ifndef Tr2Renderer_H
#define Tr2Renderer_H


#include "TriDebugTextRenderer.h"
#include "Tr2Blitter.h"

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

enum PROJECTION_TYPE
{
	PT_PERSPECTIVE = 0,
	PT_ORTHOGONAL,

	PT_UNKNOWN,
};

class TriSettings;
class Tr2LineGraph;
class TriShadowMap;
class TriPoolAllocator;
class TriViewport;
class Tr2ShaderMaterial;

BLUE_DECLARE_INTERFACE( ITr2ShaderMaterial );
BLUE_DECLARE_INTERFACE( ITr2ShaderState );

// See http://core/wiki/Tr2Renderer

// Tr2Renderer is the successor to TriDevice, with a cleaner, simpler and better abstracted interface
// to the underlying hardware. Tr2Renderer focuses on effects based rendering, with no support for
// fixed function rendering.
class Tr2Renderer
{
public:
	static char* GetEnlightenErrorBuffer();
	static bool Initialize();
	static void Shutdown();

	static void PrepareDeviceResources();
	static void ReleaseDeviceResources( TriStorage s );

	static void SetShaderModel( TR2SHADERMODEL sm );
	static TR2SHADERMODEL GetShaderModel();
	static TR2SHADERMODEL GetMaxShaderModelSupported();
	static const char* GetShaderModelString( TR2SHADERMODEL sm );

	// Get the default start registers for the currently set shader model
	static unsigned int GetPerFrameVSStartRegister();
	static unsigned int GetPerFramePSStartRegister();
	static unsigned int GetPerObjectVSStartRegister();
	static unsigned int GetPerObjectPSStartRegister();
	static unsigned int GetPerObjectVSFFEStartRegister();
	static unsigned int GetPerObjectVSGUIStartRegister();
	// Get some register numbers for the currently set shader model
	static unsigned int GetMaxNumOfPSRegistersI();
	static unsigned int GetMaxNumOfPSRegistersF();


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


	static bool IsRightHanded();

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
	static void ClearMipLevelSkipExclusionDirectories();

	// Register an effect. All registered effects are automatically reinitialized upon shader model change.
	// Reinit's can also be triggered from somewhere else, for example when toggling shadows
	static void RegisterEffect( Tr2Effect* f );
	static void UnregisterEffect( Tr2Effect* f );
	static void ReinitializeRegisteredEffects();

	// Register a material.  All registered materials are rebound after shader model change.
	static void RegisterMaterial( Tr2ShaderMaterial* material );
	// Unregister a material.
	static void UnregisterMaterial( Tr2ShaderMaterial* material );

	// Gets the global situation
	static const std::vector<unsigned int>& GetGlobalSituation( void );
	// Globally add a situation flag to all shader materials
	static void AddGlobalSituationFlag( unsigned int situationFlag );
	// Globally remove a situation flag from all shader materials
	static void RemoveGlobalSituationFlag( unsigned int situationFlag );
	// Check if a global situation flag is set
	static bool HasGlobalSituationFlag( unsigned int situationFlag );
	// Rebind all shader materials
	static void RebindAllShaderMaterials( void );

	// access to global vertex/index buffers
	static Tr2VertexBufferAL& GetQuadVertexBuffer();
	// pointer since this can fail
	static Tr2IndexBufferAL*  GetQuadListIndexBuffer( unsigned int numOfQuads );

	static void BeginFrame();
	static void EndFrame();
	static unsigned long GetCurrentFrameCounter();

	static HRESULT BeginRenderContext();
	static HRESULT EndRenderContext();
	static unsigned long GetCurrentRenderContextCounter();

	static void ClearDepthBuffer( float z = 1.0f );

    static const Matrix& GetIdentityTransform();
	static const Matrix& GetNullTransform();

    static void GetBackBufferDimensions( unsigned int& w, unsigned int& h );
    
	// Sets up a perspective projection transform based on the field of view, front/back plane and aspect ratio
	static void SetPerspectiveProjection( float fov, float front, float back, float asp );

	// Sets up a perspective projection transform based on left/right/bottom/top parameters, and front/back planes
	static void SetPerspectiveProjection( float left, float right, float bottom, float top, float front, float back );

	// Sets up an orthogonal projection transform
	static void SetOrthoProjection( float width, float height, float front, float back );

	// Sets up an orthogonal projection transform based on left/right/bottom/top parameters, and front/back planes
	static void SetOrthoProjection( float left, float right, float bottom, float top, float front, float back );

	// Adjusts the existing projection by scaling and then translation, useful for
	// picking and other methods where rendering subrects from the frame
	// is required
	static void AdjustProjection( const Vector2& scaling, const Vector2& translation );

	// Explicitly sets a projection transform. Note that field-of-view, aspect ratio and front/back planes are
	// extracted from this - use only for special purpose projection, such as in picking.
	static void SetProjectionTransform( const Matrix& proj );

    static const Matrix& GetProjectionTransform();
	static const Matrix& GetProjectionRawTransform();
    static const Matrix& GetInverseProjectionTransform();
	static const PROJECTION_TYPE GetCurrentProjectionType();
	static void AdjustClipCoordsForViewport( Vector3& tl, Vector3& br );

	static Vector3 ProjectWorldToScreen( const Vector3& worldPos, const Tr2Viewport& vp );

	static void SetFullScreenViewport();
	static void SetViewport( const TriViewport& vp );
	static void SetViewport( int width, int height, int x, int y, float minZ, float maxZ );
	static const TriViewport& GetViewport();
	static const Tr2Viewport& GetDeviceViewport();
		
    static float GetFrontClip();
    static float GetBackClip();
	static float GetFrustumRadius();
    static float GetFieldOfView();
    static float GetAspectRatio();
	static float GetOrthoWidth();
	static float GetOrthoHeight();
	static unsigned int GetRenderTargetWidth();
	static unsigned int GetRenderTargetHeight();
	
    static void SetWorldTransform( const Matrix& m );
    static const Matrix& GetWorldTransform();

    static void SetViewTransform( const Matrix& m );
    static const Matrix& GetViewTransform();
    static const Matrix& GetInverseViewTransform();
    
    static const Vector3& GetViewPosition();
    static const Vector3& GetViewLookAt();

	static float GetAnimationTime();
	static float GetAnimationTimeElapsed( float startTime );

	// Text output for debugging purposes.
	// Calls to Printf gather up text - text is rendered on RenderDebugInfo.
	// This allows calls to Printf outside the rendering phase.
	static void Printf( int x, int y, uint32_t color, const char* msg, ... );
	static void Printf( TriDebugFont font, const Rect& rect, uint32_t format, uint32_t color, const char* msg, ... );
    static void Printf( TriDebugFont font, const Vector3& pos, uint32_t color, const char* msg, ... );

	// Text output for debugging purposes.
	// Text is rendered immediately so these functions can only be called within
	// a render context.
	static void PrintfImmediate( int x, int y, uint32_t color, uint32_t format, const char* msg, ... );
	static void PrintfImmediate( TriDebugFont font, const Rect& rect, uint32_t format, uint32_t color, const char* msg, ... );
	static void PrintfImmediate( TriDebugFont font, const Vector3& pos, uint32_t color, const char* msg, ... );

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

	static bool DrawTexture( Tr2TextureAL& texture, const Vector2& tlTexCoord = Vector2( 0.0f, 0.0f ), const Vector2& brTexCoord = Vector2( 1.0f, 1.0f ), Tr2Blitter::Filtering filter = Tr2Blitter::FILTER_POINT );
	static bool DrawTexture( ITr2ShaderMaterial* effect, Tr2TextureAL& texture );
	static bool DrawTexture( ITr2ShaderMaterial* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord );
    static bool DrawTexture( ITr2ShaderMaterial* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, const Vector2& tlVertexCoord, const Vector2& brVertexCoord );
	static bool DrawTexture( ITr2ShaderMaterial* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord );

	static void DrawScreenQuad( ITr2ShaderMaterial* effect );
	static void DrawScreenQuad( Tr2Effect* effect, const Vector2 &topLeft, const Vector2 &bottomRight );
	static bool DrawCubeTexture( Tr2TextureAL& texture, Tr2RenderContextEnum::CubemapFace face, unsigned int mipLevel = 0 );
	static void DrawCameraSpaceScreenQuad( ITr2ShaderState* shader, ITr2ShaderMaterial* material );
	static bool DrawFullScreenWithShader( ITr2ShaderMaterial * material );

	static bool RunComputeShader( ITr2ShaderMaterial* effect, unsigned groupDimX, unsigned groupDimY, unsigned groupDimZ, Tr2RenderContext& renderContext );
	static bool RunComputeShaderIndirect( ITr2ShaderMaterial* effect, Tr2GpuBufferAL& indirectParams, unsigned offset, Tr2RenderContext& renderContext );

	// ***** Note: consider Tr2PushPopRT instead *****
	static void PushRenderTarget( Tr2RenderContext& renderContext );	// does not set any RT, just stores the current one so it can be safely changed later
	static void PushRenderTarget( unsigned slot, Tr2RenderContext& renderContext );	// does not set any RT, just stores the current one so it can be safely changed later
	static void PushRenderTarget( const Tr2RenderTargetAL& rt, Tr2RenderContext& renderContext );
	static void PushRenderTarget( const Tr2RenderTargetAL& rt, unsigned slot, Tr2RenderContext& renderContext );
	static void PopRenderTarget( Tr2RenderContext& renderContext );
	static void PopRenderTarget( unsigned slot, Tr2RenderContext& renderContext );
	static bool SetRenderTarget( unsigned int index, const Tr2RenderTargetAL& rt, Tr2RenderContext& renderContext, bool updateViewport = true );

	// ***** Note: consider Tr2PushPopDS instead *****
	static bool PushDepthStencilBuffer( Tr2RenderContext& renderContext );	// does not set a DS, just stores it so it can be safely changed later
	static bool PushDepthStencilBuffer( const Tr2DepthStencilAL& ds, Tr2RenderContext& renderContext );
	static void PopDepthStencilBuffer( Tr2RenderContext& renderContext );
	static bool SetDepthStencilBuffer( const Tr2DepthStencilAL& ds, Tr2RenderContext& renderContext );

	static void PushProjection();
	static void PopProjection();
	static void PushViewport();
	static void PopViewport();
	static void PushViewTransform();
	static void PopViewTransform();

	static void SetTbbWorkerThreadCount( int threads );

	static TriSettings& GetSettings();

	// Material Support
	static void UpdateMaterials();

	// Used to detect if the creation of resources is allowed
	static bool IsResourceCreationAllowed();
	// Used to prevent python triggered device resets from within a resetting context
	static bool IsDeviceResetting();
	// Used to prevent python triggered device resets from within a resetting context
	// Will cause certain python calls to raise exceptions
	static void SetIsDeviceResetting( bool resetInProgress );


private:

	static void SetResourceCreationAllowed( bool isAllowed );
	
	friend class TriDevice;

};

#endif // Tr2Renderer_H