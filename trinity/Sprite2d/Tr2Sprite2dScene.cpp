// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dScene.h"
#include "TriDevice.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Shader/Tr2Shader.h"
#include "Tr2VariableStore.h"
#include "Curves/TriCurveSet.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Sprite2d.h"
#include "TriProjection.h"
#include "TriView.h"
#include "Tr2TextureAtlasMan.h"
#include "Tr2TextureAtlas.h"
#include "RenderJob/TriRenderJob.h"
#include "TriSettingsRegistrar.h"
#include "include/ITr2DebugRenderer.h"
#include "Include/TriMath.h"


using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( spriteSceneDrawCallCount, "Trinity/SpriteScene/DrawCallCount", true, CST_COUNTER_LOW, "Count of drawcalls for sprites" );
CCP_STATS_DECLARE( spriteSceneDrawCallEmpty, "Trinity/SpriteScene/DrawCallEmpty", true, CST_COUNTER_LOW, "Count of empty drawcalls for sprites" );
CCP_STATS_DECLARE( spriteSceneDrawCallTexture, "Trinity/SpriteScene/DrawCallTexture", true, CST_COUNTER_LOW, "Count of drawcalls caused by texture changing" );
CCP_STATS_DECLARE( spriteSceneDrawCallEffect, "Trinity/SpriteScene/DrawCallEffect", true, CST_COUNTER_LOW, "Count of drawcalls caused by sprite effect changing" );

CCP_STATS_DECLARE( spriteSceneTextureCount, "Trinity/SpriteScene/TextureCount", true, CST_COUNTER_LOW, "Textures used per frame" );
CCP_STATS_DECLARE( spriteSceneTextureNotReady, "Trinity/SpriteScene/TextureNotReady", true, CST_COUNTER_LOW, "Sprites skipped due to its textures not being ready" );

CCP_STATS_DECLARE( spriteSceneCount, "Trinity/SpriteScene/Count", true, CST_COUNTER_HIGH, "Count of sprites rendered per frame" );
CCP_STATS_DECLARE( spriteSceneSpriteArea, "Trinity/SpriteScene/SpriteArea", true, CST_COUNTER_HIGH, "Area of sprites rendered per frame" );

CCP_STATS_DECLARE( spriteSceneTransforms, "Trinity/SpriteScene/Transforms", true, CST_COUNTER_HIGH, "Count of nontrivial transform matrices per frame" );
CCP_STATS_DECLARE( spriteSceneTransformSplits, "Trinity/SpriteScene/TransformSplits", true, CST_COUNTER_HIGH, "Number of times active transform count caused additional draw calls" );

CCP_STATS_DECLARE( spriteSceneDisplayListsCreated, "Trinity/SpriteScene/DisplayListsCreated", true, CST_COUNTER_HIGH, "Number of display lists generated per frame" );
CCP_STATS_DECLARE( spriteSceneDisplayListsUsed, "Trinity/SpriteScene/DisplayListsUsed", true, CST_COUNTER_HIGH, "Number of display lists used per frame" );

CCP_STATS_DECLARED_ELSEWHERE( displayListVertexBufferSize );
CCP_STATS_DECLARED_ELSEWHERE( displayListIndexBufferSize );

static const int MAX_RETRIES_ON_LOCK_FAILURE = 10;


static Tr2VertexDefinition s_vertexDesc;

static const int SPRITE_COUNT_MAX = 65535 / 4;

static const char* EFFECT_UBERSHADER_RESPATH = "res:/Graphics/Effect/UI/UberShader.fx";
static const char* EFFECT_RENDER_UBERSHADER_RESPATH_3D = "res:/Graphics/Effect/UI/UberShader3d.fx";

static const unsigned int VB_ALIGNMENT = 16;
static const unsigned int IB_ALIGNMENT = 16;

Tr2Sprite2dScene::Tr2Sprite2dScene( IRoot* lockobj ) :
	PARENTLOCK( m_children ),
	PARENTLOCK( m_background ),
	PARENTLOCK( m_curveSets ),
	m_display( true ),
	m_spriteEffect( TR2_SFX_FILL ),
	m_blendMode( TR2_SBM_NONE ),
	m_spriteTarget( Tr2SpriteTarget::COLOR ),
	m_glowBrightness( 1.f ),
	m_numTexturesUsed( 0 ),
	m_isAntiAliased( false ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_depth( 0.0f ),
	m_depthMin( 0.0f ),
	m_depthMax( 0.0f ),
	m_displayWidth( 1.0f ),
	m_displayHeight( 1.0f ),
	m_pickState( TR2_SPS_ON ),
	m_backgroundColor( 0x000000ff ),
	m_color( 1.0f, 1.0f, 1.0f, 1.0f ),
	m_outlineColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_outlineThreshold( 0.0f ),
	m_accumulatedAlpha( 1.0f ),
	m_drawWireFrame( false ),
	m_clearBackground( false ),
	m_clearFinishedCurveSets( false ),
	m_transformStack( NULL ),
	m_depthStack( NULL ),
	m_clipStack( NULL ),
	m_ignoreClip( false ),
	m_currentVertexData( NULL ),
	m_currentIndexData( NULL ),
	m_vertexDecl( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_vertexCount( 0 ),
	m_indexCount( 0 ),
	m_isFullscreen( false ),
	m_is2dRender( true ),
	m_is2dPick( true ),
	m_is2dRenderContext( true ),
	m_stackOfStacks( "Tr2Sprite2dScene/m_stackOfStacks" ),
	m_defaultTextureFlash( 0 ),
	m_defaultTextureUpdates( false ),
	m_transformCurrent( 0 ),
	m_itemsRendered( 0 ),
	m_maxItemsToRender( 0xffffffff ),
	m_drawCallsRendered( 0 ),
	m_maxDrawCallsToRender( 0xffffffff ),
	m_captureDisplayList( nullptr ),
	m_preCaptureVertexData( nullptr ),
	m_preCaptureIndexData( nullptr ),
	m_captureVertexDataSize( 0 ),
	m_captureVertexDataCapacity( 0 ),
	m_captureIndexDataSize( 0 ),
	m_captureIndexDataCapacity( 0 ),
	m_captureStartIndex( 0 ),
	m_viewportSizeVar( "UIViewportSize", Vector4( 0.0f, 0.0f, 0.0f, 0.0f ) ),
	m_dotVectorVar( "g_DotVector", Vector4( 0.0f, 0.0f, 0.0f, 1.0f ) ),
	m_maxSpriteCount( 1024 ),
	m_tileMode( S2D_TS_NONE ),
	m_drawCallStartIndex( 0 ),
	m_transformsHandle(),
	m_useLinearColorSpace( false ),
	m_isGammaCorrectingText( true ),
	m_realTime( 0 ),
	m_simTime( 0 )
{
	m_transformStack = CCP_NEW( "Tr2Sprite2dScene/m_transformStack" ) TransformStack_t( "Tr2Sprite2dScene/m_transformStack" );
	m_depthStack = CCP_NEW( "Tr2Sprite2dScene/m_depthStack" ) DepthStack_t( "Tr2Sprite2dScene/m_depthStack" );
	m_clipStack = CCP_NEW( "Tr2Sprite2dScene/m_clipStack" ) ClipStack_t( "Tr2Sprite2dScene/m_clipStack" );

	m_uberShader2d.CreateInstance();
	m_uberShader2d->SetEffectPathName( EFFECT_UBERSHADER_RESPATH );

	m_uberShader3d.CreateInstance();
	m_uberShader3d->SetEffectPathName( EFFECT_RENDER_UBERSHADER_RESPATH_3D );

	m_textureRegisters[0] = 0;
	m_textureRegisters[1] = 1;

	if( g_textureAtlasMan )
	{
		Tr2TextureAtlas* atlas = g_textureAtlasMan->FindAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM );
		if( !atlas )
		{
			g_textureAtlasMan->AddAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM, 2048, 2048 );
			atlas = g_textureAtlasMan->FindAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM );
		}

		CCP_ASSERT( atlas );
	}

	std::fill_n( m_transformArray, TR2_SS_MAX_TRANSFORM_COUNT, IdentityMatrix() );

	for( unsigned int i = 0; i < s_textureMax; ++i )
	{
		m_texture[i] = nullptr;
		char name[128];
		sprintf_s( name, "g_texelSizeUI%u", i );
		m_texelSizeVar[i].Register( name, Vector4( 0.0f, 0.0f, 0.0f, 0.0f ) );
	}

	for( int j = 0; j < 2; ++j )
	{
		m_textureSettings[j].useTransform = false;
	}

	m_captureVertexDataCapacity = 1024;
	unsigned int vbSize = m_captureVertexDataCapacity * sizeof( Tr2Sprite2dD3DVertex );
	m_captureVertexData = CcpAlignedMallocBuffer( "Tr2Sprite2dScene/m_captureVertexData", vbSize, VB_ALIGNMENT );

	m_captureIndexDataCapacity = m_captureVertexDataCapacity + m_captureVertexDataCapacity / 2;
	unsigned int ibSize = m_captureIndexDataCapacity * sizeof( unsigned int );
	m_captureIndexData = CcpAlignedMallocBuffer( "Tr2Sprite2dScene/m_captureIndexData", ibSize, IB_ALIGNMENT );
}

Tr2Sprite2dScene::~Tr2Sprite2dScene()
{
	m_captureIndexData.clear();
	m_captureVertexData.clear();

	CCP_ASSERT( m_transformStack->empty() );
	CCP_DELETE( m_transformStack );
	m_transformStack = NULL;

	CCP_ASSERT( m_depthStack->empty() );
	CCP_DELETE( m_depthStack );
	m_depthStack = NULL;

	CCP_ASSERT( m_clipStack->empty() );
	CCP_DELETE( m_clipStack );
	m_clipStack = NULL;
}

void Tr2Sprite2dScene::Update( Be::Time realTime, Be::Time simTime )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_realTime = realTime;
	m_simTime = simTime;

	if( !m_display )
	{
		return;
	}

	Vector3 tmp = Tr2Renderer::GetViewLookAt();
	Quaternion tweak( 0.332621f, 0.332621f, 0.000000, 0.882455f );
	TriVectorRotateQuaternion( &tmp, &tmp, &tweak );

	Vector4 tmpw( tmp.x, tmp.y, tmp.z, 1.0f );
	m_dotVectorVar = tmpw;

	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( realTime, simTime );
	}

	if( m_clearFinishedCurveSets )
	{
		RemoveFinishedCurveSets();
	}
}

void Tr2Sprite2dScene::Render( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	D3DPERF_EVENT( L"Tr2Sprite2dScene::Render" );

	if( !m_display || !renderContext.IsValid() )
	{
		return;
	}

	bool isReady = PrepareResourcesForRender();
	if( !isReady )
	{
		return;
	}

	renderContext.AddGpuMarker( __FUNCTION__ );

	if( m_useLinearColorSpace )
	{
		renderContext.SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 1 );
	}

	// Flash default texture to make it easier to spot missing textures
	FlashDefaultTexture();

	m_lastViewMatrix = Tr2Renderer::GetViewTransform();
	m_lastProjectionMatrix = Tr2Renderer::GetProjectionTransform();

	DetermineWorldTransform();
	DetermineViewportSize();

	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	PrepareRenderContextForRendering( renderContext );

	PrepareStacksBeforeRender();

	ResetBufferPointers();

	m_itemsRendered = 0;
	m_drawCallsRendered = 0;

	for( ITr2SpriteObjectVector::reverse_iterator it = m_background.rbegin(); it != m_background.rend(); ++it )
	{
		( *it )->GatherSprites( this );
	}
	for( ITr2SpriteObjectVector::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it )
	{
		( *it )->GatherSprites( this );
	}

	CCP_ASSERT( !m_captureDisplayList );

	IssueDrawCall();

	PrepareRenderContextAfterRendering( renderContext );

	CleanUpStacksAfterRender();

	renderContext.m_esm.PopDepthStencilBuffer();

	m_is2dRenderContext = m_is2dRender;

	renderContext.AddGpuMarker( "Tr2Sprite2dScene::Render End" );

	if( m_useLinearColorSpace )
	{
		renderContext.SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 0 );
	}
}

ITr2SpriteObject* Tr2Sprite2dScene::PickObject( int x, int y, const TriProjection* proj, const TriView* view, const TriViewport* vp, Be::Optional<int> )
{
	CCP_ASSERT( m_transformStack->empty() );

	float finalX;
	float finalY;
	if( !m_is2dPick )
	{
		float fx, fy;
		Vector3 startWorld;
		Vector3 dirWorld;

		gTriDev->ScreenToProjection( x, y, &fx, &fy, vp );

		Matrix worldTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );

		Matrix worldViewTransform;
		Matrix viewTransform = view->GetTransform();
		worldViewTransform = worldTransform * viewTransform;

		Matrix worldViewProjectionTransform;
		Matrix projectionTransform = proj->GetTransform();
		worldViewProjectionTransform = worldViewTransform * projectionTransform;

		Matrix invWorldViewProjectionTransform = Inverse( worldViewProjectionTransform );

		Vector3 rayStart = TransformCoord( Vector3( fx, fy, 0.0f ), invWorldViewProjectionTransform );

		Vector3 rayEnd = TransformCoord( Vector3( fx, fy, 0.5f ), invWorldViewProjectionTransform );

		float slopeX = ( rayEnd.z - rayStart.z ) / ( rayEnd.x - rayStart.x );
		finalX = ( slopeX * rayStart.x - rayStart.z ) / slopeX;

		float slopeY = ( rayEnd.z - rayStart.z ) / ( rayEnd.y - rayStart.y );
		finalY = ( slopeY * rayStart.y - rayStart.z ) / slopeY;

		finalY = 1.0f - finalY;

		if( m_is2dRender )
		{
			finalX *= m_displayWidth;
			finalY *= m_displayHeight;
			finalX += m_displayWidth / 2.0f;
			finalY -= m_displayHeight / 2.0f;
		}
		else
		{
			// Rendering
			finalX += m_displayWidth / 2.0f;
			finalY += m_displayHeight / 2.0f;
		}

		extern ITr2DebugRendererPtr g_debugRenderer;
		if( g_debugRenderer )
		{
			g_debugRenderer->DrawLine( rayStart, rayEnd );
			g_debugRenderer->Printf( 50, 50, 0xffffffff, "%d, %d, %d, %d", vp->x, vp->y, vp->width, vp->height );
			g_debugRenderer->Printf( 50, 60, 0xffffffff, "%4.2f, %4.2f, %4.2f", rayStart.x, rayStart.y, rayStart.z );
			g_debugRenderer->Printf( 50, 70, 0xffffffff, "%4.2f, %4.2f, %4.2f", rayEnd.x, rayEnd.y, rayEnd.z );
			g_debugRenderer->Printf( 50, 80, 0xffffffff, "%4.4f, %4.4f - %4.2f, %4.2f", fx, fy, finalX, finalY );
		}
	}
	else
	{
		finalX = (float)x;
		finalY = (float)y;
	}

	// Store the last pick pos for debugging purposes
	m_lastPickPos.x = finalX;
	m_lastPickPos.y = finalY;

	ITr2SpriteObject* obj = NULL;

	for( ITr2SpriteObjectVector::iterator it = m_children.begin(); it != m_children.end(); ++it )
	{
		obj = ( *it )->PickPoint( finalX, finalY, this );
		CCP_ASSERT( m_transformStack->empty() );
		if( obj )
		{
			break;
		}
	}

	return obj;
}

void Tr2Sprite2dScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
}

void Tr2Sprite2dScene::PushTranslation( const Vector2& t )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	TransformStackEntry entry;
	entry.isTranslationOnlySet = true;

	if( m_transformStack->empty() )
	{
		entry.translation = t;
		entry.isTranslationOnly = true;
	}
	else
	{
		const TransformStackEntry& top = m_transformStack->back();
		if( top.isTranslationOnly )
		{
			entry.translation = t + top.translation;
			entry.isTranslationOnly = true;
		}
		else
		{
			const Matrix& parent = m_transformStack->back().transform;

			Matrix transform = TranslationMatrix( t.x, t.y, 0.0f );
			entry.transform = transform * parent;

			entry.isTranslationOnly = false;
		}
	}

	m_transformStack->push_back( entry );
}

void Tr2Sprite2dScene::PopTranslation()
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( !m_transformStack->empty() );
	CCP_ASSERT( m_transformStack->back().isTranslationOnlySet );
	m_transformStack->pop_back();
}


const Vector2& Tr2Sprite2dScene::GetTranslation() const
{
	if( m_transformStack->empty() )
	{
		static const Vector2 zero( 0.0f, 0.0f );
		return zero;
	}
	else
	{
		CCP_ASSERT( m_transformStack->back().isTranslationOnlySet );

		return m_transformStack->back().translation;
	}
}

void Tr2Sprite2dScene::PushDepthRange( float depthMin, float depthMax )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	if( m_is2dRenderContext )
	{
		return;
	}

	CCP_ASSERT( depthMin >= -1.0f );
	CCP_ASSERT( depthMin <= 1.0f );
	CCP_ASSERT( depthMax >= -1.0f );
	CCP_ASSERT( depthMax <= 1.0f );

	// To simplify the code below the stack is primed with scene values for depth range
	CCP_ASSERT( !m_depthStack->empty() );

	// Relative range is from -1 to 1
	float normDepthMin = 0.5f * ( depthMin + 1.0f );
	float normDepthMax = 0.5f * ( depthMax + 1.0f );

	const Vector2& currentDepthValues = m_depthStack->back();
	float depthRange = currentDepthValues.y - currentDepthValues.x;
	Vector2 range( currentDepthValues.x + depthRange * normDepthMin, currentDepthValues.x + depthRange * normDepthMax );
	m_depthStack->push_back( range );
}

void Tr2Sprite2dScene::PopDepthRange()
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	if( m_is2dRenderContext )
	{
		return;
	}

	CCP_ASSERT( !m_depthStack->empty() );
	m_depthStack->pop_back();
}

void Tr2Sprite2dScene::SetDepth( float depth )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	if( m_is2dRenderContext )
	{
		return;
	}

	const Vector2& currentDepthValues = m_depthStack->back();
	float depthRange = currentDepthValues.y - currentDepthValues.x;
	float normDepth = 0.5f * ( depth + 1.0f );
	m_depth = currentDepthValues.x + normDepth * depthRange;
}

void Tr2Sprite2dScene::PushTransform( const Matrix& m )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	TransformStackEntry entry;
	entry.isTranslationOnly = false;
	entry.isTranslationOnlySet = false;

	if( m_transformStack->empty() )
	{
		entry.transform = m;
	}
	else
	{
		const TransformStackEntry& top = m_transformStack->back();
		if( top.isTranslationOnly )
		{
			Matrix parent = TranslationMatrix( top.translation.x, top.translation.y, 0.0f );
			entry.transform = m * parent;
		}
		else
		{
			const Matrix& parent = m_transformStack->back().transform;
			entry.transform = m * parent;
		}
	}

	m_transformStack->push_back( entry );
}

void Tr2Sprite2dScene::PopTransform()
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( !m_transformStack->empty() );
	CCP_ASSERT( !m_transformStack->back().isTranslationOnly );
	m_transformStack->pop_back();
}

void Tr2Sprite2dScene::PushTransformAbsolute()
{
	TransformStackEntry entry;
	entry.isTranslationOnly = true;
	entry.isTranslationOnlySet = true;
	entry.translation = Vector2( 0.0f, 0.0f );

	m_transformStack->push_back( entry );
}

void Tr2Sprite2dScene::PopTransformAbsolute()
{
	CCP_ASSERT( !m_transformStack->empty() );
	CCP_ASSERT( m_transformStack->back().isTranslationOnly );
	CCP_ASSERT( m_transformStack->back().isTranslationOnlySet );

	m_transformStack->pop_back();
}

void Tr2Sprite2dScene::PushClipRectangle( float x, float y, float width, float height )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( width >= 0.0f );
	CCP_ASSERT( height >= 0.0f );

	if( m_ignoreClip )
	{
		return;
	}

	// TODO: Implement for 3d rendering
	if( !m_is2dRender )
	{
		return;
	}

	Tr2Sprite2dClipRect rect;

	if( !m_transformStack->empty() )
	{
		const TransformStackEntry& top = m_transformStack->back();

		if( top.isTranslationOnly )
		{
			rect.left = ( x + top.translation.x );
			rect.top = ( y + top.translation.y );
			rect.right = ( rect.left + width );
			rect.bottom = ( rect.top + height );
		}
		else
		{
			const Matrix& transform = top.transform;

			Vector4 corners[4];
			for( int i = 0; i < 4; ++i )
			{
				corners[i] = Vector4( 0.f, 0.f, 0.f, 1.f );
			}

			corners[0].x = x;
			corners[0].y = y;
			corners[1].x = x + width;
			corners[1].y = y;
			corners[2].x = x + width;
			corners[2].y = y + height;
			corners[3].x = x;
			corners[3].y = y + height;

			XMVector4TransformStream( (XMFLOAT4*)corners,
									  sizeof( Vector4 ),
									  (const XMFLOAT4*)corners,
									  sizeof( Vector4 ),
									  4,
									  transform );

			float minX = FLT_MAX;
			float maxX = -FLT_MAX;
			float minY = FLT_MAX;
			float maxY = -FLT_MAX;

			for( int i = 0; i < 4; ++i )
			{
				if( corners[i].x < minX )
				{
					minX = corners[i].x;
				}
				if( corners[i].x > maxX )
				{
					maxX = corners[i].x;
				}
				if( corners[i].y < minY )
				{
					minY = corners[i].y;
				}
				if( corners[i].y > maxY )
				{
					maxY = corners[i].y;
				}
			}

			rect.left = minX;
			rect.top = minY;
			rect.right = maxX;
			rect.bottom = maxY;
		}
	}
	else
	{
		rect.left = x;
		rect.top = y;
		rect.right = ( rect.left + width );
		rect.bottom = ( rect.top + height );
	}

	if( !m_clipStack->empty() )
	{
		// We're already clipping - limit to a subrect within that rect
		const Tr2Sprite2dClipRect& currentRect = m_clipStack->back();

		if( rect.left < currentRect.left )
		{
			rect.left = currentRect.left;
		}
		if( rect.right > currentRect.right )
		{
			rect.right = currentRect.right;
		}
		if( rect.top < currentRect.top )
		{
			rect.top = currentRect.top;
		}
		if( rect.bottom > currentRect.bottom )
		{
			rect.bottom = currentRect.bottom;
		}
	}
	m_clipStack->push_back( rect );
}

void Tr2Sprite2dScene::PopClipRectangle()
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	if( m_ignoreClip )
	{
		return;
	}

	// TODO: Implement for 3d rendering
	if( !m_is2dRender )
	{
		return;
	}

	CCP_ASSERT( !m_clipStack->empty() );
	m_clipStack->pop_back();
}


const Tr2Sprite2dClipRect& Tr2Sprite2dScene::GetClipRectangle() const
{
	CCP_ASSERT( !m_clipStack->empty() );

	return m_clipStack->back();
}

void Tr2Sprite2dScene::SetTexture( unsigned ix, Tr2AtlasTexturePtr tex, Tr2Sprite2dTextureSettings settings )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	CCP_STATS_INC( spriteSceneTextureCount );

	CCP_ASSERT( ix < 2 );

#if !CCP_DEPLOY
	if( !tex )
	{
		tex = m_defaultTexture;
	}
#endif

	Tr2TextureAL* texAL = nullptr;
	if( tex )
	{
		texAL = tex->GetTexture();
		if( !texAL )
		{
			if( tex->GetRenderTarget() )
			{
				texAL = tex->GetRenderTarget();
			}
		}
	}

	// Note that we can't assume that the same value for 'tex' results in the
	// same D3D texture - the underlying texture may shift in the atlas.
	if( !m_texture[ix] || texAL != m_texture[ix]->GetTexture() )
	{
		CCP_STATS_INC( spriteSceneDrawCallTexture );
		IssueDrawCall();
		m_texture[ix] = tex;

		if( ix == 0 )
		{
			Vector4 texelSize;
			if( texAL )
			{
				texelSize.x = 1.0f / texAL->GetWidth();
				texelSize.y = 1.0f / texAL->GetHeight();
			}
			else
			{
				texelSize.x = 0.0f;
				texelSize.y = 0.0f;
			}
			texelSize.z = 0.0f;
			texelSize.w = 0.0f;
			m_texelSizeVar[ix] = texelSize;
		}
	}

	m_texture[ix] = tex;

	TextureSetting& texSettings = m_textureSettings[ix];
	if( tex )
	{
		tex->GetTextureWindow( texSettings.textureWindow );
	}
	else
	{
		texSettings.textureWindow = Vector4( 0.0f, 0.0f, 1.0f, 1.0f );
	}

	texSettings.repeatMode = ( settings & S2D_TS_REPEAT_CLAMP ) ? Tr2Sprite2dScene::TextureSetting::TR_Clamp :
		( settings & S2D_TS_REPEAT_MIRROR )                     ? Tr2Sprite2dScene::TextureSetting::TR_Mirror :
																  Tr2Sprite2dScene::TextureSetting::TR_Tile;

	texSettings.tileX = ( settings & S2D_TS_TILE_X ) != 0;
	texSettings.tileY = ( settings & S2D_TS_TILE_Y ) != 0;
}

void Tr2Sprite2dScene::SetTextureWindow( unsigned int ix, float x, float y, float width, float height )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( ix < 2 );

	if( m_texture[ix] )
	{
		TextureSetting& texSettings = m_textureSettings[ix];
		m_texture[ix]->CalcSubTextureWindow( texSettings.textureWindow, x, y, width, height );
	}
}

void Tr2Sprite2dScene::SetTextureTransform( unsigned int ix, Matrix* m )
{
	//CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( ix < 2 );

	if( m )
	{
		m_textureSettings[ix].useTransform = true;
		m_textureSettings[ix].transform = *m;
	}
	else
	{
		m_textureSettings[ix].useTransform = false;
	}
}

inline bool Tr2Sprite2dScene::TexturesReady() const
{
	for( int i = 0; i < m_numTexturesUsed; ++i )
	{
		if( !m_texture[i] )
		{
			return false;
		}

		if( !m_texture[i]->IsGood() )
		{
			return false;
		}
	}
	return true;
}

bool Tr2Sprite2dScene::PrepareSpriteVerts(
	Tr2Sprite2dD3DVertex* destVerts,
	const Vector2& pos,
	float width,
	float height,
	Tr2SpriteObjectEffect sfx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !TexturesReady() )
	{
		CCP_STATS_INC( spriteSceneTextureNotReady );
		return false;
	}

	//texture coordinates
	Vector2 uv[2][4];
	Vector2 uvInitial[2][4];

	SetSpriteVerticesUVs( uv, width, height );
	memcpy( uvInitial, uv, sizeof( Vector2 ) * 2 * 4 );


	//set vertex data
	Vector2 verts[4] = {
		pos,
		pos + Vector2( width, 0.f ),
		pos + Vector2( width, height ),
		pos + Vector2( 0.f, height )
	};


	if( ( sfx == TR2_SFX_BLUR ) || ( sfx == TR2_SFX_GLOW ) )
	{
		float textureWidthReciprocal = 1.f;
		float textureHeightReciprocal = 1.f;
		if( m_texture[0] )
		{
			textureWidthReciprocal = 1.f / m_texture[0]->GetTextureWidth();
			textureHeightReciprocal = 1.f / m_texture[0]->GetTextureHeight();
		}

		verts[0].x -= 1;
		verts[0].y -= 1;
		uv[0][0].x -= textureWidthReciprocal;
		uv[0][0].y -= textureHeightReciprocal;

		verts[1].x += 1;
		verts[1].y -= 1;
		uv[0][1].x += textureWidthReciprocal;
		uv[0][1].y -= textureHeightReciprocal;

		verts[2].x += 1;
		verts[2].y += 1;
		uv[0][2].x += textureWidthReciprocal;
		uv[0][2].y += textureHeightReciprocal;

		verts[3].x -= 1;
		verts[3].y += 1;
		uv[0][3].x -= textureWidthReciprocal;
		uv[0][3].y += textureHeightReciprocal;
	}

	{
		//CCP_STATS_ZONE( __FUNCTION__ " vertices" );
		for( int i = 0; i < 4; ++i )
		{
			Tr2Sprite2dD3DVertex& vertex = destVerts[i];
			vertex.position.x = verts[i].x;
			vertex.position.y = verts[i].y;
			vertex.position.z = m_depth;
			vertex.color = m_color;
			vertex.texCoord[0] = uv[0][i];
			if( ( sfx == TR2_SFX_BLUR ) || ( sfx == TR2_SFX_GLOW ) || ( sfx == TR2_SFX_OUTLINE ) )
			{
				vertex.texCoord[1] = uvInitial[0][( i + 2 ) % 4];
			}
			else
			{
				vertex.texCoord[1] = uv[1][i];
			}
			if( sfx == TR2_SFX_OUTLINE )
			{
				vertex.outlineColor = m_outlineColor;
				vertex.outlineThreshold = m_outlineThreshold;
			}
			vertex.glowBrightness = m_glowBrightness;
			vertex.blendMode = PackBlendMode( m_blendMode, m_spriteTarget );
			vertex.spriteEffect = m_spriteEffect;
			vertex.transformIndex = 0;
			vertex.tileMode = m_tileMode;
		}
	}

	return true;
}

bool Tr2Sprite2dScene::PrepareTriangleVerts( Tr2Sprite2dD3DVertex* destVerts, Tr2Sprite2dVertexBase* verts, unsigned int stride, unsigned int vertexCount )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !TexturesReady() )
	{
		CCP_STATS_INC( spriteSceneTextureNotReady );
		return false;
	}

	//
	// First gather all data we're setting or using to adjust vertices, then iterate over
	// all verts and set the data. It's very important that we only iterate once over
	// the vertices and update their fields in the right order for optimal cache behavior.
	//

	//
	// Process the vertices into the vertex buffer
	//

	Tr2Sprite2dVertexBase* curVertex = verts;
	for( unsigned int i = 0; i < vertexCount; ++i )
	{
		destVerts->position.x = curVertex->position.x;
		destVerts->position.y = curVertex->position.y;
		destVerts->position.z = curVertex->position.z;

		destVerts->color.r = curVertex->color.r * m_color.r;
		destVerts->color.g = curVertex->color.g * m_color.g;
		destVerts->color.b = curVertex->color.b * m_color.b;
		destVerts->color.a = curVertex->color.a * m_color.a;

		for( int ix = 0; ix < m_numTexturesUsed; ++ix )
		{
			TextureSetting& texSettings = m_textureSettings[ix];

			Vector2 dstUV;
			if( texSettings.useTransform )
			{
				// Apply texture transformation
				float x = curVertex->texCoord[ix].x;
				float y = curVertex->texCoord[ix].y;
				Vector4 uv( x, y, 0.0f, 1.0f );
				uv = Transform( uv, texSettings.transform );
				dstUV.x = uv.x * texSettings.textureWindow.z + texSettings.textureWindow.x;
				dstUV.y = uv.y * texSettings.textureWindow.w + texSettings.textureWindow.y;
			}
			else
			{
				dstUV.x = curVertex->texCoord[ix].x * texSettings.textureWindow.z + texSettings.textureWindow.x;
				dstUV.y = curVertex->texCoord[ix].y * texSettings.textureWindow.w + texSettings.textureWindow.y;
			}
			destVerts->texCoord[ix] = dstUV;
		}

		if( m_isAntiAliased )
		{
			destVerts->texCoord[0] = curVertex->texCoord[0];
			destVerts->texCoord[1] = curVertex->texCoord[1];
		}

		destVerts->glowBrightness = m_glowBrightness;
		destVerts->blendMode = PackBlendMode( m_blendMode, m_spriteTarget );
		destVerts->spriteEffect = m_spriteEffect;
		destVerts->tileMode = m_tileMode;

		destVerts->transformIndex = 0;

		++destVerts;
		curVertex = (Tr2Sprite2dVertexBase*)( (uint8_t*)curVertex + stride );
	}

	return true;
}

void Tr2Sprite2dScene::RenderTriangleVerts( Tr2Sprite2dD3DVertex* verticesSrc, unsigned int vertexCount, unsigned short* indices, unsigned short indexCount )
{
	if( m_transformCurrent >= TR2_SS_MAX_TRANSFORM_COUNT - 1 )
	{
		// Buffers are full, kick off what we've got
		IssueDrawCall();
	}

	// Offset applied to indices
	int vertexOffset;

	bool canRender = EnsureBufferSpace( vertexCount, indexCount, vertexOffset );
	if( !canRender )
	{
		return;
	}

	ProcessVertices( verticesSrc, vertexCount );
	CopyIndicesWithOffset( indices, indexCount, vertexOffset );
}

void Tr2Sprite2dScene::RenderTriangleVerts( Tr2BufferAL& verticesSrc, unsigned int vertexCount, Tr2BufferAL& indices, unsigned short indexCount )
{
	CCP_ASSERT( !m_captureDisplayList );

	SelectEffect();

	IssueDrawCall();

	USE_MAIN_THREAD_RENDER_CONTEXT();

	renderContext.m_esm.ApplyIndexBuffer( indices );
	renderContext.m_esm.ApplyStreamSource( 0, verticesSrc, 0, sizeof( Tr2Sprite2dD3DVertex ) );

	m_indexCount = indexCount;
	m_vertexCount = vertexCount;

	if( m_effect )
	{
		m_effect->Render( this, renderContext );
	}

	m_vertexCount = 0;

	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer.GetBuffer() );
}

void Tr2Sprite2dScene::IssueDrawCall()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_STATS_INC( spriteSceneDrawCallCount );

	if( !m_indexCount )
	{
		CCP_STATS_INC( spriteSceneDrawCallEmpty );

		return;
	}

	bool effectOK = SelectEffect();

	if( m_captureDisplayList )
	{
		CCP_ASSERT( m_effect );

		if( effectOK )
		{
			renderContext.AddGpuMarker( "Tr2Sprite2dScene::IssueDrawCall Captured" );

			Tr2Sprite2dDisplayList::Entry entry;

			entry.job = nullptr;

			memcpy( entry.transformArray, m_transformArray, sizeof( m_transformArray ) );

			entry.numVertices = m_captureVertexDataSize;
			entry.primitiveCount = m_indexCount / 3;
			entry.startIndex = m_captureStartIndex;
			entry.texture0 = m_texture[0];
			entry.texture1 = m_texture[1];
			entry.effect = m_effect;
			m_texelSizeVar[0].GetValue( entry.texelSize0 );
			m_texelSizeVar[1].GetValue( entry.texelSize1 );

			//entry.vsConstantTable = m_vsConstantTable;
			//entry.transformsHandle = m_transformsHandle;
			entry.m_uiTransformsCb = &m_uiTransformsCb;

			m_captureDisplayList->entries.push_back( entry );
		}

		m_vertexCount = 0;
		m_indexCount = 0;
		m_transformCurrent = 0;

		m_captureStartIndex = m_captureIndexDataSize;
		return;
	}

	++m_drawCallsRendered;

	if( effectOK && ( m_drawCallsRendered < m_maxDrawCallsToRender ) )
	{
		renderContext.AddGpuMarker( "Tr2Sprite2dScene::IssueDrawCall Direct" );

		uint32_t vertexBufferOffset;
		if( FAILED( m_vertexBuffer.PutData(
				m_vertexBufferData.get(),
				m_vertexCount * sizeof( Tr2Sprite2dD3DVertex ),
				vertexBufferOffset,
				renderContext ) ) )
		{
			CCP_LOGERR( "Failed to update stream vertex buffer for UI" );
		}
		else
		{
			if( FAILED( m_indexBuffer.PutData(
					m_indexBufferData.get(),
					m_indexCount * sizeof( uint32_t ),
					m_drawCallStartIndex,
					renderContext ) ) )
			{
				CCP_LOGERR( "Failed to update stream index buffer for UI" );
			}
			else
			{
				if( auto desc = m_effect->GetPassDescription( 0, 0 ) )
				{
					for( uint32_t i = 0; i < 2; ++i )
					{
						Tr2TextureAL* texAL = nullptr;
						if( m_texture[i] )
						{
							texAL = m_texture[i]->GetTexture();
							if( !texAL )
							{
								if( m_texture[i]->GetRenderTarget() )
								{
									texAL = m_texture[i]->GetRenderTarget();
								}
							}
						}
						auto colorSpace = m_useLinearColorSpace ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
						desc->m_resourceSetDirty |= desc->m_resourceSetDesc.SetSrv( PIXEL_SHADER, m_textureRegisters[i], texAL ? *texAL : Tr2TextureAL(), colorSpace );
					}
				}

				m_drawCallStartIndex /= sizeof( uint32_t );
				renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer.GetBuffer(), vertexBufferOffset, sizeof( Tr2Sprite2dD3DVertex ) );
				renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer.GetBuffer() );
				m_effect->Render( this, renderContext );
				m_indexBuffer.DoneUsingData( renderContext );
			}
			m_vertexBuffer.DoneUsingData( renderContext );
		}

		ResetBufferPointers();
	}

	m_vertexCount = 0;
	m_indexCount = 0;
	m_transformCurrent = 0;
}

void Tr2Sprite2dScene::SubmitGeometry( Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Error checking is done in IssueDrawCall - we won't get here unless the effect
	// is valid, as well as the constant table and transforms handle.
	Matrix transposedMatrixes[TR2_SS_MAX_TRANSFORM_COUNT];
	for( unsigned i = 0; i < TR2_SS_MAX_TRANSFORM_COUNT; ++i )
	{
		transposedMatrixes[i] = Transpose( m_transformArray[i] );
	}

	bool result = FillAndSetConstants(
		m_uiTransformsCb,
		transposedMatrixes,
		sizeof( transposedMatrixes[0] ) * TR2_SS_MAX_TRANSFORM_COUNT,
		VERTEX_SHADER,
		Tr2Renderer::GetPerObjectVSGUIStartRegister(),
		renderContext );

	if( !result )
	{
		CCP_LOGWARN( "Tr2Sprite2dScene::SubmitGeometry - couldn't set VS constant" );
	}

	renderContext.SetTopology( TOP_TRIANGLES );
	HRESULT hr = renderContext.DrawIndexedPrimitive( m_indexCount, m_drawCallStartIndex, m_indexCount / 3 );
	if( FAILED( hr ) )
	{
		CCP_LOGWARN( "Tr2Sprite2dScene::SubmitGeometry - DrawIndexedPrimitive failed (0x%x)", hr );
	}
}

void Tr2Sprite2dScene::ReleaseResources( TriStorage s )
{
	m_uiTransformsCb = Tr2ConstantBufferAL();

	m_effect = nullptr;
	m_defaultTexture = nullptr;

	if( s & TRISTORAGE_MANAGEDMEMORY )
	{
		m_vertexDecl = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	}
}

bool Tr2Sprite2dScene::OnPrepareResources()
{
	if( m_vertexDecl == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		if( s_vertexDesc.empty() )
		{
			// This vertex declaration matches the Tr2Sprite2dD3DVertex defined in ITr2Sprite2dRenderer.h
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_3, s_vertexDesc.POSITION );
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_4, s_vertexDesc.COLOR, 0 );

			// Texture coordinates, primary texture
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_2, s_vertexDesc.TEXCOORD, 0 );

			// Texture coordinates, secondary texture
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_2, s_vertexDesc.TEXCOORD, 1 );

			// Clip rect
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_4, s_vertexDesc.TEXCOORD, 2 );

			// Glow brightness
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_1, s_vertexDesc.TEXCOORD, 3 );

			// Matrix index
			s_vertexDesc.Add( s_vertexDesc.UBYTE_4, s_vertexDesc.BLENDINDICES );

			// Outline color
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_4, s_vertexDesc.COLOR, 1 );

			// Outline threshold
			s_vertexDesc.Add( s_vertexDesc.FLOAT32_1, s_vertexDesc.TEXCOORD, 4 );
		}

		m_vertexDecl = Tr2EffectStateManager::GetVertexDeclarationHandle( s_vertexDesc );
	}

	unsigned int indexCount = m_maxSpriteCount * 6;
	unsigned int ibSize = indexCount * sizeof( uint32_t );
	unsigned int vbSize = m_maxSpriteCount * 4 * sizeof( Tr2Sprite2dD3DVertex );

	if( !m_vertexBuffer.Create( vbSize ) )
	{
		CCP_LOGERR( "Tr2Sprite2dScene::OnPrepareResources failed to create streaming vertex buffer" );
		return false;
	}
	m_vertexBuffer.SetName( "UI Vertex Buffer" );

	if( !m_indexBuffer.Create( indexCount, 4 ) )
	{
		CCP_LOGERR( "Tr2Sprite2dScene::OnPrepareResources failed to create streaming index buffer" );
		return false;
	}
	m_indexBuffer.SetName( "UI Index Buffer" );

	m_vertexBufferData.resize( "Tr2Sprite2dScene::m_vertexBufferData", vbSize );
	if( m_vertexBufferData.empty() )
	{
		CCP_LOGERR( "Tr2Sprite2dScene::OnPrepareResources failed to allocate vertex buffer mirror" );
		return false;
	}

	m_indexBufferData.resize( "Tr2Sprite2dScene::m_indexBufferData", ibSize );
	if( m_indexBufferData.empty() )
	{
		CCP_LOGERR( "Tr2Sprite2dScene::OnPrepareResources failed to allocate index buffer mirror" );
		return false;
	}

	// Ensure we have an atlas
	CCP_ASSERT( g_textureAtlasMan );
	Tr2TextureAtlas* atlas = g_textureAtlasMan->FindAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM );
	if( !atlas )
	{
		g_textureAtlasMan->AddAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM, 2048, 2048 );
		atlas = g_textureAtlasMan->FindAtlas( PIXEL_FORMAT_B8G8R8A8_UNORM );
	}

	CCP_ASSERT( atlas );

	// Create the default texture. Use video because we need CPU write access
	// to fill it with initial data.
	atlas->CreateTexture( 2, 2, Tr2TextureAtlas::ATT_VIDEO, &m_defaultTexture );

	if( m_defaultTexture )
	{
		//Make a little tiny deliberately ugly checker pattern
		void* data = NULL;
		unsigned pitch = 0;
		if( m_defaultTexture->LockBuffer( data, pitch ) )
		{
			static const unsigned char defaultPixels[2][8] = {
				{ 0xff, 0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0xff },
				{ 0x00, 0xff, 0x00, 0xff, 0xff, 0x00, 0xff, 0x00 }
			};
			memcpy( data, defaultPixels[0], 8 );
			memcpy( (char*)data + pitch, defaultPixels[1], 8 );
			m_defaultTexture->UnlockBuffer();
		}
	}

	CCP_ASSERT( m_defaultTexture );

	return true;
}

Vector2 Tr2Sprite2dScene::InverseTransformPoint( const Vector2& point ) const
{
	if( m_transformStack->empty() )
	{
		return point;
	}
	else
	{
		const TransformStackEntry& topEntry = m_transformStack->back();

		if( topEntry.isTranslationOnly )
		{
			return point - topEntry.translation;
		}
		else
		{
			auto transformed = Transform( Vector4( point.x, point.y, 0, 1 ), Inverse( topEntry.transform ) );
			return Vector2( transformed.x, transformed.y );
		}
	}
}

bool Tr2Sprite2dScene::IsInside( const Vector2& pointIn, const Vector2& topLeft, float width, float height, float radius )
{
	if( !IsInsideClipRect( pointIn ) )
	{
		return false;
	}

	//local copy so we can manipulate it
	//this makes non-translation transforms easier to implement
	Vector2 point = pointIn;

	float top;
	float left;
	float bottom;
	float right;

	if( m_transformStack->empty() )
	{
		left = topLeft.x;
		top = topLeft.y;
		right = left + width;
		bottom = top + height;
	}
	else
	{
		const TransformStackEntry& topEntry = m_transformStack->back();

		if( topEntry.isTranslationOnly )
		{
			left = topLeft.x + topEntry.translation.x;
			top = topLeft.y + topEntry.translation.y;
			right = left + width;
			bottom = top + height;
		}
		else
		{
			left = topLeft.x;
			top = topLeft.y;
			right = left + width;
			bottom = top + height;

			const Matrix& transform = topEntry.transform;

			//construct inverse, transform point by this, compare against
			// untransformed bounding rectangle
			Matrix inv = Inverse( transform );

			TransformPoint( point, point, inv );
		}
	}

	if( point.x < left )
	{
		return false;
	}
	if( point.x > right )
	{
		return false;
	}
	if( point.y < top )
	{
		return false;
	}
	if( point.y > bottom )
	{
		return false;
	}

	if( radius != 0.0f )
	{
		float centerX = 0.5f * ( right - left ) + left;
		float centerY = 0.5f * ( bottom - top ) + top;

		float dX = centerX - point.x;
		float dY = centerY - point.y;

		if( radius > 0.0f )
		{
			if( dX * dX + dY * dY > radius * radius )
			{
				return false;
			}
		}
		else if( radius == -1.0f )
		{
			float rwidth = right - left;
			float rheight = bottom - top;

			float a, b;
			if( rwidth > rheight )
			{
				a = rwidth * 0.5f;
				b = rheight * 0.5f;
			}
			else
			{
				a = rheight * 0.5f;
				b = rwidth * 0.5f;
			}

			if( dX * dX / ( a * a ) + dY * dY / ( b * b ) > 1.0f )
			{
				return false;
			}
		}
	}

	return true;
}

bool Tr2Sprite2dScene::IsInsideLineSegment( const Vector2& pointIn, const Vector2& start, const Vector2& end, float lineWidth )
{
	if( !IsInsideClipRect( pointIn ) )
	{
		return false;
	}

	Vector2 startTransformed;
	Vector2 endTransformed;

	if( m_transformStack->empty() )
	{
		startTransformed.x = start.x;
		startTransformed.y = start.y;
		endTransformed.x = end.x;
		endTransformed.y = end.y;
	}
	else
	{
		const TransformStackEntry& topEntry = m_transformStack->back();

		if( topEntry.isTranslationOnly )
		{
			startTransformed.x = start.x + topEntry.translation.x;
			startTransformed.y = start.y + topEntry.translation.y;
			endTransformed.x = end.x + topEntry.translation.x;
			endTransformed.y = end.y + topEntry.translation.y;
		}
		else
		{
			const Matrix& transform = topEntry.transform;

			TransformPoint( startTransformed, start, transform );
			TransformPoint( endTransformed, end, transform );
		}
	}

	// http://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line

	float dx = endTransformed.x - startTransformed.x;
	float dy = endTransformed.y - startTransformed.y;

	float den = sqrtf( dx * dx + dy * dy );
	if( den < FLT_EPSILON )
	{
		return false;
	}

	float nom = fabs( dy * pointIn.x - dx * pointIn.y - startTransformed.x * endTransformed.y + startTransformed.y * endTransformed.x );
	float distance = nom / den;

	if( distance > lineWidth )
	{
		return false;
	}

	// Check that we are between the end points of the segment

	Vector2 centerPoint = 0.5f * ( startTransformed + endTransformed );
	Vector2 pointToCenter = pointIn - centerPoint;
	float distanceFromCenter = sqrtf( pointToCenter.x * pointToCenter.x + pointToCenter.y * pointToCenter.y );
	if( distanceFromCenter > den * 0.5f )
	{
		return false;
	}

	return true;
}

static float Sign( const Vector2& v1, const Vector2& v2, const Vector2& v3 )
{
	return ( v1.x - v3.x ) * ( v2.y - v3.y ) - ( v2.x - v3.x ) * ( v1.y - v3.y );
}

static bool IsDegenerateTriangle( const Vector2& v0, const Vector2& v1, const Vector2& v2 )
{
	if( ( fabs( v0.x - v1.x ) < FLT_EPSILON ) && ( fabs( v0.y - v1.y ) < FLT_EPSILON ) )
	{
		return true;
	}

	if( ( fabs( v0.x - v2.x ) < FLT_EPSILON ) && ( fabs( v0.y - v2.y ) < FLT_EPSILON ) )
	{
		return true;
	}

	return false;
}

bool Tr2Sprite2dScene::IsInsideTriangle( const Vector2& pointIn, const Vector2& v0, const Vector2& v1, const Vector2& v2 )
{
	if( !IsInsideClipRect( pointIn ) )
	{
		return false;
	}

	if( IsDegenerateTriangle( v0, v1, v2 ) )
	{
		return false;
	}

	Vector2 vT[3];

	if( m_transformStack->empty() )
	{
		vT[0] = v0;
		vT[1] = v1;
		vT[2] = v2;
	}
	else
	{
		const TransformStackEntry& topEntry = m_transformStack->back();

		if( topEntry.isTranslationOnly )
		{
			vT[0] = v0 + topEntry.translation;
			vT[1] = v1 + topEntry.translation;
			vT[2] = v2 + topEntry.translation;
		}
		else
		{
			const Matrix& transform = topEntry.transform;

			TransformPoint( vT[0], v0, transform );
			TransformPoint( vT[1], v1, transform );
			TransformPoint( vT[2], v2, transform );
		}
	}

	if( Sign( pointIn, vT[0], vT[1] ) < 0.0f )
	{
		return false;
	}
	if( Sign( pointIn, vT[1], vT[2] ) < 0.0f )
	{
		return false;
	}
	if( Sign( pointIn, vT[2], vT[0] ) < 0.0f )
	{
		return false;
	}

	return true;
}

void Tr2Sprite2dScene::StartLayer( Tr2TextureAL& rt )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	D3DPERF_EVENT( L"Tr2Sprite2dScene::StartLayer" );

	// Finish outstanding rendering before changing render target
	IssueDrawCall();

	// A scene rendered in 3d renders in 2d within a layer
	m_is2dRenderContext = true;

	// Set the render target to the top level surface of the given texture
	renderContext.m_esm.PushRenderTarget( rt );

	Vector4 vpSize;
	vpSize.x = (float)rt.GetWidth();
	vpSize.y = (float)rt.GetHeight();
	vpSize.z = 0.0f;
	vpSize.w = 1.0f;
	m_viewportSizeVar = vpSize;

	// Coordinate transformation and clipping is reset - push the current stacks
	// onto a stack for later retrieval when we end the layer. Remember that layers
	// may be nested.
	StackOfStacksEntry_t entry;
	entry.transformStack = m_transformStack;
	entry.clipStack = m_clipStack;
	entry.renderTargetTexture = &rt;
	entry.renderTargetWrapper.CreateInstance();
	entry.renderTargetWrapper->SetRenderTarget( &rt );
	m_stackOfStacks.push_back( entry );

	m_transformStack = CCP_NEW( "Tr2Sprite2dScene/m_transformStack" ) TransformStack_t( "Tr2Sprite2dScene/m_transformStack" );
	m_clipStack = CCP_NEW( "Tr2Sprite2dScene/m_clipStack" ) ClipStack_t( "Tr2Sprite2dScene/m_clipStack" );
	m_clipStack->push_back( Tr2Sprite2dClipRect( 0.0f, 0.0f, vpSize.x, vpSize.y ) );
}

void Tr2Sprite2dScene::EndLayer( float x, float y, float width, float height, ITr2Sprite2dTexture* secondaryTexture )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	D3DPERF_EVENT( L"Tr2Sprite2dScene::EndLayer" );

	CCP_ASSERT( m_transformStack->empty() );
	CCP_DELETE( m_transformStack );

	m_clipStack->pop_back();
	CCP_ASSERT( m_clipStack->empty() );
	CCP_DELETE( m_clipStack );

	// Finish outstanding rendering before resetting render target
	IssueDrawCall();

	renderContext.m_esm.PopRenderTarget();

	// Set the coordinate transformations and clipping to what it was before we started
	// this layer.
	const StackOfStacksEntry_t& entry = m_stackOfStacks.back();

	m_transformStack = entry.transformStack;
	m_clipStack = entry.clipStack;


	// Render the layer into the current one as a sprite
	SetTexture( 0, entry.renderTargetWrapper, S2D_TS_NONE );
	SetTextureTransform( 0, nullptr );
	SetTileMode( 0 );

	if( secondaryTexture )
	{
		secondaryTexture->Apply( this, 1 );
	}

	Tr2Sprite2dD3DVertex vertices[4];
	PrepareSpriteVerts( &vertices[0], Vector2( x, y ), width, height, m_spriteEffect );

	static unsigned short s_layerIndices[6] = { 0, 1, 3, 3, 1, 2 };
	RenderTriangleVerts( &vertices[0], 4, s_layerIndices, 6 );

	m_stackOfStacks.pop_back();

	if( m_stackOfStacks.empty() )
	{
		// Once we exit the final layer the 2d render context is determined by the scene
		m_is2dRenderContext = m_is2dRender;
	}

	DetermineViewportSize();
}

void Tr2Sprite2dScene::SetColor( const Color& color )
{
	m_color = color;
}

void Tr2Sprite2dScene::SetOutlineColor( const Color& outlineColor )
{
	m_outlineColor = outlineColor;
}

void Tr2Sprite2dScene::SetOutlineThreshold( float outlineThreshold )
{
	m_outlineThreshold = outlineThreshold;
}

bool Tr2Sprite2dScene::SelectEffect()
{
	Tr2Effect* newEffect;

	newEffect = m_is2dRender ? m_uberShader2d : m_uberShader3d;

	if( newEffect != m_effect )
	{
		m_effect = nullptr;

		if( newEffect && newEffect->GetShaderStateInterface() )
		{
			m_textureRegisters[0] = 0;
			m_textureRegisters[1] = 1;

			// In DX11 g_uiTransforms is in a separate constant buffer and is not exposed
			// in constant table, so we just believe we have UI shader.
			m_effect = newEffect;

			if( auto shader = m_effect->GetShaderStateInterface() )
			{
				auto& resources = shader->GetEffectDescription().techniques[0].passes[0].stageInputs[PIXEL_SHADER].resources;
				for( auto& resource : resources )
				{
					if( strcmp( resource.second.name, "PrimaryTexture0" ) == 0 )
					{
						m_textureRegisters[0] = uint32_t( resource.first );
					}
					else if( strcmp( resource.second.name, "PrimaryTexture1" ) == 0 )
					{
						m_textureRegisters[1] = uint32_t( resource.first );
					}
				}
			}
			return true;
		}
		return false;
	}

	CCP_ASSERT( m_effect );

	return true;
}

void Tr2Sprite2dScene::SetAccumulatedAlpha( float a )
{
	m_accumulatedAlpha = a;
}

float Tr2Sprite2dScene::GetAccumulatedAlpha() const
{
	return m_accumulatedAlpha;
}

void Tr2Sprite2dScene::RunJob( TriRenderJob* job )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( job );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	IssueDrawCall();

	if( m_captureDisplayList )
	{
		Tr2Sprite2dDisplayList::Entry entry;
		entry.job = job;
		entry.numVertices = 0;
		entry.startIndex = 0;
		entry.primitiveCount = 0;
		entry.texture0 = nullptr;
		entry.texture1 = nullptr;
		entry.texelSize0 = Vector4( 0, 0, 0, 0 );
		entry.texelSize1 = Vector4( 0, 0, 0, 0 );

		m_captureDisplayList->entries.push_back( entry );
	}
	else
	{
		RunJobHelper( job );

		// The render job may have set different textures - clear
		// our state for texture settings
		for( unsigned int i = 0; i < s_textureMax; ++i )
		{
			m_texture[i] = nullptr;
		}

		renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer.GetBuffer() );
	}
}

void Tr2Sprite2dScene::SetTileMode( uint8_t tileMode )
{
	tileMode &= S2D_TS_TILE_X | S2D_TS_TILE_Y;
	m_tileMode = tileMode;
}

void Tr2Sprite2dScene::SetSpriteEffect( Tr2SpriteObjectEffect sfx )
{
	if( m_spriteEffect != sfx )
	{
		m_spriteEffect = sfx;

		m_numTexturesUsed = 0;
		m_isAntiAliased = false;

		if( m_spriteEffect == TR2_SFX_FILL_AA )
		{
			m_isAntiAliased = true;
		}

		if( m_spriteEffect >= TR2_SFX_ONE_TEXTURE )
		{
			++m_numTexturesUsed;
			if( m_spriteEffect >= TR2_SFX_TWO_TEXTURES )
			{
				++m_numTexturesUsed;
			}
		}
	}
}

void Tr2Sprite2dScene::ProcessVertices( Tr2Sprite2dD3DVertex* verticesSrc, unsigned int vertexCount )
{
	if( !m_currentVertexData )
	{
		// It's possible we failed to lock the vertex buffer, better not to draw
		// than to crash.
		return;
	}

	//transform stack
	Vector2 pos;
	int transformIndex = 0;
	{
		if( m_transformStack->empty() )
		{
			pos = Vector2( 0.0f, 0.0f );
		}
		else
		{
			const TransformStackEntry& top = m_transformStack->back();

			if( top.isTranslationOnly )
			{
				const Vector2& translation = top.translation;
				pos = translation;
			}
			else
			{
				transformIndex = ++m_transformCurrent;
				const Matrix& transform = top.transform;
				CCP_STATS_INC( spriteSceneTransforms );
				m_transformArray[transformIndex] = transform;
				pos = Vector2( 0.0f, 0.0f );
			}
		}
	}

	memcpy( m_currentVertexData, verticesSrc, vertexCount * sizeof( Tr2Sprite2dD3DVertex ) );

	if( ( pos.x != 0.0f ) || ( pos.y != 0.0f ) )
	{
		for( unsigned int i = 0; i < vertexCount; ++i )
		{
			Tr2Sprite2dD3DVertex& curVertex = m_currentVertexData[i];
			curVertex.position.x += pos.x;
			curVertex.position.y += pos.y;
		}
	}

	if( m_accumulatedAlpha != 1.0f )
	{
		for( unsigned int i = 0; i < vertexCount; ++i )
		{
			Tr2Sprite2dD3DVertex& curVertex = m_currentVertexData[i];
			curVertex.color.a *= m_accumulatedAlpha;
		}
	}

	if( !m_is2dRenderContext )
	{
		for( unsigned int i = 0; i < vertexCount; ++i )
		{
			Tr2Sprite2dD3DVertex& curVertex = m_currentVertexData[i];
			curVertex.position.z = m_depth;
		}
	}

	if( transformIndex != 0 )
	{
		for( unsigned int i = 0; i < vertexCount; ++i )
		{
			Tr2Sprite2dD3DVertex& curVertex = m_currentVertexData[i];
			curVertex.transformIndex = transformIndex;
		}
	}

	{
		const Tr2Sprite2dClipRect& clip = m_clipStack->back();

		for( unsigned int i = 0; i < vertexCount; ++i )
		{
			Tr2Sprite2dD3DVertex& curVertex = m_currentVertexData[i];
			curVertex.clipRect = clip;
		}
	}

	m_currentVertexData += vertexCount;
	m_vertexCount += vertexCount;
}

void Tr2Sprite2dScene::SetBlendmode( Tr2SpriteObjectBlendMode bm )
{
	m_blendMode = bm;
}

void Tr2Sprite2dScene::SetSpriteTarget( Tr2SpriteTarget target )
{
	m_spriteTarget = target;
}

void Tr2Sprite2dScene::SetGlowBrightness( float glowBrightness )
{
	m_glowBrightness = glowBrightness;
}

//////////////////////////////////////////////////////////////////////////
// Description:
//  Starts a display list capture for the given owner.
// Arguments:
//  owner - the owner of the display list. It will be queried for
//          number of vertices in the display list.
// Returns:
//  Success or failure
//
// On success, EndCapture must be called to finalize the display list.
//////////////////////////////////////////////////////////////////////////
bool Tr2Sprite2dScene::StartCapture( ITr2SpriteObject* owner )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_STATS_INC( spriteSceneDisplayListsCreated );

	CCP_ASSERT( !m_captureDisplayList );

	IssueDrawCall();

	if( !m_captureVertexData || !m_captureIndexData )
	{
		CCP_LOGERR( "%s failed to allocate vertex or index data", __FUNCTION__ );
		return false;
	}

	m_captureDisplayList = CCP_NEW( "Tr2Sprite2dScene/m_captureDisplayList" ) Tr2Sprite2dDisplayList( owner );

	m_captureVertexDataSize = 0;
	m_captureIndexDataSize = 0;

	CCP_ASSERT( m_currentVertexData );
	m_preCaptureVertexData = m_currentVertexData;
	m_currentVertexData = reinterpret_cast<Tr2Sprite2dD3DVertex*>( m_captureVertexData.get() );

	CCP_ASSERT( m_currentIndexData );
	m_preCaptureIndexData = m_currentIndexData;
	m_currentIndexData = reinterpret_cast<unsigned int*>( m_captureIndexData.get() );

	m_captureStartIndex = 0;

	return true;
}

Tr2Sprite2dDisplayList* Tr2Sprite2dScene::EndCapture( Tr2Sprite2dDisplayList* previousDisplayList )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( m_captureDisplayList );
	if( !m_captureDisplayList )
	{
		return nullptr;
	}

	IssueDrawCall();

	HRESULT hr;

	USE_MAIN_THREAD_RENDER_CONTEXT();

	// Create a vertex buffer for capturing upcoming vertices. This is a static vertex buffer,
	// even though it may be short-lived. Filling a vertex buffer in the managed pool is faster
	// than if it were in the default pool so we use the managed pool.

	unsigned int vbSize = m_captureVertexDataSize * sizeof( Tr2Sprite2dD3DVertex );
	unsigned int ibSize = m_captureIndexDataSize * sizeof( unsigned int );

	if( !vbSize || !ibSize )
	{
		ResetCapture();
		return nullptr;
	}

	{
		bool reusedVb = false;
		if( previousDisplayList && previousDisplayList->vertexBuffer.IsValid() &&
			previousDisplayList->vertexBuffer.GetSize() >= vbSize && previousDisplayList->vertexBuffer.GetSize() / 2 <= vbSize )
		{
			if( SUCCEEDED( previousDisplayList->vertexBuffer.UpdateBuffer( 0, vbSize, m_captureVertexData.get(), renderContext ) ) )
			{
				m_captureDisplayList->vertexBuffer = previousDisplayList->vertexBuffer;
				previousDisplayList->vertexBuffer = Tr2BufferAL();
				reusedVb = true;
			}
		}
		if( !reusedVb )
		{
			vbSize = std::min( m_captureVertexDataCapacity, m_captureVertexDataSize + m_captureVertexDataSize / 4 ) * sizeof( Tr2Sprite2dD3DVertex );
			hr = m_captureDisplayList->vertexBuffer.Create( 1, vbSize, Tr2GpuUsage::VERTEX_BUFFER, Tr2CpuUsage::WRITE, m_captureVertexData.get(), renderContext );
			if( FAILED( hr ) )
			{
				CCP_LOGERR( "%s failed (%d) to create vertex buffer for %d vertices (%d KiB)", __FUNCTION__, hr, m_captureVertexDataSize, vbSize / 1024 );
				ResetCapture();
				return nullptr;
			}
			else
			{
				CCP_STATS_ADD( displayListVertexBufferSize, vbSize );
				m_captureDisplayList->vertexBuffer.SetName( "UI Captured Vertex Buffer" );
			}
		}
	}

	{
		bool reusedIb = false;
		if( previousDisplayList && previousDisplayList->indexBuffer.IsValid() &&
			previousDisplayList->indexBuffer.GetSize() >= ibSize && previousDisplayList->indexBuffer.GetSize() / 2 <= ibSize )
		{
			if( SUCCEEDED( previousDisplayList->indexBuffer.UpdateBuffer( 0, ibSize, m_captureIndexData.get(), renderContext ) ) )
			{
				m_captureDisplayList->indexBuffer = previousDisplayList->indexBuffer;
				previousDisplayList->indexBuffer = Tr2BufferAL();
				reusedIb = true;
			}
		}
		if( !reusedIb )
		{
			m_captureIndexDataSize = std::min( m_captureIndexDataCapacity, m_captureIndexDataSize + m_captureIndexDataSize / 4 );
			ibSize = m_captureIndexDataSize * sizeof( unsigned int );
			hr = m_captureDisplayList->indexBuffer.Create( 4, m_captureIndexDataSize, Tr2GpuUsage::INDEX_BUFFER, Tr2CpuUsage::WRITE, m_captureIndexData.get(), renderContext );

			if( FAILED( hr ) )
			{
				CCP_LOGERR( "%s failed to create index buffer (%d); ibSize=%d", __FUNCTION__, hr, ibSize );
				ResetCapture();
				return nullptr;
			}
			else
			{
				CCP_STATS_ADD( displayListIndexBufferSize, ibSize );
				m_captureDisplayList->indexBuffer.SetName( "UI Captured Index Buffer" );
			}
		}
	}

	Tr2Sprite2dDisplayList* dl = m_captureDisplayList;
	m_captureDisplayList = nullptr;

	ResetCapture();

	return dl;
}

void Tr2Sprite2dScene::ReplayCapture( Tr2Sprite2dDisplayList* dl )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	IssueDrawCall();

	if( !dl->vertexBuffer.IsValid() || !dl->indexBuffer.IsValid() || dl->entries.empty() )
	{
		return;
	}

	CCP_STATS_INC( spriteSceneDisplayListsUsed );

	renderContext.m_esm.ApplyIndexBuffer( dl->indexBuffer );
	renderContext.m_esm.ApplyStreamSource( 0, dl->vertexBuffer, 0, sizeof( Tr2Sprite2dD3DVertex ) );

	for( auto it = dl->entries.begin(); it != dl->entries.end(); ++it )
	{
		auto& entry = *it;

		CCP_STATS_ADD( spriteSceneCount, entry.primitiveCount / 2 );

		m_itemsRendered++;
		if( m_itemsRendered >= m_maxItemsToRender )
		{
			continue;
		}

		if( entry.job )
		{
			RunJobHelper( entry.job );

			renderContext.m_esm.ApplyIndexBuffer( dl->indexBuffer );
			renderContext.m_esm.ApplyStreamSource( 0, dl->vertexBuffer, 0, sizeof( Tr2Sprite2dD3DVertex ) );
		}
		else
		{
			m_texelSizeVar[0] = entry.texelSize0;
			m_texelSizeVar[1] = entry.texelSize1;

			auto desc = entry.effect->GetPassDescription( 0, 0 );
			auto colorSpace = m_useLinearColorSpace ? Tr2RenderContextEnum::COLOR_SPACE_SRGB : Tr2RenderContextEnum::COLOR_SPACE_LINEAR;
			desc->m_resourceSetDirty |= desc->m_resourceSetDesc.SetSrv( PIXEL_SHADER, m_textureRegisters[0], ( entry.texture0 && entry.texture0->GetTexture() ) ? *entry.texture0->GetTexture() : Tr2TextureAL(), colorSpace );
			desc->m_resourceSetDirty |= desc->m_resourceSetDesc.SetSrv( PIXEL_SHADER, m_textureRegisters[1], ( entry.texture1 && entry.texture1->GetTexture() ) ? *entry.texture1->GetTexture() : Tr2TextureAL(), colorSpace );

			CCP_STATS_INC( spriteSceneDrawCallCount );

			entry.effect->Render( &entry, renderContext );
		}
	}

	for( unsigned int i = 0; i < s_textureMax; ++i )
	{
		m_texture[i] = nullptr;
	}

	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer.GetBuffer() );
}

bool Tr2Sprite2dScene::IsCapturing() const
{
	return m_captureDisplayList != nullptr;
}

void Tr2Sprite2dScene::RunJobHelper( TriRenderJob* job )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	renderContext.m_esm.EndManagedRendering();

	if( m_useLinearColorSpace )
	{
		renderContext.SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 0 );
	}

	// When rendering of this scene started, a NULL value was pushed
	// for depth/stencil buffer. Pop it here, to get whatever was
	// in use before back. The null value is pushed again after
	// the job has finished.
	renderContext.m_esm.PopDepthStencilBuffer();

	renderContext.m_esm.PushRenderTarget( Tr2TextureAL(), 1 );

	renderContext.m_esm.PushViewport();
	job->Run( m_realTime, m_simTime );
	renderContext.m_esm.PopViewport();

	renderContext.m_esm.PopRenderTarget( 1 );

	renderContext.m_esm.PushDepthStencilBuffer( Tr2TextureAL() );

	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_SPRITE2D );

	if( m_useLinearColorSpace )
	{
		renderContext.SetRenderState( Tr2RenderContextEnum::RS_SRGBWRITEENABLE, 1 );
	}

	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDecl );
}

void Tr2Sprite2dScene::ResetBufferPointers()
{
	m_currentVertexData = reinterpret_cast<Tr2Sprite2dD3DVertex*>( m_vertexBufferData.get() );
	m_currentIndexData = reinterpret_cast<uint32_t*>( m_indexBufferData.get() );
}

unsigned int Tr2Sprite2dScene::GetMaxVertexCountPerDrawCall()
{
	return m_maxSpriteCount * 4;
}

unsigned int Tr2Sprite2dScene::GetMaxIndexCountPerDrawCall()
{
	return m_maxSpriteCount * 6;
}

bool Tr2Sprite2dScene::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_maxSpriteCount ) )
	{
		if( m_maxSpriteCount > SPRITE_COUNT_MAX )
		{
			m_maxSpriteCount = SPRITE_COUNT_MAX;
		}
		ReleaseResources( TRISTORAGE_ALL );
	}

	return true;
}

void Tr2Sprite2dScene::ResetCapture()
{
	CCP_DELETE m_captureDisplayList;
	m_captureDisplayList = nullptr;

	m_captureVertexDataSize = 0;
	m_captureIndexDataSize = 0;

	m_currentVertexData = m_preCaptureVertexData;
	CCP_ASSERT( m_currentVertexData );

	m_currentIndexData = m_preCaptureIndexData;
	CCP_ASSERT( m_currentIndexData );
}

void Tr2Sprite2dScene::FlashDefaultTexture()
{
	if( m_defaultTextureUpdates )
	{
		uint32_t pixelValue = 0xff000000;
		pixelValue |= m_defaultTextureFlash << 16;
		pixelValue |= m_defaultTextureFlash << 8;
		pixelValue |= m_defaultTextureFlash;

		m_defaultTextureFlash += 5;
		m_defaultTextureFlash %= 255;

		void* data = NULL;
		unsigned int pitch;
		m_defaultTexture->LockBuffer( data, pitch );

		CCP_ASSERT( data );

		uint32_t* pixels = (uint32_t*)data;
		pixels[0] = pixelValue;
		pixels[1] = pixelValue;
		data = (void*)( (uint8_t*)data + pitch );
		pixels = (uint32_t*)data;
		pixels[0] = pixelValue;
		pixels[1] = pixelValue;

		m_defaultTexture->UnlockBuffer();
	}
}

void Tr2Sprite2dScene::RemoveFinishedCurveSets()
{
	// Remember, m_curveSets is a BlueList, must use Remove to get ref-count right.
	for( ssize_t i = 0; i < m_curveSets.GetSize(); )
	{
		TriCurveSet* cs = m_curveSets[i];
		if( !cs->IsPlaying() )
		{
			m_curveSets.Remove( i );
		}
		else
		{
			++i;
		}
	}
}

// Prepare required resources for rendering. If any of them are not ready
// this function returns false, otherwise it returns true and rendering can
// go forward.
bool Tr2Sprite2dScene::PrepareResourcesForRender()
{
	// Prepare resources if we don't have valid index or vertex buffers
	if( !m_indexBuffer.IsValid() || !m_vertexBuffer.IsValid() )
	{
		PrepareResources();
		// Bail out if the buffers still aren't valid
		if( !m_indexBuffer.IsValid() || !m_vertexBuffer.IsValid() )
		{
			return false;
		}
	}

	Tr2EffectResPtr effectRes = m_uberShader2d->GetEffectRes();
	if( !effectRes || effectRes->IsLoading() )
	{
		// Effect is still loading - can't render any UI
		return false;
	}

	if( !effectRes->IsGood() )
	{
		// Effect load failed - can't render any UI
		CCP_LOGERR( "%s: Effect used for rendering failed to load - attempting to reload", __FUNCTION__ );
		effectRes->Reload();
		return false;
	}

	return true;
}

void Tr2Sprite2dScene::SetViewportSizeToVariableStore( float displayWidth, float displayHeight )
{
	Vector4 vpSize;
	vpSize.x = displayWidth;
	vpSize.y = displayHeight;
	vpSize.z = m_is2dRender ? 1.0f : 0.0f;
	vpSize.w = 1.0f;
	m_viewportSizeVar = vpSize;
}

void Tr2Sprite2dScene::DetermineViewportSize()
{
	float displayWidth;
	float displayHeight;

	if( m_isFullscreen )
	{
		const TriViewport& vp = Tr2Renderer::GetViewport();
		m_displayWidth = (float)vp.width;
		m_displayHeight = (float)vp.height;

		displayWidth = m_displayWidth;
		displayHeight = m_displayHeight;
	}
	else
	{
		displayWidth = m_displayWidth;
		displayHeight = m_displayHeight;
	}

	SetViewportSizeToVariableStore( displayWidth, displayHeight );
}

void Tr2Sprite2dScene::DetermineWorldTransform()
{
	Matrix worldTransform;

	// 2d render context starts out as the global setting for the scene.
	// In a 3d scene this switches to 2d if we hit a layer.
	m_is2dRenderContext = m_is2dRender;
	if( m_is2dRender )
	{
		worldTransform = Tr2Renderer::GetInverseViewTransform();
	}
	else
	{
		worldTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
	}
	Tr2Renderer::SetWorldTransform( worldTransform );
}

void Tr2Sprite2dScene::PrepareRenderContextForRendering( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_SPRITE2D );

	if( m_clearBackground )
	{
		renderContext.Clear( CLEARFLAGS_TARGET, m_backgroundColor, 0, 0 );
	}

	renderContext.m_esm.SetWireframeRendering( m_drawWireFrame );

	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDecl );
	renderContext.m_esm.ApplyIndexBuffer( m_indexBuffer.GetBuffer() );
}

void Tr2Sprite2dScene::PrepareRenderContextAfterRendering( Tr2RenderContext& renderContext )
{
	renderContext.m_esm.EndManagedRendering();
	renderContext.m_esm.SetWireframeRendering( false );

	// EndManagedRendering clears textures set on device
	for( unsigned int i = 0; i < s_textureMax; ++i )
	{
		m_texture[i] = nullptr;
	}
}

void Tr2Sprite2dScene::PrepareStacksBeforeRender()
{
	CCP_ASSERT( m_depthStack->empty() );
	CCP_ASSERT( m_clipStack->empty() );
	CCP_ASSERT( m_transformStack->empty() );

	m_depthStack->push_back( Vector2( m_depthMin, m_depthMax ) );
	m_clipStack->push_back( Tr2Sprite2dClipRect( -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX ) );
}

void Tr2Sprite2dScene::CleanUpStacksAfterRender()
{
	m_depthStack->pop_back();
	m_clipStack->pop_back();

	CCP_ASSERT( m_depthStack->empty() );
	CCP_ASSERT( m_clipStack->empty() );
	CCP_ASSERT( m_transformStack->empty() );
}

void Tr2Sprite2dScene::SetSpriteVerticesUVs( Vector2 uv[2][4], float width, float height )
{
	for( int i = 0; i < m_numTexturesUsed; ++i )
	{
		const TextureSetting& texSetting = m_textureSettings[i];

		uv[i][0] = Vector2( 0.0f, 0.0f );
		uv[i][1] = Vector2( 1.0f, 0.0f );
		uv[i][2] = Vector2( 1.0f, 1.0f );
		uv[i][3] = Vector2( 0.0f, 1.0f );

		if( texSetting.tileX )
		{
			Tr2AtlasTexture* at = m_texture[i];

			// TexturesReady at the start guarantees that at is not null
			float rTexWidth = at->GetWidthReciprocal();
			for( int j = 0; j < 4; ++j )
			{
				uv[i][j].x *= width * rTexWidth;
			}
		}
		if( texSetting.tileY )
		{
			Tr2AtlasTexture* at = m_texture[i];

			// TexturesReady at the start guarantees that at is not null
			float rTexHeight = at->GetHeightReciprocal();
			for( int j = 0; j < 4; ++j )
			{
				uv[i][j].y *= height * rTexHeight;
			}
		}

		if( texSetting.useTransform )
		{
			for( int j = 0; j < 4; ++j )
			{
				Vector4 uvT( uv[i][j].x, uv[i][j].y, 0.f, 1.f );
				uvT = Transform( uvT, texSetting.transform );
				uv[i][j].x = uvT.x;
				uv[i][j].y = uvT.y;
			}
		}

		float offsetX = texSetting.textureWindow.x;
		float offsetY = texSetting.textureWindow.y;
		float scaleX = texSetting.textureWindow.z;
		float scaleY = texSetting.textureWindow.w;

		for( int j = 0; j < 4; ++j )
		{
			Vector2& currentUV = uv[i][j];
			currentUV.x = currentUV.x * scaleX + offsetX;
			currentUV.y = currentUV.y * scaleY + offsetY;
		}
	}
}

// Grow the vertex buffer used for capturing display lists. 'vertexCount' is the number
// vertices that need to be added.
void Tr2Sprite2dScene::GrowCaptureVertexBuffer( unsigned int vertexCount )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_captureVertexDataCapacity *= 2;
	if( m_captureVertexDataSize + vertexCount >= m_captureVertexDataCapacity )
	{
		m_captureVertexDataCapacity = m_captureVertexDataSize + vertexCount;
	}

	size_t vbSize = m_captureVertexDataCapacity * sizeof( Tr2Sprite2dD3DVertex );
	m_captureVertexData.resize( "Tr2Sprite2dScene/m_captureVertexData", vbSize );

	CCP_ASSERT( m_captureVertexData );
}

// Grow the index buffer used for capturing display lists. 'indexCount' is the number
// indices that need to be added.
void Tr2Sprite2dScene::GrowCaptureIndexBuffer( unsigned short indexCount )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_captureIndexDataCapacity *= 2;
	if( m_captureIndexDataSize + indexCount >= m_captureIndexDataCapacity )
	{
		m_captureIndexDataCapacity = m_captureIndexDataSize + indexCount;
	}

	size_t ibSize = m_captureIndexDataCapacity * sizeof( unsigned int );
	m_captureIndexData.resize( "Tr2Sprite2dScene/m_captureIndexData", ibSize );

	CCP_ASSERT( m_captureIndexData );
}

// Copy indices to current index data, adding the given vertex offset
void Tr2Sprite2dScene::CopyIndicesWithOffset( unsigned short* indices, unsigned short indexCount, int vertexOffset )
{
	unsigned short* curIndex = indices;
	for( unsigned int i = 0; i < indexCount; ++i )
	{
		*m_currentIndexData = *curIndex + vertexOffset;
		++m_currentIndexData;
		++curIndex;
	}

	m_indexCount += indexCount;
}

bool Tr2Sprite2dScene::EnsureBufferSpace( unsigned int vertexCount, unsigned short indexCount, int& vertexOffset )
{
	if( m_captureDisplayList )
	{
		CCP_STATS_ZONE( "Tr2Sprite2dScene::EnsureBufferSpace capture" );

		// Ensure we have enough space for vertices.
		if( m_captureVertexDataSize + vertexCount >= m_captureVertexDataCapacity )
		{
			GrowCaptureVertexBuffer( vertexCount );

			if( !m_captureVertexData )
			{
				return false;
			}

			m_currentVertexData = reinterpret_cast<Tr2Sprite2dD3DVertex*>( m_captureVertexData.get() ) + m_captureVertexDataSize;
		}

		// Index count is estimated, allow the index buffer to grow as needed.
		if( m_captureIndexDataSize + indexCount >= m_captureIndexDataCapacity )
		{
			GrowCaptureIndexBuffer( indexCount );

			if( !m_captureIndexData )
			{
				return false;
			}

			m_currentIndexData = reinterpret_cast<unsigned int*>( m_captureIndexData.get() ) + m_captureIndexDataSize;
		}

		vertexOffset = m_captureVertexDataSize;
		m_captureVertexDataSize += vertexCount;
		m_captureIndexDataSize += indexCount;
	}
	else
	{
		CCP_STATS_INC( spriteSceneCount );

		m_itemsRendered++;
		if( m_itemsRendered >= m_maxItemsToRender )
		{
			return false;
		}

		if( ( m_vertexCount + vertexCount >= m_maxSpriteCount * 4 ) || ( m_indexCount + indexCount >= m_maxSpriteCount * 6 ) )
		{
			// Buffers are full, kick off what we've got
			IssueDrawCall();
		}

		CCP_ASSERT( ( m_vertexCount + vertexCount <= m_maxSpriteCount * 4 ) && ( m_indexCount + indexCount <= m_maxSpriteCount * 6 ) );
		if( ( m_vertexCount + vertexCount > m_maxSpriteCount * 4 ) || ( m_indexCount + indexCount > m_maxSpriteCount * 6 ) )
		{
			return false;
		}

		vertexOffset = m_vertexCount;
	}

	// We have supposedly ensured above that there is room for vertices and indices being added.
	// Nonetheless we have crash dumps that indicate we get here with a null pointer for either
	// m_currentVertexData or m_currentIndexData. This could happen if locking vertex/index buffer
	// failed - rather than crashing we ignore this batch and hope we are successful in locking
	// the buffers later.
	CCP_ASSERT( m_currentVertexData );
	if( !m_currentVertexData )
	{
		return false;
	}

	CCP_ASSERT( m_currentIndexData );
	if( !m_currentIndexData )
	{
		return false;
	}

	return true;
}

bool Tr2Sprite2dScene::IsInsideClipRect( const Vector2& point )
{
	// Clip stack is in absolute coordinates. If a clip rectangle is set, we first
	// look to see if the point is inside it.
	if( !m_clipStack->empty() )
	{
		const Tr2Sprite2dClipRect& clipRect = m_clipStack->back();

		if( point.x < clipRect.left )
		{
			return false;
		}
		if( point.x > clipRect.right )
		{
			return false;
		}

		if( point.y < clipRect.top )
		{
			return false;
		}
		if( point.y > clipRect.bottom )
		{
			return false;
		}
	}

	return true;
}

void Tr2Sprite2dScene::TransformPoint( Vector2& result, const Vector2& point, const Matrix& m )
{
	Vector4 point4( point.x, point.y, 0, 1 );
	Vector4 transformed = Transform( point4, m );

	result.x = transformed.x;
	result.y = transformed.y;
}

bool Tr2Sprite2dScene::IsUsingLinearColorSpace() const
{
	return m_useLinearColorSpace;
}

void Tr2Sprite2dScene::SetUseLinearColorSpace( bool use )
{
	m_useLinearColorSpace = use;
	if( m_uberShader2d )
	{
		m_uberShader2d->SetOption( BlueSharedString( "COLOR_SPACE" ), BlueSharedString( m_useLinearColorSpace ? "COLOR_SPACE_LINEAR" : "COLOR_SPACE_SRGB" ) );
	}
	if( m_uberShader3d )
	{
		m_uberShader3d->SetOption( BlueSharedString( "COLOR_SPACE" ), BlueSharedString( m_useLinearColorSpace ? "COLOR_SPACE_LINEAR" : "COLOR_SPACE_SRGB" ) );
	}
}
void Tr2Sprite2dScene::SetGammaCorrectText( bool use )
{
	m_isGammaCorrectingText = use;
	m_uberShader2d->SetOption( BlueSharedString( "FONT_GAMMACORRECT" ), BlueSharedString( m_isGammaCorrectingText ? "ENABLED" : "DISABLED" ) );
}

bool Tr2Sprite2dScene::IsGammaCorrectingText()
{
	return m_isGammaCorrectingText;
}
