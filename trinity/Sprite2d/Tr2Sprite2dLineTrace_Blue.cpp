// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dLineTrace.h"

BLUE_DEFINE( Tr2Sprite2dLineTraceVertex );
BLUE_DEFINE( Tr2Sprite2dLineTrace );


#if BLUE_WITH_PYTHON

namespace
{

bool ToFloatTuple( PyObject* value, ssize_t length, float* destination )
{
	if( PyTuple_Check( value ) && PyTuple_GET_SIZE( value ) == length )
	{
		for( ssize_t i = 0; i < length; ++i )
		{
			auto item = PyTuple_GET_ITEM( value, i );
			if( PyFloat_Check( item ) )
			{
				destination[i] = float( PyFloat_AsDouble( item ) );
			}
#if PY_MAJOR_VERSION == 2
			else if( PyInt_Check( item ) )
			{
				destination[i] = float( PyInt_AsLong( item ) );
			}
#endif
			else if( PyLong_Check( item ) )
			{
				destination[i] = float( PyLong_AsDouble( item ) );
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

}


PyObject* Tr2Sprite2dLineTrace::PyAppendVertices( PyObject* self, PyObject* args )
{
	auto pThis = BluePythonCast<Tr2Sprite2dLineTrace*>( self );
	PyObject* pyPositions = nullptr;
	PyObject* pyTransform = nullptr;
	PyObject* pyColors = nullptr;
	PyObject* pyNames = nullptr;

	if( !PyArg_ParseTuple( args, "OOO|O", &pyPositions, &pyTransform, &pyColors, &pyNames ) )
	{
		return nullptr;
	}

	Vector2 position;
	Color color;
	std::string name;

	Matrix transform( XMMatrixIdentity() );
	if( Py_None == pyTransform )
	{
		transform = XMMatrixIdentity();
	}
	else if( !PyTuple_Check( pyTransform ) || PyTuple_GET_SIZE( pyTransform ) != 3 ||
			 !ToFloatTuple( PyTuple_GET_ITEM( pyTransform, 0 ), 3, &transform._11 ) ||
			 !ToFloatTuple( PyTuple_GET_ITEM( pyTransform, 1 ), 3, &transform._21 ) ||
			 !ToFloatTuple( PyTuple_GET_ITEM( pyTransform, 2 ), 3, &transform._41 ) )
	{
		PyErr_SetString( PyExc_TypeError, "positionTransform parameter must be a 3x3 matrix or None" );
		return nullptr;
	}

	bool constPosition = ToFloatTuple( pyPositions, 2, &position.x );
	if( !constPosition && !PySequence_Check( pyPositions ) )
	{
		PyErr_SetString( PyExc_TypeError, "positions parameter must be a 2-tuple or a sequence of 2-tuples" );
		return nullptr;
	}
	bool constColor = ToFloatTuple( pyColors, 4, &color.r );
	if( !constColor && !PySequence_Check( pyColors ) )
	{
		PyErr_SetString( PyExc_TypeError, "colors parameter must be a 4-tuple or a sequence of 4-tuples" );
		return nullptr;
	}
	if( pyNames )
	{
		if( !PySequence_Check( pyNames ) )
		{
			PyErr_SetString( PyExc_TypeError, "names parameter must be a sequence of strings" );
			return nullptr;
		}
	}

	for( ssize_t index = 0;; ++index )
	{
		if( !constPosition )
		{
			auto item = PySequence_GetItem( pyPositions, index );
			if( !item )
			{
				PyErr_Clear();
				break;
			}
			bool success = ToFloatTuple( item, 2, &position.x );
			Py_DECREF( item );
			if( !success )
			{
				PyErr_SetString( PyExc_TypeError, "positions parameter must be a 2-tuple or a sequence of 2-tuples" );
				return nullptr;
			}
		}
		if( !constColor )
		{
			auto item = PySequence_GetItem( pyColors, index );
			if( !item )
			{
				PyErr_Clear();
				break;
			}
			bool success = ToFloatTuple( item, 4, &color.r );
			Py_DECREF( item );
			if( !success )
			{
				PyErr_SetString( PyExc_TypeError, "colors parameter must be a 4-tuple or a sequence of 4-tuples" );
				return nullptr;
			}
		}
		if( pyNames )
		{
			auto item = PySequence_GetItem( pyNames, index );
			if( !item )
			{
				PyErr_Clear();
				break;
			}
			if( !PyVerCompat::IsPyString( item ) )
			{
				PyErr_SetString( PyExc_TypeError, "names parameter must be a sequence of strings" );
				return nullptr;
			}
			name = FromPython<std::string>( item );
		}

		Tr2Sprite2dLineTraceVertexPtr vertex;
		vertex.CreateInstance();
		vertex->m_position = XMVector2TransformCoord( position, transform );
		vertex->m_color = color;
		vertex->m_name = name;

		pThis->m_vertices.Append( vertex );

		if( constPosition && constColor )
		{
			break;
		}
	}
	pThis->SetDirty();
	Py_RETURN_NONE;
}

PyObject* Tr2Sprite2dLineTrace::PySetVertices( PyObject* self, PyObject* args )
{
	auto pThis = BluePythonCast<Tr2Sprite2dLineTrace*>( self );
	PyObject* pyPositions = nullptr;
	PyObject* pyTransform = nullptr;
	PyObject* pyColors = nullptr;
	PyObject* pyNames = nullptr;

	if( !PyArg_ParseTuple( args, "O|OOO", &pyPositions, &pyTransform, &pyColors, &pyNames ) )
	{
		return nullptr;
	}

	if( Py_None == pyPositions )
	{
		pyPositions = nullptr;
	}
	if( Py_None == pyColors )
	{
		pyColors = nullptr;
	}
	if( Py_None == pyNames )
	{
		pyNames = nullptr;
	}

	Vector2 position;
	Color color;
	std::string name;

	Matrix transform( XMMatrixIdentity() );
	if( Py_None == pyTransform || pyTransform == nullptr )
	{
		transform = XMMatrixIdentity();
	}
	else if( !PyTuple_Check( pyTransform ) || PyTuple_GET_SIZE( pyTransform ) != 3 ||
			 !ToFloatTuple( PyTuple_GET_ITEM( pyTransform, 0 ), 3, &transform._11 ) ||
			 !ToFloatTuple( PyTuple_GET_ITEM( pyTransform, 1 ), 3, &transform._21 ) ||
			 !ToFloatTuple( PyTuple_GET_ITEM( pyTransform, 2 ), 3, &transform._41 ) )
	{
		PyErr_SetString( PyExc_TypeError, "positionTransform parameter must be a 3x3 matrix or None" );
		return nullptr;
	}

	bool constPosition = true;
	if( pyPositions )
	{
		constPosition = ToFloatTuple( pyPositions, 2, &position.x );
		if( !constPosition && !PySequence_Check( pyPositions ) )
		{
			PyErr_SetString( PyExc_TypeError, "positions parameter must be a 2-tuple or a sequence of 2-tuples" );
			return nullptr;
		}
	}
	bool constColor = true;
	if( pyColors )
	{
		constColor = ToFloatTuple( pyColors, 4, &color.r );
		if( !constColor && !PySequence_Check( pyColors ) )
		{
			PyErr_SetString( PyExc_TypeError, "colors parameter must be a 4-tuple or a sequence of 4-tuples" );
			return nullptr;
		}
	}
	bool constName = true;
	if( pyNames )
	{
		constName = PyVerCompat::IsPyString( pyNames );
		if( !constName && !PySequence_Check( pyNames ) )
		{
			PyErr_SetString( PyExc_TypeError, "names parameter must be a string or a sequence of strings" );
			return nullptr;
		}
	}

	for( ssize_t index = 0; index < ssize_t( pThis->m_vertices.size() ); ++index )
	{
		auto vertex = pThis->m_vertices[index];
		if( pyPositions )
		{
			if( !constPosition )
			{
				auto item = PySequence_GetItem( pyPositions, index );
				if( !item )
				{
					PyErr_Clear();
					break;
				}
				bool success = ToFloatTuple( item, 2, &position.x );
				Py_DECREF( item );
				if( !success )
				{
					PyErr_SetString( PyExc_TypeError, "positions parameter must be a 2-tuple or a sequence of 2-tuples" );
					return nullptr;
				}
			}
			vertex->m_position = XMVector2TransformCoord( position, transform );
		}
		if( pyColors )
		{
			if( !constColor )
			{
				auto item = PySequence_GetItem( pyColors, index );
				if( !item )
				{
					PyErr_Clear();
					break;
				}
				bool success = ToFloatTuple( item, 4, &color.r );
				Py_DECREF( item );
				if( !success )
				{
					PyErr_SetString( PyExc_TypeError, "colors parameter must be a 4-tuple or a sequence of 4-tuples" );
					return nullptr;
				}
			}
			vertex->m_color = color;
		}
		if( pyNames )
		{
			if( !constName )
			{
				auto item = PySequence_GetItem( pyNames, index );
				if( !item )
				{
					PyErr_Clear();
					break;
				}
				if( !PyVerCompat::IsPyString( item ) )
				{
					Py_DECREF( item );
					PyErr_SetString( PyExc_TypeError, "texCoords0 parameter must be a 2-tuple or a sequence of 2-tuples" );
					return nullptr;
				}
				name = FromPython<std::string>( item );
				Py_DECREF( item );
			}
			vertex->m_name = name;
		}
	}
	pThis->SetDirty();
	Py_RETURN_NONE;
}

#endif

const Be::ClassInfo* Tr2Sprite2dLineTrace::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dLineTrace, "" )
		MAP_INTERFACE( Tr2Sprite2dLineTrace )

		MAP_ATTRIBUTE(
			"vertices",
			m_vertices,
			"Vertices used to render line trace",
			Be::READ | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"lineWidth",
			m_lineWidth,
			"Width of line rendered",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textureWidth",
			m_textureWidth,
			"Width of the texture along the line",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"textureOffset",
			m_textureOffset,
			"Offset of the texture along the line. Can be used for animations",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"start",
			m_start,
			"Relative start of line trace - from 0 to 1",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"end",
			m_end,
			"Relative end of line trace - from 0 to 1",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"isLoop",
			m_isLoop,
			"If set, the line trace is treated as a closed loop",
			Be::READWRITE | Be::NOTIFY )

		MAP_ATTRIBUTE(
			"cornerType",
			m_cornerType,
			"Determines the shapes of corners connecting the line segments",
			Be::READWRITE | Be::NOTIFY )

#if BLUE_WITH_PYTHON
		MAP_METHOD(
			"AppendVertices",
			PyAppendVertices,
			"Adds vertices to the line.\n"
			":param positions: either a sequence of 2-tuples with vertex positions or a single position\n"
			":type positions: sequence[(float, float)]|(float, float)\n"
			":param positionTransform: optional 3x3 matrix to pre-transform positions or None for identity matrix\n"
			":type positionTransform: ((float, float, float), (float, float, float), (float, float, float))|None\n"
			":param colors: either a sequence of 4-tuples with vertex colors or a single 4-tuple color\n"
			":type colors: sequence[(float, float, float, float)]|(float, float, float, float)\n"
			":param names: optional sequence of string names\n"
			":type names: sequence[str]\n"
			":rtype: None" )
		MAP_METHOD(
			"SetVertices",
			PySetVertices,
			"Changes line vertices.\n"
			":param positions: either None for no change, a sequence of 2-tuples with vertex positions or a single position\n"
			":type positions: None|sequence[(float, float)]|(float, float)\n"
			":param positionTransform: optional 3x3 matrix to pre-transform positions or None for identity matrix\n"
			":type positionTransform: ((float, float, float), (float, float, float), (float, float, float))|None\n"
			":param colors: either None for no change, a sequence of 4-tuples with vertex colors or a single 4-tuple color\n"
			":type colors: None|sequence[(float, float, float, float)]|(float, float, float, float)\n"
			":param names: optional sequence of string names\n"
			":type names: sequence[str]\n"
			":rtype: None" )
#endif
	EXPOSURE_CHAINTO( Tr2TexturedSpriteObject )
}


const Be::ClassInfo* Tr2Sprite2dLineTraceVertex::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dLineTraceVertex, "" )
		MAP_INTERFACE( Tr2Sprite2dLineTraceVertex )

		MAP_ATTRIBUTE(
			"name",
			m_name,
			"Name of this vertex",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"position",
			m_position,
			"Position of the vertex",
			Be::READWRITE | Be::PERSIST )

		MAP_ATTRIBUTE(
			"color",
			m_color,
			"Color of the vertex",
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
