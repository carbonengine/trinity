// Copyright © 2010 CCP ehf.

#pragma once

#ifndef Tr2Sprite2dLineTrace_h
#define Tr2Sprite2dLineTrace_h

#include "Tr2SpriteObject.h"

BLUE_DECLARE( Tr2Sprite2dLineTrace );
BLUE_DECLARE_INTERFACE( ITr2Sprite2dTexture );
BLUE_DECLARE( Tr2Sprite2dLineTraceVertex );
BLUE_DECLARE_VECTOR( Tr2Sprite2dLineTraceVertex );

BLUE_CLASS( Tr2Sprite2dLineTraceVertex ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();
	Tr2Sprite2dLineTraceVertex( IRoot* lockobj = NULL );

private:
	friend class Tr2Sprite2dLineTrace;

	std::string m_name;
	Vector2 m_position;
	Color m_color;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dLineTraceVertex );

class Tr2Sprite2dLineTrace : public Tr2TexturedSpriteObject,
							 public IListNotify
{
public:
	EXPOSE_TO_BLUE();

	Tr2Sprite2dLineTrace( IRoot* lockobj = NULL );
	~Tr2Sprite2dLineTrace();

	//////////////////////////////////////////////////////////////////////////
	// ITr2SpriteObject
	void GatherSprites( Tr2Sprite2dScene* renderer );

	ITr2SpriteObject* PickPoint( float x, float y, Tr2Sprite2dScene* renderer );

	//////////////////////////////////////////////////////////////////////////
	// IListNotify
	void OnListModified(
		long event, // BLUELISTEVENT values
		ssize_t key,
		ssize_t key2,
		IRoot* value,
		const IList* theList );

	enum CornerType
	{
		CORNERTYPE_MITER,
		CORNERTYPE_ROUND,
		CORNERTYPE_NONE
	};

private:
	void ClearVertices();
	float CalcLength();
	void AddSegment(
		Tr2Sprite2dScene* renderer,
		const Vector2& from,
		const Color& fromColor,
		float capAngleFrom,
		const Vector2& to,
		const Color& toColor,
		float capAngleTo );
	void AddVertex( Tr2Sprite2dVertexBase& v, float xOffset, float yOffset, float halfWidth, bool isAA, float texOffset1, Color color );
	void AddRoundJoint(
		Tr2Sprite2dScene* renderer,
		float capAngleTo,
		Tr2Sprite2dVertexBase v2,
		Tr2Sprite2dVertexBase v3,
		Vector2 normal,
		Color modulatedToColor,
		bool isAA,
		float pixelWidthInTexels,
		float halfWidth );
	float ClampAngle( float angle );
	unsigned int GetEstimatedVertexCount();
	void AddMiterJoint(
		Tr2Sprite2dScene* renderer,
		float capAngleTo,
		Tr2Sprite2dVertexBase v2,
		Tr2Sprite2dVertexBase v3,
		Vector2 normal,
		Color modulatedToColor,
		bool isAA,
		float pixelWidthInTexels,
		float halfWidth );
	Vector2 GetMiterPoint( float halfWidth, Vector2 basePoint, float startAngle, float endAngle, float sign );

private:
#if BLUE_WITH_PYTHON
	static PyObject* PyAppendVertices( PyObject* self, PyObject* args );
	static PyObject* PySetVertices( PyObject* self, PyObject* args );
#endif
	std::wstring m_name;

	PTr2Sprite2dLineTraceVertexVector m_vertices;

	float m_lineWidth;
	float m_textureWidth;
	float m_textureOffset;
	float m_textureOffsetAccum;
	float m_length;
	int m_cornerType;

	float m_start;
	float m_end;
	bool m_isLoop;

	struct DrawCall
	{
		uint32_t vertexOffset;
		uint32_t indexOffset;
		uint16_t vertexCount;
		uint16_t indexCount;
	};

	TrackableStdVector<uint16_t> m_renderIndices;
	TrackableStdVector<Tr2Sprite2dD3DVertex> m_renderVertices;
	TrackableStdVector<DrawCall> m_drawCalls;
};

TYPEDEF_BLUECLASS( Tr2Sprite2dLineTrace );

#endif