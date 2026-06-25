// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightQuad.h"
#include "Tr2QuadRenderer.h"
#include "Shader/Tr2Effect.h"
#include "TriMath.h"


EveSmartLightQuad::EveSmartLightQuad( IRoot* lockobj ) :
	m_staticOffsetTranslation( 0.f, 0.f, 0.f ),
	m_display( true ),
	m_softQuad( false ),
	m_staticQuadScale( 1.f, 1.f, 1.f ),
	m_activationStrength( 1.f ),
	m_color( 0.f, 0.f, 0.f, 1.f ),
	m_brightness( 1.f ),
	m_editMode( false ),
	m_effectKey( 0 )
{
	m_effect.CreateInstance();

	if( m_effect )
	{
		if( m_softQuad )
		{
			m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/flarequadsoft.fx" );
		}
		else
		{
			m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/FlareQuad.fx" );
		}
	}
}

bool EveSmartLightQuad::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_softQuad ) )
	{
		if( m_effect )
		{
			if( m_softQuad )
			{
				m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/flarequadsoft.fx" );
			}
			else
			{
				m_effect->SetEffectPathName( "res:/Graphics/Effect/Managed/Space/SpecialFX/FlareQuad.fx" );
			}
		}
	}

	return true;
}

bool EveSmartLightQuad::Initialize()
{
	if( m_effect )
	{
		m_effectKey = m_effect->GetHashValue();
		Tr2QuadRenderer::Instance()->RegisterEffect( m_effectKey, TRIBATCHTYPE_ADDITIVE, sizeof( SimplifiedQuad ), 1, EveChildQuad::GetQuadDefinition(), m_effect );
	}

	return true;
}


void EveSmartLightQuad::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	quadRenderer.RegisterEffect( m_effectKey, TRIBATCHTYPE_ADDITIVE, sizeof( SimplifiedQuad ), 1, EveChildQuad::GetQuadDefinition(), m_effect );
}

void EveSmartLightQuad::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	m_activationStrength = params.activationStrength;

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

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->UpdateSyncronous( updateContext, params, 1.f );
	}
}

void EveSmartLightQuad::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	Matrix localToWorldTransform = params.localToWorldTransform;

	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}

	UpdateTransform( localToWorldTransform );
}

void EveSmartLightQuad::AddQuadsToQuadRenderer( const PlacementDataWithIdentifierStructureList& placements, size_t size, const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_display && m_effect )
	{
		SimplifiedQuad quad;
		Vector3 color, quadPosition;
		for( size_t index = 0; index < size; index++ )
		{
			float scaleX = placements[index].initialScale.x * placements[index].additionalScale.x * m_staticQuadScale.x;
			float scaleY = placements[index].initialScale.y * placements[index].additionalScale.y * m_staticQuadScale.y;
			float scaleZ = placements[index].initialScale.z * placements[index].additionalScale.z * m_staticQuadScale.z;

			quadPosition = Vector3( 0.f, 0.f, 0.f );
			Quaternion quadRotation = placements[index].initialRotation * placements[index].additionalRotation;

			if( m_staticOffsetTranslation != quadPosition )
			{
				TriVectorRotateQuaternion( &quadPosition, &m_staticOffsetTranslation, &quadRotation );
			}

			Vector3 quadDirection( 0.f, 1.f, 0.f );
			TriVectorRotateQuaternion( &quadDirection, &quadDirection, &quadRotation );
			TriVectorRotateMatrix( &quadDirection, &quadDirection, &m_worldTransform );

			Vector4 groupColor = this->GetGroupColor() * m_activationStrength;
			quadPosition += placements[index].initialTranslation + placements[index].additionalTranslation;
			float maxScale = max( max( scaleX, scaleY ), scaleZ );
			Vector4 visibilityCheck = Vector4( Vector3( XMVector3TransformCoord( quadPosition, m_worldTransform ) ), maxScale );
			if( frustum.IsSphereVisible( &visibilityCheck ) )
			{
				color = groupColor.GetXYZ();

				for( auto attributeModifier : m_attributeModifiers )
				{
					attributeModifier->ProcessAttributeModifier( color, placements[index], visibilityCheck.GetXYZ(), quadDirection, m_activationStrength );
				}

				quad.m_parentTransform0 = Vector4( m_worldTransform._11, m_worldTransform._21, m_worldTransform._31, m_worldTransform._41 );
				quad.m_parentTransform1 = Vector4( m_worldTransform._12, m_worldTransform._22, m_worldTransform._32, m_worldTransform._42 );
				quad.m_parentTransform2 = Vector4( m_worldTransform._13, m_worldTransform._23, m_worldTransform._33, m_worldTransform._43 );
				quad.m_localTransform0 = Vector4( scaleX, 0.f, 0.f, quadPosition.x );
				quad.m_localTransform1 = Vector4( 0.f, scaleY, 0.f, quadPosition.y );
				quad.m_localTransform2 = Vector4( 0.f, 0.f, scaleZ, quadPosition.z );
				quad.m_color[0] = Float_16( color.x );
				quad.m_color[1] = Float_16( color.y );
				quad.m_color[2] = Float_16( color.z );
				quad.m_color[3] = Float_16( m_color.a );
				quad.m_brightness[0] = Float_16( m_brightness );
				quad.m_brightness[1] = Float_16( 0.f );

				quadRenderer.AddQuads( m_effectKey, &quad, 1 );
			}
		}
	}
}
