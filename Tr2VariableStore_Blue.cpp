////////////////////////////////////////////////////////////
//
//    Created:   December 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"
#include "Tr2VariableStore.h"
#include "Resources/TriTextureRes.h"
#include "Tr2RenderTarget.h"
#include "Tr2DepthStencil.h"
#include "Include/ITr2GpuBuffer.h"

BLUE_DEFINE( Tr2VariableStore );

#if BLUE_WITH_PYTHON
static PyObject* PyWrapVariable( TriVariable* variable )
{
	if( variable == NULL )
	{
		PyErr_SetString(PyExc_KeyError, "Variable store variable is not set.");
		return NULL;
	}
	switch( variable->GetType() )
	{
	case TRIVARIABLE_INVALID:
	case TRIVARIABLE_UNKNOWN_TEXTURE:
	case TRIVARIABLE_UNKNOWN_FLOAT:
		{
			Py_RETURN_NONE;
		}
	case TRIVARIABLE_TEXTURE_RES:
		{
			ITr2TextureProvider* res = NULL;
			variable->GetValue( res );

			return PyOS->WrapBlueObject( res );
		}
	case TRIVARIABLE_GPUBUFFER:
		{
			ITr2GpuBuffer* res = NULL;
			variable->GetValue( res );

			return PyOS->WrapBlueObject( res );
		}
	case TRIVARIABLE_TEXTURE_AL:
		{
			Tr2TextureAL* value;
			variable->GetValue( value );
			if( value )
			{
				TriTextureResPtr resource;
				resource.CreateInstance();
				resource->SetTexture( *value );

				return PyOS->WrapBlueObject( resource->GetRawRoot() );
			}
			else
			{
				Py_RETURN_NONE;
			}
		}
	case TRIVARIABLE_INT:
		{
			int value;
			variable->GetValue( value );
			return Py_BuildValue( "i", value );
		}
	case TRIVARIABLE_FLOAT:
		{
			float value;
			variable->GetValue( value );
			return Py_BuildValue( "f", value );
		}
	case TRIVARIABLE_FLOAT2:
		{
			Vector2 value;
			variable->GetValue( value );
			return Py_BuildValue( "ff", value.x, value.y );
		}
	case TRIVARIABLE_FLOAT3:
		{
			Vector3 value;
			variable->GetValue( value );
			return Py_BuildValue( "(fff)", value.x, value.y, value.z );
		}
	case TRIVARIABLE_FLOAT4:
		{
			Vector4 value;
			variable->GetValue( value );
			return Py_BuildValue( "(ffff)", value.x, value.y, value.z, value.w );
		}
	case TRIVARIABLE_FLOAT4X4:
		{
			Matrix value;
			variable->GetValue( value );

			PyObject* ret = PyTuple_New( 4 );

			// Start of floating point of array so we can iterate simply over the data
			const float* array = &value._11;
			for( int i = 0; i < 4; ++i )
			{
				PyObject* tuple = PyTuple_New( 4 );
				// Marshall row 'i' in the array as a 4-tuple
				for( int k = 0; k < 4; ++k )
				{
					PyTuple_SET_ITEM( tuple, k, PyFloat_FromDouble( array[4*i + k ] ) );
				}
				// Collect the tuple into a tuple of rows
				PyTuple_SET_ITEM( ret, i, tuple );
			}

			return ret;
		}
	case TRIVARIABLE_COLOR:
		{
			Color value;
			variable->GetValue( value );
			return Py_BuildValue( "(ffff)", value.r, value.g, value.b, value.a );
		}
	case TRIVARIABLE_IROOT:
		{
			IRoot *value;
			variable->GetValue( value );
			return PyOS->WrapBlueObject( value );
		}
	default:
		return NULL;
	}
}

static PyObject* PyRegisterVariable( PyObject* self, PyObject* args )
{
	Tr2VariableStore* pThis = BluePythonCast<Tr2VariableStore*>( self );

	const char* name;
	PyObject* valueArg = NULL;
	if( !PyArg_ParseTuple( args, "sO", &name, &valueArg ) )
	{
		return NULL;
	}

	TriVariable* variable = NULL;

	float valFloat;
	Vector2 valVector2;
	Vector3 valVector3;
	Vector4 valVector4;
	Matrix valMatrix;
	Color valColor;

	if( TriTextureRes* value = BluePythonCast<TriTextureRes*>( valueArg ) )
	{
		variable = pThis->RegisterVariable( name, value );
	}
	else if( ITriTextureRes* value = BluePythonCast<ITriTextureRes*>( valueArg ) )
	{
		variable = pThis->RegisterVariable( name, value );
	}
	else if( Tr2RenderTarget* value = BluePythonCast<Tr2RenderTarget*>( valueArg ) )
	{
		variable = pThis->RegisterVariable( name, value );
	}
	else if( Tr2DepthStencil* value = BluePythonCast<Tr2DepthStencil*>( valueArg ) )
	{
		variable = pThis->RegisterVariable( name, value );
	}
	else if( PyInt_Check( valueArg ) )
	{
		variable = pThis->RegisterVariable( name, (int)PyInt_AsLong( valueArg ) );
	}
	else if( BlueExtractFloat( valueArg, valFloat ) )
	{
		variable = pThis->RegisterVariable( name, valFloat );
	}
	else if( BlueExtractVector( valueArg, valVector2, 2 ) )
	{
		variable = pThis->RegisterVariable( name, valVector2 );
	}
	else if( BlueExtractVector( valueArg, valVector3, 3 ) )
	{
		variable = pThis->RegisterVariable( name, valVector3 );
	}
	else if( BlueExtractVector( valueArg, &valVector4.x, 4 ) )
	{
		variable = pThis->RegisterVariable( name, valVector4 );
	}
	else if( BlueExtractMatrix( valueArg, &valMatrix.m[0][0], 16 ) )
	{
		variable = pThis->RegisterVariable( name, valMatrix );
	}
	else if( BlueExtractVector( valueArg, valColor, 4 ) )
	{
		variable = pThis->RegisterVariable( name, valColor );
	}
	else if( valueArg == Py_None )
	{
		variable = pThis->RegisterVariable( name );
	}
	else if( IRoot* valIroot = BluePythonCast<IRoot*>( self ) )
	{
		variable = pThis->RegisterVariable( name, valIroot );
	}

	return PyWrapVariable( variable );
}

static PyObject* PyFindVariable( PyObject* self, PyObject* args )
{
	Tr2VariableStore* pThis = BluePythonCast<Tr2VariableStore*>( self );
	const char* name;
	if( !PyArg_ParseTuple( args, "s", &name ) )
	{
		return NULL;
	}

	TriVariable* variable = pThis->FindVariable( name );
	return PyWrapVariable( variable );
}

static PyObject* PyFindLocalVariable( PyObject* self, PyObject* args )
{
	Tr2VariableStore* pThis = BluePythonCast<Tr2VariableStore*>( self );
	const char* name;
	if( !PyArg_ParseTuple( args, "s", &name ) )
	{
		return NULL;
	}

	TriVariable* variable = pThis->FindLocalVariable( name );
	return PyWrapVariable( variable );
}

static PyObject* PyGetVariable( PyObject* self, PyObject* args )
{
	Tr2VariableStore* pThis = BluePythonCast<Tr2VariableStore*>( self );
	const char* name;
	if( !PyArg_ParseTuple( args, "s", &name ) )
	{
		return NULL;
	}

	TriVariable* variable = pThis->GetVariable( name );
	return PyWrapVariable( variable );
}

static PyObject* PyGetLocalVariable( PyObject* self, PyObject* args )
{
	Tr2VariableStore* pThis = BluePythonCast<Tr2VariableStore*>( self );
	const char* name;
	if( !PyArg_ParseTuple( args, "s", &name ) )
	{
		return NULL;
	}

	TriVariable* variable = pThis->GetLocalVariable( name );
	return PyWrapVariable( variable );
}

static PyObject* PyGetVariableStore( PyObject* self, PyObject* args )
{
	PyObject* ret = PyOS->WrapBlueObject( &GlobalStore() );
	return ret; 
}

MAP_FUNCTION( "GetVariableStore", PyGetVariableStore, "Gets the global variable store" );
#endif

const Be::ClassInfo* Tr2VariableStore::ExposeToBlue()
{
    EXPOSURE_BEGIN( Tr2VariableStore, "Variable map used by shaders to get parameters" )
		MAP_INTERFACE( Tr2VariableStore )

		MAP_PROPERTY( "parentStore", GetParentVariableStore, SetParentVariableStore, "Parent variable store" )
		MAP_METHOD( 
			"RegisterVariable", 
			PyRegisterVariable, 
			"Registers a new variable in this store." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable"
			"\nvalue - Value for the new variable" )
		MAP_METHOD( 
			"FindVariable", 
			PyFindVariable, 
			"Returns a variable in the local store or its parents. Throws an exception if the variable is not found." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable" )
		MAP_METHOD( 
			"FindLocalVariable", 
			PyFindLocalVariable, 
			"Returns a variable in the local store. Throws an exception if the variable is not found." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable" )
		MAP_METHOD( 
			"GetVariable", 
			PyGetVariable, 
			"Returns a variable in the local store or its parents. If the variable is not found the function creates"
			" a new variable in this store." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable" )
		MAP_METHOD( 
			"GetLocalVariable", 
			PyGetLocalVariable, 
			"Returns a variable in the local store. If the variable is not found the function creates"
			" a new variable in this store." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable" )
		MAP_METHOD_AND_WRAP( 
			"UnregisterVariable", 
			UnregisterVariable, 
			"Unregisters an existing variable. Looks for the variable in this store and its parents." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable" )
		MAP_METHOD_AND_WRAP( 
			"UnregisterLocalVariable", 
			UnregisterLocalVariable, 
			"Unregisters an existing variable. Looks for the variable only in this store." 
			"\n"
			"\nArguments:"
			"\nname - Name of the variable" )
		MAP_METHOD_AND_WRAP( 
			"GetLocalNames", 
			GetLocalNames, 
			"Returns a list of names of all local variables in the store." )
	EXPOSURE_END()
}