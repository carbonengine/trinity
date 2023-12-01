#include "StdAfx.h"

#include "Tr2PrimitiveScene.h"

#include "TriProjection.h"
#include "TriView.h"
#include "TriViewport.h"

BLUE_DEFINE( Tr2PrimitiveScene );

#if BLUE_WITH_PYTHON
static PyObject* PyPickPointAndObject( PyObject* self, PyObject* args )
{
	Tr2PrimitiveScene* pThis = BluePythonCast<Tr2PrimitiveScene*>( self );

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


const Be::ClassInfo* Tr2PrimitiveScene::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2PrimitiveScene, "" )
        MAP_INTERFACE( Tr2PrimitiveScene )
		MAP_INTERFACE( ITr2Scene )

		MAP_ATTRIBUTE( "primitives", m_primitives, "A list of primitive sets to render for this scene", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "textLabels", m_textLabels, "A list of text labels to render for this scene", Be::READ | Be::PERSIST )
		MAP_ATTRIBUTE( "manipulator", m_manipulator, "The active manipulator working for the scene", Be::READWRITE )

		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"PickObject", 
			PickObjectOnly,
			1,
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as the mesh and area indices"
			"\nreturns <Object> or None if nothing pickable was hit by the ray"
			"\n"
			"\n:param x: integer x coordinate of the mouse over the viewport"
			"\n:param y: integer y coordinate of the mouse over the viewport"
			"\n:param projection: The TriProjection to use to pick into the scene"
			"\n:param view: The TriView to use to pick into the scene"
			"\n:param viewport: The TriViewport of the viewport to use to pick into the scene"
			"\n:param pickFlags: unused\n"
		)

		MAP_METHOD( 
			"PickPointAndObject", 
			PyPickPointAndObject, 
			"Given mouse position and a view setup, returns the object that the mouse is over, as well as the mesh and area indices"
			"\n returns (<Object>,(x,y,z)) or None if nothing pickable was hit by the ray"
			"\n"
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
			"\n:rtype: None | (blue.IRoot, (float, float, float))"
		)

    EXPOSURE_END()
}
