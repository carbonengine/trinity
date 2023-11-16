#include "StdAfx.h"
#include "include/TriMatrix.h"
#include "Include/ITriVector.h"
#include "Include/ITriQuaternion.h"

BLUE_DEFINE( TriMatrix );

const Be::ClassInfo* TriMatrix::ExposeToBlue()
{
	EXPOSURE_BEGIN(TriMatrix, TRIMATRIX_Description)
		MAP_INTERFACE(ITriMatrix)
#if BLUE_WITH_PYTHON
		MAP_INTERFACE(IPythonMethods)
#endif

		MAP_ATTRIBUTE( "_11", _11, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_12", _12, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_13", _13, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_14", _14, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_21", _21, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_22", _22, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_23", _23, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_24", _24, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_31", _31, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_32", _32, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_33", _33, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_34", _34, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_41", _41, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_42", _42, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_43", _43, "", Be::READWRITE | Be::PERSIST )
		MAP_ATTRIBUTE( "_44", _44, "", Be::READWRITE | Be::PERSIST )

		////////////////////////////////////////////////////////////////////////////
		//               __init__
		MAP_METHOD_AS_METHOD
		( 
			"__init__",
			Py__init__, 
			"Constructor arguments" 
		)

		////////////////////////////////////////////////////////////////////////////
		//               Determinant
		MAP_METHOD_AND_WRAP
		( 
			"Determinant",
			Determinant, 
			"" 
		)
		////////////////////////////////////////////////////////////////////////////
		//               Identity
		MAP_METHOD_AND_WRAP
		( 
			"Identity",
			Identity, 
			"" 
		)
		////////////////////////////////////////////////////////////////////////////
		//               Inverse
		MAP_METHOD_AND_WRAP
		( 
			"Inverse",
			Inverse, 
			"" 
		)
		////////////////////////////////////////////////////////////////////////////
		//               LookAtRH
		MAP_METHOD_AND_WRAP
		( 
			"LookAtRH",
			PyLookAtRH, 
			":param eye: \n"
			":param at: \n"
			":param up: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               Multiply
		MAP_METHOD_AND_WRAP
		( 
			"Multiply",
			PyMultiply, 
			":param other: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               RotationAxis
		MAP_METHOD_AND_WRAP
		( 
			"RotationAxis",
			PyRotationAxis, 
			":param axis: \n"
			":param angle: \n"
		)////////////////////////////////////////////////////////////////////////////
		//               RotationQuaternion
		MAP_METHOD_AND_WRAP
		( 
			"RotationQuaternion",
			PyRotationQuaternion, 
			":param quaternion: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               RotationX
		MAP_METHOD_AND_WRAP
		( 
			"RotationX",
			RotationX, 
			":param angle: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               RotationY
		MAP_METHOD_AND_WRAP
		( 
			"RotationY",
			RotationY, 
			":param angle: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               RotationYawPitchRoll
		MAP_METHOD_AND_WRAP
		( 
			"RotationYawPitchRoll",
			RotationYawPitchRoll, 
			":param yaw: \n"
			":param pitch: \n"
			":param roll: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               RotationZ
		MAP_METHOD_AND_WRAP
		( 
			"RotationZ",
			RotationZ, 
			":param angle: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               Scaling
		MAP_METHOD_AND_WRAP
		( 
			"Scaling",
			Scaling, 
			":param sx: \n"
			":param sy: \n"
			":param sz: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               Transformation
		MAP_METHOD_AND_WRAP
		( 
			"Transformation",
			PyTransformation, 
			":param scalingCenter: \n"
			":param scalingRotation: \n"
			":param scaling: \n"
			":param rotationCenter: \n"
			":param rotation: \n"
			":param translation: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               Translation
		MAP_METHOD_AND_WRAP
		( 
			"Translation",
			Translation, 
			":param x: \n"
			":param y: \n"
			":param z: \n"
		)
		////////////////////////////////////////////////////////////////////////////
		//               Transpose
		MAP_METHOD_AND_WRAP
		( 
			"Transpose",
			Transpose, 
			"" 
		)			

	EXPOSURE_END()

}
