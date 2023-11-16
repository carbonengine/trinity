#pragma once
#ifndef Tr2GpuLight_h_
#define Tr2GpuLight_h_

struct Tr2GpuLight
{
	Vector4		xyzr;
	Vector4		dirxyzL;
	Vector4		rgbX;
};

inline void MakePointLight(	const Vector3 xyz, 
							const float radius,
							const Vector3 rgb,
							Tr2GpuLight& out )
{
	out.xyzr = Vector4( xyz.x, xyz.y, xyz.z, radius );
	out.dirxyzL = Vector4( 0,0,0,0 );
	out.rgbX = Vector4( rgb.x, rgb.y, rgb.z, 0 );
}

inline void MakeLineSegment(const Vector3 from, 
							const Vector3 to,
							const float radius,
							const Vector3 rgb,
							Tr2GpuLight& out )
{
	out.xyzr = Vector4( from.x, from.y, from.z, radius );
	Vector3 dir = to - from;
	float L = sqrtf( dir.x * dir.x + dir.y * dir.y + dir.z * dir.z );
	if( L > 0.001f )
	{
		dir /= L;
		out.dirxyzL = Vector4( dir.x, dir.y, dir.z, L );
	}
	else
	{
		out.dirxyzL = Vector4( 0,0,0,0 );
	}
	out.rgbX = Vector4( rgb.x, rgb.y, rgb.z, 0 );
}

#endif // Tr2GpuLight_h_
