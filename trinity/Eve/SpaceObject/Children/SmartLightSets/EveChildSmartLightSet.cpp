// Copyright © 2025 CCP ehf.

#include "StdAfx.h"
#include "EveChildSmartLightSet.h"
#include "Tr2DebugRenderer.h"
#include "Tr2LightManager.h"

EveChildSmartLightSet::EveChildSmartLightSet( IRoot* lockobj ) :
	EveChildTransform(),
	PARENTLOCK( m_lightGroups ),
	m_display( true )
{
	m_lightGroups.SetNotify( this );
}

const char* EveChildSmartLightSet::GetName() const
{
	return m_name.c_str();
}

void EveChildSmartLightSet::SetName( const char* name )
{
	m_name = name;
}

void EveChildSmartLightSet::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_lightGroups && ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
	{
		if( m_inheritProperties )
		{
			if( IEveSmartLightGroupPtr obj = BlueCastPtr( value ) )
			{
				obj->SetInheritProperties( m_inheritProperties->GetProperties() );
			}
		}
	}

	if( list == &m_lightGroups && ( event & BELIST_LOADING ) == 0 )
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
				for( ssize_t i = 0; i < list->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( list->GetAt( i ) ) )
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

void EveChildSmartLightSet::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	UpdateTransform( params.localToWorldTransform );

	if( m_distribution != nullptr )
	{
		m_distribution->UpdateSyncronous( updateContext, params );
	}

	for( auto it : m_lightGroups )
	{
		it->UpdateSyncronous( updateContext, params, m_distribution );
	}
}

void EveChildSmartLightSet::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_distribution != nullptr )
	{
		m_distribution->UpdateAsyncronous( updateContext, params );
	}

	for( auto it : m_lightGroups )
	{
		it->UpdateAsyncronous( updateContext, params, m_distribution );
	}
}

void EveChildSmartLightSet::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( m_distribution != nullptr && m_display )
	{
		for( auto it : m_lightGroups )
		{
			it->UpdateVisibility( updateContext, parentTransform, parentLod );
		}
	}
}

bool EveChildSmartLightSet::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_display ) || IsMatch( value, m_distribution ) )
	{
		ReRegister();
	}
	return true;
}

void EveChildSmartLightSet::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_distribution && m_display )
	{
		for( auto it : m_lightGroups )
		{
			if( EveEntityPtr entity = BlueCastPtr( it ) )
			{
				entity->Register( registry );
			}
		}
	}
}

void EveChildSmartLightSet::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto it : m_lightGroups )
		{
			if( EveEntityPtr entity = BlueCastPtr( it ) )
			{
				entity->UnRegister( registry );
			}
		}
	}
}

void EveChildSmartLightSet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_distribution != nullptr && m_display )
	{
		for( auto it : m_lightGroups )
		{
			it->GetRenderables( renderables );
		}
	}
}

void EveChildSmartLightSet::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildSmartLightSet::SetControllerVariable( const char* name, float value )
{
	if( m_distribution )
	{
		m_distribution->SetControllerVariable( name, value );
	}

	for( auto it : m_lightGroups )
	{
		it->SetControllerVariable( name, value );
	}
}

void EveChildSmartLightSet::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( m_display && m_distribution != nullptr )
	{
		for( auto it : m_lightGroups )
		{
			it->RenderDebugInfo( renderer, *m_distribution->GetPlacementData(), m_distribution->GetNumberOfPlacements() );
		}
	}
}

void EveChildSmartLightSet::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_display && m_distribution != nullptr )
	{
		for( auto it : m_lightGroups )
		{
			it->AddQuadsToQuadRenderer( *m_distribution->GetPlacementData(), m_distribution->GetNumberOfPlacements(), frustum, quadRenderer );
		}
	}
}

void EveChildSmartLightSet::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it : m_lightGroups )
	{
		it->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildSmartLightSet::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "smartLightSets" );
}

void EveChildSmartLightSet::SetInheritProperties( const Color* colorSet )
{
	if( !m_inheritProperties )
	{
		m_inheritProperties.CreateInstance();
	}
	m_inheritProperties->SetProperties( colorSet );

	for( auto group : m_lightGroups )
	{
		group->SetInheritProperties( colorSet );
	}
}
