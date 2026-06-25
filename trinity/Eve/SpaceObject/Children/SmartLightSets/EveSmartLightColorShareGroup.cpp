// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveSmartLightColorShareGroup.h"
#include "TriMath.h"

const PlacementDataWithIdentifier s_PlacementDataWithIdentifierDefaultKey;

EveSmartLightColorShareGroup::EveSmartLightColorShareGroup( IRoot* lockobj ) :
	PARENTLOCK( m_lightGroups ),
	m_display( true )
{
	m_attributeModifiers.SetNotify( this );
	m_lightGroups.SetNotify( this );
}

bool EveSmartLightColorShareGroup::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}
	return true;
}

void EveSmartLightColorShareGroup::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList )
{
	if( theList == &m_attributeModifiers )
	{
		if( event == BELIST_INSERTED && m_parentColorSet != nullptr )
		{
			if( IEveSmartLightGroupAttributeModifierPtr attributeModifier = BlueCastPtr( value ) )
			{
				attributeModifier->SetInheritProperties( m_parentColorSet );
			}
		}
	}

	if( theList == &m_lightGroups )
	{
		if( event == BELIST_INSERTED && m_parentColorSet != nullptr )
		{
			if( IEveSmartLightGroupPtr lightGroup = BlueCastPtr( value ) )
			{
				lightGroup->SetInheritProperties( m_parentColorSet );
			}
		}
	}

	if( theList == &m_lightGroups && ( event & BELIST_LOADING ) == 0 )
	{
		if( IsInRegistry() )
		{
			switch( event & BELIST_EVENTMASK )
			{
			case BELIST_INSERTED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->Register( GetComponentRegistry() );
				}
				break;
			case BELIST_REMOVED:
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->UnRegister( GetComponentRegistry() );
				}
				break;
			case BELIST_UNLOADSTART:
				for( ssize_t i = 0; i < theList->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( theList->GetAt( i ) ) )
					{
						entity->UnRegister( GetComponentRegistry() );
					}
				}
				break;
			default:
				break;
			}
		}
	}
}

void EveSmartLightColorShareGroup::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		for( auto group : m_lightGroups )
		{
			if( EveEntityPtr entity = BlueCastPtr( group ) )
			{
				entity->Register( registry );
			}
		}
	}
}

void EveSmartLightColorShareGroup::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto group : m_lightGroups )
		{
			if( EveEntityPtr entity = BlueCastPtr( group ) )
			{
				entity->UnRegister( registry );
			}
		}
	}
}

void EveSmartLightColorShareGroup::AddQuadsToQuadRenderer( const PlacementDataWithIdentifierStructureList& placements, size_t size, const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_display )
	{
		return;
	}

	for( auto group : m_lightGroups )
	{
		group->AddQuadsToQuadRenderer( placements, size, frustum, quadRenderer );
	}
}

void EveSmartLightColorShareGroup::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	for( auto group : m_lightGroups )
	{
		group->GetRenderables( renderables );
	}
}

void EveSmartLightColorShareGroup::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	for( auto group : m_lightGroups )
	{
		group->UpdateSyncronous( updateContext, params, distribution );
	}

	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->UpdateSyncronous( updateContext, params, 1.f );
	}
}

void EveSmartLightColorShareGroup::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params, IEveDistributionMethod* distribution )
{
	const Vector3 up( 0.f, 1.f, 0.f );
	Vector3 colorValues = Vector4( GetGroupColor() ).GetXYZ();
	for( auto attributeModifier : m_attributeModifiers )
	{
		attributeModifier->ProcessAttributeModifier( colorValues, s_PlacementDataWithIdentifierDefaultKey, s_PlacementDataWithIdentifierDefaultKey.initialTranslation, up, params.activationStrength );
	}
	Color col = Color( colorValues.x, colorValues.y, colorValues.z, m_color.a );

	for( auto group : m_lightGroups )
	{
		group->SetColor( col );
		group->UpdateAsyncronous( updateContext, params, distribution );
	}
}

void EveSmartLightColorShareGroup::SetControllerVariable( const char* name, float value )
{
	EveSmartLightBaseGroup::SetControllerVariable( name, value );

	for( auto group : m_lightGroups )
	{
		group->SetControllerVariable( name, value );
	}
}

void EveSmartLightColorShareGroup::SetInheritProperties( const Color* colorSet )
{
	if( colorSet )
	{
		EveSmartLightBaseGroup::SetInheritProperties( colorSet );
		for( auto group : m_lightGroups )
		{
			group->SetInheritProperties( colorSet );
		}
	}
}

void EveSmartLightColorShareGroup::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto group : m_lightGroups )
	{
		group->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveSmartLightColorShareGroup::RenderDebugInfo( ITr2DebugRenderer2& renderer, const PlacementDataWithIdentifierStructureList& placements, size_t size )
{
	if( !m_display )
	{
		return;
	}

	for( auto group : m_lightGroups )
	{
		group->RenderDebugInfo( renderer, placements, size );
	}
}

void EveSmartLightColorShareGroup::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	for( auto group : m_lightGroups )
	{
		group->UpdateVisibility( updateContext, parentTransform, parentLod );
	}
}
