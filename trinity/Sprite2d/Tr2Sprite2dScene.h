#pragma once

#ifndef Tr2Sprite2dScene_h
#define Tr2Sprite2dScene_h


#include "include/ITr2Scene.h"
#include "ITr2SpriteObject.h"
#include "ITr2Sprite2dRenderer.h"
#include "Tr2SpriteObject.h"
#include "Tr2Sprite2dDisplayList.h"
#include "Tr2Variable.h"
#include "Tr2DynamicRingBuffer.h"
#include "Shader/Tr2EffectDescription.h"

BLUE_DECLARE( Tr2Sprite2dScene );
BLUE_DECLARE( Tr2Sprite2d );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE_VECTOR( TriCurveSet );
BLUE_DECLARE_IVECTOR( ITr2SpriteObject );
BLUE_DECLARE( TriViewport );
BLUE_DECLARE( Tr2Effect );

class TriProjection;
class TriView;

class Tr2Sprite2dScene:
     public ITr2Scene,
	 public INotify,
	 public IRenderCallback,
	 public Tr2DeviceResource
{
public:
    EXPOSE_TO_BLUE();
    Tr2Sprite2dScene( IRoot* lockobj = NULL );
	Tr2Sprite2dScene( const Tr2Sprite2dScene& ) = delete;
	Tr2Sprite2dScene& operator=( const Tr2Sprite2dScene& ) = delete;
	~Tr2Sprite2dScene();

	ITr2SpriteObject* PickObject( int x, int y, const TriProjection* proj, const TriView* view, const TriViewport* vp, Be::Optional<int> );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Scene
	virtual void Update( Be::Time realTime, Be::Time simTime );
	virtual void Render( Tr2RenderContext& renderContext );

	virtual void RenderDebugInfo( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITr2Sprite2dRenderer
	void PushTranslation( const Vector2& t );
	void PopTranslation();
	const Vector2& GetTranslation() const;

	void PushDepthRange( float depthMin, float depthMax );
	void PopDepthRange();
	void SetDepth( float depth );
	void PushTransform( const Matrix& m );
	void PopTransform();

	// Set aside current transform stack, start fresh with absolute coordinates
	void PushTransformAbsolute();
	// Pop back to previous transform stack
	void PopTransformAbsolute();

	void PushClipRectangle( float x, float y, float width, float height );
	void PopClipRectangle();
	const Tr2Sprite2dClipRect& GetClipRectangle() const;

	void StartLayer( Tr2TextureAL& rt );
	void EndLayer( float x, float y, float width, float height, ITr2Sprite2dTexture* secondaryTexture );
	void RunJob( TriRenderJob* job );

	void RunJobHelper( TriRenderJob* job );

	void SetTexture( unsigned int ix, Tr2AtlasTexturePtr tex, Tr2Sprite2dTextureSettings settings );
	void SetTextureWindow( unsigned int ix, float x, float y, float width, float height );
	void SetTextureTransform( unsigned int ix, Matrix* m );
	void SetSpriteEffect( Tr2SpriteObjectEffect sfx );
	void SetTileMode( uint8_t tileMode );
	void SetBlendmode( Tr2SpriteObjectBlendMode bm );
	void SetSpriteTarget( Tr2SpriteTarget target );
	void SetGlowBrightness( float glowBrightness );
	void SetColor( const Color& color );
	void SetAccumulatedAlpha( float a );
	float GetAccumulatedAlpha() const;

	bool IsInside( const Vector2& point, const Vector2& topLeft, float width, float height, float radius );

	bool IsInsideLineSegment( const Vector2& pointIn, const Vector2& start, const Vector2& end, float lineWidth );
	bool IsInsideTriangle( const Vector2& pointIn, const Vector2& v0, const Vector2& v1, const Vector2& v2 );
	bool IsInsideClipRect( const Vector2& point );

	Vector2 InverseTransformPoint( const Vector2& point ) const;

	bool PrepareSpriteVerts( 
		Tr2Sprite2dD3DVertex* destVerts,
		const Vector2& pos,
		float width,
		float height,
		Tr2SpriteObjectEffect sfx
		);

	void SetSpriteVerticesUVs( Vector2 uv[2][4], float width, float height );

	bool PrepareTriangleVerts(
		Tr2Sprite2dD3DVertex* destVerts,
		Tr2Sprite2dVertexBase* verts, 
		unsigned int stride, 
		unsigned int vertexCount
		);

	void RenderTriangleVerts( 
		Tr2Sprite2dD3DVertex* verticesSrc, 
		unsigned int vertexCount,
		unsigned short* indices, 
		unsigned short indexCount );

	void RenderTriangleVerts( 
		Tr2BufferAL& verticesSrc, 
		unsigned int vertexCount,
		Tr2BufferAL& indices, 
		unsigned short indexCount );

	bool EnsureBufferSpace( unsigned int vertexCount, unsigned short indexCount, int &vertexOffset );

	bool IsCapturing() const;
	bool StartCapture( ITr2SpriteObject* owner );
	Tr2Sprite2dDisplayList* EndCapture( Tr2Sprite2dDisplayList* previousDisplayList );
	void ReplayCapture( Tr2Sprite2dDisplayList* dl );

	unsigned int GetMaxVertexCountPerDrawCall();
	unsigned int GetMaxIndexCountPerDrawCall();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* value );

	//////////////////////////////////////////////////////////////////////////
	// Tr2Effect::IRenderCallback
	virtual void SubmitGeometry( Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	virtual void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
	bool SelectEffect();
	void IssueDrawCall();

	void ResetBufferPointers();

	// Reset all variables related to display list capture
	void ResetCapture();

	// Copies vertices from source to destination and adds clipping and transform information
	void ProcessVertices( Tr2Sprite2dD3DVertex* verticesSrc, unsigned int vertexCount );
	void CopyIndicesWithOffset( unsigned short* indices, unsigned short indexCount, int vertexOffset );

	void FlashDefaultTexture();
	void RemoveFinishedCurveSets();
	bool PrepareResourcesForRender();
	void DetermineWorldTransform();
	void DetermineViewportSize();
	void SetViewportSizeToVariableStore( float displayWidth, float displayHeight );
	void PrepareRenderContextAfterRendering( Tr2RenderContext &renderContext );
	void PrepareRenderContextForRendering( Tr2RenderContext &renderContext );
	void CleanUpStacksAfterRender();
	void PrepareStacksBeforeRender();
	void GrowCaptureIndexBuffer( unsigned short indexCount );
	void GrowCaptureVertexBuffer( unsigned int vertexCount );


	void TransformPoint( Vector2& result, const Vector2& point, const Matrix& m );

	bool IsUsingLinearColorSpace() const;
	void SetUseLinearColorSpace( bool use );
	void SetGammaCorrectText( bool use );
	bool IsGammaCorrectingText();

private:
	inline bool TexturesReady() const;
	std::wstring m_name;
	bool m_display;

	Be::Time m_realTime;
	Be::Time m_simTime;

	PITr2SpriteObjectVector m_children;
	PITr2SpriteObjectVector m_background;
	PTriCurveSetVector m_curveSets;
	Tr2EffectPtr m_effect;
	uint32_t m_textureRegisters[2];

	Tr2EffectConstant m_transformsHandle;

	// The desired visual effect
	Tr2SpriteObjectEffect m_spriteEffect;

	uint8_t m_tileMode;

	// Blend mode
	Tr2SpriteObjectBlendMode m_blendMode;

	Tr2SpriteTarget m_spriteTarget;
	float m_glowBrightness;

	// Number of textures used by current sprite effect
	int m_numTexturesUsed;

	// Is current sprite effect anti-aliased?
	bool m_isAntiAliased;

	Tr2EffectPtr m_uberShader2d;
	Tr2EffectPtr m_uberShader3d;

	Tr2AtlasTexturePtr m_defaultTexture;
	int m_defaultTextureFlash;
	bool m_defaultTextureUpdates;

	struct TransformStackEntry
	{
		bool isTranslationOnly;
		
		// Keep track of whether the action that set this was translation only
		bool isTranslationOnlySet; 

		Vector2 translation;
		Matrix transform;
	};

	Vector3 m_translation;
	Quaternion m_rotation;
	Vector3 m_scaling;
	float m_depthMin;
	float m_depthMax;
	float m_depth;
	float m_displayWidth;
	float m_displayHeight;
	Color m_backgroundColor;
	Color m_color;
	float m_accumulatedAlpha;

	Vector2 m_lastPickPos;
	Matrix m_lastViewMatrix;
	Matrix m_lastProjectionMatrix;

	Tr2SpriteObjectPickState m_pickState;

	// If set, fill mode is set to wire frame while rendering.
	bool m_drawWireFrame;
	
	// Is the scene fullscreen? If so, m_displayWidth/m_displayHeight
	// are set every frame to match the fullscreen viewport. Note
	// that this is normally done by uilib when used for ui rendering.
	// This should only be used for scenes that are used outside of the
	// the scope of the uilib.
	bool m_isFullscreen;

	// Is the scene rendered in 2d?
	bool m_is2dRender;

	// Is picking done in 3d? A scene rendered to a texture that is
	// then applied to an object in 3d needs 2d rendering and 3d picking.
	bool m_is2dPick;

	// Is the current rendered context in 2d? Note that a scene rendered
	// in 3d switches to a 2d context while rendering layers.
	bool m_is2dRenderContext;

	bool m_clearBackground;
	bool m_clearFinishedCurveSets;

	bool m_useLinearColorSpace;
	bool m_isGammaCorrectingText;

	typedef TrackableStdVector<TransformStackEntry> TransformStack_t;
	typedef TrackableStdVector<Vector2> DepthStack_t;
	typedef TrackableStdVector<Tr2Sprite2dClipRect> ClipStack_t;
	struct StackOfStacksEntry_t
	{
		TransformStack_t* transformStack;
		DepthStack_t* depthStack;
		ClipStack_t* clipStack;
		Tr2TextureAL* renderTargetTexture;
		Tr2AtlasTexturePtr renderTargetWrapper;
	};

	typedef TrackableStdList<StackOfStacksEntry_t> StackOfStacks_t;

	TransformStack_t* m_transformStack;
	DepthStack_t* m_depthStack;
	ClipStack_t* m_clipStack;
	StackOfStacks_t m_stackOfStacks;

	// For performance analysis
	bool m_ignoreClip;

	unsigned int m_vertexCount;
	unsigned int m_indexCount;
	Tr2Sprite2dD3DVertex* m_currentVertexData;
	uint32_t* m_currentIndexData;
	uint32_t m_drawCallStartIndex;

	CcpMallocBuffer m_vertexBufferData;
	CcpMallocBuffer m_indexBufferData;

	unsigned int m_vertexDecl;
	Tr2RingVertexBuffer m_vertexBuffer;
	Tr2RingIndexBuffer m_indexBuffer;

	Tr2Variable m_viewportSizeVar;
	Tr2Variable m_dotVectorVar;

	const static unsigned s_textureMax = 2;
	Tr2AtlasTexturePtr m_texture[s_textureMax];
	Tr2Variable m_texelSizeVar[s_textureMax];
	
	struct TextureSetting
	{
		// Cached texture window, set when texture (or explicit texture window) is set
		Vector4 textureWindow;
		
		bool useTransform;
		Matrix transform;

		bool tileX;
		bool tileY;

		//repetition
		enum TextureRepeat {
			TR_Tile = 0x0,
			TR_Mirror = 0x1,
			TR_Clamp = 0x2
		};
		TextureRepeat repeatMode;
	};

	TextureSetting m_textureSettings[s_textureMax];

	Matrix m_transformArray[ TR2_SS_MAX_TRANSFORM_COUNT ];
	unsigned m_transformCurrent;

	Tr2Sprite2dDisplayList* m_captureDisplayList;
	CcpAlignedMallocBuffer m_captureVertexData;

	Tr2Sprite2dD3DVertex* m_preCaptureVertexData;
	unsigned int m_captureVertexDataSize;
	unsigned int m_captureVertexDataCapacity;
	
	unsigned int* m_preCaptureIndexData;
	CcpAlignedMallocBuffer m_captureIndexData;
	unsigned int m_captureIndexDataSize;
	unsigned int m_captureIndexDataCapacity;
	unsigned int m_captureStartIndex;

	unsigned int m_itemsRendered;
	unsigned int m_maxItemsToRender;
	unsigned int m_drawCallsRendered;
	unsigned int m_maxDrawCallsToRender;

	Tr2ConstantBufferAL	m_uiTransformsCb;

	unsigned int m_maxSpriteCount;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dScene );

#endif //Tr2Sprite2dScene_h
