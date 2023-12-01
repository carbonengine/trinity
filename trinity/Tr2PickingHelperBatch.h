#pragma once
#ifndef Tr2PickingHelperBatch_H
#define Tr2PickingHelperBatch_H

#include "TriRenderBatch.h"
#include "TriDebugResourceHelper.h"

class Tr2PickingHelperBatch: public TriRenderBatch
{
public:
	Tr2PickingHelperBatch()
		:m_areaId( 0 )
	{
		static Tr2EffectPtr s_effect;
		if( !s_effect )
		{
			s_effect.CreateInstance();
			s_effect->SetEffectPathName( "res:/graphics/effect/managed/space/system/picking.fx" );
		}
		SetShaderMaterial( s_effect );
	}

	void AddSphere( const Vector3& center, float radius )
	{
		const int segments = 4;

		float a0cos = 1;
		float a0sin = 0;
		for( int i = 0; i < segments * 2; ++i )
		{
			float a1 = float( i + 1 ) / segments / 2 * XM_PI * 2;
			float a1cos = std::cos( a1 );
			float a1sin = std::sin( a1 );

			float b0cos = 1;
			float b0sin = 0;
			for( int j = 0; j < segments; ++j )
			{
				float b1 = float( j + 1 ) / segments * XM_PI;
				float b1cos = std::cos( b1 );
				float b1sin = std::sin( b1 );

				Vector3 v00 = Vector3( a0cos * b0sin, a0sin * b0sin, b0cos ) * radius + center;
				Vector3 v01 = Vector3( a0cos * b1sin, a0sin * b1sin, b1cos ) * radius + center;
				Vector3 v10 = Vector3( a1cos * b0sin, a1sin * b0sin, b0cos ) * radius + center;
				Vector3 v11 = Vector3( a1cos * b1sin, a1sin * b1sin, b1cos ) * radius + center;

				m_vertices.push_back( Vector4( v00, 0 ) );
				m_vertices.push_back( Vector4( v10, 0 ) );
				m_vertices.push_back( Vector4( v01, 0 ) );
				m_vertices.push_back( Vector4( v01, 0 ) );
				m_vertices.push_back( Vector4( v10, 0 ) );
				m_vertices.push_back( Vector4( v11, 0 ) );

				b0cos = b1cos;
				b0sin = b1sin;
			}

			a0cos = a1cos;
			a0sin = a1sin;
		}
	}

	void AddBox( const Matrix& transform )
	{
		const Vector3 corners[] = {
			Vector3( -0.5f,  0.5f,  0.5f ),
			Vector3( 0.5f,  0.5f,  0.5f ),
			Vector3( -0.5f, -0.5f,  0.5f ),
			Vector3( 0.5f, -0.5f,  0.5f ),
			Vector3( -0.5f,  0.5f, -0.5f ),
			Vector3( 0.5f,  0.5f, -0.5f ),
			Vector3( -0.5f, -0.5f, -0.5f ),
			Vector3( 0.5f, -0.5f, -0.5f ),
		};
		const unsigned int indices[] =
		{
			6, 4, 5,
			5, 7, 6,
			2, 1, 0,
			1, 2, 3,
			7, 5, 1,
			7, 1, 3,
			2, 0, 4,
			2, 4, 6,
			5, 4, 0,
			5, 0, 1,
			2, 6, 7, 
			2, 7, 3,
		};

		for( auto it = std::begin( indices ); it != std::end( indices ); ++it )
		{
			m_vertices.push_back( Vector4( XMVector3Transform( corners[*it], transform ) ) );
		}
	}

	void SetAreaID( uint16_t areaId )
	{
		m_areaId = areaId;
	}
	unsigned int GetPickingData() const 
	{ 
		return m_areaId; 
	}
	void SubmitGeometry( Tr2RenderContext& renderContext )
	{
		renderContext.m_esm.ApplyVertexDeclaration( g_debugResourceHelper.GetVertexPosColorDecl() );

		renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
		renderContext.DrawPrimitiveUP( uint32_t( m_vertices.size() / 3 ), &m_vertices.front(), uint32_t( sizeof( Vector4 ) ) );
	}
private:


	uint16_t m_areaId;
	std::vector<Vector4> m_vertices;
};

#endif