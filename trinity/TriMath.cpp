#include "StdAfx.h"
#include "include/TriMath.h"

#include <algorithm>
#include <random>

/*
#include <limits>
#include "nrutil/matrix.h"
using namespace math;
#include "TriPhysics.h"
*/

#include "Utilities/Vector3d.h"
#include "Utilities/Vector4d.h"


/////////////////////////////////////////////////////////////////////////////////////////
// Vector extensions
/////////////////////////////////////////////////////////////////////////////////////////

bool TriVectorIsIdentical( const Vector3* v1, const Vector3* v2, float epsilon )
{
	return ( LengthSq( *v1 - *v2 ) < epsilon * epsilon );
}

Vector3* TriVectorRotateQuaternion(
	Vector3* out, 
	const Vector3* v,
	const Quaternion* q
	)
{
	// q*v*qcomplement, where we treat v as a pure quaternion

	float ww = q->w * q->w;
	float wx = q->w * q->x;
	float wy = q->w * q->y;
	float wz = q->w * q->z;

	float xx = q->x * q->x;
	float xy = q->x * q->y;
	float xz = q->x * q->z;

	float yy = q->y * q->y;
	float yz = q->y * q->z;

	float zz = q->z * q->z;

	Vector3 temp;
	temp.x = v->x*(ww + xx - yy - zz) + 2.0f*(v->y*(xy - wz) + v->z*(xz + wy));
	temp.y = v->y*(ww - xx + yy - zz) + 2.0f*(v->x*(xy + wz) + v->z*(yz - wx));
	temp.z = v->z*(ww - xx - yy + zz) + 2.0f*(v->x*(xz - wy) + v->y*(yz + wx));
	*out = temp;
	return out;
}


// Projects a point onto a plane
Vector3 TriVectorProjectOnPlane( const Vector3& point, const Vector3& p0, const Vector3& n )
{
	Vector3 out = point - p0;
	out = n * Dot( out, n );
	out = point - out;
	return out;
}

// Projects a vector onto plane but maintains it's length so it's effectively like moving/rotating it onto the plane
Vector3 TriVectorRotateToPlane( const Vector3& point, const Vector3& p0, const Vector3& n )
{
	Vector3 out = point - p0;
	float length = Length( out );
	out = n * Dot( out, n );
	out = point - out;
	// We have the projected position, now scale it to the original distance
	out -= p0;
	out = Normalize( out ) * length + p0;
	return out;
}

Vector3* TriVectorRotateMatrix(
	Vector3* out, 
	const Vector3* v,
	const Matrix* m
	)
{
	// can be optimized quite a lot by skipping the matrix op
	// that have to do with translation instead of cancelling them out
	Vector3 temp;
	temp.x = v->x*m->_11 + v->y*m->_21 + v->z*m->_31;
	temp.y = v->x*m->_12 +v->y*m->_22 +v->z*m->_32;
	temp.z = v->x*m->_13 +v->y*m->_23 +v->z*m->_33;
	*out = temp;
	return out;
}

//Rotate a unit vector aligned to one of the axes 
//(defined by xyz) by the quaternion
//to understand this code, refer to TriVectorRotateQuaternion
//10 multis, 11 adds opposed to 22 multis, 21 adds
//the weird arrays are so we don't have to do switch-statements
Vector3* TriVectorRotatedBasisQuaternion(
	Vector3* out, 
	const TRITRANSFORMAXIS xyz,
	const Quaternion* q
	)
{
	float ww = q->w * q->w;

	//[-xx, -yy, -zz]
	float SQ[] = {- q->x * q->x, - q->y * q->y, - q->z * q->z};
	//extend with q.x, q.y so we don't have to do modulo thingies with xyz
	float QUAT[] = {q->x, q->y, q->z, q->x, q->y};

	//ok this is directly from TriVectorRotateQuaternion, just
	//substitute 1 into xyz and 0 into the other basis
	SQ[xyz] = -SQ[xyz];
	float basis0 = ww + SQ[0] + SQ[1] + SQ[2];
	float basis1 = 2.0f*(QUAT[xyz]*QUAT[xyz+1] + q->w*QUAT[xyz+2]);
	float basis2 = 2.0f*(QUAT[xyz]*QUAT[xyz+2] - q->w*QUAT[xyz+1]);

	//finally we have to select the correct axises into the correct components
	//reversed and extended so we don't have to do modulo thingies with xyz
	float RET[] = {basis2, basis1, basis0, basis2, basis1};
	out->x = RET[xyz + 2]; 
	out->y = RET[xyz + 1];
	out->z = RET[xyz];
	return out;
}

//Rotate a unit vector aligned to one of the axes 
//(defined by xyz) by the matrix
Vector3* TriVectorRotatedBasisMatrix(
	Vector3* out, 
	const TRITRANSFORMAXIS xyz,
	const Matrix* m
	)
{
	*out = Vector3(m->m[xyz][0], m->m[xyz][1],m->m[xyz][2]);
	return out;
}

Vector3* TriVectorSpherical( 
	Vector3* v, 
	float phi,
	float theta,
	float rad
	)
{
	v->x  = rad * sinf( phi );
	v->z  = v->x * sinf( theta);
	v->x *= cosf(theta);
	v->y  = rad * cosf( phi );
	return v;
}

Vector3* TriVectorExponentialDecayInteger( 
	Vector3* pos, 
	const Vector3* x,
	const Vector3* v,
	const Vector3* a,
	const float m,
	const float k,
	float t
	)
{	
	*pos = *x + *a*t/k + m*(*v*k-*a)/(k*k)*(1.0f - powf(TRI_E, -k*t/m));
	return pos;
}

Vector3* TriVectorExponentialDecay( 
	Vector3* vel, 
	const Vector3* v,
	const Vector3* a,
	const float m,
	const float k,
	const float t
	)
{	
	*vel = *a/k + (*v - *a/k)*(powf(TRI_E, -k*t/m));
	return vel;
}

Vector3* TriVectorExponentialDecayInteger( 
	Vector3* pos, 
	const Vector3* x,
	const Vector3* v,
	const Vector3* a,
	const float m,
	const float k,
	const float t,
	const float pow
	)
{	
	*pos = *x + *a*t/k + m*(*v*k-*a)/(k*k)*(1.0f - pow);
	return pos;
}

Vector3* TriVectorExponentialDecay( 
	Vector3* vel, 
	const Vector3* v,
	const Vector3* a,
	const float k,
	const float pow
	)
{	
	*vel = *a/k + (*v - *a/k)*(pow);
	return vel;
}



/////////////////////////////////////////////////////////////////////////////////////////
// Float extensions
/////////////////////////////////////////////////////////////////////////////////////////

void TriRandomSeed( unsigned int seed )
{
	srand( seed );
}

float TriFloatRandom01()
{
	return (float)rand() / (float)RAND_MAX;
}

float TriFloatSign( float f )
{
	if( f < 0.f )
	{
		return -1.f;
	}
	return 1.f;
}

float TriFloatRandomGauss( float mu, float deviation )
{
	// box-muller
	float rand1 = TriFloatRandom01();
	if( rand1 < 1e-100 )
	{
		rand1 = 1e-100;
	}
	rand1 = -2.f * log( rand1 );
	float rand2 = TriFloatRandom01() * TRI_2PI;
 	return mu + deviation * pow( rand1, 0.75f ) * cos( rand2 );
}

/////////////////////////////////////////////////////////////////////////////////////////
// Color extensions
/////////////////////////////////////////////////////////////////////////////////////////

Color* TriColorFromVector( 
	Color* c,
	const Vector3* v, 
	float height
	)
{
	c->r = 0.5f * v->x + 0.5f;
    c->g = 0.5f * v->y + 0.5f;
    c->b = 0.5f * v->z + 0.5f;
    c->a = height;
	return c;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Quaternion extensions
/////////////////////////////////////////////////////////////////////////////////////////

Quaternion* TriQuaternionDirVector(
	Quaternion* out, 
	const Vector3* v
	)
{
	Vector3 nv = Normalize( *v );
	*out = Quaternion(nv.x, nv.y, nv.z, 0.0f);
	return out;
}

Quaternion* TriQuaternionScale(
	Quaternion* out, 
	const Quaternion* in,
	float length
	)
{
	if (length == 1.0f)
	{
		*out = *in;
		return out;
	}
	out->x = in->x*length;
	out->y = in->y*length;
	out->z = in->z*length;
	out->w = in->w*length;
	return out;
}

Quaternion* TriQuaternionSqrt(
	Quaternion* out, 
	const Quaternion* q1 
	)
{
	Quaternion test = *q1;
	*out = *q1;
	float magic1 = out->w;
	float magic = 0.99999f;
	float magic2 = magic1 + magic;
	if ( 0.0f > magic2)
	{
		Vector3 axis(out->x, out->y, out->z);
		axis *= 1000000.0f;
		axis = Normalize( axis );
		if( LengthSq( axis ) < 0.5 )
		{

			//The vector could not be normalized :-(
			out->x = 1.0f;
			out->y = 0.000f;
			out->z = 0.000f;
			out->w = 0.0f;
		}
		else
		{
			out->x = axis.x;
			out->y = axis.y;
			out->z = axis.z;
			out->w = 0.0f;
		}
		return out;
	}
	else
	{
		out->w++;
		*out = Normalize( *out );
		return out;
	}
}

Quaternion* TriQuaternionRotationArc(
	Quaternion* out, 
	const Vector3* v1, 
	const Vector3* v2 
	)
{
	TriQuaternionDirVector(out,v1);
	Quaternion q1;
	TriQuaternionDirVector(&q1,v2);
	q1 = Conjugate( q1 );
	*out = *out * q1;
	return TriQuaternionSqrt(out, out);
}

Quaternion* TriQuaternionArcFromForward(
	Quaternion* out, 
	const Vector3* v 
	)
{
	Vector3 temp = Normalize( *v );
	if (temp.z < 0.99999f)
	{
		temp.z = 1.0f - temp.z;
		temp.z = sqrtf(temp.z);
		float div = 0.707106781187f/temp.z;
		*out = Quaternion(temp.y*div, -temp.x*div, 0.0f, 0.707106781187f*temp.z);
		return out;
	}
	*out = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
	return out;

}

void TriQuaternionToYawPitchRoll (
    float* yaw, 
    float* pitch, 
    float* roll,
	const Quaternion* q
	)
{
	
	static float invSqrtOf2 = 0.70710678118654752440084436210485f;
	//equivalent to  q * (0,0,-1,0) * q^-1  
	float y = 2.0f*(q->x*q->w - q->z*q->y);
	float w;
	{
		float temp = 1.0f - y*y;
		if (temp < 0.0f) temp = 0.0f;
		w = sqrtf(temp);
	}
	//precalculate for speed
	float gamma = invSqrtOf2/sqrtf(w+1.0f);

	// q = qYaw*qPitch*qRoll 

	// let qPitch be the quaternion that satisfies
	// a) y = z = 0
	// b) qPitch * (0,0,-1,0) * qPitch^-1 = (0,0,y,0)

	//boring pre for asinf
	if (fabs(y) < 1.0f){}
	else if (y - 1.0f >  0.0f) y = 1.0f;
	else if (y + 1.0f < 0.0f) y = -1.0f;
	float phi = asinf(y); //unambigous
	Quaternion qPitch;
	qPitch.x = y*gamma;
	qPitch.y = 0.0f;
	qPitch.z = 0.0f;
	qPitch.w = (w+1.0f)*gamma;

	//remove the pitch
	// ie get qYaw*qRoll
	Quaternion yawRoll;

	//hmmm stuff gets screwed in denominator if y == +-1.0f 
	// well if y == +-1.0f we can take a short cut....
	if (fabs(fabs(y) - 1.0f) < 0.00001f)
	{
		Quaternion combinedRoll;
		//use as temp
		combinedRoll = Conjugate( qPitch );
		combinedRoll = *q*combinedRoll;
		//boring pre for acos
		if (fabs(combinedRoll.w) < 1.0f){}
		else if (combinedRoll.w - 1.0f >  0.0f) combinedRoll.w = 1.0f;
		else if (combinedRoll.w + 1.0f < 0.0f) combinedRoll.w = -1.0f;
		float psi = 2.0f*acosf(combinedRoll.w);
		if (psi > XM_PI) psi -= XM_PI*2.0f;
		//sign(rollConZ) = -sign(yawRoll.z)  
		if (combinedRoll.z > 0.0f)
		psi =  - psi;  
		*yaw = 0.0f;
		*pitch = phi;
		*roll = -psi;
		return;
	}

	float denominator = 1.0f/(qPitch.x*qPitch.x - qPitch.w*qPitch.w);
	yawRoll.x = (q->w*qPitch.x - q->x*qPitch.w)*denominator;
	yawRoll.y = -(q->z*qPitch.x + q->y*qPitch.w)*denominator;
	yawRoll.z = -(q->z*qPitch.w + q->y*qPitch.x)*denominator;
	yawRoll.w = (q->x*qPitch.x - q->w*qPitch.w)*denominator;

/*************************************************************************************************/
	// Nonni this was causing div by Zero erros
	// I added handler which uses 0.0 in that case, please check
	float divByZero = sqrtf(yawRoll.w*yawRoll.w + yawRoll.z*yawRoll.z);
	float rollGamma = divByZero == 0.0 ? 0.0f : 1.0f/divByZero;
/*************************************************************************************************/
	// this is the w part of qRoll
	// we don't need anything else
	float rollConW = yawRoll.w*rollGamma; 

	//boring pre for acos
	if (fabs(rollConW) < 1.0f){}
	else if (rollConW - 1.0f >  0.0f) rollConW = 1.0f;
	else if (rollConW + 1.0f < 0.0f) rollConW = -1.0f;
	float psi = 2.0f*acosf(rollConW);
	if (psi > XM_PI) psi -= XM_PI*2.0f;
	//sign(rollConZ) = -sign(yawRoll.z)  
	if (yawRoll.z < 0.0f)
		psi =  - psi;  

	//Now we are multiplying qYaw*qRoll with qRoll^-1 from the left
	//Leaving us with the qYaw
	//yawRoll.y must be calculated before yawRoll.w because the original 
	//yawRoll.w is used in the calculation
	yawRoll.y = (yawRoll.x*yawRoll.z + yawRoll.y*yawRoll.w)*rollGamma;
	yawRoll.w = (yawRoll.z*yawRoll.z + yawRoll.w*yawRoll.w)*rollGamma;

	//boring pre for asin
	if (fabs(yawRoll.y) < 1.0f){}
	else if (yawRoll.y - 1.0f >  0.0f) yawRoll.y = 1.0f;
	else if (yawRoll.y + 1.0f < 0.0f) yawRoll.y = -1.0f;
	float theta = asinf(yawRoll.y);
	if (yawRoll.w < 0.0f )
		theta =  XM_PI - theta;  
	if (theta < 0.0f) theta += XM_PI;

	*yaw = theta*2.0f;
	*pitch = phi;
	*roll = psi;
	/*
	//Testing suite
	//Get the expected value in a parameters eYaw etc...
	static float deltaYaw = 0.0;
	static float deltaPitch = 0.0;
	static float deltaRoll = 0.0;
	if (fabs(*yaw - eYaw) > deltaYaw  ) 
	{
		if (fabs(fabs(*yaw - eYaw) - XM_PI*2.0f) < 0.2f)
		{
			float deltaYawTemp = fabsf(fabsf(*yaw - eYaw) - XM_PI*2.0f);
			if (deltaYawTemp > deltaYaw) deltaYaw = deltaYawTemp; 
		}
		else
		{
			deltaYaw = fabsf(*yaw -eYaw);
		}
	}
	if (fabsf(*pitch - ePitch) > deltaPitch) 
		deltaPitch = fabsf(*pitch - ePitch);
	if (fabsf(*roll - eRoll) > deltaRoll) 
	{
		if (fabsf(fabsf(*roll- eRoll) - XM_PI) < 0.2f)
		{
			float deltaRollTemp = fabsf(fabsf(*roll - eRoll) - XM_PI);
			if (deltaRollTemp > deltaRoll) deltaRoll = deltaRollTemp; 
		}
		else
		{
			deltaRoll = fabsf(*roll -eRoll);
		}
	}
	*/
}


/////////////////////////////////////////////////////////////////////////////////////////
// Matrix extensions
/////////////////////////////////////////////////////////////////////////////////////////

Matrix* TriMatrixTranspose(Matrix* out, const Matrix* in, unsigned int sizeInBytes)
{
	float f;

	// transpose 2x2 parts
	out->_11 = in->_11;
	out->_22 = in->_22;
	f = in->_12; out->_12 = in->_21; out->_21 = f;
	// transpose 3x3 parts, but check if allowed to store!
	f = in->_13; out->_13 = in->_31; if( sizeInBytes > 32 ) out->_31 = f;
	f = in->_23; out->_23 = in->_32; if( sizeInBytes > 32 ) out->_32 = f;
	f = in->_14; out->_14 = in->_41; if( sizeInBytes > 48 ) out->_41 = f;
	f = in->_24; out->_24 = in->_42; if( sizeInBytes > 48 ) out->_42 = f;
	if( sizeInBytes > 32 )
	{
		out->_33 = in->_33;
		f = in->_34; out->_34 = in->_43; if( sizeInBytes > 48 ) out->_43 = f;
		if( sizeInBytes > 48 )
		{
			out->_44 = in->_44;
		}
	}

	return out;
}


Matrix* TriMatrixRotationArc( Matrix* out, const Vector3* v1, const Vector3* v2	)
{
	// re-route it through quaternions
	Quaternion rotQuat;
	TriQuaternionRotationArc( &rotQuat, v1, v2 );
	*out = RotationMatrix( rotQuat );
	return out;
}


Matrix* TriMatrixArcFromForward(
	Matrix* out, 
	const Vector3* v 
	)
{
	Vector3 norm = Normalize( *v );
	*out = IdentityMatrix();
	if (norm.z < -0.99999f)
	{
		return out;
	}
	if (norm.z > 0.99999f)
	{
		out->_22 = -1.0f;
		out->_33 = -1.0f;
		return out;
	}
	const float h = (1.0f + norm.z)/(norm.x*norm.x + norm.y*norm.y);
	out->_11 = h*norm.y*norm.y - norm.z;
	out->_12 = -h*norm.x*norm.y;
	out->_13 = norm.x;

	out->_21 = out->_12;
	out->_22 = h*norm.x*norm.x - norm.z;
	out->_23 = norm.y;

	out->_31 = -norm.x;
	out->_32 = -norm.y;
	out->_33 = -norm.z;;
	//*out = tempMat;
	return out;
}

Matrix* TriMatrixTranslate(Matrix* out, const Matrix* m, const Vector3* v)
{
	*out = *m;
	out->_41 += v->x;
	out->_42 += v->y;
	out->_43 += v->z;
	return out;
}

Matrix* TriMatrixTranslate(Matrix* out, const Vector3* v, const Matrix* m)
{
	*out = *m;
	out->_41 += v->x*out->_11 +  v->y*out->_21  + v->z*out->_31;
	out->_42 += v->x*out->_12 +  v->y*out->_22  + v->z*out->_32;
	out->_43 += v->x*out->_13 +  v->y*out->_23  + v->z*out->_33;
	return out;
}

Matrix* TriMatrixRotate(Matrix* out, const Matrix* m, const Quaternion* q)
{
	//we might be able to use a niftier way to do this later
	Matrix tmpResult = RotationMatrix( *q );
	*out = *m * tmpResult;
	return out;
}

Matrix* TriMatrixRotate(Matrix* out, const Quaternion* q, const Matrix* m)
{
	//we might be able to use a niftier way to do this later
	Matrix tmpResult = RotationMatrix( *q );
	*out = tmpResult * *m;
	return out;
}

Matrix* TriMatrixChangeBase(Matrix* out, const Vector3* fwd, const Vector3* up)
{
	Vector3 cross = Cross( *up, *fwd );
	out->_11 = cross.x;
	out->_12 = cross.y;
	out->_13 = cross.z;
	out->_14 = 0.0f;

	out->_21 = up->x;
	out->_22 = up->y;
	out->_23 = up->z;
	out->_24 = 0.0f;

	out->_31 = fwd->x;
	out->_32 = fwd->y;
	out->_33 = fwd->z;
	out->_34 = 0.0f;


	out->_41 = 0.0f;
	out->_42 = 0.0f;
	out->_43 = 0.0f;
	out->_44 = 1.0f;
	return out;
}


// --------------------------------------------------------------------------------
// Description:
//   Removes any scaling from a matrix.
// Arguments:
//   out - return matrix without scaling
//   in - input matrix with scaling
// SeeAlso:
//   Matrix
// --------------------------------------------------------------------------------
Matrix* TriMatrixRemoveScaling( Matrix* out, const Matrix* in )
{
	if( out != in )
	{
		*out = *in;
	}
	out->GetX() = Normalize( out->GetX() );
	out->GetY() = Normalize( out->GetY() );
	out->GetZ() = Normalize( out->GetZ() );
	return out;
}

// --------------------------------------------------------------------------------
// Description:
//   Removes any translation from a matrix.
// Arguments:
//   out - return matrix without translation
//   in - input matrix with translation
// SeeAlso:
//   Matrix
// --------------------------------------------------------------------------------
Matrix* TriMatrixRemoveTranslation( Matrix* out, const Matrix* in )
{
	if( out != in )
	{
		*out = *in;
	}
	out->_41 = out->_42 = out->_43 = 0.f;
	return out;
}

// --------------------------------------------------------------------------------
// Description:
//   Overwrite any translation in a matrix.
// Arguments:
//   out - return matrix with new translation
//   in - input matrix with obsolete translation
//   t - input translation used to overwrite
// SeeAlso:
//   Matrix
// --------------------------------------------------------------------------------
Matrix* TriMatrixOverwriteTranslation( Matrix* out, const Matrix* in, const Vector3* t )
{
	if( out != in )
	{
		*out = *in;
	}
	out->_41 = t->x;
	out->_42 = t->y;
	out->_43 = t->z;
	return out;
}


/////////////////////////////////////////////////////////////////////////////////////////
// Double precision matrix functions
/////////////////////////////////////////////////////////////////////////////////////////
double* Matrix4dInvert( double* result, const double* mat )
{
	double tmp[12]; /* temp array for pairs */
	double src[16]; /* array of transpose source matrix */
	double det; /* determinant */
	/* transpose matrix */
	for (int i = 0; i < 4; i++) {
		src[i] = mat[i * 4];
		src[i + 4] = mat[i * 4 + 1];
		src[i + 8] = mat[i * 4 + 2];
		src[i + 12] = mat[i * 4 + 3];
	}
	/* calculate pairs for first 8 elements (cofactors) */
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	/* calculate first 8 elements (cofactors) */
	result[0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
	result[0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
	result[1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
	result[1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
	result[2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
	result[2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
	result[3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
	result[3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
	result[4] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
	result[4] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
	result[5] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
	result[5] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
	result[6] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
	result[6] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
	result[7] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
	result[7] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
	/* calculate pairs for second 8 elements (cofactors) */
	tmp[0] = src[2]*src[7];
	tmp[1] = src[3]*src[6];
	tmp[2] = src[1]*src[7];
	tmp[3] = src[3]*src[5];
	tmp[4] = src[1]*src[6];
	tmp[5] = src[2]*src[5];
	tmp[6] = src[0]*src[7];
	tmp[7] = src[3]*src[4];
	tmp[8] = src[0]*src[6];
	tmp[9] = src[2]*src[4];
	tmp[10] = src[0]*src[5];
	tmp[11] = src[1]*src[4];
	/* calculate second 8 elements (cofactors) */
	result[8] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
	result[8] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
	result[9] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
	result[9] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
	result[10] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
	result[10]-= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
	result[11] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
	result[11]-= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
	result[12] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
	result[12]-= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
	result[13] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
	result[13]-= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
	result[14] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
	result[14]-= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
	result[15] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
	result[15]-= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
	/* calculate determinant */
	det=src[0]*result[0]+src[1]*result[1]+src[2]*result[2]+src[3]*result[3];
	/* calculate matrix inverse */
	det = 1.0 /det;
	for (int j = 0; j < 16; j++)
	{
		result[j] *= det;
	}
	return result;
}


double* Matrix4dMultiply( double* result, double* m0, double* m1 )
{
	for( int r = 0; r < 4; r++ )
	{
		for( int c = 0; c < 4; c++ )
		{
			result[r*4 + c] = m0[r*4] * m1[c] + m0[r*4+1] * m1[c+4] + m0[r*4+2] * m1[c+8] + m0[r*4+3] * m1[c+12];
		}
	}
	return result;
}

double* Matrix4dCopy( double* result, double* mat )
{
	memcpy( result, mat, sizeof( double ) * 16 );
	return result;
}

Vector4d Matrix4dTransform( Vector4d point, double* mat )
{
	return Vector4d(
		point.x * mat[0] + point.y * mat[4] + point.z * mat[8] + point.w * mat[12],
		point.x * mat[1] + point.y * mat[5] + point.z * mat[9] + point.w * mat[13],
		point.x * mat[2] + point.y * mat[6] + point.z * mat[10] + point.w * mat[14],
		point.x * mat[3] + point.y * mat[7] + point.z * mat[11] + point.w * mat[15]
	);
}


Matrix Matrix4dToMatrix( double* mat )
{
	Matrix m;
	for( int r = 0; r < 4; r++ )
	{
		for( int c = 0; c < 4; c++ )
		{
			m.m[r][c] = (float)mat[r*4 + c];
		}
	}
	return m;
}


double* Matrix4dFromMatrix( double* result, const Matrix& mat )
{
	for( int r = 0; r < 4; r++ )
	{
		for( int c = 0; c < 4; c++ )
		{
			 result[r*4 + c] = mat.m[r][c];
		}
	}
	return result;
}



/////////////////////////////////////////////////////////////////////////////////////////
// Misc extensions
/////////////////////////////////////////////////////////////////////////////////////////
bool IsFinite( float value )
{
    return value == value && 
           value != std::numeric_limits<float>::infinity() &&
           value != -std::numeric_limits<float>::infinity();
}

bool IsFinite( const Vector3& vec )
{
	return IsFinite( vec.x ) && IsFinite( vec.y ) && IsFinite( vec.z );
}

float TriClamp(
	float f,
	float min,
	float max
	)
{
	return ( f < min ) ? min : ( ( f > max ) ? max : f );
}

int32_t ClampInt(
	int32_t f,
	int32_t min,
	int32_t max
	)
{
	return ( f < min ) ? min : ( ( f > max ) ? max : f );
}

uint32_t ClampUInt(
	uint32_t f,
	uint32_t min,
	uint32_t max )
{
	return ( f < min ) ? min : ( ( f > max ) ? max : f );
}

float Lerp(
	float min, 
	float max, 
	float s
	)
{
	return min + s * (max - min);
}

float Hermite(
	float v1,
	float r,
	float v2,
	float l,
	float t,
	float dt
	)
{
	// minimize number of math operations for max speed:
    float    _1_T = 1.0f /    dt;
    float   _1_T2 = _1_T *  _1_T;
    float      t2 =    t *     t;
    float    t2_T =   t2 *  _1_T;
    float   t3_T2 =   t2 *     t * _1_T2;
    float _2t3_T3 = 2.0f * t3_T2 *  _1_T;
    float _3t2_T2 = 3.0f *    t2 * _1_T2;

    return ( v1 * (   _2t3_T3 - _3t2_T2 + 1.0f     ) +
             v2 * ( - _2t3_T3 + _3t2_T2            ) +
              r * (     t3_T2 -    t2_T - t2_T + t ) +
              l * (     t3_T2 -    t2_T            ) );
}

float TriLinearize( float min, float max, float v )
{
	return TriClamp( ( v - min ) / ( max - min ), 0.f, 1.f );
}

float SinSmooth(
	float f
	)
{
	float ret = f*XM_PI - XM_PI/2.0f;
	return sinf(ret)/2.0f + 0.5f;
}

float CubicInterpolate( float pm1, float p0, float p1, float p2, float s )
{ 
	return p0 + 0.5f * s*(p1 - pm1 + s*(2.f*pm1 - 5.f*p0 + 4.f*p1 - p2 + s*(3.f*(p0 - p1) + p2 - pm1)));
}

/////////////////////////////////////////////////////////////////////////////////////////
// random functions
/////////////////////////////////////////////////////////////////////////////////////////
uint32_t jran = 1234;
void TriSrand( Be::Time seed )
{
	jran = uint32_t(seed) % 714025;
};

void TriSrand( uint32_t seed )
{
	jran = seed % 714025;
};

float TriRand()
{
	jran <<= 12;
	jran += 150889;
	jran %= 714025;
	return float(jran)/714025.0f;
};

int TriRandInt(int hi)
{
	jran <<= 12;
	jran += 150889;
	jran %= 714025;
	return (hi*jran)/714025;
};

int TriRandInt(int lo, int hi)
{
	jran <<= 12;
	jran += 150889;
	jran %= 714025;
	return ((hi - lo)*jran)/714025 + lo;
};

int TriRandGetSeed()
{
	return jran;
};


namespace
{
[[nodiscard]] double SCurve( double t )
{
	return t * t * ( 3.0 - 2.0 * t );
}
}


TriPerlinNoise::TriPerlinNoise( uint32_t seed )
{
	std::mt19937 randGen( seed );
	std::generate( m_gradients.begin(), m_gradients.end(), [&randGen]() {
		return double( int32_t( randGen() % ( TABLE_SIZE + TABLE_SIZE ) ) - TABLE_SIZE ) / double( TABLE_SIZE );
	} );
}

TriPerlinNoise::TriPerlinNoise() :
	TriPerlinNoise( std::random_device()() )
{
}

double TriPerlinNoise::operator()( double x ) const
{
	int32_t b0 = int32_t( std::floor( x ) ) & TABLE_MASK;
	int32_t b1 = ( b0 + 1 ) & TABLE_MASK;

	double rx0 = x - std::floor( x );
	double rx1 = rx0 - 1.0;

	double v0 = rx0 * m_gradients[b0];
	double v1 = rx1 * m_gradients[b1];

	return v0 + SCurve( rx0 ) * ( v1 - v0 );
}

double TriPerlinNoise::FractalSum( double x, size_t octaves, double amplitudeScale, double frequencyScale ) const
{
	double sum = 0.0;
	double scale = 1.0;

	for( size_t i = 0; i < octaves; ++i )
	{
		double val = operator()( x );
		sum += val * scale;
		scale *= amplitudeScale;
		x *= frequencyScale;
	}
	return sum;
}

double PerlinNoise1D( double x, double invAmplitude, double frequency, int octaves )
{
	static TriPerlinNoise noise{ 0 }; // fixed seed: reproducible across runs (matches legacy)
	return noise.FractalSum( x, size_t( std::max( 0, octaves ) ), 1.0 / invAmplitude, frequency );
}


bool ConvertProjectionCoordToWorldPickRay( float x, float y, const Matrix* projMat, const Matrix* viewMat, Vector3* rayStart, Vector3* rayDir )
{
	Matrix projection2view;
	Matrix view2world;

	if( !Inverse( projection2view, *projMat ) )
	{
		return false;
	}

	if( !Inverse( view2world, *viewMat ) )
	{
		return false;
	}

	// Initialize 'rayStart' with projection coordinates and transform it to world
	rayStart->x = x;
	rayStart->y = y;
	rayStart->z = 0.0f;

	// Note that this method does _unprojection_ i.e. divides by w
	*rayStart = TransformCoord( *rayStart, projection2view );
	*rayStart = TransformCoord( *rayStart, view2world );

	// Initialize 'rayEnd' with projection coordinates and put it halfway into projection space
	Vector3 rayEnd( x, y, 0.5f );
	rayEnd = TransformCoord( rayEnd, projection2view );
	rayEnd = TransformCoord( rayEnd, view2world );

	*rayDir = Normalize( rayEnd - *rayStart );

	return true;
}

