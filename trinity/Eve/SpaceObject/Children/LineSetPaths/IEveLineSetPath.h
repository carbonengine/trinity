#pragma once

BLUE_INTERFACE( IEveLineSetPath ) :
	public IRoot
{
public:
	virtual bool Update( EveUpdateContext& updateContext, const EveChildUpdateParams& params ) = 0;
	virtual void UpdateBuffer( Tr2RenderContext & renderContext, uint8_t*& data, const Matrix& systemLocation, const unsigned stride ) = 0;

	virtual void GeneratePoints( const Matrix& parentTransform = IdentityMatrix() ) = 0;
	virtual void GetPointCount( unsigned& count) = 0;
	virtual void AddLinesToSet( EveCurveLineSet & lineSet, const Vector4& color, const Vector4& animColor, float scrollSpeed ) = 0;

	virtual void CalculateBoundingSphere( float meshSize = 0.0, bool reCalculateChildren = true ) = 0;
	virtual void GetBoundingSphere( Vector4& sphere ) = 0;
	virtual void UpdateVisibility( const TriFrustum& frustum, Tr2Lod parentLod, const Matrix& systemLocation ) = 0;

	// Debug render
    virtual void GetDebugOptions( Tr2DebugRendererOptions& options ) = 0;
    virtual void RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& systemLocation ) = 0;
};

BLUE_DECLARE_INTERFACE( IEveLineSetPath );
BLUE_DECLARE_IVECTOR( IEveLineSetPath );

inline void CalculateBoundingSphereForLineSetPaths( Vector4& boundingSphere, PIEveLineSetPathVector& lines, bool reCalculateChildren, float meshSize )
{
	if( lines.empty() )
	{
		return;
	}

	if( reCalculateChildren )
	{
		for( auto it = begin( lines ); it != end( lines ); ++it )
		{
			( *it )->CalculateBoundingSphere( meshSize, reCalculateChildren );
		}
	}

	Vector4 sphere;
	Vector3 sum( 0.f, 0.f, 0.f );
	float biggestRad = 0;
	float distSq = 0;

	// This creates a bounding sphere that is slightly to large but there are subLevel culls
	// withing the child classes Thus this one just serves as a preliminary elimination and
	// thus no need for Welzl's / Ritter's

	for( auto it = begin( lines ); it != end( lines ); ++it )
	{
		(*it)->GetBoundingSphere( sphere );
		sum += sphere.GetXYZ();
		biggestRad = max( biggestRad, sphere.w );
	}

	sum /= float( lines.size() );

	for( auto it = begin( lines ); it != end( lines ); ++it )
	{
		(*it)->GetBoundingSphere( sphere );
		distSq = max( distSq, LengthSq( sphere.GetXYZ() - sum ) );
	}

	boundingSphere = Vector4( sum, sqrt( distSq ) + biggestRad );
}
