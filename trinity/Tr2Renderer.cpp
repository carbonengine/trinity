#include "StdAfx.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Tr2Variable.h"
#include "TriDevice.h"
#include "TriLineSet.h"
#include "TriSettingsRegistrar.h"
#include "TriPoolAllocator.h"

using namespace Tr2RenderContextEnum;

bool Tr2Renderer::m_disableGeometryLoad		= false;
bool Tr2Renderer::m_disableTextureLoad		= false;
bool Tr2Renderer::m_disableEffectLoad		= false;
bool Tr2Renderer::m_disableAsyncLoad		= false;

extern unsigned long long g_currentFrameCounter;

// Expose this for Tr2RenderTargetALDx9.cpp.
// The whole s_blitter thing needs to be rethough anyway.
Tr2Blitter* s_blitter = NULL;

Tr2Variable s_renderTimeVar;

namespace
{
	// This flag is used to prevent code from inadvertently creating device resources in the middle of a device reset/invalidate attempt
	bool s_isResourceCreationAllowed = true;

	TR2SHADERMODEL s_shaderModel = TR2SM_3_0_HI;
	TR2SHADERMODEL s_prevShaderModel = TR2SM_3_0_HI;

	// these are actually constant buffer indices
	unsigned int s_perFrameVSStartRegister  = 1;
	unsigned int s_perFramePSStartRegister  = 2;
	unsigned int s_perObjectVSStartRegister = 3;
	unsigned int s_perObjectPSStartRegister = 4;
	unsigned int s_perObjectVSGUIStartRegister = 6;


	TriPoolAllocator* s_poolAllocator = NULL;
	// keep an array of directories which are to exclude from texture-sizing
	std::vector<std::string> s_dirsToExclude;

	// ShaderOptions, only GEOMETRY_SHADER_SUPPORT at the moment
	const size_t s_shaderOptionCount = 1;
	Tr2ShaderOption s_shaderOptions[s_shaderOptionCount];
	bool s_shaderOptionsInitialized = false;

	PROJECTION_TYPE s_currentProjectionType = PT_PERSPECTIVE;

	Matrix s_projectionTransform;
	Matrix s_projectionRawTransform;
	Matrix s_inverseProjectionTransform;
	Matrix s_viewport2projectionAdjustment;

	Matrix s_viewTransform;
	Matrix s_inverseViewTransform;

	Matrix s_viewProjectionTransform;

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
		s_aspectRatio = ( proj._11 ? proj._22 / proj._11 : 0.0f );
		s_fieldOfView = ( proj._22 ? 2.0f*atan( 1.0f / proj._22 ) : 0.0f );
		s_frontClip = ( proj._33 ? proj._43 / proj._33 : 0.0f );
		s_backClip = proj._43 / ( 1.0f + proj._33 );
	}

    Matrix s_worldTransform;
    Matrix s_inverseWorldTransform;


	float s_pickingOffsetX = 0.0f;
	float s_pickingOffsetY = 0.0f;
	float s_pickingWidth = 0.0f;
	float s_pickingHeight = 0.0f;


    TriDebugTextRenderer* s_debugTextRenderer = NULL;	

	TriLineSet* s_debugLineSet = NULL;

	// shared vertex/index buffers
	Tr2BufferAL s_quadVertexBuffer;
	// Various sizes of index buffers
	// 16 bit
	Tr2BufferAL s_quadListIndexBuffer16Bit;
	uint16_t s_quadListSize16Bit = 0;
	// 32 bit
	Tr2BufferAL s_quadListIndexBuffer32Bit;
	uint32_t s_quadListSize32Bit = 0;

	Tr2TextureAL s_fallbackTextures[2][3];
	bool s_debugFallbackTexture = false;

	std::list<std::pair<Matrix, Matrix>> s_projectionStack;
	std::list<Matrix> s_viewTransformStack;

	Tr2Variable s_projectionMatrixVar;
	Tr2Variable s_viewMatrixVar;
	Tr2Variable s_projectionMatrixInvVar;
	Tr2Variable s_viewMatrixInvVar;
	Tr2Variable s_viewProjectionMatrixVar;
	Tr2Variable s_worldMatrixVar;
	Tr2Variable s_frustumPlanes[6];


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
		outputTextures[0].Create( Tr2BitmapDimensions( 1, 1, 1, PIXEL_FORMAT_B8G8R8A8_UNORM ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData, renderContext );
		outputTextures[1].Create( Tr2BitmapDimensions( TEX_TYPE_3D, PIXEL_FORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, initialData, renderContext );
		outputTextures[2].Create( Tr2BitmapDimensions( TEX_TYPE_CUBE, PIXEL_FORMAT_B8G8R8A8_UNORM, 1, 1, 1, 1 ), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::READ, initialData, renderContext );
	}

	void DestroyFallbackTextures( Tr2TextureAL* fallbackTextures )
	{
		fallbackTextures[0] = Tr2TextureAL();
		fallbackTextures[1] = Tr2TextureAL();
		fallbackTextures[2] = Tr2TextureAL();
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

	// To deal with the adjusted viewport that D3D9 forces us to create when viewport
	// extends beyond the screen
	void AdjustTextureCoordsToViewport( Tr2RenderContext& renderContext, Vector2& topLeft, Vector2& bottomRight )
	{
		Vector2 tlTexCoordAdjusted = topLeft;

		auto& viewportOnDevice = renderContext.m_esm.GetDeviceViewport();
		auto& viewport = renderContext.m_esm.GetViewport();

		int deltaX = (int)viewportOnDevice.m_x - viewport.x;
		int deltaY = (int)viewportOnDevice.m_y - viewport.y;

		float xOffset = float( deltaX ) / viewport.width;
		float yOffset = float( deltaY ) / viewport.height;

		tlTexCoordAdjusted.x += xOffset * bottomRight.x;
		tlTexCoordAdjusted.y += yOffset * bottomRight.y;

		Vector2 brTexCoordAdjusted;
		brTexCoordAdjusted = bottomRight;
		brTexCoordAdjusted.x *= float( viewportOnDevice.m_width + deltaX ) / viewport.width;
		brTexCoordAdjusted.y *= float( viewportOnDevice.m_height + deltaY ) / viewport.height;

		topLeft = tlTexCoordAdjusted;
		bottomRight = brTexCoordAdjusted;
	}

	
	template <typename T>
	bool CreateIndexBuffer( T count, Tr2BufferAL& buffer )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();
		// Re-create the index buffer with the correct type

		std::vector<T> indices( count * 6 );
		T* pInds = &indices[0];
		for( T i = 0; i < count; ++i )
		{
			pInds[0] = 0 + 4 * i;
			pInds[1] = 2 + 4 * i;
			pInds[2] = 1 + 4 * i;
			pInds[3] = 0 + 4 * i;
			pInds[4] = 3 + 4 * i;
			pInds[5] = 2 + 4 * i;
			pInds += 6;
		}
		HRESULT hr = buffer.Create( sizeof( T ), count * 6, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::NONE, &indices[0], renderContext );
		if( FAILED( hr ) )
		{
			CCP_LOGERR( "CreateIndexBuffer failed to create index buffer (%d)", hr );
			return false;
		}
		return true;
	}

}

bool Tr2Renderer::Initialize()
{
	s_debugTextRenderer = CCP_NEW( "Tr2Renderer/s_debugTextRenderer" ) TriDebugTextRenderer;
	s_renderTimeVar			 .Register(  "Time",				Vector4(0.0f, 0.0f, 0.0f, 0.0f) );
	s_worldMatrixVar		 .Register(  "WorldMat",			s_worldTransform );
	s_viewMatrixVar.Register( "ViewMat", s_viewTransform );
	s_viewMatrixInvVar.Register( "ViewInvMat", s_viewTransform );
	s_projectionMatrixVar.Register( "ProjectionMat", s_projectionTransform );
	s_projectionMatrixInvVar.Register( "ProjectionInvMat", s_projectionTransform );
	s_viewProjectionMatrixVar.Register( "ViewProjectionMat", s_viewProjectionTransform );

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


void Tr2Renderer::SetPerspectiveProjection( float fov, float front, float back, float asp, const Matrix& viewportAdjustment )
{
	s_currentProjectionType = PT_PERSPECTIVE;
	s_fieldOfView = fov;
	s_frontClip = front;
	s_backClip = back;
	s_aspectRatio = asp;

	s_projectionTransform = PerspectiveFovMatrix( s_fieldOfView, s_aspectRatio, s_frontClip, s_backClip );
	s_viewport2projectionAdjustment = viewportAdjustment;

	SetProjectionDerivedValues();
}

void Tr2Renderer::SetPerspectiveProjection( float left, float right, float bottom, float top, float front, float back, const Matrix& viewportAdjustment )
{
	s_currentProjectionType = PT_PERSPECTIVE;
	s_projectionTransform = PerspectiveOffCenterMatrix( left, right, bottom, top, front, back );
	s_viewport2projectionAdjustment = viewportAdjustment;

	UpdateProjectionParameters( s_projectionTransform );

	SetProjectionDerivedValues();
}

void Tr2Renderer::SetOrthoProjection( float width, float height, float front, float back, const Matrix& viewportAdjustment )
{
	s_currentProjectionType = PT_ORTHOGONAL;

	s_projectionTransform = OrthoMatrix( width, height, front, back );
	s_viewport2projectionAdjustment = viewportAdjustment;

	s_orthoWidth = width;
	s_orthoHeight = height;
	s_aspectRatio = ( height ? width / height : 1.0f );
	s_fieldOfView = 1.0f;
	s_frontClip = front;
	s_backClip = back;

	SetProjectionDerivedValues();
}

void Tr2Renderer::SetOrthoProjection( float left, float right, float bottom, float top, float front, float back, const Matrix& viewportAdjustment )
{
	s_currentProjectionType = PT_ORTHOGONAL;

	s_projectionTransform = OrthoOffCenterMatrix( left, right, bottom, top, front, back );
	s_viewport2projectionAdjustment = viewportAdjustment;

	s_orthoWidth = right - left;
	s_orthoHeight = top - bottom;
	s_aspectRatio = ( s_orthoHeight ? s_orthoWidth / s_orthoHeight : 1.0f );
	s_fieldOfView = 1.0f;
	s_frontClip = front;
	s_backClip = back;
	SetProjectionDerivedValues();
}

void Tr2Renderer::SetProjectionTransform( const Matrix& proj, const Matrix& viewportAdjustment )
{
	s_currentProjectionType = PT_UNKNOWN;
	s_projectionTransform = proj;
	s_viewport2projectionAdjustment = viewportAdjustment;
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

	return Vector3( vp.m_x + vp.m_width * ( 0.5f + 0.5f * p.x ),
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
	return *(Vector3*)( &s_inverseViewTransform._41 );
}

Vector3 Tr2Renderer::GetViewLookAt()
{
	Vector3 v;
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

	Tr2Rect rect;
	rect.top = y;
	rect.left = x;
	rect.bottom = rect.top + 512;
	rect.right = rect.left + 1024;
	s_debugTextRenderer->Vprintf( TRI_DBG_FONT_SMALL, rect, TRI_DFS_LEFT, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::PrintfImmediate( Tr2RenderContext& renderContext, int x, int y, uint32_t color, uint32_t format, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
	va_start( args, msg );

	Tr2Viewport viewport;
	renderContext.GetViewport( viewport );
	
	Tr2Rect rect;
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
	s_debugTextRenderer->VprintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, format, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::Printf( TriDebugFont font, const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

    va_list args;
    va_start( args, msg );
    s_debugTextRenderer->Vprintf( font, rect, format, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::PrintfImmediate( Tr2RenderContext& renderContext, TriDebugFont font, const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
	va_start( args, msg );
	s_debugTextRenderer->VprintfImmediate( renderContext, font, rect, format, ColorToVec4( color ), msg, args );
}

void Tr2Renderer::Printf( TriDebugFont font, const Vector3& pos, uint32_t color, const char* msg, ... )
{
	if( !s_debugTextRenderer )
	{
		return;
	}

	va_list args;
    va_start( args, msg );

	Tr2Rect rect;
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

	Tr2Rect rect;
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

bool Tr2Renderer::DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, Tr2TextureAL& texture )
{
	if( s_blitter )
	{
		return s_blitter->Draw( renderContext, effect, texture );
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

bool Tr2Renderer::DrawFullScreenWithShader( Tr2RenderContext& renderContext, Tr2Material * shader )
{
	if( s_blitter )
	{
		return s_blitter->Draw( renderContext, shader );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2RenderContext& renderContext, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord, Tr2Blitter::Filtering filter )
{
	if( s_blitter )
	{
		Vector2 tlTexCoordAdjusted = tlTexCoord;
		Vector2 brTexCoordAdjusted = brTexCoord;
		AdjustTextureCoordsToViewport( renderContext, tlTexCoordAdjusted, brTexCoordAdjusted );

		return s_blitter->Draw( renderContext, texture, tlTexCoordAdjusted, brTexCoordAdjusted, filter );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, Tr2TextureAL& texture, const Vector2& tlTexCoord, const Vector2& brTexCoord )
{
	if( s_blitter )
	{
		Vector2 tlTexCoordAdjusted = tlTexCoord;
		Vector2 brTexCoordAdjusted = brTexCoord;
		AdjustTextureCoordsToViewport( renderContext, tlTexCoordAdjusted, brTexCoordAdjusted );

		return s_blitter->Draw( renderContext, effect, texture, tlTexCoordAdjusted, brTexCoordAdjusted );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, const Vector2& tlTexCoord, const Vector2& brTexCoord )
{
	if( s_blitter )
	{
		Vector2 tlTexCoordAdjusted = tlTexCoord;
		Vector2 brTexCoordAdjusted = brTexCoord;
		AdjustTextureCoordsToViewport( renderContext, tlTexCoordAdjusted, brTexCoordAdjusted );

		return s_blitter->Draw( renderContext, effect, tlTexCoordAdjusted, brTexCoordAdjusted );
	}
	return false;
}

bool Tr2Renderer::DrawTexture( Tr2RenderContext& renderContext, Tr2Material* effect, Tr2TextureAL& texture,
                               const Vector2& tlTexCoord, const Vector2& brTexCoord,
                               const Vector2& tlVertexCoord, const Vector2& brVertexCoord )
{
    if( s_blitter )
    {
        Vector2 tlTexCoordAdjusted = tlTexCoord;
        Vector2 brTexCoordAdjusted = brTexCoord;
        AdjustTextureCoordsToViewport( renderContext, tlTexCoordAdjusted, brTexCoordAdjusted );

        if( effect )
        {
            return s_blitter->Draw( renderContext, effect, texture,
                                    tlTexCoordAdjusted, brTexCoordAdjusted, 
                                    tlVertexCoord, brVertexCoord );
        }
        else
        {
            return s_blitter->Draw( renderContext, texture, tlTexCoordAdjusted, brTexCoordAdjusted,
                                    tlVertexCoord, brVertexCoord );
        }
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
		}
	}
	return true;
}

void Tr2Renderer::PushProjection()
{
	s_projectionStack.push_front( std::make_pair( s_projectionRawTransform, s_viewport2projectionAdjustment ) );
}

void Tr2Renderer::PopProjection()
{
	CCP_ASSERT( !s_projectionStack.empty() );

	auto transforms = s_projectionStack.front();
	s_projectionTransform = transforms.first;
	s_viewport2projectionAdjustment = transforms.second;
	s_projectionStack.pop_front();
	UpdateProjectionParameters( s_projectionTransform );
	SetProjectionDerivedValues();
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


void Tr2Renderer::DrawScreenQuad( Tr2RenderContext& renderContext, Tr2Material* effect )
{
	if( s_blitter )
	{
		s_blitter->Draw( renderContext, effect );
	}
}

void Tr2Renderer::DrawScreenQuad( Tr2RenderContext& renderContext, Tr2Effect* effect, const Vector2 &topLeft, const Vector2 &bottomRight )
{
	if( s_blitter )
	{
		s_blitter->Draw( renderContext, effect, Vector2(0,0), Vector2(1,1), topLeft, bottomRight );
	}
}

void Tr2Renderer::DrawCameraSpaceScreenQuad( Tr2RenderContext& renderContext, Tr2Shader* shader, Tr2Material* material )
{
	if( s_blitter )
	{
		s_blitter->DrawInCameraSpace( renderContext, shader, material );
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
	Vector4 timeDataOld( 0, 0, 0, 0 );
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

unsigned long long Tr2Renderer::GetCurrentFrameCounter()
{
	return g_currentFrameCounter;
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

unsigned int Tr2Renderer::GetPerObjectVSGUIStartRegister()
{
	return s_perObjectVSGUIStartRegister;
}

unsigned int Tr2Renderer::GetPerObjectPSStartRegister()
{
	return s_perObjectPSStartRegister;
}

Tr2BufferAL* Tr2Renderer::GetQuadListIndexBuffer( uint32_t numOfQuads )
{
	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return nullptr;
	}

	// how many indices do these quads need?
	uint32_t numOfIndices = numOfQuads * 6;

	// need to check if the index count fits in the buffers
	if( (uint16_t)numOfIndices == numOfIndices )
	{
		// we just need the 16 bit buffer
		if( numOfQuads <= s_quadListSize16Bit && s_quadListIndexBuffer16Bit.IsValid() )
		{
			return &s_quadListIndexBuffer16Bit;
		}
		if( CreateIndexBuffer<uint16_t>( (uint16_t)numOfQuads, s_quadListIndexBuffer16Bit ) )
		{
			s_quadListSize16Bit = numOfQuads;
			return &s_quadListIndexBuffer16Bit;
		}
	}
	else 
	{
		// we need the big one!
		if( numOfQuads <= s_quadListSize32Bit && s_quadListIndexBuffer32Bit.IsValid() )
		{
			return &s_quadListIndexBuffer32Bit;
		}
		if( CreateIndexBuffer<uint32_t>( (uint32_t)numOfQuads, s_quadListIndexBuffer32Bit ) )
		{
			s_quadListSize32Bit = numOfQuads;
			return &s_quadListIndexBuffer32Bit;
		}
	}

	return NULL;
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
	if( ( s & s_quadVertexBuffer.GetMemoryClass() ) || 
		( s & s_quadListIndexBuffer16Bit.GetMemoryClass() ) ||  
		( s & s_quadListIndexBuffer32Bit.GetMemoryClass() ) )
	{
		s_quadVertexBuffer = Tr2BufferAL();
		s_quadListIndexBuffer16Bit = Tr2BufferAL();
		s_quadListSize16Bit = 0;
		s_quadListIndexBuffer32Bit = Tr2BufferAL();
		s_quadListSize32Bit = 0;
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
		s_debugTextRenderer->Render( renderContext );
		s_debugTextRenderer->Clear();
	}

	if( s_debugLineSet )
	{
		s_debugLineSet->Render( renderContext );
		s_debugLineSet->Clear();
	}
}

const TriViewport& Tr2Renderer::GetViewport()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.m_esm.GetViewport();
}

TriSettings& Tr2Renderer::GetSettings()
{
	static CTriSettings s; // C-object & singleton, no reference counting magic!
	return s;
}

bool Tr2Renderer::IsResourceCreationAllowed()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	return renderContext.IsValid() && s_isResourceCreationAllowed;
}

void Tr2Renderer::SetResourceCreationAllowed( bool isAllowed )
{
	s_isResourceCreationAllowed = isAllowed;
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
		textureSet = uint32_t( GetAnimationTime() * 20 ) % 2;
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
		return s_fallbackTextures[textureSet][0];
	}
}

bool Tr2Renderer::GetSystemShaderOptions( Tr2ShaderOption** options, size_t* count )
{
	if( !s_shaderOptionsInitialized )
	{
		Tr2ShaderOption option;
		option.name = BlueSharedString( "GEOMETRY_SHADER_SUPPORT" );
		option.value = BlueSharedString( GetGeometryShaderSupport() ? "SUPPORTED" : "UNSUPPORTED" );
		s_shaderOptions[0] = option;
		s_shaderOptionsInitialized = true;
	}

	*options = s_shaderOptions;
	*count = s_shaderOptionCount;

	return true;
}

bool Tr2Renderer::GetGeometryShaderSupport()
{
#if TRINITY_PLATFORM == TRINITY_DIRECTX11
	USE_MAIN_THREAD_RENDER_CONTEXT();

	/*
	struct Vtx
	{
		float4 pos : SV_Position;
	};

	[maxvertexcount(3)]
	void main( triangle Vtx pos[3], inout TriangleStream<Vtx> triStream )
	{
		triStream.Append(pos[0]);
		triStream.Append(pos[1]);
		triStream.Append(pos[2]);
	}

	FXC.exe /T gs_5_0 /E main /Fh gs.h /Qstrip_reflect /Qstrip_debug /Qstrip_priv
	*/

	const BYTE bytecode[] =
	{
		 68,  88,  66,  67, 131,  74,
		180, 213, 103,  95, 109,  18,
		214, 138, 181,  45, 155, 163,
		 12, 252,   1,   0,   0,   0,
		 92,   1,   0,   0,   3,   0,
		  0,   0,  44,   0,   0,   0,
		 96,   0,   0,   0, 152,   0,
		  0,   0,  73,  83,  71,  78,
		 44,   0,   0,   0,   1,   0,
		  0,   0,   8,   0,   0,   0,
		 32,   0,   0,   0,   0,   0,
		  0,   0,   1,   0,   0,   0,
		  3,   0,   0,   0,   0,   0,
		  0,   0,  15,  15,   0,   0,
		 83,  86,  95,  80, 111, 115,
		105, 116, 105, 111, 110,   0,
		 79,  83,  71,  53,  48,   0,
		  0,   0,   1,   0,   0,   0,
		  8,   0,   0,   0,   0,   0,
		  0,   0,  36,   0,   0,   0,
		  0,   0,   0,   0,   1,   0,
		  0,   0,   3,   0,   0,   0,
		  0,   0,   0,   0,  15,   0,
		  0,   0,  83,  86,  95,  80,
		111, 115, 105, 116, 105, 111,
		110,   0,  83,  72,  69,  88,
		188,   0,   0,   0,  80,   0,
		  2,   0,  47,   0,   0,   0,
		106,   8,   0,   1,  97,   0,
		  0,   5, 242,  16,  32,   0,
		  3,   0,   0,   0,   0,   0,
		  0,   0,   1,   0,   0,   0,
		 93,  24,   0,   1, 143,   0,
		  0,   3,   0,   0,  17,   0,
		  0,   0,   0,   0,  92,  40,
		  0,   1, 103,   0,   0,   4,
		242,  32,  16,   0,   0,   0,
		  0,   0,   1,   0,   0,   0,
		 94,   0,   0,   2,   3,   0,
		  0,   0,  54,   0,   0,   6,
		242,  32,  16,   0,   0,   0,
		  0,   0,  70,  30,  32,   0,
		  0,   0,   0,   0,   0,   0,
		  0,   0, 117,   0,   0,   3,
		  0,   0,  17,   0,   0,   0,
		  0,   0,  54,   0,   0,   6,
		242,  32,  16,   0,   0,   0,
		  0,   0,  70,  30,  32,   0,
		  1,   0,   0,   0,   0,   0,
		  0,   0, 117,   0,   0,   3,
		  0,   0,  17,   0,   0,   0,
		  0,   0,  54,   0,   0,   6,
		242,  32,  16,   0,   0,   0,
		  0,   0,  70,  30,  32,   0,
		  2,   0,   0,   0,   0,   0,
		  0,   0, 117,   0,   0,   3,
		  0,   0,  17,   0,   0,   0,
		  0,   0,  62,   0,   0,   1
	};

	Tr2ShaderAL shader;
	return SUCCEEDED( shader.Create( GEOMETRY_SHADER, bytecode, Tr2ShaderSignatureAL(), "", renderContext ) );
#elif TRINITY_PLATFORM == TRINITY_DIRECTX12 || TRINITY_PLATFORM == TRINITY_VULKAN
	return true;
#else
	return false;
#endif
}
