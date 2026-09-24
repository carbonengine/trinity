// Copyright © 2026 CCP ehf.

#ifndef CARBON_MATH_FXH
#define CARBON_MATH_FXH

static float PI = 3.1415926535897932384626433832795;
static float _2PI = 6.283185307179586476925286766559;
static const float4x4 IDENTITY_MATRIX = float4x4(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);


// ----------------------------------------------------------------------------------------------
// extract the eye-pos in world-space from the view matrix
// ----------------------------------------------------------------------------------------------
float3 ExtractEyePosFromViewMatrix(float4x4 mat)
{
    return float3(mat[0].w, mat[1].w, mat[2].w);
}

// ----------------------------------------------------------------------------------------------
// extract the eye-dir in world-space from the view matrix
// ----------------------------------------------------------------------------------------------
float3 ExtractEyeDirFromViewMatrix(float4x4 mat)
{
    return float3(mat[0].z, mat[1].z, mat[2].z);
}

// ----------------------------------------------------------------------------------------------
// extract z_near from projection matrix
// ----------------------------------------------------------------------------------------------
float ExtractZnearFromProjectionMatrix(float4x4 mat)
{
    return mat[3][2] / mat[2][2];
}

// ----------------------------------------------------------------------------------------------
// extract z_far from projection matrix
// ----------------------------------------------------------------------------------------------
float ExtractZfarFromProjectionMatrix(float4x4 mat)
{
    return mat[3][2] / (mat[2][2] + 1.0);
}

// ----------------------------------------------------------------------------------------------
// extract FovY from projection matrix
// ----------------------------------------------------------------------------------------------
float ExtractFovYFromProjectionMatrix(float4x4 mat)
{
    return 2.0 * atan(1.0 / mat[1][1]);
}

// ----------------------------------------------------------------------------------------------
// compute a simple rotation matrix from a given vector and angle
// ----------------------------------------------------------------------------------------------
float3x3 GetRotationMatrixAxisAngle(float3 axis, float angle)
{
    float3x3 m;
	
	// pre
    float oneMinusCosA = 1.0 - cos(angle);
    float sinA = sin(angle);
	
	// matrix
    m[0][0] = 1.0 + oneMinusCosA * (axis.x * axis.x - 1.0);
    m[1][0] = -axis.z * sinA + oneMinusCosA * axis.x * axis.y;
    m[2][0] = axis.y * sinA + oneMinusCosA * axis.x * axis.z;
	
    m[0][1] = axis.z * sinA + oneMinusCosA * axis.x * axis.y;
    m[1][1] = 1.0 + oneMinusCosA * (axis.y * axis.y - 1.0);
    m[2][1] = -axis.x * sinA + oneMinusCosA * axis.y * axis.z;
	
    m[0][2] = -axis.y * sinA + oneMinusCosA * axis.x * axis.z;
    m[1][2] = axis.x * sinA + oneMinusCosA * axis.y * axis.z;
    m[2][2] = 1.0 + oneMinusCosA * (axis.z * axis.z - 1.0);
	
    return m;
}

// ----------------------------------------------------------------------------------------------
// compute a rotation matrix which rotates axis1 into axis2
// ----------------------------------------------------------------------------------------------
float3x3 GetRotationMatrixNrmToNrm(float3 normal1, float3 normal2)
{
    float3 rotAxis = normalize(cross(normal1, normal2));
    float rotAngle = acos(dot(normal1, normal2));
    return GetRotationMatrixAxisAngle(rotAxis, rotAngle);
}

// ----------------------------------------------------------------------------------------------
// Create a transformation matrix from a quaternion of the form x, y, z, w
// ----------------------------------------------------------------------------------------------
float4x4 CreateRotationMatrixFromQuaterion(float4 q)
{
    float tx2 = 2 * q.x * q.x;
    float ty2 = 2 * q.y * q.y;
    float tz2 = 2 * q.z * q.z;

    float txy = 2 * q.x * q.y;
    float txz = 2 * q.x * q.z;
    float txw = 2 * q.x * q.w;
    float tyz = 2 * q.y * q.z;
    float tyw = 2 * q.y * q.w;
    float tzw = 2 * q.z * q.w;

    float4x4 m3 =
    {
        { 1 - ty2 - tz2, txy + tzw, txz - tyw, 0 },
        { txy - tzw, 1 - tx2 - tz2, tyz + txw, 0 },
        { txz + tyw, tyz - txw, 1 - tx2 - ty2, 0 },
        { 0, 0, 0, 1 }
    };
    return m3;
}

// ----------------------------------------------------------------------------------------------
// combine a 3x3 rotation matrix and a float3 position to a 4x4 transform
// ----------------------------------------------------------------------------------------------
float4x4 CreateTransformMatrix(float3x3 rotationMatrix, float3 translation)
{
    float4x4 m;
	
    m[0] = float4(rotationMatrix[0], 0.0);
    m[1] = float4(rotationMatrix[1], 0.0);
    m[2] = float4(rotationMatrix[2], 0.0);
    m[3] = float4(translation, 1.0);
	
    return m;
}

// ----------------------------------------------------------------------------------------------
// combine a rotation quaternion and a float4 position to a 4x4 transform
// ----------------------------------------------------------------------------------------------
float4x4 CreateTransformMatrix(float4 rotationQuaternion, float4 translation)
{
    float4x4 m = CreateRotationMatrixFromQuaterion(rotationQuaternion);
    m[3] = translation;
	
    return m;
}

// ----------------------------------------------------------------------------------------------
// extract translation from transform matrix
// ----------------------------------------------------------------------------------------------
float3 ExtractTranslationFromTransformMatrix(float4x4 mat)
{
    return mat[3].xyz;
}

// ----------------------------------------------------------------------------------------------
// extract rotation from transform matrix
// ----------------------------------------------------------------------------------------------
float3x3 ExtractRotationFromTransformMatrix(float4x4 mat)
{
    float3x3 m;

    m[0] = mat[0].xyz;
    m[1] = mat[1].xyz;
    m[2] = mat[2].xyz;
    return m;
}

// ----------------------------------------------------------------------------------------------
// remove translation from a transform matrix
// ----------------------------------------------------------------------------------------------
float4x4 RemoveTranslationFromMatrix(float4x4 transformMatrix)
{
    float4x4 m = transformMatrix;
    m[3] = float4(0.0, 0.0, 0.0, 1.0);
	
    return m;
}

// ----------------------------------------------------------------------------------------------
// create an identity matrix
// ----------------------------------------------------------------------------------------------
float4x4 CreateIdentityMatrix()
{
    float4x4 m;
	
    m[0] = float4(1.0, 0.0, 0.0, 0.0);
    m[1] = float4(0.0, 1.0, 0.0, 0.0);
    m[2] = float4(0.0, 0.0, 1.0, 0.0);
    m[3] = float4(0.0, 0.0, 0.0, 1.0);
	
    return m;
}

// ----------------------------------------------------------------------------------------------
// create a non-uniform scaling matrix
// ----------------------------------------------------------------------------------------------
float4x4 CreateScalingMatrix(float3 s)
{
    float4x4 m;
	
    m[0] = float4(s.x, 0.0, 0.0, 0.0);
    m[1] = float4(0.0, s.y, 0.0, 0.0);
    m[2] = float4(0.0, 0.0, s.z, 0.0);
    m[3] = float4(0.0, 0.0, 0.0, 1.0);
	
    return m;
}

// ----------------------------------------------------------------------------------------------
// compute a perpendicular vector to a given base vector: is any vector, only perpendicular
// ----------------------------------------------------------------------------------------------
float3 GetVectorPerpendicular(float3 basis)
{
	// somehow this simple problem of constructing ANY perpendicular to a given vector
	// is unsolvable.... at least in a nice, non-hacky way
    float3 v = basis.yzx;
    return normalize(cross(v, basis));
}

// ----------------------------------------------------------------------------------------------
// "re-linearize" v from min to max
// ----------------------------------------------------------------------------------------------
float LinStep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0.0, 1.0);
}

// ----------------------------------------------------------------------------------------------
// remap a value from input range to an output range 
// ----------------------------------------------------------------------------------------------
float RemapFloatRange(float x, float inMin, float inMax, float outMin = 0.0f, float outMax = 1.0f)
{
    return outMin + ((x - inMin) / (inMax - inMin)) * (outMax - outMin);
}

// ----------------------------------------------------------------------------------------------
// Hermite spline position
// ----------------------------------------------------------------------------------------------
float3 HermiteSplinePos(float3 v1, float3 t1, float3 v2, float3 t2, float s)
{
	// spline parameters
    float3 a = 2.0 * v1 - 2.0 * v2 + t2 + t1;
    float3 b = 3.0 * v2 - 3.0 * v1 - 2.0 * t1 - t2;
    float3 c = t1;
    float3 d = v1;
	// cubic equation
    return a * s * s * s + b * s * s + c * s + d;
}

// ----------------------------------------------------------------------------------------------
// Hermite spline direction (derive!)
// ----------------------------------------------------------------------------------------------
float3 HermiteSplineDir(float3 v1, float3 t1, float3 v2, float3 t2, float s)
{
	// spline parameters
    float3 a = 2.0 * v1 - 2.0 * v2 + t2 + t1;
    float3 b = 3.0 * v2 - 3.0 * v1 - 2.0 * t1 - t2;
    float3 c = t1;
	// cubic equation becomes 2-dim cause of derive
    return 3.0 * a * s * s + 2.0 * b * s + c;
}

// ----------------------------------------------------------------------------------------------
// Convert cartesian direction vector to spherical
// ----------------------------------------------------------------------------------------------
float2 CartesianToSpherical(float3 dir)
{
	// x = sin( theta ) * sin( phi )
	// y = cos( phi )
	// z = cos( theta ) * sin( phi )
    float phi = acos(dir.y);
    float theta = PI + atan2(dir.z, dir.x);
    return float2(theta, phi);
}

// ----------------------------------------------------------------------------------------------
// Convert spherical coords into cartesian vector
// ----------------------------------------------------------------------------------------------
float3 SphericalToCartesian(float2 spherical)
{
    float z = -sin(spherical.x) * sin(spherical.y);
    float y = cos(spherical.y);
    float x = -cos(spherical.x) * sin(spherical.y);
    return float3(x, y, z);
}


float4 Vec4Cross(float4 v1, float4 v2, float4 v3)
{
    float4 result;
    result.x = v1.y * (v2.z * v3.w - v3.z * v2.w) - v1.z * (v2.y * v3.w - v3.y * v2.w) + v1.w * (v2.y * v3.z - v2.z * v3.y);
    result.y = -(v1.x * (v2.z * v3.w - v3.z * v2.w) - v1.z * (v2.x * v3.w - v3.x * v2.w) + v1.w * (v2.x * v3.z - v3.x * v2.z));
    result.z = v1.x * (v2.y * v3.w - v3.y * v2.w) - v1.y * (v2.x * v3.w - v3.x * v2.w) + v1.w * (v2.x * v3.y - v3.x * v2.y);
    result.w = -(v1.x * (v2.y * v3.z - v3.y * v2.z) - v1.y * (v2.x * v3.z - v3.x * v2.z) + v1.z * (v2.x * v3.y - v3.x * v2.y));
    return result;
}

float4x4 MatrixInverse(float4x4 m)
{
    float4 v, vec[3];
    float d;

    d = determinant(m);
    if (!d)
    {
        return m;
    }
    float4x4 result;
	[unroll]
    for (int i = 0; i < 4; i++)
    {
        float signedDet = (i % 2 == 1) ? -1.f : 1.f;
        signedDet /= d;
        for (int j = 0; j < 4; j++)
        {
            if (j != i)
            {
                int a = j;
                if (j > i)
                {
                    a = a - 1;
                }
                vec[a].x = m[j][0];
                vec[a].y = m[j][1];
                vec[a].z = m[j][2];
                vec[a].w = m[j][3];
            }
        }
        v = Vec4Cross(vec[0], vec[1], vec[2]);
        result[0][i] = signedDet * v.x;
        result[1][i] = signedDet * v.y;
        result[2][i] = signedDet * v.z;
        result[3][i] = signedDet * v.w;
    }
    return result;
}

#endif