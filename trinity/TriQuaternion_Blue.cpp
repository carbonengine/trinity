#include "StdAfx.h"
#include "include/TriQuaternion.h"
#include "Include/ITriVector.h"
#include "Include/ITriMatrix.h"

#if BLUE_WITH_PYTHON
BLUE_DEFINE( TriQuaternion );

const Be::ClassInfo* TriQuaternion::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriQuaternion, TRIQUATERNION_Description)
		MAP_INTERFACE(ITriQuaternion)
		MAP_INTERFACE(IPythonMethods)			

		////////////////////////////////////////////////////////////////////////////
		//               x
		MAP_ATTRIBUTE
		( 
			"x", 
			x, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		//               y
		MAP_ATTRIBUTE
		( 
			"y", 
			y, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		//               z
		MAP_ATTRIBUTE
		( 
			"z", 
			z, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)

		////////////////////////////////////////////////////////////////////////////
		//               w
		MAP_ATTRIBUTE
		( 
			"w", 
			w, 
			"", 
			Be::READWRITE | Be::PERSIST 
		)


		////////////////////////////////////////////////////////////////////////////			
		MAP_METHOD_AND_WRAP_OPTIONAL_ARGS( 
			"__init__",
			Py__init__, 
			4,
			"Constructor arguments\n"
			":param x: \n"
			":param y: \n"
			":param z: \n"
			":param w: \n"
			)

		////////////////////////////////////////////////////////////////////////////
		MAP_METHOD_AND_WRAP( 
			"SetXYZW",
			SetXYZW, 
			"sets the quaternion\n"
			":param x: \n"
			":param y: \n"
			":param z: \n"
			":param w: \n"
			)

		////////////////////////////////////////////////////////////////////////////			
		MAP_METHOD_AND_WRAP( 
			"Identity",
			PyIdentity, 
			"Makes the quaternion an identity quaternion")

		////////////////////////////////////////////////////////////////////////////			
		MAP_METHOD_AND_WRAP( 
			"Length",
			PyLength, 
			"Returns the length of a quaternion")

		////////////////////////////////////////////////////////////////////////////			
		MAP_METHOD_AND_WRAP( 
			"Multiply",
			PyMultiply, 
			"Multiplies a quaternion with this quaternion\n"
			":param other: \n"
			)
			
		////////////////////////////////////////////////////////////////////////////			
		MAP_METHOD_AND_WRAP( 
			"Normalize",
			PyNormalize, 
			"Normalizes this quaternion" )

		////////////////////////////////////////////////////////////////////////////
		MAP_METHOD_AND_WRAP( 
			"RotationAxis",
			PyRotationAxis, 
			"Set the rotation of the quaternion\n"
			":param axis: \n"
			":param angle: \n"
			)


		////////////////////////////////////////////////////////////////////////////
		//               SetIdentity
		MAP_METHOD_AND_WRAP
		(
			"SetIdentity",
			SetIdentity, 
			"Sets quaternion to identity quaternion" 
		)

		////////////////////////////////////////////////////////////////////////////
		//               SetRotationAxis
		MAP_METHOD_AND_WRAP
		( 
			"SetRotationAxis",
			PySetRotationAxis, 
			"Set the rotation of the quaternion\n" 
			":param axis: \n"
			":param angle: \n"
		)

		////////////////////////////////////////////////////////////////////////////
		//               SetYawPitchRoll
		MAP_METHOD_AND_WRAP
		( 
			"SetYawPitchRoll",
			SetYawPitchRoll, 
			"Set the yaw pitch roll of the quaternion\n" 
			":param yaw: \n"
			":param pitch: \n"
			":param roll: \n"
		)			

		////////////////////////////////////////////////////////////////////////////
		//               GetYawPitchRoll
		MAP_METHOD_AND_WRAP
		( 
			"GetYawPitchRoll",
			PyGetYawPitchRoll, 
			"Get the yaw pitch roll of the quaternion\n" 
			":rtype: (float, float, float)"
		)			

		////////////////////////////////////////////////////////////////////////////
		//               Scale
		MAP_METHOD_AND_WRAP
		( 
			"Scale",
			PyScale, 
			"Scales the quaternion\n" 
			":param scale: \n"
		)

	EXPOSURE_END()

}

#endif
