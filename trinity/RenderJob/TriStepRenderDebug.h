// Copyright © 2023 CCP ehf.

#pragma once
#ifndef TRISTEPRENDERDEBUG_H
#define TRISTEPRENDERDEBUG_H


#include "TriRenderStep.h"
#include "Shader/Tr2Effect.h"
#include "include/ITr2DebugRenderer.h"
#include "include/Rect.h"

BLUE_DECLARE( TriStepRenderDebug );
BLUE_DECLARE( TriLineSet );

class TriDebugTextRenderer;

class TriStepRenderDebug : public TriRenderStep,
						   public ITr2DebugRenderer,
						   public IRenderCallback
{
public:
	EXPOSE_TO_BLUE();

	TriStepRenderDebug( IRoot* lockobj = 0 );
	~TriStepRenderDebug( void );

	//////////////////////////////////////////////////////////////////////////
	// Helper functions for easier Python exposure
	void Print2D( int x, int y, uint32_t color, const std::string& msg );
	void Print2Df( int x, int y, int w, int h, uint32_t format, uint32_t color, const std::string& msg );
	void Print3D( const Vector3& pos, uint32_t color, const std::string& msg );

	void Clear();

	//////////////////////////////////////////////////////////////////////////
	// RenderStep
	TriStepResult Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext );

	//////////////////////////////////////////////////////////////////////////
	// ITriDebugRenderer.h
	void DrawPointCloud( int numPoints, float* points, int stride );
	void DrawLine( const Vector3& from, const Vector3& to, uint32_t color = 0xffffffff );
	void DrawLine( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor );
	void DrawSphere( const Vector3& center, float radius, int segments, uint32_t color = 0xffffffff );
	void DrawBox( const Vector3& min, const Vector3& max, uint32_t color = 0xffffffff );
	void DrawCapsule( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff );
	void DrawCylinder( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff );
	void DrawCone( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color = 0xffffffff );
	void DrawPlane( const Vector4& planeEquation, int segments, uint32_t color = 0xffffffff );
	void DrawAxes( const Matrix& m, float scale = 1.0f );
	void Printf( int x, int y, uint32_t color, const char* msg, ... );
	void Printf( const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... );
	void Printf( const Vector3& pos, uint32_t color, const char* msg, ... );

	// Need to expose a single draw line for python
	void PyDrawLine( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor )
	{
		DrawLine( from, fromColor, to, toColor );
	};

	//////////////////////////////////////////////////////////////////////////
	// Tr2Effect::IRenderCallback
	void SubmitGeometry( Tr2RenderContext& renderContext );

private:
	uint8_t* m_data;
	uint8_t* m_curData;
	int m_numPrimitives;
	TriLineSet* m_lineSet;
	TriDebugTextRenderer* m_textRenderer;

	// Have a separate text renderer for projected text.
	// This simplifies the logic when not clearing every frame.
	TriDebugTextRenderer* m_projectedTextRenderer;

	struct ProjectedTextEntry
	{
		std::string msg;
		Vector3 pos;
		uint32_t color;
	};

	TrackableStdList<ProjectedTextEntry> m_projectedText;

	bool m_autoClear;
};

TYPEDEF_BLUECLASS( TriStepRenderDebug );

#endif