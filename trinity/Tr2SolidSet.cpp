#include "StdAfx.h"
#include "Tr2SolidSet.h"
#include "Tr2Renderer.h"

CCP_STATS_DECLARED_ELSEWHERE( primitiveCount );
using namespace Tr2RenderContextEnum;

Tr2SolidSet::Tr2SolidSet( IRoot* lockobj /*= NULL*/ ):
	Tr2PrimitiveSet( lockobj ),
	m_currentSubmittedTriangleCount( 0 ),
	m_maxCurrentTriangleCount( 0 )
{
	m_maxCurrentTriangleCount = 100;
	m_triangles.reserve( m_maxCurrentTriangleCount );
}

Tr2SolidSet::~Tr2SolidSet()
{
	ReleaseResources( TRISTORAGE_ALL );
}

//////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool Tr2SolidSet::Initialize()
{
	PrepareResources();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
// ITriDeviceResource
void Tr2SolidSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	m_vertexBuffer = Tr2BufferAL();
}

bool Tr2SolidSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_3, vd.NORMAL );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD );

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	if( m_triangles.size() )
	{
		if( !m_vertexBuffer.IsValid() || (m_currentSubmittedTriangleCount < m_triangles.size()) )
		{
			USE_MAIN_THREAD_RENDER_CONTEXT();
			CR_RETURN_VAL(
				m_vertexBuffer.Create(
					sizeof( TriangleVertex ),
					(unsigned int)m_triangles.size() * 3,
					Tr2GpuUsage::VERTEX_BUFFER,
					Tr2CpuUsage::WRITE_OFTEN,
					nullptr,
					renderContext )
				, false );
			m_currentSubmittedTriangleCount = (unsigned int)m_triangles.size();
		}

		std::vector<TriangleVertex> vertexBuffer( m_triangles.size() * 3 );
		
		// Copy our user data to the buffer
		unsigned int j = 0;
		for( unsigned int i = 0; i < m_triangles.size(); i++,j+=3 )
		{	
			vertexBuffer[j].m_color = m_triangles[i].m_color1;
			vertexBuffer[j].m_position = m_triangles[i].m_position1;
			vertexBuffer[j].m_normal = m_triangles[i].m_normal;

			vertexBuffer[j+1].m_color = m_triangles[i].m_color2;
			vertexBuffer[j+1].m_position = m_triangles[i].m_position2;
			vertexBuffer[j+1].m_normal = m_triangles[i].m_normal;

			vertexBuffer[j+2].m_color = m_triangles[i].m_color3;
			vertexBuffer[j+2].m_position = m_triangles[i].m_position3;
			vertexBuffer[j+2].m_normal = m_triangles[i].m_normal;
		}
		
		// Create a bounding sphere
		Vector3 center( 0.0f, 0.0f, 0.0f );
		float radius = 0.0f;
		ComputeBoundingSphere( &vertexBuffer[0].m_position, (unsigned int)m_triangles.size()*3, sizeof(TriangleVertex), center, radius );

		TriangleVertex* data;
		CR_RETURN_VAL( m_vertexBuffer.MapForWriting( data, renderContext ), false );
		memcpy( data, vertexBuffer.data(), vertexBuffer.size() * sizeof( TriangleVertex ) );
		m_vertexBuffer.UnmapForWriting( renderContext );

		m_boundingSphere.x = center.x;
		m_boundingSphere.y = center.y;
		m_boundingSphere.z = center.z;
		m_boundingSphere.w = radius;
	}
	return true;
}

Vector3 Tr2SolidSet::GetCenterOfMass( void )
{
	Vector3 result = Vector3( 0.0f, 0.0f, 0.0f );
	for( unsigned int i = 0; i < m_triangles.size(); i ++ )
	{
		result.x += m_triangles[i].m_position1.x;
		result.x += m_triangles[i].m_position2.x;
		result.x += m_triangles[i].m_position3.x;
		result.y += m_triangles[i].m_position1.y;
		result.y += m_triangles[i].m_position2.y;
		result.y += m_triangles[i].m_position3.y;
		result.z += m_triangles[i].m_position1.z;
		result.z += m_triangles[i].m_position2.z;
		result.z += m_triangles[i].m_position3.z;
	}

	result.x /= (m_triangles.size()*3);
	result.y /= (m_triangles.size()*3);
	result.z /= (m_triangles.size()*3);
	result = TransformCoord( result, m_worldTransform );
	return result;
}


//////////////////////////////////////////////////////////////////////////
// ITr2GeometryProvider
void Tr2SolidSet::SubmitGeometry( Tr2RenderContext& renderContext )
{
	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	renderContext.m_esm.ApplyVertexDeclaration( m_vertexDeclHandle );
	renderContext.m_esm.ApplyStreamSource( 0, m_vertexBuffer, 0, sizeof( TriangleVertex ) );
	
	renderContext.SetTopology( TOP_TRIANGLES );
	renderContext.DrawPrimitive( 0, m_currentSubmittedTriangleCount );
}

void Tr2SolidSet::AddTriangle( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2,  const Vector3& position3, const Vector4& color3 )
{
	TriangleData newTriangle;

	newTriangle.m_position1 = position1;
	newTriangle.m_color1 = color1;
	newTriangle.m_position2 = position2;
	newTriangle.m_color2 = color2;
	newTriangle.m_position3 = position3;
	newTriangle.m_color3 = color3;

	Vector3 dir13(position1 - position3);
	Vector3 dir21(position2 - position1);

	newTriangle.m_normal = Cross( dir13, dir21 );

	newTriangle.m_normal = Normalize( newTriangle.m_normal );
	m_triangles.push_back( newTriangle );
}

bool Tr2SolidSet::SubmitChanges()
{
	if( m_triangles.size() > m_maxCurrentTriangleCount )
	{
		// increase the size of the buffer 
		m_maxCurrentTriangleCount = (unsigned int)m_triangles.capacity();
		ReleaseResources( TRISTORAGE_ALL );
	}

	// We have to make sure that we're prepared
	PrepareResources();

	return true;
}


void Tr2SolidSet::SetCurrentColor( Color& val )
{
	for ( unsigned int i = 0; i < m_triangles.size(); i++ )
	{
		m_triangles[i].m_color1 = val;
		m_triangles[i].m_color2 = val;
		m_triangles[i].m_color3 = val;
	}
	SubmitChanges();
}

void Tr2SolidSet::ClearTriangles()
{
	m_triangles.clear();
}
