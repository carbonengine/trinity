// Copyright © 2010 CCP ehf.

#include "StdAfx.h"
#include "Tr2Sprite2dPolygon.h"
#include "Tr2Sprite2d.h"

BLUE_DEFINE( Tr2Sprite2dPolygon );
BLUE_DEFINE( Tr2Sprite2dVertex );
BLUE_DEFINE( Tr2Sprite2dTriangle );

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
			else if( PyLong_Check( item ) )
			{
				destination[i] = float( PyLong_AsDouble( item ) );
			}
#if PY_MAJOR_VERSION == 2
			else if( PyInt_Check( item ) )
			{
				destination[i] = float( PyInt_AsLong( item ) );
			}
#endif
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


PyObject* Tr2Sprite2dPolygon::PyAppendVertices( PyObject* self, PyObject* args )
{
	auto pThis = BluePythonCast<Tr2Sprite2dPolygon*>( self );
	PyObject* pyPositions = nullptr;
	PyObject* pyTransform = nullptr;
	PyObject* pyColors = nullptr;
	PyObject* pyTexCoord0 = nullptr;
	PyObject* pyTexCoord1 = nullptr;

	if( !PyArg_ParseTuple( args, "OOO|OO", &pyPositions, &pyTransform, &pyColors, &pyTexCoord0, &pyTexCoord1 ) )
	{
		return nullptr;
	}

	Vector2 position;
	Color color;
	Vector2 texcoord0;
	Vector2 texcoord1;

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
	bool constTexcoord0 = true;
	if( pyTexCoord0 )
	{
		constTexcoord0 = ToFloatTuple( pyTexCoord0, 2, &texcoord0.x );
		if( !constTexcoord0 && !PySequence_Check( pyTexCoord0 ) )
		{
			PyErr_SetString( PyExc_TypeError, "texCoords0 parameter must be a 2-tuple or a sequence of 2-tuples" );
			return nullptr;
		}
	}
	else
	{
		texcoord0[0] = 0.f;
		texcoord0[1] = 0.f;
	}
	bool constTexcoord1 = true;
	if( pyTexCoord1 )
	{
		constTexcoord1 = ToFloatTuple( pyTexCoord1, 2, &texcoord1.x );
		if( !constTexcoord1 && !PySequence_Check( pyTexCoord1 ) )
		{
			PyErr_SetString( PyExc_TypeError, "texCoords1 parameter must be a 2-tuple or a sequence of 2-tuples" );
			return nullptr;
		}
	}
	else
	{
		texcoord1[0] = 0.f;
		texcoord1[1] = 0.f;
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
		if( !constTexcoord0 )
		{
			auto item = PySequence_GetItem( pyTexCoord0, index );
			if( !item )
			{
				PyErr_Clear();
				break;
			}
			bool success = ToFloatTuple( item, 2, &texcoord0.x );
			Py_DECREF( item );
			if( !success )
			{
				PyErr_SetString( PyExc_TypeError, "texCoords0 parameter must be a 2-tuple or a sequence of 2-tuples" );
				return nullptr;
			}
		}
		if( !constTexcoord1 )
		{
			auto item = PySequence_GetItem( pyTexCoord1, index );
			if( !item )
			{
				PyErr_Clear();
				break;
			}
			bool success = ToFloatTuple( item, 2, &texcoord1.x );
			Py_DECREF( item );
			if( !success )
			{
				PyErr_SetString( PyExc_TypeError, "texCoords1 parameter must be a 2-tuple or a sequence of 2-tuples" );
				return nullptr;
			}
		}

		Tr2Sprite2dVertexPtr vertex;
		vertex.CreateInstance();
		vertex->position = XMVector2TransformCoord( position, transform );
		vertex->color = color;
		vertex->texCoord[0] = texcoord0;
		vertex->texCoord[1] = texcoord1;

		pThis->m_vertices.Append( vertex );

		if( constPosition && constColor && constTexcoord0 && constTexcoord1 )
		{
			break;
		}
	}
	pThis->SetDirty();
	Py_RETURN_NONE;
}


PyObject* Tr2Sprite2dPolygon::PySetVertices( PyObject* self, PyObject* args )
{
	auto pThis = BluePythonCast<Tr2Sprite2dPolygon*>( self );
	PyObject* pyPositions = nullptr;
	PyObject* pyTransform = nullptr;
	PyObject* pyColors = nullptr;
	PyObject* pyTexCoord0 = nullptr;
	PyObject* pyTexCoord1 = nullptr;

	if( !PyArg_ParseTuple( args, "O|OOOO", &pyPositions, &pyTransform, &pyColors, &pyTexCoord0, &pyTexCoord1 ) )
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
	if( Py_None == pyTexCoord0 )
	{
		pyTexCoord0 = nullptr;
	}
	if( Py_None == pyTexCoord1 )
	{
		pyTexCoord1 = nullptr;
	}

	Vector2 position;
	Color color;
	Vector2 texcoord0;
	Vector2 texcoord1;

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
	bool constTexcoord0 = true;
	if( pyTexCoord0 )
	{
		constTexcoord0 = ToFloatTuple( pyTexCoord0, 2, &texcoord0.x );
		if( !constTexcoord0 && !PySequence_Check( pyTexCoord0 ) )
		{
			PyErr_SetString( PyExc_TypeError, "texCoords0 parameter must be a 2-tuple or a sequence of 2-tuples" );
			return nullptr;
		}
	}
	else
	{
		texcoord0[0] = 0.f;
		texcoord0[1] = 0.f;
	}
	bool constTexcoord1 = true;
	if( pyTexCoord1 )
	{
		constTexcoord1 = ToFloatTuple( pyTexCoord1, 2, &texcoord1.x );
		if( !constTexcoord1 && !PySequence_Check( pyTexCoord1 ) )
		{
			PyErr_SetString( PyExc_TypeError, "texCoords1 parameter must be a 2-tuple or a sequence of 2-tuples" );
			return nullptr;
		}
	}
	else
	{
		texcoord1[0] = 0.f;
		texcoord1[1] = 0.f;
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
			vertex->position = XMVector2TransformCoord( position, transform );
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
			vertex->color = color;
		}
		if( pyTexCoord0 )
		{
			if( !constTexcoord0 )
			{
				auto item = PySequence_GetItem( pyTexCoord0, index );
				if( !item )
				{
					PyErr_Clear();
					break;
				}
				bool success = ToFloatTuple( item, 2, &texcoord0.x );
				Py_DECREF( item );
				if( !success )
				{
					PyErr_SetString( PyExc_TypeError, "texCoords0 parameter must be a 2-tuple or a sequence of 2-tuples" );
					return nullptr;
				}
			}
			vertex->texCoord[0] = texcoord0;
		}
		if( pyTexCoord1 )
		{
			if( !constTexcoord1 )
			{
				auto item = PySequence_GetItem( pyTexCoord1, index );
				if( !item )
				{
					PyErr_Clear();
					break;
				}
				bool success = ToFloatTuple( item, 2, &texcoord1.x );
				Py_DECREF( item );
				if( !success )
				{
					PyErr_SetString( PyExc_TypeError, "texCoords1 parameter must be a 2-tuple or a sequence of 2-tuples" );
					return nullptr;
				}
			}
			vertex->texCoord[1] = texcoord1;
		}
	}
	pThis->SetDirty();
	Py_RETURN_NONE;
}

PyObject* Tr2Sprite2dPolygon::PyAppendTriangles( PyObject* self, PyObject* args )
{
	auto pThis = BluePythonCast<Tr2Sprite2dPolygon*>( self );
	PyObject* pyTriangles = nullptr;

	if( !PyArg_ParseTuple( args, "O", &pyTriangles ) )
	{
		return nullptr;
	}

	if( !PySequence_Check( pyTriangles ) )
	{
		PyErr_SetString( PyExc_TypeError, "triangles parameter must be a sequence of 3-tuples" );
		return nullptr;
	}

	for( ssize_t index = 0;; ++index )
	{
		auto item = PySequence_GetItem( pyTriangles, index );
		if( !item )
		{
			PyErr_Clear();
			break;
		}
		if( !PyTuple_Check( item ) || PyTuple_GET_SIZE( item ) != 3 ||
			!PyVerCompat::IsPyInt( PyTuple_GET_ITEM( item, 0 ) ) || !PyVerCompat::IsPyInt( PyTuple_GET_ITEM( item, 1 ) ) || !PyVerCompat::IsPyInt( PyTuple_GET_ITEM( item, 2 ) ) )
		{
			Py_DECREF( item );
			PyErr_SetString( PyExc_TypeError, "triangles parameter must be a sequence of 3-tuples" );
			return nullptr;
		}
		Tr2Sprite2dTrianglePtr triangle;
		triangle.CreateInstance();

		triangle->m_index[0] = uint16_t( FromPython<int32_t>( PyTuple_GET_ITEM( item, 0 ) ) );
		triangle->m_index[1] = uint16_t( FromPython<int32_t>( PyTuple_GET_ITEM( item, 1 ) ) );
		triangle->m_index[2] = uint16_t( FromPython<int32_t>( PyTuple_GET_ITEM( item, 2 ) ) );

		pThis->m_triangles.Append( triangle );

		Py_DECREF( item );
	}
	pThis->SetDirty();
	Py_RETURN_NONE;
}

#endif

const Be::ClassInfo* Tr2Sprite2dPolygon::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dPolygon, "" )
		MAP_INTERFACE( Tr2Sprite2dPolygon )

		MAP_ATTRIBUTE(
			"vertices",
			m_vertices,
			"Vertices used to render polygon",
			Be::READ )

		MAP_ATTRIBUTE(
			"triangles",
			m_triangles,
			"Triangles used to render polygon",
			Be::READ )

#if BLUE_WITH_PYTHON
		MAP_METHOD(
			"AppendVertices",
			PyAppendVertices,
			"Adds vertices to the polygon.\n"
			":param positions: either a sequence of 2-tuples with vertex positions or a single position\n"
			":type positions: sequence[(float, float)]|(float, float)\n"
			":param positionTransform: optional 3x3 matrix to pre-transform positions or None for identity matrix\n"
			":type positionTransform: ((float, float, float), (float, float, float), (float, float, float))|None\n"
			":param colors: either a sequence of 4-tuples with vertex colors or a single 4-tuple color\n"
			":type colors: sequence[(float, float, float, float)]|(float, float, float, float)\n"
			":param texCoords0: optional sequence of 2-tuples with the first texture coordinates or a single 2-tuple\n"
			":type texCoords0: sequence[(float, float)]|(float, float)\n"
			":param texCoords1: optional sequence of 2-tuples with the second texture coordinates or a single 2-tuple\n"
			":type texCoords1: sequence[(float, float)]|(float, float)\n"
			":rtype: None" )
		MAP_METHOD(
			"SetVertices",
			PySetVertices,
			"Changes poly vertices.\n"
			":param positions: either None for no change, a sequence of 2-tuples with vertex positions or a single position\n"
			":type positions: None|sequence[(float, float)]|(float, float)\n"
			":param positionTransform: optional 3x3 matrix to pre-transform positions or None for identity matrix\n"
			":type positionTransform: ((float, float, float), (float, float, float), (float, float, float))|None\n"
			":param colors: either None for no change, a sequence of 4-tuples with vertex colors or a single 4-tuple color\n"
			":type colors: None|sequence[(float, float, float, float)]|(float, float, float, float)\n"
			":param texCoords0: optional sequence of 2-tuples with the first texture coordinates or a single 2-tuple\n"
			":type texCoords0: None|sequence[(float, float)]|(float, float)\n"
			":param texCoords1: optional sequence of 2-tuples with the second texture coordinates or a single 2-tuple\n"
			":type texCoords1: None|sequence[(float, float)]|(float, float)\n"
			":rtype: None" )
		MAP_METHOD(
			"AppendTriangles",
			PyAppendTriangles,
			"Adds triangle indices to the polygon.\n"
			":param triangles: a sequence of 3-tuples with triangle indices\n"
			":type positions: sequence[(int, int, int)]\n"
			":rtype: None" )
#endif

	EXPOSURE_CHAINTO( Tr2TexturedSpriteObject )
}


const Be::ClassInfo* Tr2Sprite2dVertex::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dVertex, "" )
		MAP_INTERFACE( Tr2Sprite2dVertex )

		MAP_ATTRIBUTE(
			"position",
			position,
			"Position of the vertex",
			Be::READWRITE | Be::PERSIST )

		MAP_PROPERTY( "color", GetColor, SetColor, "vertex colour" )

		MAP_METHOD_AND_WRAP(
			"GetTexCoord",
			GetTexCoord,
			"returns indexed texture coordinate\n"
			":param idx: texture coordinates index" )
		MAP_METHOD_AND_WRAP(
			"SetTexCoord",
			SetTexCoord,
			"sets indexed texture coordinate\n"
			":param idx: texture coordinates index"
			":param uv: texture coordinates" )

	EXPOSURE_END()
}

const Be::ClassInfo* Tr2Sprite2dTriangle::ExposeToBlue()
{
	EXPOSURE_BEGIN( Tr2Sprite2dTriangle, "" )
		MAP_INTERFACE( Tr2Sprite2dTriangle )

		MAP_ATTRIBUTE(
			"index0",
			m_index[0],
			"Index of first vertex in this triangle",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"index1",
			m_index[1],
			"Index of second vertex in this triangle",
			Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE(
			"index2",
			m_index[2],
			"Index of third vertex in this triangle",
			Be::READWRITE | Be::PERSIST )
	EXPOSURE_END()
}
