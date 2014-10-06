#include "StdAfx.h"
#include "EveSpaceScene.h"

#include "include/IEveBallpark.h"

#include "TriProjection.h"
#include "TriPythonContext.h"
#include "TriView.h"
#include "TriConstants.h"
#include "Tr2RuntimeInstanceData.h"
#include "TriDevice.h"

BLUE_DEFINE( EveSpaceScene );
BLUE_DEFINE_INTERFACE( IEveReferencePoint );
BLUE_DEFINE_INTERFACE( IEveBallpark );

Be::VarChooser EveVisualizerChooser[] =
{
	{
		"None",     
		BeCast( EveSpaceScene::VM_NONE ),     
		"No visualizer - use normal rendering"
	},
	{
		"TexCoord0",
		BeCast( EveSpaceScene::VM_TEXCOORD0 ),     
		""
	},
	{
		"TexCoord1",
		BeCast( EveSpaceScene::VM_TEXCOORD1 ),
		""
	},
	{
		"White",
		BeCast( EveSpaceScene::VM_WHITE ),
		""
	},
	{
		"Overdraw",
		BeCast( EveSpaceScene::VM_OVERDRAW ),
		""
	},
	{
		"Wireframe",
		BeCast( EveSpaceScene::VW_WIREFRAME ),
		""
	},
	{ 0 }
};


#if BLUE_WITH_PYTHON
PyObject* PyPickObjectAndAreaID( PyObject* self, PyObject* args )
{
	TriPythonContext pythonCtx;
	EveSpaceScene* pThis = BluePythonCast<EveSpaceScene*>( self );

	PyObject* pyProjection = NULL;
	PyObject* pyView = NULL;
	PyObject* pyViewport = NULL;

	int x, y;
	if (!PyArg_ParseTuple(args, "iiOOO",
		&x,
		&y,
		&pyProjection,
		&pyView,
		&pyViewport
		))
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
	object = pThis->PickObjectAndArea( x, y, projection, view, viewport, areaID, renderContext );

	if( object )
	{
		PyObject *result =  PyTuple_New( 2 );
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

		MAP_ATTRIBUTE
		( 
			"display", 
			m_display, 
			"If display is off, scene is not rendered (may still be updated)", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"update", 
			m_update, 
			"If update is off, scene is not updated (may still be rendered)", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"enableShadows", 
			m_enableShadows, 
			"If true, shadow maps are generated for objects that are estimated to exceed"
			"'shadowThreshold' in pixel diameter.", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"enableShadowObb", 
			m_enableShadowObb, 
			"If true, use OBBs to focus the shadow maps.", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"enableShadowDistanceTweak", 
			m_enableShadowDistanceTweak, 
			"for debugging",
			Be::READWRITE | Be::PERSIST
		)		
		
		MAP_ATTRIBUTE
		( 
			"selfShadowOnly", 
			m_selfShadowOnly, 
			"If true, shadow maps that are generated for objects render only the object"
			"itself into the shadow map. This means that ships only cast shadows on"
			"themselves, not other ships, but is faster.", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"renderDebugInfo", 
			m_renderDebugInfo, 
			"If true, objects are given a chance to render debugging info.", 
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		( 
			"debugShowShadowCasters", 
			m_debugShowShadowCasters, 
			"If set, shadow casters and shadowing light direction is identified (if scene is showing debug info).", 
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"displayShadowMap", 
			m_displayShadowMap, 
			"Displays the shadowmap on top of the screen (if scene is showing debug info).", 
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		(
			"displayShadowMapMipLevel", 
			m_displayShadowMapMipLevel, 
			"Determines shadow map miplevel displayed",
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"backgroundEffect", 
			m_backgroundEffect, 
			"Effect used for rendering background. Geometry rendered is a camera space full-screen quad.", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		( 
			"starfield", 
			m_starfield, 
			"Renders stars that glimmer against the nebula background.", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		( 
			"backgroundObjects", 
			m_backgroundObjects, 
			"Objects in the background, drawn after background skybox", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"warpTunnel", 
			m_warpTunnel, 
			"The warp tunnel", 
			Be::READWRITE
		)

		MAP_ATTRIBUTE
		( 
			"backgroundRenderingEnabled", 
			m_backgroundRenderingEnabled, 
			"If true, background (nebula, starfield, and background objects) are rendered.",
			Be::READWRITE | Be::PERSIST
		)
		
		MAP_ATTRIBUTE
		( 
			"planets", 
			m_planets, 
			"Planets in space", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"objects", 
			m_objects, 
			"Objects in space", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"curveSets", 
			m_curveSets, 
			"Curvesets for animating things", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"lensflares", 
			m_lensflares, 
			"Lensflares of this scene (sun, etc.)", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_PROPERTY
		( 
			"shLightingManager", 
			GetShLightingManager, 
			SetShLightingManager,
			"SH lighting manager (Tr2ShLightingManager)"
		)

		MAP_ATTRIBUTE
		( 
			"shLightingManager", 
			m_shLightingManager, 
			"",
			Be::PERSISTONLY
		)

		MAP_ATTRIBUTE
		(
			"envMapRotation",
			m_envMapRotation,
			"Texture transform rotation applied to all envMaps",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE_WITH_CHOOSER
        ( 
            "envMapResPath", 
            m_envMapResPath, 
            "Resource path for the scene's environemnt map aka the nebula.",
            Be::READWRITE | Be::PERSIST | Be::NOTIFY,
            TriTextureChooser 
        )
		MAP_ATTRIBUTE_WITH_CHOOSER
        ( 
            "envMap1ResPath", 
            m_envMap1ResPath, 
            "Resource path for EnvMap1 shader variable.", 
            Be::READWRITE | Be::PERSIST | Be::NOTIFY, 
            TriTextureChooser 
        )
		MAP_ATTRIBUTE
		(
			"envMap1",
			m_envMap1,
			"Value of EnvMap1 shader variable.",
			Be::READ
		)
		MAP_ATTRIBUTE_WITH_CHOOSER
        ( 
            "envMap2ResPath", 
            m_envMap2ResPath, 
            "Resource path for EnvMap2 shader variable.", 
            Be::READWRITE | Be::PERSIST | Be::NOTIFY,
            TriTextureChooser 
        )
		MAP_ATTRIBUTE
		(
			"envMap2",
			m_envMap2,
			"Value of EnvMap2 shader variable.",
			Be::READ
		)
		MAP_ATTRIBUTE_WITH_CHOOSER
        ( 
            "envMap3ResPath", 
            m_envMap3ResPath, 
            "Resource path for EnvMap3 shader variable.", 
            Be::READWRITE | Be::PERSIST | Be::NOTIFY, 
            TriTextureChooser 
        )
		MAP_ATTRIBUTE
		(
			"envMap3",
			m_envMap3,
			"Value of EnvMap3 shader variable.",
			Be::READ
		)
		MAP_ATTRIBUTE( "sceneLightMgr", m_sceneLightMgr, "Manages lights and lighting for this scene", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE
		(
			"depthTexture",
			m_depthMap,
			".",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(
			"distortionTexture",
			m_distortionMap,
			".",
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		(    
			"sunBall",
			m_sunBall,
			"The position of the sun, from which to derive the sunlight direction for the scene.",
			Be::READWRITE | Be::PERSIST
		)
		MAP_ATTRIBUTE
		( 
			"dustfield", 
			m_dustfield, 
			"n/a", 
			Be::READWRITE | Be::PERSIST 
		)
		MAP_ATTRIBUTE
		( 
			"dustfieldConstraint", 
			m_dustfieldConstaint, 
			"n/a", 
			Be::READWRITE
		)
		MAP_ATTRIBUTE
		( 
			"ballpark", 
			m_ballpark, 
			"The ballpark for this space scene", 
			Be::READWRITE
		)

		MAP_ATTRIBUTE( "sunDiffuseColor", m_sunData.DiffuseColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sunDirection", m_sunData.DirWorld, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "ambientColor", m_ambientColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "nebulaIntensity", m_nebulaIntensity, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogColor", m_fogColor, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogStart", m_fogStart, "Depth at which fogging starts.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogEnd", m_fogEnd, "Depth at which the fog does not get thicker.", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogMax", m_fogMax, "Maximum strength of fog at end depth, range [0,1].", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogType", m_fogType, "Blend between static fog color (0.0) and dynamic nebula background (1.0).", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "fogBlur", m_fogBlur, "Blur level of dynamic nebula background.", Be::READWRITE | Be::PERSIST )
		
		MAP_ATTRIBUTE( "shadowMap", m_shadowMap, "Shadow map used to shadow the whole space scene.", Be::READWRITE )

		MAP_ATTRIBUTE
		( 
			"shadowThreshold", 
			m_shadowThreshold, 
			"Minimum estimated size of objects that receive shadows.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"shadowFadeThreshold", 
			m_shadowFadeThreshold, 
			"Estimated size of objects where shadow begins to fade.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"shadowReceiverMaxCount", 
			m_shadowReceiverMaxCount, 
			"To help avoid horrible performance in degenerate situations we\n"
			"put a hard limit on the number of shadow maps drawn per frame.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"shadowCasterMaxCount", 
			m_shadowCasterMaxCount, 
			"To help avoid horrible performance in degenerate situations we\n"
			"put a hard limit on the number of shadow casters drawn into\n"
			"each shadow map.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE
		( 
			"shadowCameraDistance", 
			m_shadowCameraDistance, 
			"Shadow camera distance from shadow receiver.", 
			Be::READWRITE | Be::PERSIST
		)

		MAP_ATTRIBUTE_WITH_CHOOSER
		( 
			"visualizeMethod", 
			m_visualizeMethod,
			"Changes rendering to a visualizing method instead of normal rendering",
			Be::READWRITE | Be::ENUM, 
			EveVisualizerChooser
		)

		MAP_ATTRIBUTE
		(
			"wireframe",
			m_isWireframe,
			"Forces rendering to wireframe mode",
			Be::READWRITE
		)
		
		MAP_ATTRIBUTE
		(
			"dynamicClipPlanes",
			m_dynamicClipPlanes,
			"",
			Be::READWRITE
		)
		
		MAP_ATTRIBUTE
		( 
			"nearClip", 
			m_nearClip, 
			"", 
			Be::READ
		)

		MAP_ATTRIBUTE
		( 
			"farClip", 
			m_farClip, 
			"", 
			Be::READ
		)

		MAP_METHOD_AND_WRAP(
			"PickObject",
			PickObject,
			"Given mouse position and a view setup, returns the object that the mouse is over"
			"\nreturns <Object> or None if nothing pickable was hit by the ray"
			"\n"
			"\nArguments:"
			"\nx - integer x coordinate of the mouse over the viewport"
			"\ny - integer y coordinate of the mouse over the viewport"
			"\nprojection - The TriProjection to use to pick into the scene"
			"\nview - The TriView to use to pick into the scene"
			"\nviewport - The TriViewport of the viewport to use to pick into the scene" )

		MAP_METHOD(
			"PickObjectAndAreaID",
			PyPickObjectAndAreaID,
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as an additional value depending on what has been clicked"
			"\nreturns (<Object>,<AreaID>) or None if nothing pickable was hit by the ray"
			"\n"
			"\nArguments:"
			"\nx - integer x coordinate of the mouse over the viewport"
			"\ny - integer y coordinate of the mouse over the viewport"
			"\nprojection - The TriProjection to use to pick into the scene"
			"\nview - The TriView to use to pick into the scene"
			"\nviewport - The TriViewport of the viewport to use to pick into the scene" )

		MAP_METHOD_AND_WRAP("UpdateScene", UpdateSceneFromScript, "Run the scene's update loop")
		
		MAP_METHOD_AND_WRAP("PickInfinity", PickInfinity, "Pick infinity")
		
		MAP_PROPERTY( 
			"gpuParticlePoolManager", 
			GetParticlePoolManager,
			SetParticlePoolManager,
			"Apply a particle pool manager to the update context" )

		MAP_ATTRIBUTE(
			"updateTime",
			m_updateTime,
			"Time of the last call to Update, for this scene",
			Be::READ )

    EXPOSURE_END()
}

extern bool g_eveIsSpaceObjectResourceUnloadingEnabled;

#if BLUE_WITH_PYTHON
static PyObject* PySetEveSpaceObjectResourceUnloadingEnabled( PyObject* self, PyObject* args )
{
	bool enabled = false;

	if( !PyArg_ParseTuple( args, "b", &enabled ))
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
	"for extended periods. This can reduce memory use, potentially at the cost of framefrate."
);
#endif



static float CameraDistanceFalloff( const Vector3& posView )
{
	// This is some fucked up legacy math
	float distSq = D3DXVec3Dot( &posView, &posView );

	const Matrix& projMat = Tr2Renderer::GetProjectionTransform();
	float fov = atan( 1.0f / projMat.m[1][1] );
	float fovHeight = sin( fov ) * distSq ;
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
	if ( !PyArg_ParseTuple( 
		args, 
		"O!iiO!O!O!O!f", 
		Tr2RuntimeInstanceData::ClassType_()->mTypeObject, &pyData,
		&x,
		&y, 
		&PyTuple_Type, &pyWorld,
		TriView::ClassType_()->mTypeObject, &pyView,
		TriProjection::ClassType_()->mTypeObject, &pyProj,
		TriViewport::ClassType_()->mTypeObject, &pyViewport,
		&constPickRadius
		) )
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
		return PyInt_FromLong( -1 );
	}

	const Tr2VertexDefinition& def = data->GetLayout();
	if( def.m_items.empty() )
	{
		return PyInt_FromLong( -1 );
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
		return PyInt_FromLong( -1 );
	}

	float fx, fy;
	Vector3 startWorld;
	Vector3 dirWorld;
	gTriDev->ScreenToProjection(x, y, &fx, &fy, viewport);

	Matrix projMat;
	proj->GetMatrixWithoutViewAdjustment( projMat );
	const Matrix& viewMat = view->GetTransform();

	Matrix worldView;
	D3DXMatrixMultiply( &worldView, &world, &viewMat );

	// Scaling of particles is done in view space in the shader

	Matrix projection2view;
	if( !D3DXMatrixInverse( &projection2view, NULL, &projMat ) )
	{
		return PyInt_FromLong( -1 );
	}

	Vector3 rayStart( fx, fy, 0.0f );
	Vector3 rayEnd( fx, fy, 0.5f );

	// Note that this method does _unprojection_ i.e. divides by w
	D3DXVec3TransformCoord( &rayStart, &rayStart, &projection2view );
	D3DXVec3TransformCoord( &rayEnd, &rayEnd, &projection2view );

	Vector3 rayDirection = rayEnd - rayStart;
	D3DXVec3Normalize( &rayDirection, &rayDirection );

	// We cannot assume that the particles are sorted by distance
	// since we'd frequently want to turn this off for the map (which is additive)
	// and because to know what the ID returned actually means, you wouldn't want it moving around
	
	const Vector3* closestParticle = nullptr;
	int closestParticleID = -1;

	unsigned count = data->GetCount();
	float closestParticleDistanceSq = FLT_MAX;

	for( unsigned int i = 0; i < count; ++i )
	{
		const Vector3* position = reinterpret_cast<const Vector3*>( positionStream );
		
		Vector3 particlePositionInViewSpace;
		D3DXVec3TransformCoord( &particlePositionInViewSpace, position, &worldView );

		if( particlePositionInViewSpace.z < 0.0f )
		{
			if( D3DXSphereBoundProbe(
				&particlePositionInViewSpace, 
				constPickRadius,
				&rayStart, 
				&rayDirection ) )
			{
				float distanceSq = D3DXVec3LengthSq( &particlePositionInViewSpace );
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

	return PyInt_FromLong( closestParticleID );
}

MAP_FUNCTION
( 
	"PickParticle", 
	PyPickParticle, 
	"Pick a particle from Tr2RuntimeInstanceData store using the current view and projection matrices\n"
	"assuming instances are spheres with a constant radius.\n" 
	"Arguments:\n"
	"data - Tr2RuntimeInstanceData object\n"
	"x - screen x mouse coordinate (in pixels)\n"
	"y - screen y mouse coordinate (in pixels)\n"
	"world - world transform\n"
	"view - view transform (TriView)\n"
	"proj - projection transform (TriProjection)\n"
	"viewport - current viewport (TriViewport)\n"
	"radius - instance radius"
);
#endif

