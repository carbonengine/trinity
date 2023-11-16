// This is the refactored TriViewport class - it does not expose any D3D internals and has 
// been changed so that 
//
//  1. it is compatible with D3D10 and OpenGL
//  2. it uses Blue 2.0 exposure
//  3. it doesn't implement interfaces that nobody uses
//  4. it can be used as a window (negative coords, this is required by D3D10, OpenGL and
//     our new RenderJob architecture).
//
#pragma once
#ifndef TriViewport_H
#define TriViewport_H

BLUE_DECLARE( TriViewport );

class TriViewport : public IRoot
{
public:
	// These are public for a reason - there is no need to wrap them in accessors.
	// This is also why they don't have the 'm_' prefix.
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	float minZ;
	float maxZ;

	TriViewport(IRoot* lockobj = NULL) : x(0),y(0),width(1),height(1),minZ(0.0f),maxZ(1.0f)
	{}
	virtual ~TriViewport(){}

	void py__init__( 
		int32_t _x,
		int32_t _y,
		Be::OptionalWithDefaultValue<int32_t, 1> _width,
		Be::OptionalWithDefaultValue<int32_t, 1> _height,
		float _minZ,
		Be::Optional<float> _maxZ )
	{
		x = _x;
		y = _y;
		width = _width;
		height = _height;
		minZ = _minZ;
		if( _maxZ.IsAssigned() )
		{
			maxZ = _maxZ;
		}
		else
		{
			maxZ = 1.0f;
		}
	}
	
	float GetAspectRatio() { return float(width)/height; }

	void ConvertToTr2Viewport( Tr2Viewport& viewport ) const
	{
		viewport.m_x = float( x );
		viewport.m_y = float( y );
		viewport.m_width = float( width );
		viewport.m_height = float( height );
		viewport.m_minZ = minZ;
		viewport.m_maxZ = maxZ;
	}

	EXPOSE_TO_BLUE();
};

TYPEDEF_BLUECLASS(TriViewport);

inline void Vec3TransformByViewport( Vector3& vec, const TriViewport& viewport )
{
	vec.x = viewport.x + ( 1.0f + vec.x ) * 0.5f * viewport.width;
	vec.y = viewport.y + ( 1.0f - vec.y ) * 0.5f * viewport.height;
	vec.z = viewport.minZ + vec.z * ( viewport.maxZ - viewport.minZ );
}

#endif

