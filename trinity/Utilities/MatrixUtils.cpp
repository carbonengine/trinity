// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2Renderer.h"

Float4x3::Float4x3( const Matrix& m )
{
	elements[0] = m._11;
	elements[1] = m._21;
	elements[2] = m._31;
	elements[3] = m._41;
	elements[4] = m._12;
	elements[5] = m._22;
	elements[6] = m._32;
	elements[7] = m._42;
	elements[8] = m._13;
	elements[9] = m._23;
	elements[10] = m._33;
	elements[11] = m._43;
}

Float4x3::operator Matrix() const
{
	return Matrix(
		elements[0], elements[4], elements[8], 0.0f, elements[1], elements[5], elements[9], 0.0f, elements[2], elements[6], elements[10], 0.0f, elements[3], elements[7], elements[11], 1.0f );
}

void DeconstructProjectionMatrix( const Matrix& proj, float& asp, float& fov, float& frontClip, float& backClip )
{
	// Use the fact that:
	// aspect = m_22 / m_11;
	// m_33 = z_f/(z_f-z_n), m_43 = -z_n*z_f/(z_f-z_n)
	// => front = z_n = -m_43/m_33
	// => back = z_f = -m_43/(1+m_43/z_n)
	// m_22 = cotan(fov/2) = 1 / tan(fov/2)
	// => fov = 2*tan(1/m_22)
	asp = ( proj._11 ? proj._22 / proj._11 : 0.0f );
	fov = ( proj._22 ? 2.0f * atan( 1.0f / proj._22 ) : 0.0f );

	frontClip = ( proj._33 ? proj._43 / proj._33 : 0.0f );
	backClip = frontClip * proj._33 / ( proj._33 + 1 );
}

#if WITH_GRANNY
// --------------------------------------------------------------------------------
// Description:
//   Copies a matrix from granny-typical 3x4 to dx-typical 4x4
// Arguments:
//   out - return 4x4 matrix
//   in - input 3x4 matrix
// SeeAlso:
//   Matrix, granny_matrix_3x4
// --------------------------------------------------------------------------------
Matrix* TriMatrixCopyFrom3x4( Matrix* out, const granny_matrix_3x4* in )
{
	out->_11 = ( *in )[0][0];
	out->_21 = ( *in )[0][1];
	out->_31 = ( *in )[0][2];
	out->_41 = ( *in )[0][3];
	out->_12 = ( *in )[1][0];
	out->_22 = ( *in )[1][1];
	out->_32 = ( *in )[1][2];
	out->_42 = ( *in )[1][3];
	out->_13 = ( *in )[2][0];
	out->_23 = ( *in )[2][1];
	out->_33 = ( *in )[2][2];
	out->_43 = ( *in )[2][3];
	return out;
}
#endif

// --------------------------------------------------------------------------------
// Description:
//   Copies a matrix from 3x4 to 4x4. Leaves the 4-th column of the output matrix unchanched.
// Arguments:
//   out - return 4x4 matrix
//   in - input 3x4 matrix
// SeeAlso:
//   Matrix, Float4x3
// --------------------------------------------------------------------------------
Matrix* TriMatrixCopyFrom3x4( Matrix* out, const Float4x3* in )
{
	out->_11 = ( *in ).elements[0]; //[0][0];
	out->_21 = ( *in ).elements[1]; //[0][1];
	out->_31 = ( *in ).elements[2]; //[0][2];
	out->_41 = ( *in ).elements[3]; //[0][3];
	out->_12 = ( *in ).elements[4]; //[1][0];
	out->_22 = ( *in ).elements[5]; //[1][1];
	out->_32 = ( *in ).elements[6]; //[1][2];
	out->_42 = ( *in ).elements[7]; //[1][3];
	out->_13 = ( *in ).elements[8]; //[2][0];
	out->_23 = ( *in ).elements[9]; //[2][1];
	out->_33 = ( *in ).elements[10]; //[2][2];
	out->_43 = ( *in ).elements[11]; //[2][3];
	return out;
}