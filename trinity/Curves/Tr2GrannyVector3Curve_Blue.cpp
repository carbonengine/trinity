#include "StdAfx.h"
#include "Tr2GrannyVector3Curve.h"

BLUE_DEFINE( Tr2GrannyVector3Curve );

#if BLUE_WITH_PYTHON

static PyObject* PyCreateFromPoints( PyObject* self, PyObject* args )
{
	Tr2GrannyVector3Curve* pThis = BluePythonCast<Tr2GrannyVector3Curve*>( self );

	PyObject *pylist = NULL;
	float timeDelta = 0.0f;
	if (!PyArg_ParseTuple(args, "fO", &timeDelta, &pylist ))
	{
		return NULL;
	}

	PyObject* tuple = PySequence_Fast( pylist, "expected a sequence" );
	if( !tuple )
	{
		return NULL;
	}

	PyObject** items = PySequence_Fast_ITEMS( tuple );
	int length = (int)PySequence_Size(tuple);
	std::vector<granny_real32> knots( length * 3 );

	for ( int i = 0; i < length; i++ )
	{
		Vector3 point;
		if( !BlueExtractArgument( items[i], point, i ) )
		{
			Py_DECREF( tuple );
			return NULL;
		}

		knots[i*3] = point.x;
		knots[(i*3)+1] = point.y;
		knots[(i*3)+2] = point.z;
	}
	Py_DECREF( tuple );

	pThis->CreateFromPoints(timeDelta, knots);
	Py_INCREF(Py_None);
	return Py_None;
}

#endif

const Be::ClassInfo* Tr2GrannyVector3Curve::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2GrannyVector3Curve, ":jessica-deprecated: True\n:jessica-icon: tree/trivectorcurve.png" )
        MAP_INTERFACE( Tr2GrannyVector3Curve )
		MAP_INTERFACE( ITriFunction )
		MAP_INTERFACE( ITriCurveLength )
		MAP_INTERFACE( ICustomPersist )
		MAP_METHOD(
			"CreateFromPoints", 
			PyCreateFromPoints, 
			"Creates the curve from specified control points\n"
			":param timeDelta: time interval between sample points\n"
			":type timeDelta: float\n"
			":param points: sequence of control points\n"
			":type points: list[(float, float, float)]\n"
			":rtype: None" )
		MAP_ATTRIBUTE("currentValue", m_value, "current sampled value", Be::READ )
		MAP_ATTRIBUTE("timeOffset", m_timeOffset, "internal time offset", Be::READWRITE )
		MAP_ATTRIBUTE("cycle", m_cycle, "should the animation be cycled", Be::READWRITE )
		MAP_ATTRIBUTE("duration", m_duration, "The duration of the animation", Be::PERSISTONLY )
		MAP_ATTRIBUTE_AS_CUSTOM_BINARY_BLOCK( "curve" )
    EXPOSURE_END()
}
