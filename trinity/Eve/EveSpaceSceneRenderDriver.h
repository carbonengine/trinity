// Copyright © 2026 CCP ehf.

#pragma once

#include "EveSpaceScene.h"
#include "../Tr2LightManager.h"
#include "../PostProcess/Tr2PostProcessRenderer.h"
#include "../Tr2GpuResourcePool.h"
#include "../ITr2RenderNode.h"
#include "../Tr2ProfileTimer.h"


BLUE_DECLARE( Tr2RenderTarget );
BLUE_DECLARE( TriProjection );
BLUE_DECLARE( TriView );
BLUE_DECLARE( EveSpaceScene );
BLUE_DECLARE( TriCurveSet );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE( TriStepRenderFps );
BLUE_DECLARE_IVECTOR( ITr2Scene );
BLUE_DECLARE( EveCamera );
BLUE_DECLARE( Tr2Sprite2dScene );




BLUE_CLASS( EveSpaceSceneRenderDriver ) :
	public ITr2RenderNode,
	public Tr2DeviceResource
{
public:
	enum class AntiAliasingQuality
	{
		Disabled,
		Low,
		Medium,
		High,
	};
	enum class AmbientOcclusionQuality
	{
		Disabled,
		Low,
		Medium,
		High,
	};
	struct Settings
	{
		ShadowQuality shadowQuality = ShadowQuality::SHADOW_HIGH;
		AntiAliasingQuality antiAliasingQuality = AntiAliasingQuality::High;
		AmbientOcclusionQuality aoQuality = AmbientOcclusionQuality::High;
		PostProcess::Quality postProcessingQuality = PostProcess::Quality::HIGH;
		Tr2VolumerticQuality volumetricQuality = Tr2VolumerticQuality::High;
		LightingQuality lightingQuality = LightingQuality::HIGH;
		ReflectionQuality reflectionQuality = ReflectionQuality::OFF;

		Color clearColor = Color( 0.0f, 0.0f, 0.0f, 1.0f );

		bool forceOpaqueBuffer = false;
		bool forceNormalMap = false;
		bool forceVelocityMap = false;
		bool enableUpscaling = true;
		bool enableDistortion = true; // should be tied to shader quality
		bool showFPS = false;
		EveSpaceScene::EveVisualizeMethod visualizeMethod = EveSpaceScene::VM_NONE;
	};

	EveSpaceSceneRenderDriver( IRoot* lockobj = nullptr );
	~EveSpaceSceneRenderDriver();


	bool Validate( const Span<const Tr2BitmapDimensions>& destDimensions, const Span<const BlueSharedString>& outputs, Be::Time realTime, Be::Time simTime ) override;
	void Execute( const Span<const Tr2TextureAL>& destinations, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext ) override;


	void SetSettings( const Settings& settings )
	{
		m_settings = settings;
	}

	void SetDebugMode( bool enable );
	bool GetDebugMode() const;

	std::vector<Tr2TextureReferencePtr> GetAllTempTextures() const;

	EveSpaceScene* GetScene() const;
	void SetScene( EveSpaceScene * scene );

	EXPOSE_TO_BLUE();

private:

	// The driver's features a view uses. What the scene renders is described by the
	// SceneRender::ViewInfo in RenderInfo. The features are off by default and every factory
	// sets every feature, so a new feature has to be decided for each kind of view.
	struct Features
	{
		bool upscaling = false;
		bool antiAliasing = false;
		bool ssao = false;
		bool shadows = false;
		bool velocityMap = false;
		// special background rendering (f.ex texture background)
		bool background = false;
		// special overlay rendering (f.ex texture overlay)
		bool overlay = false;
		bool dynamicLights = false;

		static Features Main();
		static Features ReflectionFace();
	};

	// A view rendered by RenderView: its camera and what the scene renders from it, the
	// driver's features, and its view in the Tr2LightManager
	struct RenderInfo
	{
		EveSceneView::Camera camera;
		EveSceneView::ViewInfo scene;
		Features features;
		// the view's index in the Tr2LightManager, set by SetLightViews: 0 is the main view,
		// then the reflection faces that have dynamic lights
		uint32_t lightView = 0;
	};

	// Targets rendered by RenderView
	struct ViewTargets
	{
		Tr2GpuResourcePool::Texture depth;
		Tr2GpuResourcePool::Texture velocity;
		Tr2GpuResourcePool::Texture opaqueColor;
	};

	// The faces the reflection probe renders in a frame, built before the lights are gathered
	struct ReflectionFaces
	{
		Tr2ReflectionProbe* probe = nullptr;
		unsigned startFace = 0;
		Vector3 viewDirection = Vector3( 0.0f, 0.0f, 0.0f );
		std::vector<RenderInfo> views;
	};

	ViewTargets RenderView( const RenderInfo& view, const Tr2TextureAL& colorTarget, const Span<TempOutput>& outputs, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext, const uint32_t colorTargetSlice = 0 );
	ReflectionFaces PrepareReflectionFaces( Tr2RenderContext& renderContext );
	void SetLightViews( const RenderInfo& mainView, ReflectionFaces& faces, Tr2RenderContext& renderContext );
	void RenderReflectionFaces( const ReflectionFaces& faces, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext );
	void SetupUpscaling( const TextureSize2D& displaySize );
	void PropagateSettings();
	void ApplyCamera( const EveSceneView::Camera& camera ) const;
	void UpdateSceneFrame( const EveSceneView::Camera& mainCamera, Be::Time realTime, Be::Time simTime, const Tr2ProfileTimer& rootTimer, Tr2RenderContext& renderContext );
	void SetCameraToRenderer( Tr2RenderContext & renderContext ) const;
	TextureSize2D GetRenderSize( const TextureSize2D& displaySize ) const;
	EveSceneView::Camera GetMainCamera( const TextureSize2D& renderSize, const Matrix& jitterMatrix, float mipLodBias ) const;

	Matrix GetJitterMatrix( uint32_t renderWidth, uint32_t renderHeight, Tr2RenderContext& renderContext ) const;
	float GetMipLodBias( ) const;

	Tr2GpuResourcePool::Texture GetDistortionMapIfNeeded( const TextureSize2D& size, bool distortion );
	Tr2GpuResourcePool::Texture GetVelocityMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs, const Features& featureSet );
	Tr2GpuResourcePool::Texture GetNormalMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs, const Features& featureSet );
	Tr2GpuResourcePool::Texture GetCustomStencilMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs );
	Tr2GpuResourcePool::Texture GetOpaqueColorMapIfNeeded( const TextureSize2D& size, const Span<TempOutput>& outputs, const Features& featureSet );

	void UpdateGpuParticleSystem( Tr2RenderContext & renderContext );
	void PrepareLightManager();

	Tr2GpuResourcePool::Texture RenderSSAO( const Tr2TextureAL& depthMap, const Tr2TextureAL& normalMap, Tr2RenderContext& renderContext );

	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	EveSpaceScenePtr m_scene;

	Tr2SSAOPtr m_ssao;


	Tr2GpuResourcePool m_gpuResourcePool;

	TriProjectionPtr m_projection;
	TriViewPtr m_view;

	EveCameraPtr m_camera;

	ImageIO::PixelFormat m_internalPixelFormat = ImageIO::PIXEL_FORMAT_R16G16B16A16_FLOAT;
	Tr2UpscalingContextAL* m_upscalingContext = nullptr;
	Tr2UpscalingAL::UpscalingInfo m_upscalingInfo;

	Settings m_settings;

	Tr2EffectPtr m_distortionEffect;

	PITr2SceneVector m_toolsScenes;

	Tr2PostProcessRendererPtr m_postProcess;
	TriStepRenderFpsPtr m_fpsRenderer;

	ITr2RenderNodePtr m_background;
	ITr2RenderNodePtr m_sceneOverlay;

	std::string m_name;
	bool m_enableRendering = true;
	bool m_mainPassRenderingEnabled = true;
	ImageIO::PixelFormat m_customStencilFormat = ImageIO::PIXEL_FORMAT_UNKNOWN;
	BlueSharedString m_depthPassTechnique = BlueSharedString( "Depth" );

	// View and projection matrices from last frame, for velocity calculations
	Matrix m_viewLast = IdentityMatrix();
	Matrix m_projectionLast = IdentityMatrix();

	bool m_reflectionCorrectionEnabled;
	TriTextureResPtr m_reflectionCorrectionMap;
	TriTextureResPtr m_blackReflectionCorrectionMap;

	struct
	{
		Tr2ProfileTimer update;
		Tr2ProfileTimer beginRender;
		Tr2ProfileTimer endRender;
		Tr2ProfileTimer postProcess;
		Tr2ProfileTimer ui3d;
		Tr2ProfileTimer background;
		Tr2ProfileTimer depthPass;
		Tr2ProfileTimer mainPass;
		Tr2ProfileTimer reflections;
		Tr2ProfileTimer shadows;
		Tr2ProfileTimer ssao;
	} m_timers;
};

TYPEDEF_BLUECLASS( EveSpaceSceneRenderDriver );
