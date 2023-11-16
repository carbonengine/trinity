#pragma once
#ifndef MatrixUtils_h
#define MatrixUtils_h

struct Matrix;

// Deconstructs a perspective projection matrix according to the current handedness into
// aspect ratio, field-of-view, front clip distance and back clip distance.
void DeconstructProjectionMatrix( const Matrix& proj, float& asp, float& fov, float& frontClip, float& backClip );

// copy a 3x4 granny-matrix into a dx 4x4 matrix
Matrix* TriMatrixCopyFrom3x4( Matrix* out, const granny_matrix_3x4* in );

#endif