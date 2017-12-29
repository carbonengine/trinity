#include "StdAfx.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Tr2Variable.h"
#include "TriDevice.h"
#include "TriLineSet.h"
#include "TriError.h"
#include "TriSettingsRegistrar.h"

#if TBB_ENABLED
#include "tbb/task_scheduler_init.h"
#endif

#include "TriPoolAllocator.h"

#if defined( __ORBIS__)
#include "TrinityAL/Tr2MemoryAllocator.h"
CCP_STATS_DECLARE( garlicAvailable, "Trinity/GarlicAvailable", false, CST_MEMORY, "Bytes of memory available on garlic bus" );
CCP_STATS_DECLARE( onionAvailable, "Trinity/OnionAvailable", false, CST_MEMORY, "Bytes of memory available on onion bus" );
#endif

using namespace Tr2RenderContextEnum;

bool Tr2Renderer::m_disableGeometryLoad		= false;
bool Tr2Renderer::m_disableTextureLoad		= false;
bool Tr2Renderer::m_disableEffectLoad		= false;
bool Tr2Renderer::m_disableAsyncLoad		= false;

extern unsigned long g_currentFrameCounter;

// Expose this for Tr2RenderTargetALDx9.cpp.
// The whole s_blitter thing needs to be rethough anyway.
Tr2Blitter* s_blitter = NULL;

Tr2Variable s_renderTimeVar;

namespace
{
	TR2SHADERMODEL s_shaderModel = TR2SM_3_0_HI;
	TR2SHADERMODEL s_prevShaderModel = TR2SM_3_0_HI;

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
	unsigned int s_perFrameVSStartRegister = 220;
	unsigned int s_perFramePSStartRegister = 200;
	unsigned int s_perObjectVSStartRegister = 16;
	unsigned int s_perObjectPSStartRegister = 40;
	unsigned int s_perObjectVSFFEStartRegister = 180;
	unsigned int s_perObjectVSGUIStartRegister = 5;
#elif( TRINITY_PLATFORM == TRINITY_DIRECTX11 )
	// these are actually constant buffer indices
	unsigned int s_perFrameVSStartRegister  = 1;
	unsigned int s_perFramePSStartRegister  = 2;
	unsigned int s_perObjectVSStartRegister = 3;
	unsigned int s_perObjectPSStartRegister = 4;
	unsigned int s_perObjectVSFFEStartRegister = 5;
	unsigned int s_perObjectVSGUIStartRegister = 6;
#elif( TRINITY_PLATFORM==TRINITY_OPENGLES2 )
	unsigned int s_perFrameVSStartRegister  = 1;
	unsigned int s_perFramePSStartRegister  = 2;
	unsigned int s_perObjectVSStartRegister = 3;
	unsigned int s_perObjectPSStartRegister = 4;
	unsigned int s_perObjectVSFFEStartRegister = 5;
	unsigned int s_perObjectVSGUIStartRegister = 6;
#elif( TRINITY_PLATFORM==TRINITY_ORBIS)
	// these are actually constant buffer indices
	unsigned int s_perFrameVSStartRegister  = 1;
	unsigned int s_perFramePSStartRegister  = 2;
	unsigned int s_perObjectVSStartRegister = 3;
	unsigned int s_perObjectPSStartRegister = 4;
	unsigned int s_perObjectVSFFEStartRegister = 5;
	unsigned int s_perObjectVSGUIStartRegister = 6;
#elif( TRINITY_PLATFORM==TRINITY_STUB)
	// these are actually constant buffer indices
	unsigned int s_perFrameVSStartRegister  = 1;
	unsigned int s_perFramePSStartRegister  = 2;
	unsigned int s_perObjectVSStartRegister = 3;
	unsigned int s_perObjectPSStartRegister = 4;
	unsigned int s_perObjectVSFFEStartRegister = 5;
	unsigned int s_perObjectVSGUIStartRegister = 6;
#elif( TRINITY_PLATFORM==TRINITY_OPENGL4 )
	unsigned int s_perFrameVSStartRegister  = 1;
	unsigned int s_perFramePSStartRegister  = 2;
	unsigned int s_perObjectVSStartRegister = 3;
	unsigned int s_perObjectPSStartRegister = 4;
	unsigned int s_perObjectVSFFEStartRegister = 5;
	unsigned int s_perObjectVSGUIStartRegister = 6;
#else
#	error "Missing platform"
#endif

	TriPoolAllocator* s_poolAllocator = NULL;
	// keep an array of directories which are to exclude from texture-sizing
	std::vector<std::string> s_dirsToExclude;

	PROJECTION_TYPE s_currentProjectionType = PT_PERSPECTIVE;

    Matrix s_projectionTransform;
	Matrix s_projectionRawTransform;
    Matrix s_inverseProjectionTransform;

    Matrix s_viewTransform;
    Matrix s_inverseViewTransform;

	Matrix s_viewProjectionTransform;

    Matrix s_worldTransform;
    Matrix s_inverseWorldTransform;

    float s_frontClip = 0.0f;
    float s_backClip = 0.0f;
    float s_frustumRadius = 0.0f;
    float s_aspectRatio = 0.0f;
    float s_fieldOfView = 0.0f;
	float s_orthoWidth = 0.0f;
	float s_orthoHeight = 0.0f;
	
	void UpdateProjectionParameters( const Matrix& proj )
	{
		// Use the fact that:
		// aspect = m_22 / m_11;
		// m_33 = z_f/(z_f-z_n), m_43 = -z_n*z_f/(z_f-z_n)
		// => front = z_n = -m_43/m_33 
		// => back = z_f = -m_43/(1+m_43/z_n)
		// m_22 = cotan(fov/2) = 1 / tan(fov/2)
		// => fov = 2*tan(1/m_22) 
		s_aspectRatio =	(proj._11	 ?  proj._22/proj._11 : 0.0f);
		s_fieldOfView = (proj._22	 ?  2.0f*atan(1.0f/proj._22) : 0.0f);
		s_frontClip =	(proj._33	 ? proj._43/proj._33 : 0.0f);
		s_backClip =	proj._43/(1.0f+proj._33);
	}

	float s_pickingOffsetX = 0.0f;
	float s_pickingOffsetY = 0.0f;
	float s_pickingWidth = 0.0f;
	float s_pickingHeight = 0.0f;

	CTriViewport s_viewport;
	// When the s_viewport extends beyond the render target bounds we clip the viewport
	// explicitly and create a projection adjustment matrix.
	Tr2Viewport s_viewportOnDevice;
	Matrix s_viewport2projectionAdjustment;

	struct viewportStackItem {
		CTriViewport viewport;
		Tr2Viewport  viewportOnDevice;
		Matrix projectionAdjustment;

		viewportStackItem( CTriViewport v, Tr2Viewport c, Matrix a ) :
			viewport( v ),
			viewportOnDevice( c ),
			projectionAdjustment( a )
			{};
	};


	int s_renderTargetWidth = 0;
	int s_renderTargetHeight = 0;

    TriDebugTextRenderer* s_debugTextRenderer = NULL;	

	TriLineSet* s_debugLineSet = NULL;

	// shared vertex/index buffers
	Tr2BufferAL s_quadVertexBuffer;
	Tr2BufferAL  s_quadListIndexBuffer;
	unsigned int s_quadListSize = 0;

	Tr2TextureAL s_fallbackTextures[2][3];
	bool s_debugFallbackTexture = false;

	std::list<Matrix> s_projectionStack;
	std::list<Matrix> s_viewTransformStack;
	std::list<viewportStackItem> s_viewportStack;

	// Handles to variable store items, used to pass engine parameters to effects	
	Tr2Variable s_projectionMatrixVar;
	Tr2Variable s_viewMatrixVar;
	Tr2Variable s_projectionMatrixInvVar;
	Tr2Variable s_viewMatrixInvVar;
	Tr2Variable s_viewProjectionMatrixVar;
	Tr2Variable s_worldMatrixVar;
	Tr2Variable s_viewportSizeVar;

	Tr2Variable s_frustumPlanes[6];

#if TBB_ENABLED
	tbb::task_scheduler_init* s_taskScheduler = NULL;
#endif

	// To deal with the adjusted viewport that D3D9 forces us to create when viewport
	// extends beyond the screen
	void AdjustTextureCoordsToViewport( Vector2& topLeft, Vector2& bottomRight )
	{
		Vector2 tlTexCoordAdjusted = topLeft;

		int deltaX = (int)s_viewportOnDevice.m_x - s_viewport.x;
		int deltaY = (int)s_viewportOnDevice.m_y - s_viewport.y;

		float xOffset = float(deltaX) /  s_viewport.width;
		float yOffset = float(deltaY) /  s_viewport.height;

		tlTexCoordAdjusted.x += xOffset * bottomRight.x;
		tlTexCoordAdjusted.y += yOffset * bottomRight.y;

		Vector2 brTexCoordAdjusted;
		brTexCoordAdjusted = bottomRight;
		brTexCoordAdjusted.x *= float(s_viewportOnDevice.m_width + deltaX) / s_viewport.width;
		brTexCoordAdjusted.y *= float(s_viewportOnDevice.m_height + deltaY) / s_viewport.height;

        topLeft = tlTexCoordAdjusted;
        bottomRight = brTexCoordAdjusted;
	}

	typedef std::set<Tr2Effect *> EffectSet;
	EffectSet& GetEffectSet()
	{
		static NeverEndingSingleton<EffectSet> effectSet;
		return effectSet.GetInstance();
	}

	void ReloadShaders()
	{
        typedef std::set<Tr2EffectRes*> EffectResSet;
        EffectResSet effectResources;

		EffectSet& l = GetEffectSet();
		for( EffectSet::iterator i = l.begin(), end = l.end(); i != end; ++i )
		{
            Tr2Effect* effect( *i );
            Tr2EffectRes* effectResource = effect->GetEffectRes();

            if( effectResource && 
                effectResources.find( effectResource ) == effectResources.end() )
            {
                effectResources.insert( effectResource );
            }
		}

    	for( EffectResSet::iterator i = effectResources.begin(), 
            end = effectResources.end(); i != end; ++i )
		{
            Tr2EffectRes* effectResource( *i );
            effectResource->Reload();
        }
    }

    void UpdateViewProjectionTransform()
	{
		s_viewProjectionTransform = s_viewTransform * s_projectionTransform;
		s_viewProjectionMatrixVar = s_viewProjectionTransform;

		s_frustumPlanes[0] = Vector4( s_viewProjectionTransform._13,
									  s_viewProjectionTransform._23,
									  s_viewProjectionTransform._33,
									  s_viewProjectionTransform._43 );
		s_frustumPlanes[1] = Vector4( s_viewProjectionTransform._14 + s_viewProjectionTransform._11,
									  s_viewProjectionTransform._24 + s_viewProjectionTransform._21,
									  s_viewProjectionTransform._34 + s_viewProjectionTransform._31,
									  s_viewProjectionTransform._44 + s_viewProjectionTransform._41 );
		s_frustumPlanes[2] = Vector4( s_viewProjectionTransform._14 - s_viewProjectionTransform._12,
									  s_viewProjectionTransform._24 - s_viewProjectionTransform._22,
									  s_viewProjectionTransform._34 - s_viewProjectionTransform._32,
									  s_viewProjectionTransform._44 - s_viewProjectionTransform._42 );
		s_frustumPlanes[3] = Vector4( s_viewProjectionTransform._14 - s_viewProjectionTransform._11,
									  s_viewProjectionTransform._24 - s_viewProjectionTransform._21,
									  s_viewProjectionTransform._34 - s_viewProjectionTransform._31,
									  s_viewProjectionTransform._44 - s_viewProjectionTransform._41 );
		s_frustumPlanes[4] = Vector4( s_viewProjectionTransform._14 + s_viewProjectionTransform._12,
									  s_viewProjectionTransform._24 + s_viewProjectionTransform._22,
									  s_viewProjectionTransform._34 + s_viewProjectionTransform._32,
									  s_viewProjectionTransform._44 + s_viewProjectionTransform._42 );
		s_frustumPlanes[5] = Vector4( s_viewProjectionTransform._14 + s_viewProjectionTransform._13,
									  s_viewProjectionTransform._24 + s_viewProjectionTransform._23,
									  s_viewProjectionTransform._34 + s_viewProjectionTransform._33,
									  s_viewProjectionTransform._44 + s_viewProjectionTransform._43 );
	}

	void SetProjectionDerivedValues()
	{
	    s_projectionRawTransform = s_projectionTransform;

		// Apply scaling and offset to projection matrix. If the viewport extends
		// the render target we need to clip the viewport and thus scale+offset the
		// projection.
		s_projectionTransform = s_projectionTransform * s_viewport2projectionAdjustment;

		// Cache inverse projection matrix
		s_inverseProjectionTransform = Inverse( s_projectionTransform );

		Vector4 corner( 1.f, 1.f, 1.f, 1.f );
		corner = Transform( corner, s_inverseProjectionTransform );
		Vector3 viewCorner( corner.x / corner.w, corner.y / corner.w, corner.z / corner.w );
		s_frustumRadius = Length( viewCorner );

		// Ensure TriVariable store is aware of the projection transform. Used by some debugging shaders.
		s_projectionMatrixVar = s_projectionTransform;
		s_projectionMatrixInvVar = s_inverseProjectionTransform;
		UpdateViewProjectionTransform();
	}

	void CreateFallbackTexturesWithColor( uint32_t color, Tr2TextureAL* outputTextures, Tr2PrimaryRenderContext& renderContext )
	{
		Tr2SubresourceData initialData[CUBEMAP_FACE_COUNT];
		for( int i = 0; i < CUBEMAP_FACE_COUNT; ++i )
		{
			initialData[i].m_sysMemPitch = 4;
			initialData[i].m_sysMemSlicePitch = 4;
			initialData[i].m_sysMem = &color;
		}
		outputTextures[0].Create2D( 1, 1, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, USAGE_IMMUTABLE, initialData, renderContext );
		outputTextures[1].CreateVolume( 1, 1, 1, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, USAGE_IMMUTABLE, initialData, renderContext );
		outputTextures[2].CreateCube( 1, 1, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, USAGE_IMMUTABLE, initialData, renderContext );
	}

	void DestroyFallbackTextures( Tr2TextureAL* fallbackTextures )
	{
		fallbackTextures[0].Destroy();
		fallbackTextures[1].Destroy();
		fallbackTextures[2].Destroy();
	}

	void CreateFallbackTextures( Tr2PrimaryRenderContext& renderContext )
	{
		if( s_debugFallbackTexture )
		{
			CreateFallbackTexturesWithColor( 0xffff00ff, s_fallbackTextures[0], renderContext );
			CreateFallbackTexturesWithColor( 0x8800ff00, s_fallbackTextures[1], renderContext );
		}
		else
		{
			CreateFallbackTexturesWithColor( 0, s_fallbackTextures[0], renderContext );
			DestroyFallbackTextures( s_fallbackTextures[1] );
		}
	}
}

bool Tr2Renderer::Initialize()
{
#if TBB_ENABLED
	s_taskScheduler = CCP_NEW( "Tr2Renderer/s_taskScheduler" ) tbb::task_scheduler_init;
#endif
	s_debugTextRenderer = CCP_NEW( "Tr2Renderer/s_debugTextRenderer" ) TriDebugTextRenderer;

	s_renderTimeVar			 .Register(  "Time",				Vector4(0.0f, 0.0f, 0.0f, 0.0f) );
	s_worldMatrixVar		 .Register(  "WorldMat",			s_worldTransform );
	s_viewMatrixVar.Register( "ViewMat", s_viewTransform );
	s_viewMatrixInvVar.Register( "ViewInvMat", s_viewTransform );
	s_projectionMatrixVar.Register( "ProjectionMat", s_projectionTransform );
	s_projectionMatrixInvVar.Register( "ProjectionInvMat", s_projectionTransform );
	s_viewProjectionMatrixVar.Register(  "ViewProjectionMat", s_viewProjectionTransform );
	s_viewportSizeVar		 .Register(  "ViewportSize",		Vector4(0.0f, 0.0f, 1.0f, 0.0f) );

	s_frustumPlanes[0].Register( "FrustumPlane0", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	s_frustumPlanes[1].Register( "FrustumPlane1", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	s_frustumPlanes[2].Register( "FrustumPlane2", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	s_frustumPlanes[3].Register( "FrustumPlane3", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	s_frustumPlanes[4].Register( "FrustumPlane4", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );
	s_frustumPlanes[5].Register( "FrustumPlane5", Vector4( 0.0f, 0.0f, 1.0f, 0.0f ) );

	s_poolAllocator = CCP_NEW( "Tr2Renderer/s_poolAllocator" ) TriPoolAllocator();

	USE_MAIN_THREAD_RENDER_CONTEXT();	//TODO!
	renderContext.m_esm.Initialize();

	s_viewport2projectionAdjustment = IdentityMatrix();

	return true;
}

void Tr2Renderer::Shutdown()
{
	GlobalStore().UnregisterVariable( "Time" );
	USE_MAIN_THREAD_RENDER_CONTEXT();	//TODO!
	renderContext.m_esm.Shutdown();

	CCP_DELETE s_blitter;
	CCP_DELETE s_debugTextRenderer;
	CCP_DELETE s_poolAllocator;

	if( s_debugLineSet )
	{
		s_debugLineSet->Unlock();
		s_debugLineSet = NULL;
	}

#if TBB_ENABLED
	CCP_DELETE s_taskScheduler;
#endif
}

void Tr2Renderer::GetBackBufferDimensions( unsigned int& w, unsigned int& h )
{
    w = unsigned( gTriDev->mWidth );
    h = unsigned( gTriDev->mHeight );
}

MAP_FUNCTION_AND_WRAP( "ReloadShaders", ReloadShaders, "Forces Trinity to reload all shaders.");

static bool IsRightHanded()
{
	return true;
}

MAP_FUNCTION_AND_WRAP( "IsRightHanded", IsRightHanded, "Returns true if Trinity rendering uses a right-handed coordinate system.");

void Tr2Renderer::AdjustProjection( const Vector2& scaling, const Vector2& translation )
{
	s_projectionTransform = s_projectionRawTransform;
	Matrix& proj = s_projectionTransform;
	proj._11 *= scaling.x;
	proj._22 *= scaling.y;
	proj._31 = scaling.x*proj._31 + proj._34*translation.x;
	proj._32 = scaling.y*proj._32 + proj._34*translation.y;

	SetProjectionDerivedValues();
}


void Tr2Renderer::SetPerspectiveProjection( float fov, float front, float back, float asp )
{
	s_currentProjectionType = PT_PERSPECTIVE;
    s_fieldOfView = fov;
    s_frontClip   = front;
    s_backClip    = back;
    s_aspectRatio = asp;

	s_projectionTransform = PerspectiveFovMatrix( s_fieldOfView, s_aspectRatio, s_frontClip, s_backClip );

	SetProjectionDerivedValues();
}

void Tr2Renderer::SetPerspectiveProjection( float left, float right, float bottom, float top, float front, float back )
{
	s_currentProjectionType = PT_PERSPECTIVE;
	s_projectionTransform = PerspectiveOffCenterMatrix( left, right, bottom, top, front, back );

	UpdateProjectionParameters( s_projectionTransform );

	SetProjectionDerivedValues();
}

void Tr2Renderer::SetOrthoProjection( float width, float height, float front, float back )
{
	s_currentProjectionType = PT_ORTHOGONAL;

	s_projectionTransform = OrthoMatrix( width, height, front, back );

	s_orthoWidth = width;
	s_orthoHeight = height;
	s_aspectRatio =	(height ? width/height : 1.0f );
	s_fieldOfView = 1.0f;
	s_frontClip =	front;
	s_backClip =	back;

	SetProjectionDerivedValues();
}

void Tr2Renderer::SetOrthoProjection( float left, float right, float bottom, float top, float front, float back )
{
	s_currentProjectionType = PT_ORTHOGONAL;

	s_projectionTransform = OrthoOffCenterMatrix( left, right, bottom, top, front, back );

	s_orthoWidth = right-left;
	s_orthoHeight = top-bottom;
	s_aspectRatio =	(s_orthoHeight ? s_orthoWidth/s_orthoHeight : 1.0f );
	s_fieldOfView = 1.0f;
	s_frontClip =	front;
	s_backClip =	back;
	SetProjectionDerivedValues();
}

void Tr2Renderer::SetProjectionTransform( const Matrix& proj )
{
	s_currentProjectionType = PT_UNKNOWN;
	s_projectionTransform = proj;
	UpdateProjectionParameters( proj );
	SetProjectionDerivedValues();
}

const PROJECTION_TYPE Tr2Renderer::GetCurrentProjectionType()
{
	return s_currentProjectionType;
}

const Matrix& Tr2Renderer::GetProjectionTransform()
{
    return s_projectionTransform;
}

Matrix Tr2Renderer::GetReversedDepthProjectionTransform()
{
	Matrix proj = s_projectionTransform;
	proj._33 = -proj._33 - 1.f;
	proj._43 = -proj._43;
	return proj;
}

const Matrix& Tr2Renderer::GetProjectionRawTransform()
{
    return s_projectionRawTransform;
}

Vector3 Tr2Renderer::ProjectWorldToScreen( const Vector3& worldPos, const Tr2Viewport& vp )
{
	Vector4 p( worldPos.x, worldPos.y, worldPos.z, 1.0f );
	p = p * GetViewTransform();
	p = p * GetProjectionTransform();
	if( p.w )
	{
		p *= 1.0f / p.w;
	}

	return Vector3(		vp.m_x + vp.m_width * ( 0.5f + 0.5f * p.x ),
						vp.m_y + vp.m_height* ( 0.5f - 0.5f * p.y ),
						vp.m_minZ + p.z * vp.m_maxZ );
}

const Matrix& Tr2Renderer::GetInverseProjectionTransform()
{
    return s_inverseProjectionTransform;
}

float Tr2Renderer::GetFrontClip()
{
    return s_frontClip;
}

float Tr2Renderer::GetBackClip()
{
    return s_backClip;
}

float Tr2Renderer::GetFrustumRadius()
{
    return s_frustumRadius;
}

void Tr2Renderer::GetFrustumPlane( size_t index, Vector4& plane )
{
	s_frustumPlanes[index].GetValue( plane );
}

float Tr2Renderer::GetFieldOfView()
{
    return s_fieldOfView;
}

float Tr2Renderer::GetAspectRatio()
{
    return s_aspectRatio;
}

float Tr2Renderer::GetOrthoWidth()
{
	return s_orthoWidth;
}

float Tr2Renderer::GetOrthoHeight()
{
	return s_orthoHeight;
}

unsigned int Tr2Renderer::GetRenderTargetWidth()
{
	return s_renderTargetWidth;
}

unsigned int Tr2Renderer::GetRenderTargetHeight()
{
	return s_renderTargetHeight;
}

void Tr2Renderer::SetWorldTransform( const Matrix& m )
{
    s_worldTransform = m;

	// Ensure variable store is aware of the change
	s_worldMatrixVar = s_worldTransform;
}

void Tr2Renderer::SetViewTransform( const Matrix& m )
{
    s_viewTransform = m;
	s_inverseViewTransform = Inverse( s_viewTransform );

	UpdateViewProjectionTransform();

	// Ensure variable store is aware of the change
	s_viewMatrixVar = s_viewTransform;
	s_viewMatrixInvVar = s_inverseViewTransform;

}
const Matrix& Tr2Renderer::GetWorldTransform()
{
	return s_worldTransform;
}

const Matrix& Tr2Renderer::GetViewTransform()
{
    return s_viewTransform;
}

const Matrix& Tr2Renderer::GetInverseViewTransform()
{
    return s_inverseViewTransform;
}

const Vector3& Tr2Renderer::GetViewPosition()
{
    return *(Vector3*)(&s_inverseViewTransform._41);
}

const Vector3& Tr2Renderer::GetViewLookAt()
{
    static Vector3 v;
    v.x = s_viewTransform._13;
    v.y = s_viewTransform._23;
    v.z = s_viewTransform._33;
    return v;
}

Vector4 ColorToVec4( uint32_t color )
{
	uint8_t r = ( color >> 16 ) & 0xffu;
	uint8_t g = ( color >>  8 ) & 0xffu;
	uint8_t b = ( color >>  0 ) & 0xffu;
	uint8_t a = ( color >> 24 ) & 0xffu;
	return Vector4( r, g, b, a ) / 255.0f;
}

void Tr2Renderer::Printf( int x, int y, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
	va_start( args, msg );

	Rect rect;
	rect.top = y;
	rect.left = x;
	rect.bottom = rect.top + 512;
	rect.right = rect.left + 1024;
	s_debugTextRenderer->Vprintf( TRI_DBG_FONT_SMALL, rect, TRI_DFS_LEFT, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::PrintfImmediate( int x, int y, uint32_t color, uint32_t format, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
	va_start( args, msg );

	Tr2Viewport viewport;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.GetViewport( viewport );
	
	Rect rect;
	if( format & TRI_DFS_RIGHT )
	{
		rect.top = y + (int32_t)viewport.m_y;
		rect.left = x + (int32_t)viewport.m_x - 1024;
		rect.bottom = rect.top + 512;
		rect.right = x + (int32_t)viewport.m_x;
	}
	else
	{
		rect.top = y + (int32_t)viewport.m_y;
		rect.left = x + (int32_t)viewport.m_x;
		rect.bottom = rect.top + 512;
		rect.right = rect.left + 1024;
	}
	s_debugTextRenderer->VprintfImmediate( TRI_DBG_FONT_SMALL, rect, format, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::Printf( TriDebugFont font, const Rect& rect, uint32_t format, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

    va_list args;
    va_start( args, msg );
    s_debugTextRenderer->Vprintf( font, rect, format, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::PrintfImmediate( TriDebugFont font, const Rect& rect, uint32_t format, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
	va_start( args, msg );
	s_debugTextRenderer->VprintfImmediate( font, rect, format, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::Printf( TriDebugFont font, const Vector3& pos, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
    va_start( args, msg );

    Rect rect;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	Tr2Viewport vp;
	renderContext.GetViewport( vp );

	Vector3 screenPos = ProjectWorldToScreen( pos, vp );

	if( (screenPos.z > 0.0f) && (screenPos.z < 1.0f) )
	{
		rect.top = (int32_t)screenPos.y;
		rect.left = (int32_t)screenPos.x;
		rect.bottom = rect.top + 512;
		rect.right = rect.left + 1024;

		s_debugTextRenderer->Vprintf( font, rect, TRI_DFS_LEFT, ColorToVec4( color ), msg, args );
	}
}

void Tr2Renderer::Printf( TriDebugFont font, int fontStyle, const Vector3& pos, Vector4 color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
    va_start( args, msg );

    Rect rect;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	Tr2Viewport vp;
	renderContext.GetViewport( vp );

	Vector3 screenPos = ProjectWorldToScreen( pos, vp );

	if( !( (screenPos.z > 0.0f) && (screenPos.z < 1.0f) ) )
	{
		return;
	}

	rect.top = (int32_t)screenPos.y - 128;
	rect.left = (int32_t)screenPos.x - 128;
	rect.bottom = rect.top + 256;
	rect.right = rect.left + 256;

	s_debugTextRenderer->Vprintf( font, rect, fontStyle, color, msg, args );
}

bool Tr2Renderer::DrawTexture( Tr2Material* effect, Tr2TextureAL& texture )
{
	if( s_blitter )
	{
		return s_blitter->Draw( effect, texture );
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   draws a unit quad with the given shader 
// Arguments:
//   shader - the ShaderMaterial to use.
// Return Value:
//   true if success, false if no blitter available.
// --------------------------------------------------------------------------------------

bool Tr2Renderer::DrawFullScreenWithShader( Tr2Material * shader )
{
	if( s_blitter )
	{
		return s_blitter->Draw( shader );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, Tr2Blitter::Filtering filter )
{
	if( s_blitter )
	{
		Vector2 tlTexCoordAdjusted = tlTexCoord;
		Vector2 brTexCoordAdjusted = brTexCoord;
		AdjustTextureCoordsToViewport( tlTexCoordAdjusted, brTexCoordAdjusted );

		return s_blitter->Draw( texture, tlTexCoordAdjusted, brTexCoordAdjusted, filter );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2Material* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord )
{
	if( s_blitter )
	{
		Vector2 tlTexCoordAdjusted = tlTexCoord;
		Vector2 brTexCoordAdjusted = brTexCoord;
		AdjustTextureCoordsToViewport( tlTexCoordAdjusted, brTexCoordAdjusted );

		return s_blitter->Draw( effect, texture, tlTexCoordAdjusted, brTexCoordAdjusted );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2Material* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord )
{
	if( s_blitter )
	{
		Vector2 tlTexCoordAdjusted = tlTexCoord;
		Vector2 brTexCoordAdjusted = brTexCoord;
		AdjustTextureCoordsToViewport( tlTexCoordAdjusted, brTexCoordAdjusted );

		return s_blitter->Draw( effect, tlTexCoordAdjusted, brTexCoordAdjusted );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2Material* effect, Tr2TextureAL& texture,
                               const Vector2& tlTexCoord, const Vector2& brTexCoord,
                               const Vector2& tlVertexCoord, const Vector2& brVertexCoord )
{
    if( s_blitter )
    {
        Vector2 tlTexCoordAdjusted = tlTexCoord;
        Vector2 brTexCoordAdjusted = brTexCoord;
        AdjustTextureCoordsToViewport( tlTexCoordAdjusted, brTexCoordAdjusted );

        if( effect )
        {
            return s_blitter->Draw( effect, texture, 
                                    tlTexCoordAdjusted, brTexCoordAdjusted, 
                                    tlVertexCoord, brVertexCoord );
        }
        else
        {
            return s_blitter->Draw( texture, tlTexCoordAdjusted, brTexCoordAdjusted, 
                                    tlVertexCoord, brVertexCoord );
        }
    }
    return false;
}

bool Tr2Renderer::DrawCubeTexture( Tr2TextureAL& texture, Tr2RenderContextEnum::CubemapFace face, unsigned int mipLevel )
{
	if( s_blitter )
	{
		return s_blitter->DrawCube( texture, face, mipLevel );
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Execute compute shaders in all passes of the specified effect. The function will
//   unset bound UAVs after each pass.
// Arguments:
//   effect - Effect containing compute shaders
//   groupDim# - group grid sizes
//   renderContext - current render context
// Return Value:
//   true On success
//   false On failure
// --------------------------------------------------------------------------------------
bool Tr2Renderer::RunComputeShader( Tr2Material* effect,
									unsigned groupDimX, 
									unsigned groupDimY, 
									unsigned groupDimZ, 
									Tr2RenderContext& renderContext )
{
	if( !effect )
	{
		return false;
	}
	auto shader = effect->GetShaderStateInterface();
	if( !shader )
	{
		return false;
	}
	bool result = false;
	auto& desc = shader->GetEffectDescription();
	if( desc.techniques.empty() )
	{
		return false;
	}
	auto& technique = desc.techniques[0];
	for( unsigned i = 0; i < technique.passes.size(); ++i )
	{
		auto& stage = technique.passes[i].stageInputs[COMPUTE_SHADER];
		if( stage.m_exists )
		{
			shader->ApplyAllStateForPass( 0, i, renderContext );
			effect->ApplyMaterialDataForPass( 0, i, renderContext );
			CR_RETURN_VAL( renderContext.RunComputeShader( groupDimX, groupDimY, groupDimZ ), false );
			// Unset UAVs
			const Tr2EffectResourceMap& uavs = stage.uavs;
			for( auto it = uavs.begin(); it != uavs.end(); ++it )
			{
				CR_RETURN_VAL( renderContext.SetUav( COMPUTE_SHADER, it->first, Tr2BufferAL() ), false );
			}
			result = true;
		}
	}
	return result;
}

// --------------------------------------------------------------------------------------
bool Tr2Renderer::RunComputeShader( 
	Tr2Material* effect,
	const BlueSharedString& techniqueName,
	unsigned groupDimX,
	unsigned groupDimY,
	unsigned groupDimZ,
	Tr2RenderContext& renderContext )
{
	if( !effect )
	{
		return false;
	}
	auto shader = effect->GetShaderStateInterface();
	if( !shader )
	{
		return false;
	}
	uint32_t techniqueIndex;
	if( !shader->GetTechniqueIndex( techniqueName, techniqueIndex ) )
	{
		return false;
	}
	bool result = false;
	auto& desc = shader->GetEffectDescription();
	if( desc.techniques.empty() )
	{
		return false;
	}
	auto& technique = desc.techniques[techniqueIndex];
	for( unsigned i = 0; i < technique.passes.size(); ++i )
	{
		auto& stage = technique.passes[i].stageInputs[COMPUTE_SHADER];
		if( stage.m_exists )
		{
			shader->ApplyAllStateForPass( techniqueIndex, i, renderContext );
			effect->ApplyMaterialDataForPass( techniqueIndex, i, renderContext );
			CR_RETURN_VAL( renderContext.RunComputeShader( groupDimX, groupDimY, groupDimZ ), false );
			// Unset UAVs
			const Tr2EffectResourceMap& uavs = stage.uavs;
			for( auto it = uavs.begin(); it != uavs.end(); ++it )
			{
				CR_RETURN_VAL( renderContext.SetUav( COMPUTE_SHADER, it->first, Tr2BufferAL() ), false );
			}
			result = true;
		}
	}
	return result;
}

// --------------------------------------------------------------------------------------
// Description:
//   Execute compute shaders in all passes of the specified effect. Group drid dimensions
//   are taken from indirectParams GPU buffer. The function will unset bound UAVs after 
//   each pass.
// Arguments:
//   effect - Effect containing compute shaders
//   indirectParams - GPU buffer containing group dimensions
//   offset - byte offset into indirectParams buffer
//   renderContext - current render context
// Return Value:
//   true On success
//   false On failure
// --------------------------------------------------------------------------------------
bool Tr2Renderer::RunComputeShaderIndirect( Tr2Material* effect, Tr2BufferAL& indirectParams, unsigned offset, Tr2RenderContext& renderContext )
{
	if( !effect )
	{
		return false;
	}
	auto shader = effect->GetShaderStateInterface();
	if( !shader )
	{
		return false;
	}

	auto& desc = shader->GetEffectDescription();
	if( desc.techniques.empty() )
	{
		return false;
	}
	auto& technique = desc.techniques[0];
	for( size_t i = 0; i < technique.passes.size(); ++i )
	{
		auto& stage = technique.passes[i].stageInputs[COMPUTE_SHADER];
		if( stage.m_exists )
		{
			shader->ApplyAllStateForPass( 0, uint32_t( i ), renderContext );
			effect->ApplyMaterialDataForPass( 0, uint32_t( i ), renderContext );
			CR_RETURN_VAL( renderContext.RunComputeShaderIndirect( indirectParams, offset ), false );
			// Unset UAVs
			const Tr2EffectResourceMap& uavs = stage.uavs;
			for( auto it = uavs.begin(); it != uavs.end(); ++it )
			{
				CR_RETURN_VAL( renderContext.SetUav( COMPUTE_SHADER, it->first, Tr2BufferAL() ), false );
			}
		}
	}
	return true;
}

bool Tr2Renderer::PushDepthStencilBuffer( Tr2RenderContext& renderContext )
{
	return SUCCEEDED( renderContext.PushDepthStencil() );
}

bool Tr2Renderer::PushDepthStencilBuffer( const Tr2DepthStencilAL& ds, Tr2RenderContext& renderContext )
{
	renderContext.PushDepthStencil();

	return Tr2Renderer::SetDepthStencilBuffer( ds, renderContext );
}

void Tr2Renderer::PopDepthStencilBuffer( Tr2RenderContext& renderContext )
{
	renderContext.PopDepthStencil();
}

void Tr2Renderer::PushRenderTarget( Tr2RenderContext& renderContext )
{
	renderContext.PushRenderTarget();
}

void Tr2Renderer::PushRenderTarget( unsigned slot, Tr2RenderContext& renderContext )
{
	renderContext.PushRenderTarget( slot );
}

void Tr2Renderer::PushRenderTarget( const Tr2RenderTargetAL& rt, Tr2RenderContext& renderContext )
{
	PushRenderTarget( rt, 0, renderContext );
}

void Tr2Renderer::PushRenderTarget( const Tr2RenderTargetAL& rt, unsigned slot, Tr2RenderContext& renderContext )
{
	renderContext.PushRenderTarget( slot );
	Tr2Renderer::SetRenderTarget( slot, rt, renderContext );
}


void UpdateRenderTargetViewport( unsigned width, unsigned height )
{
	CCP_ASSERT( width > 0 );
	CCP_ASSERT( height > 0 );

	// Keep record of the render target's width and height. This is used when
	// checking whether the viewport extends beyond the render target or not.
	s_renderTargetWidth  = width;
	s_renderTargetHeight = height;

	// Reset the viewport
	s_viewportOnDevice.m_x		= 0;
	s_viewportOnDevice.m_y		= 0;
	s_viewportOnDevice.m_height = (float)height;
	s_viewportOnDevice.m_width	= (float)width;
	s_viewport.height			= height;
	s_viewport.width			= width;
	s_viewport.x				= 0;
	s_viewport.y				= 0;
	s_viewport.minZ = s_viewportOnDevice.m_minZ = 0.0f;
	s_viewport.maxZ = s_viewportOnDevice.m_maxZ = 1.0f;

	s_viewportSizeVar = Vector4( s_viewportOnDevice.m_width, s_viewportOnDevice.m_height, (float)s_renderTargetWidth, (float)s_renderTargetHeight );

	// Reset the projection adjustment
	s_viewport2projectionAdjustment = IdentityMatrix();
}

void Tr2Renderer::PopRenderTarget( Tr2RenderContext& renderContext )
{
	PopRenderTarget( 0, renderContext );
}

void Tr2Renderer::PopRenderTarget( unsigned slot, Tr2RenderContext& renderContext )
{
	renderContext.PopRenderTarget( slot );

	unsigned width, height;
	if( slot == 0 && SUCCEEDED( renderContext.GetRenderTargetSize( width, height ) ) )
	{
		UpdateRenderTargetViewport( width, height );
	}
}

void Tr2Renderer::PushProjection()
{
	s_projectionStack.push_front( s_projectionRawTransform );
}

void Tr2Renderer::PopProjection()
{
	CCP_ASSERT( !s_projectionStack.empty() );

	s_projectionTransform = s_projectionStack.front();
	s_projectionStack.pop_front();
	UpdateProjectionParameters( s_projectionTransform );
    SetProjectionDerivedValues();
}

void Tr2Renderer::PushViewport()
{
	const viewportStackItem item( s_viewport, s_viewportOnDevice, s_viewport2projectionAdjustment );
	s_viewportStack.push_front( item );
}

void Tr2Renderer::PopViewport()
{
	CCP_ASSERT( !s_viewportStack.empty() );

	viewportStackItem item = s_viewportStack.front();
	s_viewportStack.pop_front();

	SetViewport( item.viewport );
}

void Tr2Renderer::PushViewTransform()
{
	s_viewTransformStack.push_front( s_viewTransform );
}

void Tr2Renderer::PopViewTransform()
{
	CCP_ASSERT( !s_viewTransformStack.empty() );

	const Matrix view = s_viewTransformStack.front();
	s_viewTransformStack.pop_front();

	SetViewTransform( view );
}


void Tr2Renderer::DrawScreenQuad( Tr2Material* effect )
{
	if( s_blitter )
	{
		s_blitter->Draw( effect );
	}
}

void Tr2Renderer::DrawScreenQuad( Tr2Effect* effect, const Vector2 &topLeft, const Vector2 &bottomRight )
{
	if( s_blitter )
	{
		s_blitter->Draw( effect, Vector2(0,0), Vector2(1,1), topLeft, bottomRight );
	}
}

void Tr2Renderer::DrawCameraSpaceScreenQuad( Tr2Shader* shader, Tr2Material* material )
{
	if( s_blitter )
	{
		s_blitter->DrawInCameraSpace( shader, material );
	}
}

float Tr2Renderer::GetAnimationTime()
{
	return gTriDev->GetAnimationTime();
}

float Tr2Renderer::GetAnimationTimeElapsed( float startTime )
{
	return gTriDev->GetAnimationTimeElapsed( startTime );
}

void Tr2Renderer::BeginFrame()
{
	Vector4 timeDataOld;
	s_renderTimeVar.GetValue( timeDataOld );

	Vector4 timeData;
	timeData.x = GetAnimationTime();
	timeData.y = timeData.x - floorf(timeData.x);
	timeData.z = static_cast<float>(GetCurrentFrameCounter());
	timeData.w = timeDataOld.x;
	s_renderTimeVar = timeData;
}

void Tr2Renderer::EndFrame()
{
	if( s_debugTextRenderer )
	{
		s_debugTextRenderer->Clear();
	}

	if( s_debugLineSet )
	{
		s_debugLineSet->Clear();
	}

#if defined( __ORBIS__)
	CCP_STATS_SET( garlicAvailable, Tr2MemoryAllocator::GetGarlicAvailableMemory() );
	CCP_STATS_SET( onionAvailable, Tr2MemoryAllocator::GetOnionAvailableMemory() );
#endif
}

HRESULT Tr2Renderer::BeginRenderContext()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.BeginScene();
}

HRESULT Tr2Renderer::EndRenderContext()
{
	if( s_poolAllocator )
	{
		s_poolAllocator->Clear();
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.EndScene();
}

TriPoolAllocator* Tr2Renderer::GetPoolAllocator()
{
	return s_poolAllocator;
}

unsigned long Tr2Renderer::GetCurrentFrameCounter()
{
	return g_currentFrameCounter;
}

void Tr2Renderer::ClearDepthBuffer( float z /*= 1.0f */ )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.Clear( CLEARFLAGS_ZBUFFER, 0, z, 0 );
}

void Tr2Renderer::RegisterEffect( Tr2Effect* f )
{
	GetEffectSet().insert( f );
}

void Tr2Renderer::UnregisterEffect( Tr2Effect* f )
{
	GetEffectSet().erase( f );
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-initialized all the registered effects
// See Also:
//   RegisterEffect, UnregisterEffect
// --------------------------------------------------------------------------------------
void Tr2Renderer::ReinitializeRegisteredEffects()
{
	EffectSet& l = GetEffectSet();
	for( EffectSet::iterator it = l.begin(); it != l.end(); ++it )
	{
		(*it)->Initialize();
	}
}

void Tr2Renderer::SetShaderModel( TR2SHADERMODEL sm )
{
	bool changed = ( s_shaderModel != sm );

	s_prevShaderModel = s_shaderModel;
	s_shaderModel = sm;

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )	//TODO GLES2 DX11 etc
	switch( sm )
	{
		case TR2SM_1_1:
			s_perFrameVSStartRegister = 80;
			s_perFramePSStartRegister = 0;
			s_perObjectVSStartRegister = 16;
			s_perObjectPSStartRegister = 0;
			break;

		case TR2SM_2_0_LO:
		case TR2SM_2_0_HI:
			s_perFrameVSStartRegister = 220;
			s_perFramePSStartRegister = 16;
			s_perObjectVSStartRegister = 16;
			s_perObjectPSStartRegister = 14;
			break;

		case TR2SM_3_0_LO:
		case TR2SM_3_0_HI:
		case TR2SM_3_0_DEPTH:
		case TR2SM_AUTHORING:
			s_perFrameVSStartRegister = 220;
			s_perFramePSStartRegister = 200;
			s_perObjectVSStartRegister = 16;
			s_perObjectPSStartRegister = 40;
			break;
	}
#endif	// else just leave it alone

	// if changed, please perform a device reset, cause many things must be changed
	if( changed )
	{
		// todo: is this the best way here?
		if( gTriDev && gTriDev->DeviceExists() )
		{
			gTriDev->ResetDevice();
		}

		// Reinitialize registered Tr2Effects
		ReinitializeRegisteredEffects();
	}
}

TR2SHADERMODEL Tr2Renderer::GetShaderModel()
{
	return s_shaderModel;
}

TR2SHADERMODEL Tr2Renderer::GetMaxShaderModelSupported()
{
#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
	USE_MAIN_THREAD_RENDER_CONTEXT();
	// if not yet initialized, return max
	if( !renderContext.IsValid() )
	{
		return TR2SM_3_0_HI;
	}
	unsigned int psVer = renderContext.GetCaps().GetShaderVersion();
	if(psVer >= 3)
		return TR2SM_3_0_HI;
	else if(psVer == 2)
		return TR2SM_2_0_LO;
	// problem!
	return TR2SM_1_1;
#else
	return TR2SM_3_0_HI;
#endif
}

const char* Tr2Renderer::GetShaderModelString( TR2SHADERMODEL sm )
{
	static const char* smStrings[] =
	{
		"SM_1_1",
		"SM_2_0_LO",
		"SM_2_0_HI",
		"SM_3_0_LO",
		"SM_3_0_HI",
		"SM_3_0_DEPTH",
		"SM_AUTHORING"
	};

	CCP_ASSERT( sm < TR2SM_COUNT );
	return smStrings[sm];
}

void Tr2Renderer::AddMipLevelSkipExclusionDirectory(const char* path)
{
	// make it upper-case, so all future comparing can be done with upper-case
	std::string checkDir( path );
	std::transform(checkDir.begin(), checkDir.end(), checkDir.begin(), toupper);
	// add to array
	s_dirsToExclude.push_back(checkDir);
}

bool Tr2Renderer::IsTextureToResize(const char* filename)
{
	// convert to upper case
	std::string checkFilename( filename );
	std::transform(checkFilename.begin(), checkFilename.end(), checkFilename.begin(), toupper);
	// check it with all dirs in the array
	for(std::vector<std::string>::const_iterator it = s_dirsToExclude.begin(); it != s_dirsToExclude.end(); it++)
	{
		// compare first part of filename against dir-array
		if(strncmp(it->c_str(), checkFilename.c_str(), it->length()) == 0)
			return false;
	}
	// not found, so can do a resize!
	return true;
}

bool Tr2Renderer::IsLowQuality(void)
{
	// is this low quality rendering? Decide here
	return ( GetShaderModel() <= TR2SM_3_0_LO );
}

unsigned int Tr2Renderer::GetPerFrameVSStartRegister()
{
	return s_perFrameVSStartRegister;
}

unsigned int Tr2Renderer::GetPerFramePSStartRegister()
{
	return s_perFramePSStartRegister;
}

unsigned int Tr2Renderer::GetPerObjectVSStartRegister()
{
	return s_perObjectVSStartRegister;
}

unsigned int Tr2Renderer::GetPerObjectVSFFEStartRegister()
{
	return s_perObjectVSFFEStartRegister;
}

unsigned int Tr2Renderer::GetPerObjectVSGUIStartRegister()
{
	return s_perObjectVSGUIStartRegister;
}

unsigned int Tr2Renderer::GetPerObjectPSStartRegister()
{
	return s_perObjectPSStartRegister;
}

Tr2BufferAL* Tr2Renderer::GetQuadListIndexBuffer( unsigned int numOfQuads )
{
	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return nullptr;
	}

	// if the requested size is same or smaller than the one we already have allocated, just return buffer
	if( numOfQuads <= s_quadListSize && s_quadListIndexBuffer.IsValid() )
	{
		return &s_quadListIndexBuffer;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// someone wants more quads in this buffer, so we have to re-allocate
	
	// index buffer with indices for many quads (particles, etc.)

	std::vector<unsigned short> indices( numOfQuads * 6 );
	unsigned short* pInds = &indices[0];
	for(unsigned int i = 0; i < numOfQuads; ++i)
	{
		pInds[0] = 0 + 4 * i;
		pInds[1] = 2 + 4 * i;
		pInds[2] = 1 + 4 * i;
		pInds[3] = 0 + 4 * i;
		pInds[4] = 3 + 4 * i;
		pInds[5] = 2 + 4 * i;
		pInds += 6;
	}
		
	HRESULT hr = s_quadListIndexBuffer.Create( 2, numOfQuads * 6, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, &indices[0], renderContext );
	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Tr2Renderer::GetQuadListIndexBuffer failed to create index buffer (%d)", hr );
		return NULL;
	}

	// ok we have more quads now
	s_quadListSize = numOfQuads;

	return &s_quadListIndexBuffer;
}

void Tr2Renderer::SetTbbWorkerThreadCount( int threads )
{	
#if TBB_ENABLED
	tbb::task_scheduler_init* taskScheduler = s_taskScheduler;
	taskScheduler->terminate();
	taskScheduler->initialize( threads );
#endif
}

void Tr2Renderer::PrepareDeviceResources()
{
	if( !s_blitter )
	{
		// Blitter will load an effect. Initializing it here ensures that
		// the device is ready and we have established what shader model
		// can be used.
		s_blitter = CCP_NEW( "Tr2Renderer/s_blitter" ) Tr2Blitter;
	}

	if( !s_debugLineSet )
	{
		BeClasses->CreateInstance( GetTriLineSetClsid(), BlueInterfaceIID<TriLineSet>(), (void**)&s_debugLineSet );
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// allocate and fill globals vertex/index buffers
	// just a vertex buffer with an index-like vertex, counting from 0 to 3
	float pVerts[4] = { 0, 1, 2, 3 };
	HRESULT hr = s_quadVertexBuffer.Create( sizeof( float ), 4, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::NONE, pVerts, renderContext );
	if( FAILED( hr ) )
	{
		CCP_LOGERR( "Tr2Renderer::PrepareDeviceResources failed to create quad vertex buffer (%d)", hr );
		return;
	}

	// just call the Get* function, it will do the alloc...
	GetQuadListIndexBuffer( 1 );

	CreateFallbackTextures( renderContext );
}

void Tr2Renderer::ReleaseDeviceResources( TriStorage s )
{
	if( ( s & s_quadVertexBuffer.GetMemoryClass() ) || ( s & s_quadListIndexBuffer.GetMemoryClass() ) )
	{
		s_quadVertexBuffer = Tr2BufferAL();
		s_quadListIndexBuffer = Tr2BufferAL();
		s_quadListSize = 0;
	}
	DestroyFallbackTextures( s_fallbackTextures[0] );
	DestroyFallbackTextures( s_fallbackTextures[1] );
}

void Tr2Renderer::DrawLine( const Vector3& from, const Vector3& to, uint32_t color /*= 0xffffffff */ )
{
	if( s_debugLineSet )
	{
		s_debugLineSet->Add( from, color, to, color );
	}
}

void Tr2Renderer::DrawLine( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor )
{
	if( s_debugLineSet )
	{
		s_debugLineSet->Add( from, fromColor, to, toColor );
	}
}

void Tr2Renderer::DrawSphere( const Vector3& center, float radius, int segments, uint32_t color /*= 0xffffffff */ )
{
	if( s_debugLineSet )
	{
		s_debugLineSet->AddSphere( center, radius, segments, color );
	}
}

void Tr2Renderer::DrawSphere( const Vector4& sphere, int segments, uint32_t color /*= 0xffffffff */ )
{
	if( s_debugLineSet )
	{
		s_debugLineSet->AddSphere( Vector3( sphere.x, sphere.y, sphere.z ), sphere.w, segments, color );
	}
}

void Tr2Renderer::DrawBox( const Vector3& min, const Vector3& max, uint32_t color /*= 0xffffffff */ )
{
	if( s_debugLineSet )
	{
		s_debugLineSet->AddBox( min, max, color );
	}
}

void Tr2Renderer::DrawOrientedBox( const Matrix& boxMatrix, uint32_t color /*= 0xffffffff */ )
{
	if( s_debugLineSet )
	{
		s_debugLineSet->AddOrientedBox( boxMatrix, color );
	}
}

void Tr2Renderer::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	if( s_debugTextRenderer )
	{
		s_debugTextRenderer->Render();
		s_debugTextRenderer->Clear();
	}

	if( s_debugLineSet )
	{
		s_debugLineSet->Render( renderContext );
		s_debugLineSet->Clear();
	}
}

static void SetupViewport()
{
	// See whether this viewport extends beyond the render target. If so,
	// we need to clip it.

	int x0 = s_viewport.x;
	int y0 = s_viewport.y;
	int x1 = s_viewport.x + s_viewport.width;
	int y1 = s_viewport.y + s_viewport.height;

	s_viewportOnDevice.m_x	= std::max( (float)x0, 0.0f );
	s_viewportOnDevice.m_y	= std::max( (float)y0, 0.0f );

    // Viewport width and height must be greater than zero. Using zero edge 
    // length causes dx error.
	s_viewportOnDevice.m_width  = (float)std::max( std::min( x1, s_renderTargetWidth ) - int(s_viewportOnDevice.m_x), 1 );
	s_viewportOnDevice.m_height = (float)std::max( std::min( y1, s_renderTargetHeight ) - int(s_viewportOnDevice.m_y), 1 );

	s_viewportOnDevice.m_minZ   = s_viewport.minZ;
	s_viewportOnDevice.m_maxZ   = s_viewport.maxZ;

	// In case we shrunk the viewport we need to scale and/or offset the
	// projection matrix

	float invWidth	= 1.0f / s_viewportOnDevice.m_width;
	float invHeight = 1.0f / s_viewportOnDevice.m_height;

	s_viewport2projectionAdjustment._11 = s_viewport.width * invWidth;;
	s_viewport2projectionAdjustment._22 = s_viewport.height * invHeight;
	s_viewport2projectionAdjustment._31 = (s_viewport.width - s_viewportOnDevice.m_width) * invWidth;
	s_viewport2projectionAdjustment._32 = (s_viewport.height - s_viewportOnDevice.m_height) * invHeight;

	if( s_viewport.x < 0 )
	{
		s_viewport2projectionAdjustment._31 *= -1;
	}

	if( s_viewport.y + s_viewport.height > s_renderTargetHeight )
	{
		s_viewport2projectionAdjustment._32 *= -1;
	}

	s_viewportSizeVar = Vector4( (float)s_viewport.width, (float)s_viewport.height, (float)s_renderTargetWidth, (float)s_renderTargetHeight );

	// Finally, set the (possibly clipped) viewport on the D3D device.
	USE_MAIN_THREAD_RENDER_CONTEXT();
	renderContext.SetViewport( s_viewportOnDevice );
}

void Tr2Renderer::SetFullScreenViewport()
{
	s_viewport.width  = s_renderTargetWidth;
	s_viewport.height = s_renderTargetHeight;
	s_viewport.x      = 0;
	s_viewport.y      = 0;
	s_viewport.minZ   = 0.0f;
	s_viewport.maxZ   = 1.0f;

	SetupViewport();
}

void Tr2Renderer::SetViewport( const TriViewport& vp )
{
	// As s_viewport is a CTriViewport we do a "member copy" on the original
	// viewport we got passed in
	SetViewport( vp.width, vp.height, vp.x, vp.y, vp.minZ, vp.maxZ );
}

void Tr2Renderer::SetViewport( int width, int height, int x, int y, float minZ, float maxZ )
{
	// As s_viewport is a CTriViewport we fill this struct
	s_viewport.width  = width;
	s_viewport.height = height;
	s_viewport.x      = x;
	s_viewport.y      = y;
	s_viewport.minZ   = minZ;
	s_viewport.maxZ   = maxZ;

	SetupViewport();
}

const TriViewport& Tr2Renderer::GetViewport()
{
	return s_viewport;
}

const Tr2Viewport& Tr2Renderer::GetDeviceViewport()
{
	return s_viewportOnDevice;
}

bool Tr2Renderer::SetRenderTarget( unsigned int index, const Tr2RenderTargetAL& rt, Tr2RenderContext& renderContext, bool updateViewport )
{
	if( FAILED( renderContext.SetRenderTarget( rt, index ) ) )
	{
		CCP_LOGERR( "Failed to set renderTarget to slot %d", index );
		return false;
	}

	if( !index && updateViewport )
	{
		unsigned width, height;
		if( SUCCEEDED( renderContext.GetRenderTargetSize( width, height ) ) ) 
		{	
			// don't use rt.GetWidth/Height, rt may be nullRT
			UpdateRenderTargetViewport( width, height );
		} else {
			CCP_LOGERR( "Could not update viewport for render target index 0, GetRenderTargetSize failed");
		}
	}

	return true;
}

bool Tr2Renderer::SetDepthStencilBuffer( const Tr2DepthStencilAL& ds, Tr2RenderContext& renderContext )
{
	if( !SUCCEEDED( renderContext.SetDepthStencil( ds ) ) )
	{
		CCP_LOGERR( "Failed to set depth/stencil buffer");
		return false;
	}

	return true;
}

TriSettings& Tr2Renderer::GetSettings()
{
	static CTriSettings s; // C-object & singleton, no reference counting magic!
	return s;
}

bool Tr2Renderer::IsResourceCreationAllowed()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.IsValid();
}

void Tr2Renderer::EnableFallbackTextureDebugging()
{
	if( !s_debugFallbackTexture )
	{
		s_debugFallbackTexture = true;
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CreateFallbackTextures( renderContext );
	}
}

void Tr2Renderer::DisableFallbackTextureDebugging()
{
	if( s_debugFallbackTexture )
	{
		s_debugFallbackTexture = false;
		USE_MAIN_THREAD_RENDER_CONTEXT();
		CreateFallbackTextures( renderContext );
	}
}

const Tr2TextureAL& Tr2Renderer::GetFallbackTexture( Tr2EffectResource::Type textureType, const char* debugContext )
{
	uint32_t textureSet = 0;
	if( s_debugFallbackTexture )
	{
		CCP_LOGWARN( "Using fallback texture for %s", debugContext );
		textureSet = ( uint32_t( GetAnimationTime() * 20 ) % 2 ) * 3;
	}
	switch( textureType )
	{
	case Tr2EffectResource::TEXTURE_1D:
	case Tr2EffectResource::TEXTURE_2D:
		return s_fallbackTextures[textureSet][0];
		break;
	case Tr2EffectResource::TEXTURE_3D:
		return s_fallbackTextures[textureSet][1];
		break;
	case Tr2EffectResource::TEXTURE_CUBE:
		return s_fallbackTextures[textureSet][2];
		break;
	default:
		return nullTX;
	}
}
