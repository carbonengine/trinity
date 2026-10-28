// Copyright © 2026 CCP ehf.

#ifndef CARBON_PACKEDTANGENTS_FXH
#define CARBON_PACKEDTANGENTS_FXH


struct TangentSpace 
{
	float3 tangent;
	float3 bitangent;
	float3 normal;
};

// Support for fma insrinsic varies for dx11
float _Fma( float a, float b, float c )
{
    return a * b + c;
}

// Unpacks a tangent space using quaternion representation corresponding to
// cmf::Usage::PackedTangent in carbon-mesh.
TangentSpace UnpackTangentSpace( float4 t ) 
{
	// Heavily optimized shader code that constructs the TBN matrix.

	// Extract the xyz components and square them
	float x = t.x;
	float y = t.y;
	float z = t.z;
	float x2 = x * x;
	float y2 = y * y;
	float z2 = z * z;

	// Optimized fma() chain to reconstruct W = sqrt(1 - x2 - y2 - z2)
	// Don't use the above x2, y2 and z2 values, to reduce pipeline dependencies.
	float w2 = saturate( _Fma( z, -z, _Fma( y, -y, _Fma( x, -x, 1.0f ) ) ) );
	float w = sqrt( w2 );

	// Calculate shared values.
	// These multiplications by 2.0f are free on some GPUs.
	float xy = x * y * 2.0f;
	float xz = x * z * 2.0f;
	float yz = y * z * 2.0f;
	float xw = x * w * 2.0f;
	float yw = y * w * 2.0f;
	float zw = z * w * 2.0f;

	// Compute the three vectors. 
	TangentSpace space;

	space.tangent = float3( _Fma( -2.0f, y2, _Fma( -2.0f, z2, 1.0f ) ), + xy + zw, + xz - yw );
	space.bitangent = float3( - zw + xy, _Fma( -2.0f, x2, _Fma( -2.0f, z2, 1.0f ) ), + yz + xw );
	space.normal = float3( + yw + xz, + yz - xw, _Fma( -2.0f, x2, _Fma( -2.0f, y2, 1.0f ) ) ) * t.w; // packed normal sign multiplication
	
	return space;
}


// Unpacks a tangent space using angle representation corresponding to
// cmf::Usage::PackedTangentLegacy in carbon-mesh.
TangentSpace UnpackTangentSpaceLegacy( float4 tangents )
{
    float pi = 3.1415926535897932384626433832795;

	float4 angles = tangents * 2.0 * pi - pi;
	float4 sc0, sc1;
    sincos( angles.x, sc0.x, sc0.y );
    sincos( angles.y, sc0.z, sc0.w );
    sincos( angles.z, sc1.x, sc1.y );
    sincos( angles.w, sc1.z, sc1.w );
	TangentSpace space;
	space.tangent = float3( sc0.y * abs( sc0.z ), sc0.x * abs( sc0.z ), sc0.w );
	space.bitangent = float3( sc1.y * abs( sc1.z ), sc1.x * abs( sc1.z ), sc1.w );
	space.normal = -cross( space.tangent, space.bitangent );
	space.normal = all( angles.yw > 0.0 ) ? -space.normal : space.normal;
	return space;
}



#endif