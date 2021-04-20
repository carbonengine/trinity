#pragma once

#include "Eve/SpaceObject/Children/EveChildLineSet.h"
#include "IEveLineSetPath.h"

BLUE_CLASS( EveBezierCurve ) :
	public IEveLineSetPath,
	public EveChildTransform,
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();
	EveBezierCurve( IRoot* lockobj = nullptr );
	~EveBezierCurve();

	virtual bool Update( EveUpdateContext & updateContext, const EveChildUpdateParams& params );
	virtual void UpdateBuffer( Tr2RenderContext & renderContext, uint8_t*& data, const unsigned stride );

	virtual void GeneratePoints( const Matrix& parentTransform = IdentityMatrix() );
	virtual void GetPointCount( unsigned& count );
	virtual void AddLinesToSet( EveCurveLineSet & lineSet, const Vector4& color, const Vector4& animColor, float scrollSpeed );
	
	virtual void CalculateBoundingSphere( float meshSize = 0.0, bool reCalculateChildren = true );
	virtual void GetBoundingSphere( Vector4 & sphere );
	virtual void UpdateVisibility( const TriFrustum& frustum, Tr2Lod parentLod, const Matrix& systemLocation );

	// IInitialize
	bool Initialize() override;
	
	// INotify
	bool OnModified( Be::Var * value ) override;
	
	// Debug renderable
	virtual void GetDebugOptions( Tr2DebugRendererOptions & options );
	virtual void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& systemLocation );

private:
	BlueSharedString m_name;
	std::vector<Vector3> m_points;
	Vector4 m_boundingSphere;
	Matrix m_parentTransform;
	
	Vector3 m_point1;
	Vector3 m_point2;
	Vector3 m_bezierPoint;
	Vector3 m_objectScale;
	
	float m_completeness;
	float m_segments;
	float m_lineWidth;
	float m_segmentOffset;
	float m_movementSpeed;
	float m_animValue;
	float m_meshSize;

	bool m_display;
	bool m_isVisible;
	bool m_scaleEndpoints;
	bool m_scaleSegmentsByCompleteness;
	bool m_billboardObjects;
	bool m_regeneratePoints;
};

TYPEDEF_BLUECLASS( EveBezierCurve );
