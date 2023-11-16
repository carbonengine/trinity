#pragma once

#ifndef Tr2Sprite2dPolygon_h
#define Tr2Sprite2dPolygon_h


#include "Tr2SpriteObject.h"
#include "ITr2Sprite2dTexture.h"

BLUE_DECLARE( Tr2Sprite2dPolygon );
BLUE_DECLARE( Tr2Sprite2dVertex );
BLUE_DECLARE_VECTOR( Tr2Sprite2dVertex );
BLUE_DECLARE( Tr2Sprite2dTriangle );
BLUE_DECLARE_VECTOR( Tr2Sprite2dTriangle );
BLUE_DECLARE_INTERFACE( ITr2Sprite2dTexture );
BLUE_DECLARE( Tr2AtlasTexture );

// Tr2Sprite2dVertex is used for rendering Tr2Sprite2dPolygon objects.
BLUE_CLASS( Tr2Sprite2dVertex ) :
	public IRoot,
	public Tr2Sprite2dVertexBase
{
public:
	EXPOSE_TO_BLUE();
	void SetTexCoord( unsigned i, Vector2 uv );
	Vector2 GetTexCoord( unsigned i ) const;
	void SetColor( Color c );
	Color GetColor() const;
	Tr2Sprite2dVertex( IRoot* lockobj = NULL );
};

TYPEDEF_BLUECLASS( Tr2Sprite2dVertex );

// Tr2Sprite2dTriangle is used for rendering Tr2Sprite2dPolygon objects.
// The triangle contains indices into the vertex list of a Tr2Sprite2dPolygon
// instance, one for each of the three vertices.
BLUE_CLASS( Tr2Sprite2dTriangle ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dTriangle( IRoot* lockobj = NULL );

	uint16_t m_index[3];
};

TYPEDEF_BLUECLASS( Tr2Sprite2dTriangle );

class Tr2Sprite2dPolygon:
     public Tr2TexturedSpriteObject,
	 public IListNotify
{
public:
    EXPOSE_TO_BLUE();
    Tr2Sprite2dPolygon( IRoot* lockobj = NULL );
	~Tr2Sprite2dPolygon();

	Color GetColor() const;
	void SetColor( Color val );

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	unsigned int GetVertexCount();
	void GatherSprites( Tr2Sprite2dScene* renderer );
	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event,		// BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList
		);

private:
#if BLUE_WITH_PYTHON
	static PyObject* PyAppendVertices( PyObject* self, PyObject* args );
	static PyObject* PySetVertices( PyObject* self, PyObject* args );
	static PyObject* PyAppendTriangles( PyObject* self, PyObject* args );
#endif

	PTr2Sprite2dVertexVector m_vertices;
	PTr2Sprite2dTriangleVector m_triangles;

	std::vector<std::vector<unsigned short>> m_indices;
	std::vector<std::vector<Tr2Sprite2dD3DVertex>> m_renderVertices;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dPolygon );
#endif //Tr2Sprite2dPolygon_h