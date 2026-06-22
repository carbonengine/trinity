// Copyright © 2023 CCP ehf.

#pragma once
#ifndef MatrixUtils_h
#define MatrixUtils_h

struct Matrix;

struct Float4x3
{
	Float4x3() = default;
	explicit Float4x3( const Matrix& m );

	operator Matrix() const;

	float elements[12];
};

// Deconstructs a perspective projection matrix according to the current handedness into
// aspect ratio, field-of-view, front clip distance and back clip distance.
void DeconstructProjectionMatrix( const Matrix& proj, float& asp, float& fov, float& frontClip, float& backClip );

#if WITH_GRANNY
// copy a 3x4 granny-matrix into a dx 4x4 matrix
Matrix* TriMatrixCopyFrom3x4( Matrix* out, const granny_matrix_3x4* in );
#endif

// copy a 3x4 matrix into a 4x4 matrix
Matrix* TriMatrixCopyFrom3x4( Matrix* out, const Float4x3* in );

#endif