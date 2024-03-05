#include "StdAfx.h"
#include "Tr2Renderer.h"

void DeconstructProjectionMatrix( const Matrix& proj, float& asp, float& fov, float& frontClip, float& backClip )
{
	// Use the fact that:
	// aspect = m_22 / m_11;
	// m_33 = z_f/(z_f-z_n), m_43 = -z_n*z_f/(z_f-z_n)
	// => front = z_n = -m_43/m_33 
	// => back = z_f = -m_43/(1+m_43/z_n)
	// m_22 = cotan(fov/2) = 1 / tan(fov/2)
	// => fov = 2*tan(1/m_22) 
	asp = (proj._11	 ?  proj._22/proj._11 : 0.0f);
	fov = (proj._22	 ?  2.0f*atan(1.0f/proj._22) : 0.0f);

	frontClip =	(proj._33	 ?  proj._43/proj._33 : 0.0f);
	backClip =	frontClip*proj._33/(proj._33+1);
}


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
	out->_11 = (*in)[0][0];
	out->_21 = (*in)[0][1];
	out->_31 = (*in)[0][2];
	out->_41 = (*in)[0][3];
	out->_12 = (*in)[1][0];
	out->_22 = (*in)[1][1];
	out->_32 = (*in)[1][2];
	out->_42 = (*in)[1][3];
	out->_13 = (*in)[2][0];
	out->_23 = (*in)[2][1];
	out->_33 = (*in)[2][2];
	out->_43 = (*in)[2][3];
	return out;
}

