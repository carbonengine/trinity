////////////////////////////////////////////////////////////
//
//    Created:   August 2016
//    Copyright: CCP 2016
//
#include "StdAfx.h"
#include "EveChildQuad.h"

#include "Tr2QuadRenderer.h"
#include "Shader/Tr2Effect.h"

#include "Tr2Renderer.h"
#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"


extern float g_eveSpaceSceneLODFactor;


namespace
{
	Tr2VertexDefinition& GetQuadDefinition()
	{
		static Tr2VertexDefinition def;
		if( def.empty() )
		{
			def.Add( def.FLOAT32_1, def.TEXCOORD, 5 );

			def.Add( def.FLOAT32_4, def.POSITION, 0, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 1, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 2, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 3, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 4, 1, 1 );
			def.Add( def.FLOAT32_4, def.POSITION, 5, 1, 1 );

			def.Add( def.FLOAT16_4, def.TEXCOORD, 0, 1, 1 );
			def.Add( def.FLOAT16_2, def.TEXCOORD, 1, 1, 1 );
		}
		return def;
	}
}

EveChildQuad::EveChildQuad( IRoot* lockobj ):
	m_effectKey( 0 ),
	m_position( 0.f, 0.f, 0.f ),
	m_viewRotation( 0.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_brightness( 1.f ),
	m_minScreenSize( 0.f ),
	m_currentScreenSize( -1 ),
	m_display( true ),
	m_editMode( false )
{
	memset( &m_quad, 0, sizeof( m_quad ) );
}

EveChildQuad::~EveChildQuad()
{
}

bool EveChildQuad::Initialize()
{
	if( m_effect )
	{
		m_effectKey = m_effect->GetHashValue();
		Tr2QuadRenderer::Instance()->RegisterEffect( m_effectKey, TRIBATCHTYPE_ADDITIVE, sizeof( Quad ), 1, GetQuadDefinition(), m_effect );
	}
	if( m_staticTransform )
	{
		RebuildLocalTransform();
	}
	return true;
}

const char* EveChildQuad::GetName() const
{
	return m_name.c_str();
}

void EveChildQuad::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

// --------------------------------------------------------------------------------
// Description:
//   Setup function to set data from outside, in this case just pass it to
//   function of base class
// --------------------------------------------------------------------------------
void EveChildQuad::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildQuad::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_effect )
	{
		quadRenderer.RegisterEffect( m_effectKey, TRIBATCHTYPE_ADDITIVE, sizeof( Quad ), 1, GetQuadDefinition(), m_effect );
	}
}

void EveChildQuad::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_display && m_effect )
	{
		Vector4 sphere = Vector4( 0.f, 0.f, 0.f, std::sqrt( 2.f ) );
		BoundingSphereTransform( m_worldTransform, sphere );
		if( frustum.IsSphereVisible( &sphere ) )
		{
			m_currentScreenSize = frustum.GetPixelSizeAccross( &sphere );
			if( m_currentScreenSize >= m_minScreenSize * g_eveSpaceSceneLODFactor )
			{
				quadRenderer.AddQuads( m_effectKey, &m_quad, 1 );
			}
		}
		else
		{
			m_currentScreenSize = -1;
		}
	}
}

void EveChildQuad::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
}

bool EveChildQuad::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = Vector4( 0.f, 0.f, 0.f, std::sqrt( 2.f ) );
	BoundingSphereTransform( m_worldTransform, sphere );
	return true;
}

bool EveChildQuad::HasTransparentBatches()
{
	return false;
}

void EveChildQuad::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
}

float EveChildQuad::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance;
}


void EveChildQuad::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& )
{
	if( m_editMode )
	{
		if( m_effect )
		{
			auto key = m_effect->GetHashValue();
			if( key != m_effectKey )
			{
				m_effectKey = key;
				RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
			}
		}
		else
		{
			m_effectKey = 0;
		}
	}
}

void EveChildQuad::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix parentTransform;
	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( parentTransform );
	}
	else if ( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( parentTransform );
	}
	else 
	{
		return;
	}

	UpdateTransform( parentTransform );

	m_quad.m_parentTransform0 = Vector4( parentTransform._11, parentTransform._21, parentTransform._31, parentTransform._41 );
	m_quad.m_parentTransform1 = Vector4( parentTransform._12, parentTransform._22, parentTransform._32, parentTransform._42 );
	m_quad.m_parentTransform2 = Vector4( parentTransform._13, parentTransform._23, parentTransform._33, parentTransform._43 );
	m_quad.m_localTransform0 = Vector4( m_localTransform._11, m_localTransform._21, m_localTransform._31, m_localTransform._41 );
	m_quad.m_localTransform1 = Vector4( m_localTransform._12, m_localTransform._22, m_localTransform._32, m_localTransform._42 );
	m_quad.m_localTransform2 = Vector4( m_localTransform._13, m_localTransform._23, m_localTransform._33, m_localTransform._43 );
	m_quad.m_color[0] = Float_16( m_color.r );
	m_quad.m_color[1] = Float_16( m_color.g );
	m_quad.m_color[2] = Float_16( m_color.b );
	m_quad.m_color[3] = Float_16( m_color.a );
	m_quad.m_brightness[0] = Float_16( m_brightness );
	m_quad.m_brightness[1] = Float_16( 0.f );
}

void EveChildQuad::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

Tr2PerObjectData* EveChildQuad::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return nullptr;
}