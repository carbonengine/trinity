////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#include "StdAfx.h"
#include "Tr2RuntimeInstanceData.h"
#include "Particle/Tr2ParticleElementDeclaration.h"
#include "Utilities/BoundingBox.h"

BLUE_DEFINE( Tr2RuntimeInstanceData );

#if BLUE_WITH_PYTHON
static PyObject* PySetElementLayout( PyObject* self, PyObject* args )
{
	Tr2RuntimeInstanceData* pThis = BluePythonCast<Tr2RuntimeInstanceData*>( self );

	PyObject* pyDecl;
	if ( !PyArg_ParseTuple( args, "O!", &PyList_Type, &pyDecl ) )
	{
		return nullptr;
	}

	Tr2VertexDefinition def;
	for( int i = 0; i < PyList_GET_SIZE( pyDecl ); ++i )
	{
		PyObject* el = PyList_GetItem( pyDecl, i );
		if( !PyTuple_Check( el ) || PyTuple_GET_SIZE( el ) != 3 )
		{
			PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of 3-element tuples of numbers" );
			return nullptr;
		}
		PyObject* pyUsage = PyTuple_GetItem( el, 0 );
		if( !PyNumber_Check( pyUsage ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of 3-element tuples of numbers" );
			return nullptr;
		}
		int usage = int( PyNumber_AsSsize_t( pyUsage, nullptr ) );
		if( usage < 0 || usage > Tr2ParticleElementDeclarationName::CUSTOM )
		{
			PyErr_SetString( PyExc_TypeError, "Invalid element usage" );
			return nullptr;
		}
		PyObject* pyUsageIndex = PyTuple_GetItem( el, 1 );
		if( !PyNumber_Check( pyUsageIndex ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of 3-element tuples of numbers" );
			return nullptr;
		}
		int usageIndex = int( PyNumber_AsSsize_t( pyUsageIndex, nullptr ) );
		if( usageIndex < 0 || usageIndex > 7 )
		{
			PyErr_SetString( PyExc_TypeError, "Invalid element usage index" );
			return nullptr;
		}
		PyObject* pyType = PyTuple_GetItem( el, 2 );
		if( !PyNumber_Check( pyType ) )
		{
			PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of 3-element tuples of numbers" );
			return nullptr;
		}
		int type = int( PyNumber_AsSsize_t( pyType, nullptr ) );
		if( type <= 0 || type > 4 )
		{
			PyErr_SetString( PyExc_TypeError, "Invalid element type" );
			return nullptr;
		}
		
		def.Add( 
			Tr2VertexDefinition::DataType( Tr2VertexDefinition::DT_FLOAT32 | ( ( type - 1 ) << Tr2VertexDefinition::DT_SIZE_OFFSET ) ),
			Tr2ParticleElementDeclarationName( Tr2ParticleElementDeclarationName::Type( usage ) ).GetD3DUsage(),
			usageIndex );
	}

	pThis->SetLayout( def );
	Py_RETURN_NONE;
}

static PyObject* PySetData( PyObject* self, PyObject* args )
{
	Tr2RuntimeInstanceData* pThis = BluePythonCast<Tr2RuntimeInstanceData*>( self );

	PyObject* pyData;
	if ( !PyArg_ParseTuple( args, "O!", &PyList_Type, &pyData ) )
	{
		return nullptr;
	}

	const Tr2VertexDefinition& def = pThis->GetLayout();
	if( def.m_items.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "Need to set valid element layout with SetElementLayout before calling SetData" );
		return nullptr;
	}

	float* data = reinterpret_cast<float*>( pThis->GetData( unsigned( PyList_GET_SIZE( pyData ) ) ) );
	if( !data )
	{
		PyErr_SetString( PyExc_RuntimeError, "Could not lock data" );
		return nullptr;
	}


	for( int i = 0; i < PyList_GET_SIZE( pyData ); ++i )
	{
		PyObject* item = PyList_GetItem( pyData, i );
		if( !PyTuple_Check( item ) || PyTuple_GET_SIZE( item ) != def.m_items.size() )
		{
			pThis->DestroyData();
			PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of tuples and tuples need to match element layout" );
			return nullptr;
		}
		for( size_t j = 0; j < def.m_items.size(); ++j )
		{
			PyObject* element = PyTuple_GET_ITEM( item, j );
			unsigned size = def.GetDataTypeSizeInMembers( def.m_items[j].m_dataType );
			if( size == 1 )
			{
				if( !PyFloat_Check( element ) )
				{
					pThis->DestroyData();
					PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of tuples and tuples need to match element layout" );
					return nullptr;
				}
				*data++ = float( PyFloat_AsDouble( element ) );
			}
			else
			{
				if( !PyTuple_Check( element ) )
				{
					pThis->DestroyData();
					PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of tuples and tuples need to match element layout" );
					return nullptr;
				}
				for( unsigned k = 0; k < size; ++k )
				{
					PyObject* component = PyTuple_GetItem( element, k );
					if( !PyFloat_Check( component ) )
					{
						pThis->DestroyData();
						PyErr_SetString( PyExc_TypeError, "Argument 1 needs to be a list of tuples and tuples need to match element layout" );
						return nullptr;
					}
					*data++ = float( PyFloat_AsDouble( component ) );
				}
			}
		}
	}

	Py_RETURN_NONE;
}

static PyObject* PyGetItem( PyObject* self, PyObject* args )
{
	Tr2RuntimeInstanceData* pThis = BluePythonCast<Tr2RuntimeInstanceData*>( self );

	unsigned index;
	if ( !PyArg_ParseTuple( args, "I", &index ) )
	{
		return nullptr;
	}

	if( pThis->GetCount() <= index )
	{
		PyErr_SetString( PyExc_IndexError, "Index out of range" );
		return nullptr;
	}

	const Tr2VertexDefinition& def = pThis->GetLayout();
	if( def.m_items.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "No layout set for runtime data" );
		return nullptr;
	}

	const float* data = reinterpret_cast<const float*>( reinterpret_cast<const char*>( pThis->GetData() ) + pThis->GetStride() * index );

	PyObject* pyData = PyTuple_New( def.m_items.size() );
	for( size_t j = 0; j < def.m_items.size(); ++j )
	{
		unsigned size = def.GetDataTypeSizeInMembers( def.m_items[j].m_dataType );
		if( size == 1 )
		{
			PyTuple_SET_ITEM( pyData, j, PyFloat_FromDouble( *data++ ) );
		}
		else
		{
			PyObject* element = PyTuple_New( size );
			for( unsigned k = 0; k < size; ++k )
			{
				PyTuple_SET_ITEM( element, k, PyFloat_FromDouble( *data++ ) );
			}
			PyTuple_SET_ITEM( pyData, j, PyFloat_FromDouble( *data++ ) );
		}
	}

	return pyData;
}

static PyObject* PySetItem( PyObject* self, PyObject* args )
{
	Tr2RuntimeInstanceData* pThis = BluePythonCast<Tr2RuntimeInstanceData*>( self );

	unsigned index;
	PyObject* item;
	if ( !PyArg_ParseTuple( args, "IO!", &index, &PyTuple_Type, &item ) )
	{
		return nullptr;
	}

	if( pThis->GetCount() <= index )
	{
		PyErr_SetString( PyExc_IndexError, "Index out of range" );
		return nullptr;
	}

	const Tr2VertexDefinition& def = pThis->GetLayout();
	if( def.m_items.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "Need to set valid element layout with SetElementLayout before calling SetData" );
		return nullptr;
	}

	if( PyTuple_GET_SIZE( item ) != def.m_items.size() )
	{
		pThis->DestroyData();
		PyErr_SetString( PyExc_TypeError, "Argument 2 needs to be a tuple and it needs to match element layout" );
		return nullptr;
	}

	uint8_t* dataRaw = static_cast<uint8_t*>( pThis->GetData( pThis->GetCount() ) );
	if( !dataRaw )
	{
		PyErr_SetString( PyExc_RuntimeError, "Could not lock data" );
		return nullptr;
	}
	float* data = static_cast<float*>( static_cast<void*>( dataRaw + pThis->GetStride() * index ) );


	for( size_t j = 0; j < def.m_items.size(); ++j )
	{
		PyObject* element = PyTuple_GET_ITEM( item, j );
		unsigned size = def.GetDataTypeSizeInMembers( def.m_items[j].m_dataType );
		if( size == 1 )
		{
			if( !PyFloat_Check( element ) )
			{
				pThis->DestroyData();
				PyErr_SetString( PyExc_TypeError, "Argument 2 needs to be a tuple and it needs to match element layout" );
				return nullptr;
			}
			*data++ = float( PyFloat_AsDouble( element ) );
		}
		else
		{
			if( !PyTuple_Check( element ) )
			{
				pThis->DestroyData();
				PyErr_SetString( PyExc_TypeError, "Argument 2 needs to be a tuple and it needs to match element layout" );
				return nullptr;
			}
			for( unsigned k = 0; k < size; ++k )
			{
				PyObject* component = PyTuple_GetItem( element, k );
				if( !PyFloat_Check( component ) )
				{
					pThis->DestroyData();
					PyErr_SetString( PyExc_TypeError, "Argument 2 needs to be a tuple and it needs to match element layout" );
					return nullptr;
				}
				*data++ = float( PyFloat_AsDouble( component ) );
			}
		}
	}

	Py_RETURN_NONE;
}

static PyObject* PyGetItemElement( PyObject* self, PyObject* args )
{
	Tr2RuntimeInstanceData* pThis = BluePythonCast<Tr2RuntimeInstanceData*>( self );

	unsigned index, elementIndex;
	if ( !PyArg_ParseTuple( args, "II", &index, &elementIndex ) )
	{
		return nullptr;
	}

	if( pThis->GetCount() <= index )
	{
		PyErr_SetString( PyExc_IndexError, "index out of range" );
		return nullptr;
	}

	const Tr2VertexDefinition& def = pThis->GetLayout();
	if( def.m_items.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "No layout set for runtime data" );
		return nullptr;
	}

	if( elementIndex >= def.m_items.size() )
	{
		PyErr_SetString( PyExc_IndexError, "elementIndex out of range" );
		return nullptr;
	}

	const float* data = reinterpret_cast<const float*>( reinterpret_cast<const char*>( pThis->GetData() ) + pThis->GetStride() * index + def.m_items[elementIndex].m_offset );

	unsigned size = def.GetDataTypeSizeInMembers( def.m_items[elementIndex].m_dataType );
	if( size == 1 )
	{
		return PyFloat_FromDouble( *data++ );
	}
	else
	{
		PyObject* element = PyTuple_New( size );
		for( unsigned k = 0; k < size; ++k )
		{
			PyTuple_SET_ITEM( element, k, PyFloat_FromDouble( *data++ ) );
		}
		return element;
	}
}

static PyObject* PySetItemElement( PyObject* self, PyObject* args )
{
	Tr2RuntimeInstanceData* pThis = BluePythonCast<Tr2RuntimeInstanceData*>( self );

	unsigned index;
	unsigned elementIndex;
	PyObject* element;
	if ( !PyArg_ParseTuple( args, "IIO", &index, &elementIndex, &element ) )
	{
		return nullptr;
	}

	if( pThis->GetCount() <= index )
	{
		PyErr_SetString( PyExc_IndexError, "Index out of range" );
		return nullptr;
	}

	const Tr2VertexDefinition& def = pThis->GetLayout();
	if( def.m_items.empty() )
	{
		PyErr_SetString( PyExc_RuntimeError, "Need to set valid element layout with SetElementLayout before calling SetData" );
		return nullptr;
	}

	if( elementIndex >= def.m_items.size() )
	{
		PyErr_SetString( PyExc_IndexError, "elementIndex out of range" );
		return nullptr;
	}

	uint8_t* dataRaw = reinterpret_cast<uint8_t*>( pThis->GetData( pThis->GetCount() ) );
	if( !dataRaw )
	{
		PyErr_SetString( PyExc_RuntimeError, "Could not lock data" );
		return nullptr;
	}
	float* data = static_cast<float*>( static_cast<void*>( dataRaw + pThis->GetStride() * index + def.m_items[elementIndex].m_offset ) );

	unsigned size = def.GetDataTypeSizeInMembers( def.m_items[elementIndex].m_dataType );
	if( size == 1 )
	{
		if( !PyFloat_Check( element ) )
		{
			pThis->DestroyData();
			PyErr_SetString( PyExc_TypeError, "Argument 3 needs to be a float" );
			return nullptr;
		}
		*data++ = float( PyFloat_AsDouble( element ) );
	}
	else
	{
		if( !PyTuple_Check( element ) )
		{
			pThis->DestroyData();
			PyErr_SetString( PyExc_TypeError, "Argument 3 needs to be a tuple and it needs to match element layout" );
			return nullptr;
		}
		for( unsigned k = 0; k < size; ++k )
		{
			PyObject* component = PyTuple_GetItem( element, k );
			if( !PyFloat_Check( component ) )
			{
				pThis->DestroyData();
				PyErr_SetString( PyExc_TypeError, "Argument 3 needs to be a tuple and it needs to match element layout" );
				return nullptr;
			}
			*data++ = float( PyFloat_AsDouble( component ) );
		}
	}

	Py_RETURN_NONE;
}
#endif

const Be::ClassInfo* Tr2RuntimeInstanceData::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2RuntimeInstanceData, "" )
		MAP_INTERFACE( Tr2RuntimeInstanceData )
		MAP_INTERFACE( ITr2InstanceData )
		MAP_INTERFACE( ITr2GenericEmitter )
		MAP_INTERFACE( ITr2GpuBuffer )

		MAP_ATTRIBUTE
		( 
			"name", 
			m_name, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_ATTRIBUTE
		( 
			"particleSystem", 
			m_particleSystem, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)

		MAP_PROPERTY_READONLY
		(
			"count",
			GetCount, 
			"Number of instances"
		)

		MAP_METHOD
		(
			"SetElementLayout",
			PySetElementLayout,
			"Assign instance element data layout. Invalidates data for the object.\n"
			":param layout: list of 3-tuples (usage - trinity.PARTICLE_ELEMENT_TYPE, usageIndex, elementCount)\n"
			"         describing each data element\n"
			":type layout: list[(int, int, int)]\n"
			":rtype: None"
		)

		MAP_METHOD
		(
			"SetData",
			PySetData,
			"Assign instance data.\n"
			":param data: list of tuples; each list element describes one instance; each tuple element describes\n"
			"       one element according to SetElementLayout and is either a float an n-tuple of floats\n"
			":type data: list\n"
			":rtype: None"
		)

		MAP_METHOD
		(
			"GetItem",
			PyGetItem,
			"Returns instance data for given instance index.\n"
			":param idx: instance index\n"
			":type idx: int\n"
			":rtype: tuple"
		)

		MAP_METHOD
		(
			"SetItem",
			PySetItem,
			"Set instance data for given instance index.\n"
			":param idx: instance index\n"
			":type idx: int\n"
			":param data: a tuple that describes one instance; each tuple element describes\n"
			"       one element according to SetElementLayout and is either a float an n-tuple of floats\n"
			":rtype: None"
		)

		MAP_METHOD
		(
			"GetItemElement",
			PyGetItemElement,
			"Returns one element of instance data for given instance index.\n"
			":param idx: instance index\n"
			":type idx: int\n"
			":param elementIndex: data element index\n"
			":type elementIndex: int\n"
			":rtype: float | tuple"
		)

		MAP_METHOD
		(
			"SetItemElement",
			PySetItemElement,
			"Set one element of instance data for given instance index.\n"
			":param idx: instance index\n"
			":type idx: int\n"
			":param elementIndex: data element index\n"
			":type elementIndex: int\n"
			":param data: a tuple or float that describes one element of instance data\n"
			":type data: float | tuple\n"
			":rtype: None"
		)

		MAP_METHOD_AND_WRAP
		(
			"UpdateData",
			UpdateData,
			"Uploads modified data to GPU"
		)

		MAP_METHOD_AND_WRAP
		(
			"UpdateBoundingBox",
			UpdateBoundingBox,
			"Updates bounding box after instance data was modified"
		)

		MAP_METHOD_AND_WRAP(
			"SetBoundingBox",
			SetBoundingBox,
			"Assign a bounding box explicitely (in case instance data contains no positions, for example)\n"
			":param box: axis aligned bounding box for instance data"
		)

		MAP_METHOD_AND_WRAP
		(
			"Spawn",
			Spawn,
			"Spawns particles into the particle system"
		)

		MAP_METHOD_AND_WRAP
		(
			"SaveToGranny",
			SaveToGranny,
			"Saves vertex data to a granny file\n"
			":param path: path to the granny file"
		)

		MAP_ATTRIBUTE( "aabbMin", m_aabb.m_min, "Minimum of the AABB", Be::READ )

		MAP_ATTRIBUTE( "aabbMax", m_aabb.m_max, "Maximum of the AABB", Be::READ )

	EXPOSURE_END()
}
