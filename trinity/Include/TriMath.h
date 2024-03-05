/* 
	*************************************************************************************

	TriMath.h

	Author:    Hilmar Veigar Pétursson
	Created:   March 2001
	OS:        Win32
	Project:   Trinity

	Description:   

		Usefull math functions that are "missing" from the math library. Some of
		the TriDataTypes have member functions that are direct thunkers to these functions


	Dependencies:

		DirectX 9.0, Blue

	(c) CCP 2000

	*************************************************************************************
*/

#pragma once

#ifndef _TRIMATH_H_
#define _TRIMATH_H_

#include "ITriConstants.h"

struct Vector3;
struct Vector3d;
/////////////////////////////////////////////////////////////////////////////////////////
// Vector extensions
/////////////////////////////////////////////////////////////////////////////////////////

// identical vectors with a threshold
bool TriVectorIsIdentical( const Vector3* v1, const Vector3* v2, float epsilon = 1e-5f );

//Rotate a vector by the quaternion
Vector3* TriVectorRotateQuaternion(
	Vector3* out, 
	const Vector3* v,
	const Quaternion* q
	);

//Rotate a vector by the rotation part of a matrix (ignores translation)
Vector3* TriVectorRotateMatrix(
	Vector3* out, 
	const Vector3* v,
	const Matrix* m
	);

//The next two functions are the most common application
//of the previous two functions and can be optimized greatly
//with our knowledge of the usage

//Rotate a unit vector aligned to one of the axes 
//(defined by xyz) by the quaternion
Vector3* TriVectorRotatedBasisQuaternion(
	Vector3* out, 
	const TRITRANSFORMAXIS xyz,
	const Quaternion* q
	);

//Rotate a unit vector aligned to one of the axes 
//(defined by xyz) by the matrix
Vector3* TriVectorRotatedBasisMatrix(
	Vector3* out, 
	const TRITRANSFORMAXIS xyz,
	const Matrix* m
	);


//
Vector3* TriVectorSpherical( 
	Vector3* v, 
	float phi,
	float theta,
	float rad
	);

Vector3* TriVectorExponentialDecayInteger( 
	Vector3* pos, 
	const Vector3* pos0,
	const Vector3* vel0,
	const Vector3* acc0,
	const float mass,
	const float drag,
	float time
	);

Vector3* TriVectorExponentialDecay( 
	Vector3* vel, 
	const Vector3* vel0,
	const Vector3* acc0,
	const float mass,
	const float drag,
	float time
	);

Vector3* TriVectorExponentialDecayInteger( 
	Vector3* pos, 
	const Vector3* x,
	const Vector3* v,
	const Vector3* a,
	const float m,
	const float k,
	const float t,
	const float pow
	);

Vector3* TriVectorExponentialDecay( 
	Vector3* vel, 
	const Vector3* v,
	const Vector3* a,
	const float k,
	const float pow
	);

// Projects a point onto a plane
Vector3 TriVectorProjectOnPlane( const Vector3& point, const Vector3& p0, const Vector3& n );

// Projects a vector onto plane but maintains it's length so it's effectively like moving it onto the plane
Vector3 TriVectorRotateToPlane( const Vector3& point, const Vector3& p0, const Vector3& n );


/////////////////////////////////////////////////////////////////////////////////////////
// Float extensions
/////////////////////////////////////////////////////////////////////////////////////////

// seed random number generator
void TriRandomSeed( unsigned int seed );

// Return a random number between 0.f and 1.f
float TriFloatRandom01();

// Return -1.f if < 0.f, 1.f otherwise
float TriFloatSign( float f );

// Return gaussian (normal) distribution
float TriFloatRandomGauss( float mu, float deviation );


/////////////////////////////////////////////////////////////////////////////////////////
// Color extensions
/////////////////////////////////////////////////////////////////////////////////////////

// Turns a normalized vector into a color. Used to change directions into color. 
Color* TriColorFromVector( 
	Color* c,
	const Vector3* v, 
	float height = 0.0f 
	);


/////////////////////////////////////////////////////////////////////////////////////////
// Quaternion extensions
/////////////////////////////////////////////////////////////////////////////////////////

// pre:  'in' is a unit quaternion
// post: 'out' is the sqrt of 'q1'
// Nonni's expl: a sqrt is the inverse of squaring, by definition. You can do the math yourselves
// do see that this holds for this function
// Why sqrt of Quaternion? Well... the product of two quaternions is the combined rotation of them.
// The square of a quaternion is twice as much rotation around the *same* axis as the quaternion
// Ergo, the sqrt is half as much rotation around the *same* axis as the quaternion.
// This is very useful, since a quaternion rotates by theta when w = cos(theta/2)
// See TriQuaternionRotationArc for an example of use.
Quaternion* TriQuaternionSqrt(
	Quaternion* out,
	const Quaternion* q
	);

// Builds a quaternion that is the rotation between two vectors about the origin
Quaternion* TriQuaternionRotationArc(
	Quaternion* out, 
	const Vector3* v1,
	const Vector3* v2
	);

Quaternion* TriQuaternionArcFromForward(
	Quaternion* out, 
	const Vector3* v 
	);

// does something nice, Eggert, please specify....
Quaternion* TriQuaternionAxisHeading(
	Quaternion* out, 
	const Quaternion* q,
	const Vector3* v
	);

//Takes in a vector, and returns a pure unit quaternion with the same rotation axis as the vectors heading
Quaternion* TriQuaternionDirVector(
	Quaternion* out, 
	const Vector3* v
	);

//out = in*length
Quaternion* TriQuaternionScale(
	Quaternion* out, 
	const Quaternion* in,
	float length
	);


//Pre: q is a unit quaternion
//Post: yaw in [0;2*pi[ , pitch in [-pi/2;pi/2]; roll in  [-pi; pi]
// if you yaw and then pitch and then roll you get the same result as using the quaternion
void TriQuaternionToYawPitchRoll (
    float* yaw, 
    float* pitch, 
    float* roll,
	const Quaternion* q
	);

/////////////////////////////////////////////////////////////////////////////////////////
// Matrix extensions
/////////////////////////////////////////////////////////////////////////////////////////

// advanced matrix transpose: by knowing the size it determines if to transpose a
// 4x4 or 4x3 matrix
Matrix* TriMatrixTranspose(Matrix* out, const Matrix* in, unsigned int sizeInBytes);

//These functions are used instead of creating a translation matrix and multiplying by that
//The only reason to use these is for optimization purposes

//use this to translate by v and then transform by m
//this is 20 times faster than multiplying with a translation matrix 
Matrix* TriMatrixTranslate(Matrix* out, const Matrix* m, const Vector3* v);
//use this to transform by m and then translate by v
//this is four times faster than multiplying with a translation matrix 
Matrix* TriMatrixTranslate(Matrix* out, const Vector3* v, const Matrix* m);


//with these functions we can always use quaternions instead of rotation matrices
//which helps comprehension

//use this to rotate by q and then transform by m
Matrix* TriMatrixRotate(Matrix* out, const Matrix* m, const Quaternion* q);
//use this to transform by m and then rotate by q
Matrix* TriMatrixRotate(Matrix* out, const Quaternion* q, const Matrix* m);

Matrix* TriMatrixChangeBase(Matrix* out, const Vector3* fwd, const Vector3* up);

Matrix* TriMatrixRemoveScaling( Matrix* out, const Matrix* in );
Matrix* TriMatrixRemoveTranslation( Matrix* out, const Matrix* in );
Matrix* TriMatrixOverwriteTranslation( Matrix* out, const Matrix* in, const Vector3* t );

Matrix* TriMatrixRotationArc(
	Matrix* out, 
	const Vector3* v1, 
	const Vector3* v2
	);

Matrix* TriMatrixArcFromForward(
	Matrix* out, 
	const Vector3* v 
	);


/////////////////////////////////////////////////////////////////////////////////////////
// Double precision matrix functions
/////////////////////////////////////////////////////////////////////////////////////////
double* Matrix4dMultiply( double* result, double* m0, double* m1 );
double* Matrix4dInvert( double* result, const double* mat );

Vector4d Matrix4dTransform( Vector4d vector, double* mat );
double* Matrix4dCopy( double* result, double* mat );

Matrix Matrix4dToMatrix( double* mat );
double* Matrix4dFromMatrix( double* result, const Matrix& mat );

/////////////////////////////////////////////////////////////////////////////////////////
// Misc extensions
/////////////////////////////////////////////////////////////////////////////////////////

bool IsFinite( float value );
bool IsFinite( const Vector3& vec );

float TriClamp( float f, float min, float max );
int32_t ClampInt( int32_t f, int32_t min, int32_t max );
uint32_t ClampUInt( uint32_t f, uint32_t min, uint32_t max );
float Lerp( float min, float max, float s );
float Hermite( float v1, float r, float v2, float l, float t, float dt );
float TriLinearize( float min, float max, float v );

//pre: f in [0.0 ; 1.0]
//post: returnvalue in [0.0 ; 1.0] 
float SinSmooth(
	float f
	);


float CubicInterpolate( float f0, float f1, float f2, float f3, float s );

/////////////////////////////////////////////////////////////////////////////////////////
// random functions
/////////////////////////////////////////////////////////////////////////////////////////
void TriSrand( Be::Time seed );
void TriSrand( uint32_t seed );
float TriRand();
int TriRandInt( int hi );
int TriRandInt( int lo, int hi );
int TriRandGetSeed();


/////////////////////////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////////////////////////

const float TRI_PI    = 3.141592654f;
const float TRI_1BYPI = 0.318309886f;
const float TRI_2PI   = 6.283185307f;                        
const float TRI_PIBY2 = 1.570796327f;
const float TRI_E     = 2.718281828f;//45904523536028747135266249775724709369996
const float TRI_SQRT2 = 1.414213562f;
const float TRI_SQRT3 = 1.732050807f;



/**
 * @brief Implementation of Perlin noise in 1D. Based on the original implementation by Ken Perlin.
 */
class TriPerlinNoise
{
public:
	/**
	 * @brief Constructs a TriPerlinNoise object with a seed from std::random_device. This ensures that the noise pattern is different each time the program is run.
	 */
	TriPerlinNoise();
	/**
	 * @brief Constructs a TriPerlinNoise object with a specified seed. This allows for reproducible noise patterns.
	 * @param seed The seed for the random number generator used to initialize the noise tables.
	 */
	explicit TriPerlinNoise( uint32_t seed );

	/**
	 * @brief Evaluates the Perlin noise function at a given point x.
	 * @param x The input coordinate for which to evaluate the noise. Note that internally the function casts x to a 32bit integer, so the range of x is limited.
	 * @return The Perlin noise value at the given coordinate, in the range [-1, 1].
	 */
	[[nodiscard]] double operator()( double x ) const;

	/**
	 * @brief Evaluates the fractal sum of Perlin noise at a given point x: a sum of multiple noise octaves with changing frequency and amplitude.
	 *   Based on Paul Bourke's implementation: https://paulbourke.net/fractals/noise/index.html
	 * @param x The input coordinate for which to evaluate the noise. Note that internally the function casts x to a 32bit integer, so the range of x is limited.
	 * @param octaves The number of octaves to use in the fractal sum. More octaves will result in more detail but also more computation.
	 * @param amplitudeScale The factor by which to scale the amplitude of each octave. Typically less than 1 (e.g., 0.5) to ensure that higher octaves contribute less to the final sum.
	 * @param frequencyScale The factor by which to scale the frequency of each octave. Typically greater than 1 (e.g., 2) to ensure that higher octaves have higher frequency.
	 * @return The fractal sum of Perlin noise octaves at the given coordinate.
	 */
	[[nodiscard]] double FractalSum( double x, size_t octaves, double amplitudeScale = 0.5, double frequencyScale = 2 ) const;

private:
	static constexpr int32_t TABLE_SIZE = 256;
	static constexpr int32_t TABLE_MASK = TABLE_SIZE - 1;
	static_assert( ( TABLE_SIZE & TABLE_MASK ) == 0, "TABLE_SIZE must be a power of 2" );

	std::array<double, TABLE_SIZE> m_gradients;
};

double PerlinNoise1D( double x, double invAmplitude, double frequency, int octaves );

////////////////////////////////////////
//
// Coordinate system conversion methods
//
bool ConvertProjectionCoordToWorldPickRay( float x, float y, const Matrix* projMat, const Matrix* viewMat, Vector3* rayStart, Vector3* rayDir );


#endif