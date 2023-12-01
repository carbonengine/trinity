#ifndef ITr2DebugRenderer_H
#define ITr2DebugRenderer_H

#include "Rect.h"

BLUE_INTERFACE( ITr2DebugRenderer ) : IRoot
{
	virtual void DrawPointCloud( int numPoints, float* points, int stride ) = 0;

	// Line rendering
	virtual void DrawLine( const Vector3& from, const Vector3& to, uint32_t color = 0xffffffff ) = 0;
	virtual void DrawLine( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor ) = 0;
	virtual void DrawSphere( const Vector3& center, float radius, int segments, uint32_t color = 0xffffffff ) = 0;
	virtual void DrawBox( const Vector3& min, const Vector3& max, uint32_t color = 0xffffffff ) = 0;
	virtual void DrawCapsule( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff ) = 0;
	virtual void DrawCylinder( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff ) = 0;
	virtual void DrawCone( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff ) = 0;
	virtual void DrawPlane( const Vector4& planeEquation, int segments, uint32_t color = 0xffffffff ) = 0;
	
	// Draw axis lines, red for x, green for y, blue for z for the coordinate system defined by the given
	// transform. The axes are rendered as one unit, optionally scaled by the given scale factor on top
	// of any scaling in the transform.
	virtual void DrawAxes( const Matrix& m, float scale = 1.0f ) = 0;

	// Text output
	virtual void Printf( int x, int y, uint32_t color, const char* msg, ... ) = 0;
	virtual void Printf( const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... ) = 0;
	virtual void Printf( const Vector3& pos, uint32_t color, const char* msg, ... ) = 0;
};

#endif
