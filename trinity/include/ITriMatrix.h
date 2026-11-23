// Copyright © 2000 CCP ehf.

#ifndef _ITRIMATRIX_H_
#define _ITRIMATRIX_H_

BLUE_INTERFACE( ITriMatrix ) :
	public IRoot
{
	// Construction and data access
	virtual void SetPivots(
		float _11, float _12, float _13, float _14, float _21, float _22, float _23, float _24, float _31, float _32, float _33, float _34, float _41, float _42, float _43, float _44 ) = 0;

	virtual void SetMatrix(
		const Matrix* m ) = 0;

	virtual const Matrix* GetMatrix() const = 0;

	virtual Matrix* CopyMatrix(
		Matrix * in ) const = 0;


	// Math functions
	virtual void Identity() = 0;
};

#endif