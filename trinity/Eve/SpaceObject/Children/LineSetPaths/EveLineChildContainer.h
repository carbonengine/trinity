#pragma once

#include "Eve/SpaceObject/Children/EveChildLineSet.h"
#include "IEveLineSetPath.h"

BLUE_DECLARE_INTERFACE( IEveLineSetPath );
BLUE_DECLARE_IVECTOR( IEveLineSetPath );

BLUE_CLASS( EveLineChildContainer ) :
	public IEveLineSetPath,
	public EveChildTransform,
	public INotify,
	public IListNotify
{
public:
	EXPOSE_TO_BLUE();
	EveLineChildContainer( IRoot* lockobj = nullptr );
	~EveLineChildContainer();
	
	bool Update( EveUpdateContext & updateContext, const EveChildUpdateParams& params ) override;
	void UpdateBuffer( Tr2RenderContext & renderContext, uint8_t * &data, const Matrix& systemLocation, const unsigned stride ) override;

	void GeneratePoints( const Matrix& parentTransform = IdentityMatrix() ) override;
	void GetPointCount( unsigned& count ) override;
	void AddLinesToSet( EveCurveLineSet & lineSet, const Vector4& color, const Vector4& animColor, float scrollSpeed ) override;

	void CalculateBoundingSphere( float meshSize = 0.0, bool reCalculateChildren = true ) override;
	void GetBoundingSphere( Vector4 & sphere ) override;
	void UpdateVisibility( const TriFrustum& frustum, Tr2Lod parentLod, const Matrix& systemLocation ) override;

	// INotify
	bool OnModified( Be::Var * value ) override;

	// IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;
	
	// Debug renderable
	void GetDebugOptions( Tr2DebugRendererOptions & options ) override;
	void RenderDebugInfo( ITr2DebugRenderer2 & renderer, const Matrix& parentWorldLocation ) override;

private:
	BlueSharedString m_name;

	PIEveLineSetPathVector m_lines;
	Vector4 m_boundingSphere;
	Matrix m_parentTransform;

	bool m_regenerate;
	bool m_isVisible;
	bool m_display;

	float m_meshSize;
};

TYPEDEF_BLUECLASS( EveLineChildContainer );