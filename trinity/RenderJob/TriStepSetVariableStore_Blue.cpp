// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepSetVariableStore.h"
#include "Resources/TriTextureRes.h"
#include "Tr2DepthStencil.h"
#include "Tr2RenderTarget.h"
#include "Include/ITr2GpuBuffer.h"

BLUE_DEFINE( TriStepSetVariableStore );

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
#if BLUE_WITH_PYTHON
void TriStepSetVariableStore::py__init__( Be::Optional<std::string> name, PyObject* value )
{
	if( name.IsAssigned() )
	{
		SetName( name );
	}
	if( value && value != Py_None )
	{
		SetValue( value );
	}
}

PyObject* TriStepSetVariableStore::GetValue()
{
	if( m_type == TRIVARIABLE_TEXTURE_RES )
	{
		if( !m_texture )
		{
			Py_RETURN_NONE;
		}
		return PyOS->WrapBlueObject( m_texture );
	}
	else if( m_type == TRIVARIABLE_GPUBUFFER )
	{
		if( !m_gpuBuffer )
		{
			Py_RETURN_NONE;
		}
		return PyOS->WrapBlueObject( m_gpuBuffer );
	}
	else
	{
		// TODO
		Py_RETURN_NONE;
	}
}

void TriStepSetVariableStore::SetValue( PyObject* valueArg )
{
	const unsigned valueArgPosition = 0;

	ITr2TextureProvider* valTextureRes; // TRIVARIABLE_TEXTURE_RES,
	ITr2GpuBuffer* valGpuBuffer; // TRIVARIABLE_TEXTURE_RES,
	int valInt; // TRIVARIABLE_INT,
	float valFloat; // TRIVARIABLE_FLOAT,
	Vector2 valVector2; // TRIVARIABLE_FLOAT2,
	Vector3 valVector3; // TRIVARIABLE_FLOAT3,
	Vector4 valVector4; // TRIVARIABLE_FLOAT4,
	Matrix valMatrix; // TRIVARIABLE_FLOAT4X4,
	Color valColor; // TRIVARIABLE_COLOR,

	if( BlueExtractArgument( valueArg, valTextureRes, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_TEXTURE_RES;
		m_texture = valTextureRes;
	}
	else if( BlueExtractArgument( valueArg, valGpuBuffer, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_GPUBUFFER;
		m_gpuBuffer = valGpuBuffer;
	}
	else if( BlueExtractArgument( valueArg, valInt, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_INT;
		memcpy( m_data, &valInt, sizeof( valInt ) );
	}
	else if( BlueExtractArgument( valueArg, valFloat, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_FLOAT;
		memcpy( m_data, &valFloat, sizeof( valFloat ) );
	}
	else if( BlueExtractArgument( valueArg, valVector2, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_FLOAT2;
		memcpy( m_data, &valVector2, sizeof( valVector2 ) );
	}
	else if( BlueExtractArgument( valueArg, valVector3, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_FLOAT3;
		memcpy( m_data, &valVector3, sizeof( valVector3 ) );
	}
	else if( BlueExtractArgument( valueArg, valVector4, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_FLOAT4;
		memcpy( m_data, &valVector4, sizeof( valVector4 ) );
	}
	else if( BlueExtractArgument( valueArg, valMatrix, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_FLOAT4X4;
		memcpy( m_data, &valMatrix, sizeof( valMatrix ) );
	}
	else if( BlueExtractArgument( valueArg, valColor, valueArgPosition ) )
	{
		m_type = TRIVARIABLE_COLOR;
		memcpy( m_data, &valColor, sizeof( valColor ) );
	}

	PyErr_Clear();
}
#endif

const Be::ClassInfo* TriStepSetVariableStore::ExposeToBlue()
{
	EXPOSURE_BEGIN( TriStepSetVariableStore, "Render step for setting a variable in variable store" )

		MAP_INTERFACE( TriRenderStep )
		MAP_INTERFACE( TriStepSetVariableStore )

#if BLUE_WITH_PYTHON
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS(
			"__init__",
			py__init__,
			2,
			"Create a render step that sets a variable value to the variable store"
			"\n:param name: string name of the variable"
			"\n:param value: a valid python object to set" )
#endif
		MAP_ATTRIBUTE( "variableName", m_variableName, "The name of the Tr2VariableStore variable to set", Be::READWRITE | Be::PERSIST )
#if BLUE_WITH_PYTHON
		MAP_PROPERTY( "value", GetValue, SetValue, "Setter for the variable store value" )
#endif

	EXPOSURE_CHAINTO( TriRenderStep )
}