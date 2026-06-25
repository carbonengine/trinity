// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "TriStepRenderAtlas.h"
#include "Tr2TextureAtlas.h"
#include "Tr2AtlasTexture.h"
#include "Tr2Renderer.h"
#include "Shader/Tr2Effect.h"
#include "Tr2VariableStore.h"

TriStepRenderAtlas::TriStepRenderAtlas( IRoot* lockobj ) :
	m_atlas( NULL ), m_focus( NULL ), m_tlTexCoord( 0.f, 0.f ), m_brTexCoord( 1.f, 1.f ), m_showFree( false ), m_showUsed( true ), m_borderColour( 1, 1, 0, 1 ), m_focusColour( 1, 0, 1, 1 ), m_freeColour( 0, 0.5f, 0, 1 )
{
	m_simpleOutColourHandle = GlobalStore().RegisterVariable( "simpleOutColour", Vector4( 0, 0, 0, 0 ) );
	BeClasses->CreateInstance( GetTr2EffectClsid(), BlueInterfaceIID<Tr2Effect>(), (void**)&m_areaEffect );
	m_areaEffect->SetEffectPathName( "res:/Graphics/Effect/UI/Simple.fx" );
}

TriStepRenderAtlas::~TriStepRenderAtlas()
{
	GlobalStore().UnregisterVariable( "simpleOutColour" );
}

// --------------------------------------------------------------------------------------
// Description:
//   Blue-exposed initializer.
// --------------------------------------------------------------------------------------
void TriStepRenderAtlas::py__init__( Tr2TextureAtlas* atlas, Tr2AtlasTexture* texture )
{
	SetAtlas( atlas );
	SetFocus( texture );
}

TriStepResult TriStepRenderAtlas::Execute( Be::Time realTime, Be::Time simTime, Tr2RenderContext& renderContext )
{
	if( m_atlas )
	{
		const Vector2 scale = m_brTexCoord - m_tlTexCoord;
		const Vector2 offset = Vector2( m_tlTexCoord.x / scale.x, m_tlTexCoord.y / scale.y );

		const float rw = 1.f / float( m_atlas->GetWidth() * scale.x );
		const float rh = 1.f / float( m_atlas->GetHeight() * scale.y );


		const int margin = std::max( int( m_atlas->GetMargin() ), 1 );

		if( m_showFree )
		{
			const auto freeAreas = m_atlas->GetFreeAreas();
			const static float borderScale = 0.6f;
			Vector4 freeBorderColour = m_freeColour;
			freeBorderColour.x *= borderScale;
			freeBorderColour.y *= borderScale;
			freeBorderColour.z *= borderScale;

			for( auto i = freeAreas.begin(); i != freeAreas.end(); ++i )
			{
				auto& rect = *i;
				m_simpleOutColourHandle->SetValue( freeBorderColour );
				Tr2Renderer::DrawScreenQuad( renderContext, m_areaEffect, Vector2( rect.left * rw, rect.top * rh ) - offset, Vector2( rect.right * rw, rect.bottom * rh ) - offset );

				m_simpleOutColourHandle->SetValue( m_freeColour );
				Tr2Renderer::DrawScreenQuad( renderContext, m_areaEffect, Vector2( ( rect.left + margin ) * rw, ( rect.top + margin ) * rh ) - offset, Vector2( ( rect.right - margin ) * rw, ( rect.bottom - margin ) * rh ) - offset );
			}
		}

		if( m_showUsed )
		{
			const auto usedAreas = m_atlas->GetUsedAreas();
			if( !m_focus )
			{
				m_simpleOutColourHandle->SetValue( m_borderColour );
			}

			for( auto i = usedAreas.begin(); i != usedAreas.end(); ++i )
			{
				auto& rect = *i;
				if( m_focus )
				{
					if( m_focus->GetX() - m_atlas->GetMargin() == rect.left &&
						m_focus->GetY() - m_atlas->GetMargin() == rect.top )
					{
						const float sineTime = sin( Tr2Renderer::GetAnimationTime() );
						m_simpleOutColourHandle->SetValue( m_focusColour * ( 0.5f + 0.5f * std::abs( sineTime ) ) );
					}
					else
					{
						m_simpleOutColourHandle->SetValue( m_borderColour );
					}
				}


				Tr2Renderer::DrawScreenQuad( renderContext, m_areaEffect, Vector2( rect.left * rw, rect.top * rh ) - offset, Vector2( ( rect.left + margin ) * rw, ( rect.bottom - margin ) * rh ) - offset );
				Tr2Renderer::DrawScreenQuad( renderContext, m_areaEffect, Vector2( rect.left * rw, ( rect.bottom - margin ) * rh ) - offset, Vector2( ( rect.right - margin ) * rw, rect.bottom * rh ) - offset );
				Tr2Renderer::DrawScreenQuad( renderContext, m_areaEffect, Vector2( ( rect.right - margin ) * rw, ( rect.top + margin ) * rh ) - offset, Vector2( rect.right * rw, rect.bottom * rh ) - offset );
				Tr2Renderer::DrawScreenQuad( renderContext, m_areaEffect, Vector2( ( rect.left + margin ) * rw, rect.top * rh ) - offset, Vector2( rect.right * rw, ( rect.top + margin ) * rh ) - offset );
			}
		}
	}
	return RS_OK;
}

void TriStepRenderAtlas::SetAtlas( Tr2TextureAtlas* atlas )
{
	m_atlas = atlas;
}

void TriStepRenderAtlas::SetFocus( Tr2AtlasTexture* texture )
{
	m_focus = texture;
}
