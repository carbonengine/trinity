#include "StdAfx.h"
#include "EveSpaceScene.h"

#include "include/IEveBallpark.h"

#include "TriProjection.h"
#include "TriPythonContext.h"
#include "TriView.h"
#include "TriConstants.h"
#include "Tr2RuntimeInstanceData.h"
#include "TriDevice.h"
#include "Shader/Tr2ShaderBuffer.h"
#include "Particle/Tr2GpuParticleSystem.h"

BLUE_DEFINE( EveSpaceScene );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

Be::VarChooser EveVisualizerChooser[] =
	{
		{ "None",
		  BeCast( EveSpaceScene::VM_NONE ),
		  "No visualizer - use normal rendering" },
		{ "TexCoord0",
		  BeCast( EveSpaceScene::VM_TEXCOORD0 ),
		  "" },
		{ "TexCoord1",
		  BeCast( EveSpaceScene::VM_TEXCOORD1 ),
		  "" },
		{ "White",
		  BeCast( EveSpaceScene::VM_WHITE ),
		  "" },
		{ "Overdraw",
		  BeCast( EveSpaceScene::VM_OVERDRAW ),
		  "" },
		{ "Wireframe",
		  BeCast( EveSpaceScene::VW_WIREFRAME ),
		  "" },
		{ "LightCount",
		  BeCast( EveSpaceScene::VW_LIGHT_COUNT ),
		  "" },
		{ 0 }
	};
BLUE_REGISTER_ENUM_EX( "EveVisualizeMethod", EveSpaceScene::EveVisualizeMethod, EveVisualizerChooser, ENUM_REG_ENUM_OBJECT_ON_MODULE );


#if BLUE_WITH_PYTHON
PyObject* PyPickObjectAndAreaID( PyObject* self, PyObject* args )
{
	TriPythonContext pythonCtx;
	EveSpaceScene* pThis = BluePythonCast<EveSpaceScene*>( self );

	PyObject* pyProjection = NULL;
	PyObject* pyView = NULL;
	PyObject* pyViewport = NULL;

	int x, y, flags = PICK_TYPE_PICKING | PICK_TYPE_OPAQUE;
	if( !PyArg_ParseTuple( args, "iiOOO|i", &x, &y, &pyProjection, &pyView, &pyViewport, &flags ) )
		return NULL;

	TriProjection* projection = NULL;
	if( !BlueExtractArgument( pyProjection, projection, 3 ) )
	{
		return NULL;
	}

	TriView* view = NULL;
	if( !BlueExtractArgument( pyView, view, 4 ) )
	{
		return NULL;
	}

	TriViewport* viewport = NULL;
	if( !BlueExtractArgument( pyViewport, viewport, 5 ) )
	{
		return NULL;
	}

	IRoot* object = NULL;
	unsigned int areaID = 0;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	object = pThis->PickObjectAndArea( x, y, projection, view, viewport, areaID, Tr2PickTypes( flags ), renderContext );

	if( object )
	{
		PyObject* result = PyTuple_New( 2 );
		PyTuple_SET_ITEM( result, 0, PyOS->WrapBlueObject( object ) );
		PyTuple_SET_ITEM( result, 1, PyLong_FromUnsignedLong( areaID ) );
		return result;
	}

	Py_INCREF( Py_None );
	return Py_None;
}
#endif




const Be::ClassInfo* EveSpaceScene::ExposeToBlue()
{
	EXPOSURE_BEGIN( EveSpaceScene, "" )
		MAP_INTERFACE( EveSpaceScene )
		MAP_INTERFACE( ITr2Scene )
		MAP_INTERFACE( ITr2MultiPassScene )
		MAP_INTERFACE( ITr2Updateable )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2NamedPredicate )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"The name of the scene",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"display",
			m_display,
			"If display is off, scene is not rendered (may still be updated)",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"update",
			m_update,
			"If update is off, scene is not updated (may still be rendered)",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"enableShadows",
			m_enableShadows,
			"If true, generate shadow map for scene\n"
			":jessica-group: Shadows",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"freezeFrustum",
			m_freezeFrustum,
			"If true, zoom out to see the frustum split\n"
			":jessica-group: Shadows",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"displayShadowMap",
			m_displayShadowMap,
			"Displays the shadowmap on top of the screen (unfiltered).\n"
			":jessica-group: Shadows",
			Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"DisableShadows",
			DisableShadows,
			"Set to disable shadows" )

		MAP_ATTRIBUTE(
			"backgroundEffect",
			m_backgroundEffect,
			"Effect used for rendering background. Geometry rendered is a camera space full-screen quad.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"starfield",
			m_starfield,
			"Renders stars that glimmer against the nebula background.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"backgroundObjects",
			m_backgroundObjects,
			"Objects in the background, drawn after background skybox",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"warpTunnel",
			m_warpTunnel,
			"The warp tunnel",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"backgroundRenderingEnabled",
			m_backgroundRenderingEnabled,
			"If true, background (nebula, starfield, and background objects) are rendered.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"planets",
			m_planets,
			"Planets in space",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"planetScale",
			m_planetScale,
			"Scaling factor applied to planets when rendering. This should normally be the same\n"
			"as the planetCameraScale to have planets render correctly with other objects in the\n"
			"scene.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"planetCameraScale",
			m_planetCameraScale,
			"Scaling factor applied to the camera when rendering planets. This should normally be the same\n"
			"as the planetScale to have planets render correctly with other objects in the scene, but can\n"
			"be changed independently to simulate moving the camera away.",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"distanceFields",
			m_distanceFields,
			"Distance fields used for environment atmospherics",
			Be::READ )

		MAP_ATTRIBUTE(
			"objects",
			m_objects,
			"Objects in space",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"uiObjects",
			m_uiObjects,
			"Objects in space",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"debugRenderer",
			m_debugRenderer,
			"Object used for rendering debug information",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"curveSets",
			m_curveSets,
			"Curvesets for animating things",
			Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"lensflares",
			m_lensflares,
			"Lensflares of this scene (sun, etc.)",
			Be::READ | Be::PERSIST )

		MAP_PROPERTY_READONLY(
			"cameraAttachments",
			GetCameraAttachments,
			"List of effect children attached to the camera" )

		MAP_PROPERTY(
			"shLightingManager",
			GetShLightingManager,
			SetShLightingManager,
			"SH lighting manager (Tr2ShLightingManager)" )

		MAP_ATTRIBUTE(
			"shLightingManager",
			m_shLightingManager,
			"",
			Be::PERSISTONLY )

		MAP_ATTRIBUTE(
			"envMapRotation",
			m_envMapRotation,
			"Texture transform rotation applied to all envMaps\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"envMapResPath",
			m_envMapResPath,
			"Resource path for the scene's environemnt map aka the nebula.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			TriTextureChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"envMap1ResPath",
			m_envMap1ResPath,
			"Resource path for EnvMap1 shader variable.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			TriTextureChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"lowQualityNebulaResPath",
			m_lowQualityNebulaResPath,
			"Resource path for lowQualityNebulaResPath, relates to externalParameter: LQ_Nebula.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			TriTextureChooser )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"lowQualityNebulaMixResPath",
			m_lowQualityNebulaMixResPath,
			"Resource path for lowQualityNebulaResPath, used to populate externalParameter: LQ_NebulaMix1 and LQ_NebulaMix2.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			TriTextureChooser )
		MAP_ATTRIBUTE(
			"envMap1",
			m_envMap1,
			"Value of EnvMap1 shader variable.\n"
			":jessica-group: Lighting",
			Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"envMap2ResPath",
			m_envMap2ResPath,
			"Resource path for EnvMap2 shader variable.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			TriTextureChooser )
		MAP_ATTRIBUTE(
			"envMap2",
			m_envMap2,
			"Value of EnvMap2 shader variable.\n"
			":jessica-group: Lighting",
			Be::READ )
		MAP_ATTRIBUTE_WITH_CHOOSER(
			"envMap3ResPath",
			m_envMap3ResPath,
			"Resource path for EnvMap3 shader variable.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY,
			TriTextureChooser )
		MAP_ATTRIBUTE(
			"envMap3",
			m_envMap3,
			"Value of EnvMap3 shader variable.\n"
			":jessica-group: Lighting",
			Be::READ )
		MAP_ATTRIBUTE(
			"reflectionProbe",
			m_reflectionProbe,
			"Reflection probe for the scene.\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::NOTIFY )
		MAP_ATTRIBUTE(
			"depthTexture",
			m_depthMap,
			".",
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"colorTexture",
			m_colorMap,
			".",
			Be::READWRITE 
		)
		MAP_ATTRIBUTE(
			"opaqueColorTexture",
			m_opaqueColorMap,
			".",
			Be::READWRITE 
		)
		MAP_ATTRIBUTE
		(
			"normalTexture",
			m_normalMap,
			".",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"SSAO",
			m_ssao,
			".",
			Be::READ )
		MAP_ATTRIBUTE(
			"distortionTexture",
			m_distortionMap,
			".",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"velocityMap",
			m_velocityMap,
			".",
			Be::READWRITE )
		MAP_ATTRIBUTE(
			"sunBall",
			m_sunBall,
			"The position of the sun, from which to derive the sunlight direction for the scene.",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"staticParticles",
			m_staticParticles,
			"n/a",
			Be::READ )
		MAP_ATTRIBUTE(
			"dataTextureMgr",
			m_dataTextureMgr,
			"A module helping with data transfer from CPU to GPU via textures.",
			Be::READ )
		MAP_ATTRIBUTE(
			"ballpark",
			m_ballpark,
			"The ballpark for this space scene",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"sunDiffuseColor", m_sunColor, "Sun color when dynamic lighting is turned off or when useSunDiffuseColorWithDynamicLights is False\n"
										   ":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"sunDiffuseColorWithDynamicLights",
			m_sunColorWithDynamicLights,
			"Sun color when dynamic lighting is turned on and useSunDiffuseColorWithDynamicLights is True\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"useSunDiffuseColorWithDynamicLights",
			m_useSunColorWithDynamicLights,
			"Use sunDiffuseColorWithDynamicLights when dynamic lighting is turned on\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sunDirection", m_sunData.DirWorld, ":jessica-group: Lighting", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "ambientColor", m_ambientColor, ":jessica-group: Lighting", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"nebulaIntensity",
			m_nebulaIntensity,
			"How intense is the nebula\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogColor", m_fogColor, ":jessica-group: Fog\n:jessica-tuple-type: linearcolor", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogStart", m_fogStart, "Depth at which fogging starts.\n:jessica-group: Fog", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogEnd", m_fogEnd, "Depth at which the fog does not get thicker.\n:jessica-group: Fog", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogMax", m_fogMax, "Maximum strength of fog at end depth, range [0,1].\n:jessica-group: Fog", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE( "cascadedShadowMap", m_cascadedShadowMap, "This should get created in python.", Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE_WITH_CHOOSER(
			"visualizeMethod",
			m_visualizeMethod,
			"Changes rendering to a visualizing method instead of normal rendering",
			Be::READWRITE | Be::ENUM,
			EveVisualizerChooser )

		MAP_ATTRIBUTE( "perFrameDebug", m_perFrameDebug, "This is a debug/visualization value, which gets passed to the perframePS data, so the shader can use it.", Be::READWRITE )

		MAP_METHOD_AND_WRAP(
			"GetPostProcessPSBuffer",
			GetPostProcessPSBuffer,
			"" )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"PickObject",
			PickObject,
			1,
			"Given mouse position and a view setup, returns the object that the mouse is over\n"
			"returns <Object> or None if nothing pickable was hit by the ray\n"
			":param x: integer x coordinate of the mouse over the viewport\n"
			":param y: integer y coordinate of the mouse over the viewport\n"
			":param projection: The TriProjection to use to pick into the scene\n"
			":param view: The TriView to use to pick into the scene\n"
			":param viewport: The TriViewport of the viewport to use to pick into the scene\n"
			":param filter: Bitfield of pickable object types" )

		MAP_METHOD(
			"PickObjectAndAreaID",
			PyPickObjectAndAreaID,
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as an additional value depending on what has been clicked\n"
			"returns (<Object>,<AreaID>) or None if nothing pickable was hit by the ray\n"
			":param x: integer x coordinate of the mouse over the viewport\n"
			":type x: int\n"
			":param y: integer y coordinate of the mouse over the viewport\n"
			":type y: int\n"
			":param projection: The TriProjection to use to pick into the scene\n"
			":type projection: int\n"
			":param view: The TriView to use to pick into the scene\n"
			":type view: int\n"
			":param viewport: The TriViewport of the viewport to use to pick into the scene\n"
			":type viewport: int\n"
			":rtype: None | (IRoot, long)\n" )

		MAP_METHOD_AND_WRAP(
			"UpdateScene",
			UpdateSceneFromScript,
			"Run the scene's update loop\n"
			":param time: current time" )

		MAP_METHOD_AND_WRAP(
			"PickInfinity",
			PickInfinity,
			"Pick infinity\n"
			":param x: integer x coordinate of the mouse over the viewport\n"
			":param y: integer y coordinate of the mouse over the viewport\n"
			":param projection: The TriProjection to use to pick into the scene\n"
			":param view: The TriView to use to pick into the scene\n" )

		MAP_ATTRIBUTE(
			"updateTime",
			m_updateTime,
			"Time of the last call to Update, for this scene",
			Be::READ )

		MAP_ATTRIBUTE(
			"pixelOffsetScale",
			m_taaPixelOffsetScale,
			"Size of the offset in pixels for TAA",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"taaSubpixelPattern",
			m_taaPattern,
			"Pattern type for TAA sampling offsets."
			"\n 0 - none"
			"\n 1 - totally random"
			"\n 2 - 2x pattern"
			"\n 3 - 3x pattern"
			"\n 4 - 4x pattern",
			Be::READWRITE )


		MAP_PROPERTY_READONLY(
			"quadRenderer",
			GetQuadRenderer,
			"Quad renderer used for batch-rendering of space object attachments" )

		MAP_PROPERTY( "gpuParticleSystem", GetGpuParticleSystem, SetGpuParticleSystem, "" );

		MAP_ATTRIBUTE( "externalParameters", m_externalParameters, "List of external parameters to bind to scene elements", Be::READ | Be::PERSIST )

		MAP_ATTRIBUTE(
			"reflectionIntensity",
			m_reflectionIntensity,
			"Determines how intense the reflection is\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"reflectionBackLightingContrast",
			m_reflectionBackLightingContrast,
			"Reflection probe back light contrast\n"
			":jessica-group: Lighting",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"reflectionBackLightingColor",
			m_reflectionBackLightingColor,
			"Reflection probe back light color\n"
			":jessica-group: Lighting\n"
			":jessica-widget: color",
			Be::READWRITE | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"dynamicObjectReflectionEnabled",
			m_dynamicObjectReflectionEnabled,
			"Are reflections enabled for selected dynamic objects\n"
			":jessica-group: Lighting\n",
			Be::READWRITE )


		MAP_ATTRIBUTE(
			"impostorManager",
			m_impostorManager,
			"Impostor manager",
			Be::READWRITE )

		MAP_ATTRIBUTE(
			"componentRegistry",
			m_componentRegistry,
			"Component Registry",
			Be::READ )

		MAP_ATTRIBUTE(
			"postprocess",
			m_postProcess,
			"The post process",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"virtualCameraSystem",
			m_virtualCameraSystem,
			"The virtual camera system",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"volumetricsRenderer",
			m_volumetricsRenderer,
			"Renderer for volumetric objects (clouds, etc.)",
			Be::READ )

		MAP_METHOD_AND_WRAP(
			"ReregisterEntities",
			ReregisterEntities,
			"Re registers all entities" )

	EXPOSURE_END()
}

extern bool g_eveIsSpaceObjectResourceUnloadingEnabled;

#if BLUE_WITH_PYTHON
static PyObject* PySetEveSpaceObjectResourceUnloadingEnabled( PyObject* self, PyObject* args )
{
	bool enabled = false;

	if( !PyArg_ParseTuple( args, "b", &enabled ) )
	{
		return NULL;
	}

	g_eveIsSpaceObjectResourceUnloadingEnabled = enabled;

	Py_RETURN_NONE;
}

MAP_FUNCTION(
	"SetEveSpaceObjectResourceUnloadingEnabled",
	PySetEveSpaceObjectResourceUnloadingEnabled,
	"If set, Eve space objects unload rendering resources if objects are not visible\n"
	"for extended periods. This can reduce memory use, potentially at the cost of framefrate.\n"
	":param enabled: enable/disable unloading\n"
	":type eanbled: bool\n"
	":rtype: None" );
#endif



static float CameraDistanceFalloff( const Vector3& posView )
{
	// This is some fucked up legacy math
	float distSq = Dot( posView, posView );

	const Matrix& projMat = Tr2Renderer::GetProjectionTransform();
	float fov = atan( 1.0f / projMat.m[1][1] );
	float fovHeight = sin( fov ) * distSq;
	return 0.2274f * pow( fovHeight, 0.185f );
}

#if BLUE_WITH_PYTHON
static PyObject* PyPickParticle( PyObject* self, PyObject* args )
{
	int x, y;
	float constPickRadius;
	PyObject* pyData;
	PyObject* pyWorld;
	PyObject* pyProj;
	PyObject* pyView;
	PyObject* pyViewport;
	if( !PyArg_ParseTuple(
			args,
			"O!iiO!O!O!O!f",
			Tr2RuntimeInstanceData::ClassType_()->mTypeObject,
			&pyData,
			&x,
			&y,
			&PyTuple_Type,
			&pyWorld,
			TriView::ClassType_()->mTypeObject,
			&pyView,
			TriProjection::ClassType_()->mTypeObject,
			&pyProj,
			TriViewport::ClassType_()->mTypeObject,
			&pyViewport,
			&constPickRadius ) )
	{
		return nullptr;
	}

	Matrix world;
	if( !BlueExtractMatrix( pyWorld, &world.m[0][0], 16 ) )
	{
		PyErr_SetString( PyExc_TypeError, "Argument 4 needs to be a matrix" );
		return nullptr;
	}

	Tr2RuntimeInstanceData* data = BluePythonCast<Tr2RuntimeInstanceData*>( pyData );
	TriProjection* proj = BluePythonCast<TriProjection*>( pyProj );
	TriView* view = BluePythonCast<TriView*>( pyView );
	TriViewport* viewport = BluePythonCast<TriViewport*>( pyViewport );

	if( data->GetData() == 0 )
	{
		return PyLong_FromLong( -1 );
	}

	const Tr2VertexDefinition& def = data->GetLayout();
	if( def.m_items.empty() )
	{
		return PyLong_FromLong( -1 );
	}

	const char* positionStream = nullptr;

	for( auto it = def.m_items.begin(); it != def.m_items.end(); ++it )
	{
		if( it->m_usage == Tr2VertexDefinition::POSITION && it->m_usageIndex == 0 )
		{
			positionStream = reinterpret_cast<const char*>( data->GetData() ) + it->m_offset;
			break;
		}
	}

	if( positionStream == nullptr )
	{
		return PyLong_FromLong( -1 );
	}

	float fx, fy;
	Vector3 startWorld;
	Vector3 dirWorld;
	gTriDev->ScreenToProjection( x, y, &fx, &fy, viewport );

	Matrix projMat;
	proj->GetMatrixWithoutViewAdjustment( projMat );
	const Matrix& viewMat = view->GetTransform();

	Matrix worldView = world * viewMat;

	// Scaling of particles is done in view space in the shader

	Matrix projection2view;
	if( !Inverse( projection2view, projMat ) )
	{
		return PyLong_FromLong( -1 );
	}

	Vector3 rayStart( fx, fy, 0.0f );
	Vector3 rayEnd( fx, fy, 0.5f );

	// Note that this method does _unprojection_ i.e. divides by w
	rayStart = TransformCoord( rayStart, projection2view );
	rayEnd = TransformCoord( rayEnd, projection2view );

	Vector3 rayDirection = Normalize( rayEnd - rayStart );

	// We cannot assume that the particles are sorted by distance
	// since we'd frequently want to turn this off for the map (which is additive)
	// and because to know what the ID returned actually means, you wouldn't want it moving around

	int closestParticleID = -1;

	unsigned count = data->GetCount();
	float closestParticleDistanceSq = FLT_MAX;

	for( unsigned int i = 0; i < count; ++i )
	{
		const Vector3* position = reinterpret_cast<const Vector3*>( positionStream );

		Vector3 particlePositionInViewSpace = TransformCoord( *position, worldView );

		if( particlePositionInViewSpace.z < 0.0f )
		{
			if( SphereBoundProbe(
					particlePositionInViewSpace,
					constPickRadius,
					rayStart,
					rayDirection ) )
			{
				float distanceSq = LengthSq( particlePositionInViewSpace );
				if( distanceSq < closestParticleDistanceSq )
				{
					// p is closer
					closestParticleDistanceSq = distanceSq;
					closestParticleID = (int)i;
				}
			}
		}
		positionStream += data->GetStride();
	}

	return PyLong_FromLong( closestParticleID );
}

MAP_FUNCTION(
	"PickParticle",
	PyPickParticle,
	"Pick a particle from Tr2RuntimeInstanceData store using the current view and projection matrices\n"
	"assuming instances are spheres with a constant radius.\n"
	":param data: Tr2RuntimeInstanceData object\n"
	":type data: Tr2RuntimeInstanceData\n"
	":param x: screen x mouse coordinate (in pixels)\n"
	":type x: float\n"
	":param y: screen y mouse coordinate (in pixels)\n"
	":type y: float\n"
	":param world: world transform\n"
	":type world: tuple[tuple[float]]\n"
	":param view: view transform (TriView)\n"
	":type view: tuple[tuple[float]]\n"
	":param proj: projection transform (TriProjection)\n"
	":type proj: tuple[tuple[float]]\n"
	":param viewport - current viewport (TriViewport)\n"
	":type viewport: TriViewport\n"
	":param radius: instance radius\n"
	":type radius: float\n"
	":rtype: int" );
#endif
