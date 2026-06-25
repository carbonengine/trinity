// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriStepRenderDebug.h"
#include "Tr2Renderer.h"
#include "TriDebugResourceHelper.h"
#include "TriLineSet.h"
#include "TriDebugTextRenderer.h"



const int NUM_POINTS_MAX = 32000;

using namespace Tr2RenderContextEnum;

TriStepRenderDebug::TriStepRenderDebug( IRoot* lockobj ) :
	m_data( NULL ),
	m_numPrimitives( 0 ),
	m_lineSet( nullptr ),
	m_projectedText( "TriStepRenderDebug/m_projectedText" ),
	m_autoClear( true )
{
	m_data = (uint8_t*)CCP_MALLOC( "TriStepRenderDebug/m_data", NUM_POINTS_MAX * ( 3 * sizeof( float ) + sizeof( uint32_t ) ) );
	m_curData = m_data;
	BeClasses->CreateInstance( GetTriLineSetClsid(), BlueInterfaceIID<TriLineSet>(), (void**)&m_lineSet );

	m_textRenderer = CCP_NEW( "TriStepRenderDebug/m_textRenderer" ) TriDebugTextRenderer;
	m_textRenderer->PrepareResources();

	m_projectedTextRenderer = CCP_NEW( "TriStepRenderDebug/m_projectedTextRenderer" ) TriDebugTextRenderer;
	m_projectedTextRenderer->PrepareResources();
}

TriStepRenderDebug::~TriStepRenderDebug( void )
{
	if( m_data )
	{
		CCP_FREE( m_data );
	}
	m_lineSet->Unlock();

	CCP_DELETE m_textRenderer;
	CCP_DELETE m_projectedTextRenderer;
}

void TriStepRenderDebug::SubmitGeometry( Tr2RenderContext& renderContext )
{
	renderContext.SetTopology( TOP_POINTS );
	renderContext.DrawPrimitiveUP( m_numPrimitives, m_data, 3 * sizeof( float ) );
}

Vector4 ColorToVec4( uint32_t color );

TriStepResult TriStepRenderDebug::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	// Render point clouds
	renderContext.m_esm.ApplyVertexDeclaration( g_debugResourceHelper.GetVertexPosColorDecl() );
	g_debugResourceHelper.GetEffect()->Render( this, renderContext );

	// Render lines
	m_lineSet->Render( renderContext );

	Tr2Viewport viewport;
	renderContext.GetViewport( viewport );

	for( TrackableStdList<ProjectedTextEntry>::iterator it = m_projectedText.begin(); it != m_projectedText.end(); ++it )
	{
		ProjectedTextEntry e = *it;

		Tr2Rect rect;
		Vector3 screenPos = Tr2Renderer::ProjectWorldToScreen( e.pos, viewport );

		if( ( screenPos.z > 0.0f ) && ( screenPos.z < 1.0f ) )
		{
			rect.top = (int32_t)screenPos.y;
			rect.left = (int32_t)screenPos.x;
			rect.bottom = rect.top + 512;
			rect.right = rect.left + 1024;

			m_projectedTextRenderer->PrintfImmediate( renderContext, TRI_DBG_FONT_SMALL, rect, TRI_DFS_LEFT, ColorToVec4( e.color ), e.msg.c_str() );
		}
	}

	// Render text
	m_textRenderer->Render( renderContext );

	if( m_autoClear )
	{
		Clear();
	}
	return RS_OK;
}

void TriStepRenderDebug::DrawPointCloud( int numPoints, float* points, int stride )
{
	if( m_numPrimitives + numPoints < NUM_POINTS_MAX )
	{
		float* dst = (float*)m_curData;
		float* src = points;
		for( int i = 0; i < numPoints; ++i )
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];

			dst += 3;
			*(uint32_t*)dst = 0xffffffff;

			++dst;

			src = (float*)( (uint8_t*)src + stride );
		}

		m_curData += numPoints * ( 3 * sizeof( float ) + sizeof( uint32_t ) );
		m_numPrimitives += numPoints;
	}
}

void TriStepRenderDebug::DrawLine( const Vector3& from, const Vector3& to, uint32_t color /*= 0xffffffff */ )
{
	m_lineSet->Add( from, color, to, color );
}

void TriStepRenderDebug::DrawLine( const Vector3& from, uint32_t fromColor, const Vector3& to, uint32_t toColor )
{
	m_lineSet->Add( from, fromColor, to, toColor );
}

void TriStepRenderDebug::DrawSphere( const Vector3& center, float radius, int segments, uint32_t color /*= 0xffffffff */ )
{
	m_lineSet->AddSphere( center, radius, segments, color );
}

void TriStepRenderDebug::DrawBox( const Vector3& min, const Vector3& max, uint32_t color /*= 0xffffffff */ )
{
	m_lineSet->AddBox( min, max, color );
}

void TriStepRenderDebug::Printf( int x, int y, uint32_t color, const char* msg, ... )
{
	va_list args;
	va_start( args, msg );

	Tr2Rect rect;
	rect.top = y;
	rect.left = x;
	rect.bottom = rect.top + 512;
	rect.right = rect.left + 1024;

	m_textRenderer->Vprintf( TRI_DBG_FONT_SMALL, rect, TRI_DFS_LEFT, ColorToVec4( color ), msg, args );
	va_end( args );
}

void TriStepRenderDebug::Printf( const Tr2Rect& rect, uint32_t format, uint32_t color, const char* msg, ... )
{
	va_list args;
	va_start( args, msg );

	m_textRenderer->Vprintf( TRI_DBG_FONT_SMALL, rect, format, ColorToVec4( color ), msg, args );
	va_end( args );
}

void TriStepRenderDebug::Printf( const Vector3& pos, uint32_t color, const char* msg, ... )
{
	va_list args;
	va_start( args, msg );

	const int BUFFER_SIZE = 1024;
	static char buffer[BUFFER_SIZE];

	vsnprintf_s( buffer, BUFFER_SIZE, _TRUNCATE, msg, args );

	ProjectedTextEntry e;
	e.msg = buffer;
	e.pos = pos;
	e.color = color;

	m_projectedText.push_back( e );
	va_end( args );
}

void TriStepRenderDebug::DrawCapsule( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color /*= 0xffffffff */ )
{
	m_lineSet->AddCylinder( start, end, radius, segments, color );
	m_lineSet->AddSphere( start, radius, segments, color );
	m_lineSet->AddSphere( end, radius, segments, color );
}

void TriStepRenderDebug::DrawCylinder( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color /*= 0xffffffff */ )
{
	m_lineSet->AddCylinder( start, end, radius, segments, color );
}

void TriStepRenderDebug::DrawCone( const Vector3& start, const Vector3& end, float radius, int segments, uint32_t color /*= 0xffffffff */ )
{
	m_lineSet->AddCone( start, end, radius, segments, color );
}

void TriStepRenderDebug::DrawPlane( const Vector4& planeEquation, int segments, uint32_t color /*= 0xffffffff */ )
{
}

void TriStepRenderDebug::DrawAxes( const Matrix& m, float scale )
{
	Vector3 origin = TransformCoord( Vector3( 0.0f, 0.0f, 0.0f ), m );
	Vector3 xAxis = TransformCoord( Vector3( scale, 0.0f, 0.0f ), m );
	Vector3 yAxis = TransformCoord( Vector3( 0.0f, scale, 0.0f ), m );
	Vector3 zAxis = TransformCoord( Vector3( 0.0f, 0.0f, scale ), m );

	m_lineSet->Add( origin, 0xffff0000, xAxis, 0xffff0000 );
	m_lineSet->Add( origin, 0xff00ff00, yAxis, 0xff00ff00 );
	m_lineSet->Add( origin, 0xff0000ff, zAxis, 0xff0000ff );
}

void TriStepRenderDebug::Print2D( int x, int y, uint32_t color, const std::string& msg )
{
	Printf( x, y, color, msg.c_str() );
}

void TriStepRenderDebug::Print2Df( int x, int y, int w, int h, uint32_t format, uint32_t color, const std::string& msg )
{
	Tr2Rect rect = { x, y, w, h };
	Printf( rect, color, format, msg.c_str() );
}

void TriStepRenderDebug::Print3D( const Vector3& pos, uint32_t color, const std::string& msg )
{
	Printf( pos, color, msg.c_str() );
}

void TriStepRenderDebug::Clear()
{
	m_curData = m_data;
	m_numPrimitives = 0;
	m_lineSet->Clear();
	m_textRenderer->Clear();
	m_projectedText.clear();
}
