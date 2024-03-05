#include "StdAfx.h"
#include "Tr2LineSet.h"


CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );
using namespace Tr2RenderContextEnum;

Tr2LineSet::Tr2LineSet( IRoot* lockobj /*= NULL*/ ):
	Tr2PrimitiveSet( lockobj ),
	m_pickingVertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_maxCurrentTriangleCount( 0 ),
	m_currentSubmittedTriangleCount( 0 ),
	m_currentSubmittedLineCount( 0 ),
	m_maxCurrentLineCount( 0 )
{
}

Tr2LineSet::~Tr2LineSet()
{
	ReleaseResources( TRISTORAGE_ALL );
}

//////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool Tr2LineSet::Initialize()
{
	m_maxCurrentLineCount = 100;
	m_maxCurrentTriangleCount = 100;
	m_lines.reserve( m_maxCurrentLineCount );
	m_triangles.reserve( m_maxCurrentTriangleCount );
	PrepareResources();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
// ITriDeviceResource
void Tr2LineSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_pickingVertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	// CComPtr, this is safe!
	m_vertexBuffer = Tr2BufferAL();
	m_pickingVertexBuffer = Tr2BufferAL();
}

bool Tr2LineSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();


	// Visible lines
	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD );

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	// Picking geometry
	if( m_pickingVertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );

		m_pickingVertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
		if( m_pickingVertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	if( m_lines.size() )
	{
		if( !m_vertexBuffer.IsValid() || ( m_currentSubmittedLineCount < m_lines.size() ) )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL(
				m_vertexBuffer.Create(
					sizeof( LineData ),
					(unsigned int)m_lines.size(),
					Tr2GpuUsage::VERTEX_BUFFER,
					Tr2CpuUsage::WRITE_OFTEN,
					nullptr,
					renderContext )
				, false );
			m_currentSubmittedLineCount = (unsigned int)m_lines.size();
		}

		// Create a bounding sphere around the visible lines
		Vector3 center( 0.0f, 0.0f, 0.0f );
		float radius = 0.0f;
		ComputeBoundingSphere( static_cast<Vector3*>( &m_lines[0].m_position1 ), (unsigned int)m_lines.size() * 2, sizeof( LineData ) / 2, center, radius );

		void* vertexBuffer;
		CR_RETURN_VAL( m_vertexBuffer.MapForWriting( vertexBuffer, renderContext ), false );
		memcpy( vertexBuffer, &m_lines[0], sizeof( LineData ) * m_lines.size() );	
		m_vertexBuffer.UnmapForWriting( renderContext );

		m_boundingSphere.x = center.x;
		m_boundingSphere.y = center.y;
		m_boundingSphere.z = center.z;
		m_boundingSphere.w = radius;
	}

	
	if( m_triangles.size() )
	{
		if( !m_pickingVertexBuffer.IsValid() || (m_currentSubmittedTriangleCount < m_triangles.size()) )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL(
				m_pickingVertexBuffer.Create(
					sizeof( Triangle ),
					(unsigned int)m_triangles.size(),
					Tr2GpuUsage::VERTEX_BUFFER,
					Tr2CpuUsage::WRITE_OFTEN,
					nullptr,
					renderContext )
				, false );
			m_currentSubmittedTriangleCount = (unsigned int)m_triangles.size();
		}
		void* vertexBuffer;
		CR_RETURN_VAL( m_pickingVertexBuffer.MapForWriting( vertexBuffer, renderContext ), false );
		memcpy( vertexBuffer, &m_triangles[0], sizeof( Triangle )*m_triangles.size() );
		m_pickingVertexBuffer.UnmapForWriting( renderContext );
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// ITr2GeometryProvider
void Tr2LineSet::SubmitGeometry( Tr2RenderContext& renderContext )
{	
	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( (m_currentSubmittedTriangleCount > 0) && m_isDrawingForPicking )
	{
		if( m_pickingVertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return;
		}

		renderContext.m_esm.ApplyVertexDeclaration( m_pickingVertexDeclHandle );
		renderContext.m_esm.ApplyStreamSource( 0, m_pickingVertexBuffer, 0, sizeof( Triangle )/3 );
		renderContext.SetTopology( TOP_TRIANGLES );
		renderContext.DrawPrimitive( 0, m_currentSubmittedTriangleCount );
	}
	else
	{
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return;
		}

		renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
		renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( LineData ) / 2 );
		renderContext.SetTopology( TOP_LINES );
		renderContext.DrawPrimitive( 0, m_currentSubmittedLineCount );
	}
}

void Tr2LineSet::AddLine( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2 )
{
	LineData newLine;

	newLine.m_position1 = position1;
	newLine.m_color1 = color1;
	newLine.m_position2 = position2;
	newLine.m_color2 = color2;

	m_lines.push_back( newLine );
}

// Python Exposed Methods
void Tr2LineSet::AddPickingTriangle( const Vector3& position1, const Vector3& position2, const Vector3& position3 )
{
	Triangle newTriangle;

	newTriangle.m_position1 = position1;
	newTriangle.m_position2 = position2;
	newTriangle.m_position3 = position3;

	m_triangles.push_back( newTriangle );
}

bool Tr2LineSet::SubmitChanges()
{
	if( m_lines.size() > m_maxCurrentLineCount )
	{
		// increase the size of the buffer 
		m_maxCurrentLineCount = (unsigned int)m_lines.capacity();
		if( m_triangles.size() > m_maxCurrentTriangleCount )
		{
			m_maxCurrentTriangleCount = (unsigned int)m_triangles.capacity();
		}
	}

	ReleaseResources( TRISTORAGE_ALL );
	PrepareResources();

	return true;
}

void Tr2LineSet::SetCurrentColor( Color& val )
{
	for ( unsigned int i = 0; i < m_lines.size(); i++ )
	{
		m_lines[i].m_color1 = val;
		m_lines[i].m_color2 = val;
	}
	SubmitChanges();
}

void Tr2LineSet::ClearLines()
{
	m_lines.clear();
}

void Tr2LineSet::ClearPickingTriangles()
{
	m_triangles.clear();
}