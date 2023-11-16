#include "StdAfx.h"

#include "Tr2InteriorScene.h"

#include "TriConstants.h"
#include "TriPythonContext.h"
#include "TriProjection.h"
#include "TriView.h"
#include "TriViewport.h"

BLUE_DEFINE( Tr2InteriorScene );

static Be::VarChooser VisualizerChooser[] =
{
	{
		"VM_NONE",     
		BeCast( VM_NONE ),     
		"No visualizer - use normal rendering"
	},
	{
		"VM_WHITE",     
		BeCast( VM_WHITE ),     
		"Pixel shader returns white (useful to verify that something is output)"
	},

	{
		"VM_OBJECT_NORMAL",     
		BeCast( VM_OBJECT_NORMAL ),     
		"Normal from vertices"
	},
	{
		"VM_TANGENT",     
		BeCast( VM_TANGENT ),     
		"Tangent from vertices"
	},
	{
		"VM_BITANGENT",     
		BeCast( VM_BITANGENT ),     
		"Bitangent from vertices"
	},

	{
		"VM_TEXCOORD0",     
		BeCast( VM_TEXCOORD0 ),     
		"Texture coordinate 0"
	},
	{
		"VM_TEXCOORD1",     
		BeCast( VM_TEXCOORD1 ),     
		"Texture coordinate 0"
	},

	{
		"VM_TEXELDENSITY0",     
		BeCast( VM_TEXELDENSITY0 ),     
		"Density of texels mapped through texture coordinate 0"
	},
	{
		"VM_NORMALMAP",     
		BeCast( VM_NORMALMAP ),     
		"Tangent space normal from map"
	},
	{
		"VM_DIFFUSEMAP",     
		BeCast( VM_DIFFUSEMAP ),     
		"Diffuse map"
	},
	{
		"VM_SPECULARMAP",     
		BeCast( VM_SPECULARMAP ),     
		"Specular map"
	},
	{
		"VM_OVERDRAW",
		BeCast( VM_OVERDRAW ),
		"See the overdraw of the scene"
	},
	{
		"VM_EN_ONLY",
		BeCast( VM_EN_ONLY ),
		"Enlighten only)"
	},
	{
		"VM_DEPTH",
		BeCast( VM_DEPTH ),
		"See the depth buffer of the scene"
	},
	{
		"VM_ALL_LIGHTING",
		BeCast( VM_ALL_LIGHTING ),
		"See cummulative direct and secondary lighting"
	},
	{
		"VM_LIGHT_PRE_PASS_NORMALS",
		BeCast( VM_LIGHT_PRE_PASS_NORMALS ),
		"normal in worldspace from light pre-pass texture"
	},
	{
		"VM_LIGHT_PRE_PASS_DEPTH",
		BeCast( VM_LIGHT_PRE_PASS_DEPTH ),
		"depth in clip space from light pre-pass texture"
	},
	{
		"VM_LIGHT_PRE_PASS_WORLD_POSITION",
		BeCast( VM_LIGHT_PRE_PASS_WORLD_POSITION ),
		"world position from light pre-pass texture"
	},
	{
		"VM_LIGHT_PRE_PASS_LIGHTING",
		BeCast( VM_LIGHT_PRE_PASS_LIGHTING ),
		"resulting lighting from pre-pass light accumulation texture"
	},
	{
		"VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW",
		BeCast( VM_LIGHT_PRE_PASS_LIGHT_OVERDRAW ),
		"light geometry overdraw during light accumulation pass"
	},
	{
		"VM_LIGHT_PRE_PASS_DIFFUSE_LIGHTING",
		BeCast( VM_LIGHT_PRE_PASS_DIFFUSE_LIGHTING ),
		"resulting deffuse lighting from pre-pass light accumulation texture"
	},
	{
		"VM_LIGHT_PRE_PASS_SPECULAR_LIGHTING",
		BeCast( VM_LIGHT_PRE_PASS_SPECULAR_LIGHTING ),
		"resulting specular lighting from pre-pass light accumulation texture"
	},
	{
		"VM_OCCLUSION",
		BeCast( VM_OCCLUSION ),
		"render occlusion geometry on top of the normal geometry"
	},
	{ 0 }
};

// Register the enum as trinity.Tr2InteriorVisualizerMethod
BLUE_REGISTER_ENUM_EX( "Tr2InteriorVisualizerMethod", 
					  VisualizeMethod, 
					  VisualizerChooser, 
					  ENUM_REG_ENUM_OBJECT_ON_MODULE );

static Be::VarChooser PickComponentChooser[] =
{
	{
		"PICK_OBJECT",     
		BeCast( ITr2PickableScene::PICK_OBJECT ),     
		"Return object the mouse is over"
	},
	{
		"PICK_AREA",     
		BeCast( ITr2PickableScene::PICK_AREA ),     
		"Return mesh and area indexes of the object the mouse is over"
	},
	{
		"PICK_POSITION",     
		BeCast( ITr2PickableScene::PICK_POSITION ),     
		"Return world position of the point the mouse is over"
	},
	{
		"PICK_UV",     
		BeCast( ITr2PickableScene::PICK_UV ),     
		"Return object UV coordinates of the point the mouse is over"
	},
	{ 0 }
};

// Register the enum as trinity.Tr2PickComponent
BLUE_REGISTER_ENUM_EX( "Tr2PickComponent", 
					  ITr2PickableScene::PickComponent, 
					  PickComponentChooser, 
					  ENUM_REG_ENUM_OBJECT_ON_MODULE );

#if BLUE_WITH_PYTHON
static PyObject* PyPick(PyObject* self, PyObject* args)
{
	TriPythonContext pythonCtx;
	Tr2InteriorScene* pThis = BluePythonCast<Tr2InteriorScene*>( self );

	PyObject* pyProjection = NULL;
	PyObject* pyView = NULL;
	PyObject* pyViewport = NULL;

	int x, y, components;
	if (!PyArg_ParseTuple(args, "iiOOOi",
		&x,
		&y,
		&pyProjection,
		&pyView,
		&pyViewport,
		&components
		))
	{
		return NULL;
	}

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

	ITr2PickableScene::PickResults results;
	results.components = components;
	results.object = NULL;
	results.area = 0;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	pThis->PickObject(renderContext, x, y, projection, view, viewport, results );

	PyObject* result = PyDict_New();
	if( results.components & ITr2PickableScene::PICK_OBJECT )
	{
		PyObject* key = PyInt_FromLong( ITr2PickableScene::PICK_OBJECT );
		PyObject* value;
		if( results.object )
		{
			value = PyOS->WrapBlueObject( results.object );
		}
		else
		{
			value = Py_None;
			Py_IncRef( value );
		}
		PyDict_SetItem( result, key, value );
		Py_DECREF( key );
		Py_DECREF( value );
	}
	if( results.components & ITr2PickableScene::PICK_AREA )
	{
		unsigned int areaID = ((1<<8) - 1) & results.area;
		unsigned int meshID = (results.area - areaID)>>8;

		PyObject* key = PyInt_FromLong( ITr2PickableScene::PICK_AREA );

		PyObject* value = PyTuple_New( 2 );
		PyTuple_SET_ITEM( value, 0, PyLong_FromUnsignedLong( meshID ) ); 
		PyTuple_SET_ITEM( value, 1, PyLong_FromUnsignedLong( areaID - 1 ) );

		PyDict_SetItem( result, key, value );

		Py_DECREF( key );
		Py_DECREF( value );
	}
	if( results.components & ITr2PickableScene::PICK_POSITION )
	{
		PyObject* key = PyInt_FromLong( ITr2PickableScene::PICK_POSITION );
		PyObject* value = Py_BuildValue("(fff)", results.position.x, results.position.y, results.position.z );
		PyDict_SetItem( result, key, value );
		Py_DECREF( key );
		Py_DECREF( value );
	}
	if( results.components & ITr2PickableScene::PICK_UV )
	{
		PyObject* key = PyInt_FromLong( ITr2PickableScene::PICK_UV );
		PyObject* value = Py_BuildValue("(ff)", results.uv.x, results.uv.y );
		PyDict_SetItem( result, key, value );
		Py_DECREF( key );
		Py_DECREF( value );
	}
	return result;
}

PyObject* PyPickObjectAndArea(PyObject* self, PyObject* args)
{
	TriPythonContext pythonCtx;
	Tr2InteriorScene* pThis = BluePythonCast<Tr2InteriorScene*>( self );

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

	ITr2PickableScene::PickResults results;
	results.components = ITr2PickableScene::PICK_OBJECT | ITr2PickableScene::PICK_AREA;
	results.object = NULL;
	results.area = 0;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	pThis->PickObject( renderContext, x, y, projection, view, viewport, results );

	if( results.object )
	{
		PyObject *result =  PyTuple_New(2);
		PyObject *meshAndArea =  PyTuple_New(2);

		unsigned int areaID = ((1<<8) - 1) & results.area;
		unsigned int meshID = (results.area - areaID)>>8;

		PyTuple_SET_ITEM(meshAndArea, 0, PyLong_FromUnsignedLong(meshID));
		// areaID goes from 1 upwards, rebase to 0
		PyTuple_SET_ITEM(meshAndArea, 1, PyLong_FromUnsignedLong(areaID-1));

		PyTuple_SET_ITEM(result, 0, PyOS->WrapBlueObject(results.object)); 
		PyTuple_SET_ITEM(result, 1, meshAndArea);
		return result;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

PyObject* PyInteriorPickPointAndObject( PyObject* self, PyObject* args )
{

	TriPythonContext pythonCtx;
	Tr2InteriorScene* pThis = BluePythonCast<Tr2InteriorScene*>( self );

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

	ITr2PickableScene::PickResults results;
	results.components = ITr2PickableScene::PICK_OBJECT | ITr2PickableScene::PICK_POSITION;
	results.object = NULL;

	USE_MAIN_THREAD_RENDER_CONTEXT();
	pThis->PickObject( renderContext, x, y, projection, view, viewport, results );

	if( results.object )
	{
		PyObject *result =  PyTuple_New(2);
		PyTuple_SET_ITEM(result, 0, PyOS->WrapBlueObject(results.object) ); 
		PyTuple_SET_ITEM(result, 1, Py_BuildValue("(fff)", results.position.x, results.position.y, results.position.z ) );
		return result;
	}

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

const Be::ClassInfo* Tr2InteriorScene::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2InteriorScene, "" )
		MAP_INTERFACE( Tr2InteriorScene )
		MAP_INTERFACE( ITr2Scene )
		MAP_INTERFACE( ITr2MultiPassScene )
		MAP_INTERFACE( IInitialize )
		MAP_INTERFACE( INotify )
		MAP_INTERFACE( ITr2Updateable )

		MAP_ATTRIBUTE( "lights", m_lights, "List of scene light sources", Be::READ | Be::PERSIST | Be::NOTIFY )
		MAP_ATTRIBUTE( "dynamics", m_dynamics, "List of dynamic objects in scene", Be::READ | Be::PERSIST | Be::NOTIFY )

		MAP_ATTRIBUTE( "sunDiffuseColor", m_sunDiffuseColor, "Sun diffuse color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sunSpecularColor", m_sunSpecularColor, "Sun specular color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "sunDirection", m_sunDirection, "Sun direction", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "ambientColor", m_ambientColor, "Scene Ambient color", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "curveSets", m_curveSets, "", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "lightRenderTargets", m_lightRenderTargets, "Spotlight Shadow Maps", Be::READ )

		MAP_ATTRIBUTE( "backgroundEffect", m_backgroundEffect, "The effect used to render the background behind any objects", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE_WITH_CHOOSER( "backgroundCubemapPath", m_backgroundCubeMapPath, "The path used to load the background environment map", Be::READWRITE | Be::PERSIST | Be::NOTIFY, TriTextureChooser )
		MAP_ATTRIBUTE( "backgroundCubemapRes", m_backgroundCubeMapRes, "The background environment map", Be::READ )
		
		MAP_ATTRIBUTE_WITH_CHOOSER( "visualizeMethod", m_visualizeMethod, "Changes rendering to a visualizing method instead of normal rendering", Be::READWRITE | Be::ENUM | Be::NOTIFY, VisualizerChooser )

        MAP_ATTRIBUTE( "fogColor", m_fogColor, "Fog color", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "maxFogAmount", m_maxFogAmount, "Maximum fog density amount (from 0 to 1)", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "minFogDistance", m_minFogDistance, "Distance where fog density starts to grow from 0", Be::READWRITE | Be::PERSIST )
        MAP_ATTRIBUTE( "maxFogDistance", m_maxFogDistance, "Distance where fog reaches maximum density", Be::READWRITE | Be::PERSIST )

		MAP_METHOD( 
			"Pick", 
			PyPick, 
			"Given mouse position and a view setup, can return the object that the mouse is over, the mesh and area indices "
			"and object\'s texture coordinates"
			"\n returns dict (pick attribute -> value) or empty dict if nothing pickable was hit by the ray"
			"\n:param x: integer x coordinate of the mouse over the viewport"
			"\n:type x: int"
			"\n:param y: integer y coordinate of the mouse over the viewport"
			"\n:type y: int"
			"\n:param projection: The TriProjection to use to pick into the scene"
			"\n:type projection: TriProjection"
			"\n:param view: The TriView to use to pick into the scene"
			"\n:type view: TriView"
			"\n:param viewport: The TriViewport of the viewport to use to pick into the scene"
			"\n:type viewport: TriViewport"
			"\n:param components: bitfield for components to pick"
			"\n:type components: int"
			"\n:rtype: dict"
		)

		MAP_METHOD( 
			"PickObjectAndArea", 
			PyPickObjectAndArea, 
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as the mesh and area indices"
			"\n returns (<Object>,(<MeshID>,<AreaID>)) or None if nothing pickable was hit by the ray"
			"\n:param x: integer x coordinate of the mouse over the viewport"
			"\n:type x: int"
			"\n:param y: integer y coordinate of the mouse over the viewport"
			"\n:type y: int"
			"\n:param projection: The TriProjection to use to pick into the scene"
			"\n:type projection: TriProjection"
			"\n:param view: The TriView to use to pick into the scene"
			"\n:type view: TriView"
			"\n:param viewport: The TriViewport of the viewport to use to pick into the scene"
			"\n:type viewport: TriViewport"
			"\n:rtype: None | tuple"
		)
		
		MAP_METHOD( 
			"PickPointAndObject", 
			PyInteriorPickPointAndObject, 
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as the mesh and area indices"
			"\n returns (<Object>,(x,y,z)) or None if nothing pickable was hit by the ray"
			"\n:param x: integer x coordinate of the mouse over the viewport"
			"\n:type x: int"
			"\n:param y: integer y coordinate of the mouse over the viewport"
			"\n:type y: int"
			"\n:param projection: The TriProjection to use to pick into the scene"
			"\n:type projection: TriProjection"
			"\n:param view: The TriView to use to pick into the scene"
			"\n:type view: TriView"
			"\n:param viewport: The TriViewport of the viewport to use to pick into the scene"
			"\n:type viewport: TriViewport"
			"\n:rtype: None | tuple"
		)
		
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"PickObject", 
			PickObjectOnly, 
			1,
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as the mesh and area indices"
			"\nreturns <Object> or None if nothing pickable was hit by the ray"
			"\n:param x: integer x coordinate of the mouse over the viewport"
			"\n:param y: integer y coordinate of the mouse over the viewport"
			"\n:param projection: The TriProjection to use to pick into the scene"
			"\n:param view: The TriView to use to pick into the scene"
			"\n:param viewport: The TriViewport of the viewport to use to pick into the scene"
			"\n:param filter: Bitfield of pickable object types"
		)

		MAP_METHOD_AND_WRAP( 
			"PickObjectUV", 
			PickObjectUV, 
			"Given mouse position and a view setup, returns the UV coordinates of the texel the mouse is over."
			"\nreturns 2-float tuple with UV coordinates"
			"\n:param x: integer x coordinate of the mouse over the viewport"
			"\n:param y: integer y coordinate of the mouse over the viewport"
			"\n:param projection: The TriProjection to use to pick into the scene"
			"\n:param view: The TriView to use to pick into the scene"
			"\n:param viewport: The TriViewport of the viewport to use to pick into the scene"
		)

		MAP_METHOD_AND_WRAP( "RebuildSceneData", RebuildSceneData, "Rebuilds the internal data in all cells" )

		 MAP_METHOD_AND_WRAP( 
			"AddLightSource", 
			AddLightSource, 
			"Add an interior lightsource to the scene"
			"\n:param light: The light to add" )

		MAP_METHOD_AND_WRAP( 
			"RemoveLightSource", 
			RemoveLightSource, 
			"Remove an interior lightsource from the scene"
			"\n:param light: The light to remove" )

		MAP_METHOD_AND_WRAP( 
			"AddDynamic", 
			AddDynamic, 
			"Add an interior dynamic (avatar, placeable, etc.) to the scene"
			"\n:param object: The ITr2InteriorDynamic (Tr2InteriorPlaceable or Tr2InteriorAvatar) to add")

		MAP_METHOD_AND_WRAP( 
			"RemoveDynamic", 
			RemoveDynamic, 
			"Remove an interior dynamic (avatar, placeable, etc.) from the scene"
			"\n:param object: The ITr2InteriorDynamic (Tr2InteriorPlaceable or Tr2InteriorAvatar) to remove")

		MAP_METHOD_AND_WRAP(
			"UpdateScene", 
			UpdateSceneFromScript, 
			"Run the scene's update loop\n"
			":param time: current time" )

		MAP_METHOD_AND_WRAP(
			"SetupShadowMaps",
			SetupShadowMaps,
			"Set up the shadow render targets")

		MAP_ATTRIBUTE
		(
			"debugRenderer",
			m_debugRenderer,
			"Object used for rendering debug information",
			Be::READWRITE
		)

		MAP_ATTRIBUTE( "visibilityResults", m_visibilityResults, "Results of the visibility query", Be::READ )

		MAP_ATTRIBUTE( "shadowCount", m_shadowCount, "Amount of shadows allowed", Be::READWRITE | Be::NOTIFY)

		MAP_ATTRIBUTE("shadowSize", m_shadowSize, "Size of shadows", Be::READWRITE | Be::NOTIFY)

		MAP_ATTRIBUTE("optimizeShadows", m_optimizeShadows, "Should we shrink the light frustum to camera view?", Be::READWRITE | Be::NOTIFY)

		MAP_ATTRIBUTE("renderShadows", m_renderShadows, "", Be::READWRITE | Be::NOTIFY)
				
		MAP_ATTRIBUTE("debugRenderShadowMaps", m_debugRenderShadowMaps, "", Be::READWRITE | Be::NOTIFY)

	EXPOSURE_END()
}

