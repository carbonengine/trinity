////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2022
// Copyright:	CCP 2022
//

#include "StdAfx.h"
#include "Tr2MaterialBoundsAdjustment.h"


CcpMath::AxisAlignedBox Tr2MaterialBoundsAdjustment::AdjustBounds( const CcpMath::AxisAlignedBox& box ) const
{
	auto bounds = box;
	bounds.Scale( maxLocalScale );
	bounds.Grow( maxLocalDisplacement );
	if( rotatesVertices )
	{
		float radius = 0;
		bounds.EnumerateVertices( [&radius]( const Vector3& vtx ) {
			radius = std::max( radius, LengthSq( vtx ) );
		} );
		radius = sqrt( radius );
		bounds = CcpMath::AxisAlignedBox( CcpMath::Sphere( Vector3( 0, 0, 0 ), radius ) );
	}
	return bounds;
}


#if BLUE_WITH_PYTHON
bool BlueExtractArgumentImpl( BlueScriptArguments argument, Tr2MaterialBoundsAdjustment& result, unsigned int argID, std::false_type isBlueType )
{
	if( !PyTuple_Check( argument ) )
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type.", argID );
		return false;
	}
	int rotated = 0;
	if( PyArg_ParseTuple( argument, "ffi:Tr2MaterialBoundsAdjustment", &result.maxLocalScale, &result.maxLocalDisplacement, &rotated ) )
	{
		result.rotatesVertices = rotated != 0;
		return true;
	}
	else
	{
		PyErr_Format( PyExc_TypeError, "Could not match argument %i to expected type.", argID );
		return false;
	}
}

BlueScriptValue BlueWrapReturnValueImpl( BlueScriptArguments args, const Tr2MaterialBoundsAdjustment& val )
{
	return Py_BuildValue( "ffO", val.maxLocalScale, val.maxLocalDisplacement, val.rotatesVertices ? Py_True : Py_False );
}
#endif