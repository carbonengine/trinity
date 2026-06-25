// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2ManipulationTool_h
#define Tr2ManipulationTool_h

BLUE_DECLARE( Tr2PrimitiveSet );
BLUE_DECLARE_VECTOR( Tr2PrimitiveSet );
BLUE_DECLARE_INTERFACE( ITr2Renderable );

class TriViewport;

extern const Vector4 v4Red;
extern const Vector4 v4Green;
extern const Vector4 v4Blue;
extern const Vector4 v4Cyan;
extern const Vector4 v4Yellow;
extern const Vector4 v4LightGray;
extern const Vector3 v3Zero;
extern const Vector3 v3X;
extern const Vector3 v3Y;
extern const Vector3 v3Z;

BLUE_CLASS( Tr2ManipulationTool ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	Tr2ManipulationTool( IRoot* lockobj = NULL );
	~Tr2ManipulationTool();

	// typedef to shorten the iterator declaration
	typedef Tr2PrimitiveSetVector::const_iterator PrimitiveIterator;
	PTr2PrimitiveSetVector m_primitives;

	// The actual interface all the tools must impliment
	virtual void Update() = 0;
	virtual void GenLineSets() = 0;
	virtual void ResetPrimitiveColors() = 0;
	virtual std::vector<ITr2Renderable*>& GetPrimitivesToRender() = 0;
	virtual void Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix ) = 0;

	// Callback for when the gizmo is being moved ( currentValue, nextValue )
	BlueScriptCallback m_callback;
	bool OnMoveCallback( Matrix & currentTransform, Matrix & nextTransform );

	// An optional pivot that controls the center point
	Vector3 m_pivot;
	// The transforms
	Matrix m_localTransform;
	Matrix m_worldTransform;

	// Intermediate output
	Vector3 m_movement;
	bool m_moved;

#if BLUE_WITH_PYTHON
	// for latching on any python objects that might be of use to the programmers
	PyObject* m_pythonUserData;
#endif

	// What is selected
	std::string m_selectedAxis;
	void SelectAxis( std::string axisName );

	// Captured
	bool m_captured;
	// The vector of primitives to actually render
	std::vector<ITr2Renderable*> m_visibleObjects;
	virtual void Init( Matrix & initialTransform )
	{
		m_localTransform = initialTransform;
	}
	// Utility functions
	virtual Vector3 GetDesiredPlaneNormal( Vector3 & ray, Matrix & viewMatrix );
	void GetBaseVectors( Vector3 & x, Vector3 & y, Vector3 & z );
	Vector3 MovePointOnPlane( int mx1, int my1, int mx2, int my2, Vector3& pointOnPlane, Vector3& planeNormal, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix );
	int RayToSphereIntersection( Vector3 & spCenter, float radius, Vector3& startPoint, Vector3& ray, Vector3& outValues );
	Vector3 RayToPlaneIntersection( Vector3 & P, Vector3 & d, Vector3 & Q, Vector3 & n );
	void ScreenCoordinatesToRay( int x, int y, Vector3& outRay, Vector3& outStart, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix );
	void PointToScreenCoordinates( Vector3 & inPoint, Vector2 & outPoint, Tr2Viewport & viewport, Matrix & viewMatrix, Matrix & projectionMatrix );

	static Vector3* GetTrianglesAroundLine( const Vector3& start, const Vector3& end, float radius, int* numVectors );
	static Vector3* GetConeTriangles( float height, float width, int subd, int* numVectors );
	static Vector3* GetBoxTriangles( const Vector3& min, const Vector3& max, int* numVectors );
	static Vector3* GetCircleTriangles( float radius, int subd, int* numVectors );
	static Vector3* GetCirclePoints( float radius, int subd, int* numVectors );

	void PyMove(
		int x,
		int y,
		int dx,
		int dy,
		TriViewport* viewport,
		Matrix view,
		Matrix proj );

	void SetMoveCallback( const BlueScriptCallback& callback );
};

TYPEDEF_BLUECLASS( Tr2ManipulationTool );
#endif //Tr2ManipulationTool_h
