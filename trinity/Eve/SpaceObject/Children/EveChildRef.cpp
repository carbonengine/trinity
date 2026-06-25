// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveChildRef.h"

#include "Utilities/BoundingSphere.h"
#include "Eve/EveUpdateContext.h"


EveChildRef::EveChildRef( IRoot* lockobj ) :
	EveChildTransform(),
	m_display( true ),
	m_loadChildAutomatically( true )
{
}

EveChildRef::~EveChildRef()
{
}

const char* EveChildRef::GetResPath() const
{
	return m_resPath.c_str();
}

void EveChildRef::SetResPath( const char* resPath )
{
	if( m_resPath != resPath )
	{
		m_resPath = resPath;
		if( m_loadChildAutomatically )
		{
			LoadChild();
		}
	}
}

void EveChildRef::Reload( bool bypassAutoLoadBlocker )
{
	if( m_loadChildAutomatically || bypassAutoLoadBlocker )
	{
		LoadChild();
	}
}

void EveChildRef::SetAutoLoadBlocker( bool shouldBlockAutoLoad )
{
	m_loadChildAutomatically = !shouldBlockAutoLoad;
}

bool EveChildRef::Initialize()
{
	if( m_loadChildAutomatically )
	{
		LoadChild();
	}

	return true;
}

bool EveChildRef::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_resPath ) )
	{
		if( m_loadChildAutomatically )
		{
			LoadChild();
		}
	}
	if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}
	return true;
}

const char* EveChildRef::GetName() const
{
	return m_name.c_str();
}

void EveChildRef::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildRef::RegisterComponents()
{
	if( IsInRegistry() && m_child != nullptr && m_display )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_child ) )
		{
			entity->Register( GetComponentRegistry() );
		}
	}
}

void EveChildRef::UnRegisterComponents()
{
	if( IsInRegistry() && m_child != nullptr )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_child ) )
		{
			entity->UnRegister( GetComponentRegistry() );
		}
	}
}

IEveSpaceObjectChildPtr EveChildRef::GetEffectChildByName( const char* name ) const
{
	if( auto ref = dynamic_cast<IEveEffectChildrenOwner*>( m_child.p ) )
	{
		return ref->GetEffectChildByName( name );
	}
	return nullptr;
}

void EveChildRef::AddToEffectChildrenList( IEveSpaceObjectChild* child )
{
	if( auto ref = dynamic_cast<IEveEffectChildrenOwner*>( m_child.p ) )
	{
		ref->AddToEffectChildrenList( child );
	}
}

void EveChildRef::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
	if( auto ref = dynamic_cast<IEveEffectChildrenOwner*>( m_child.p ) )
	{
		ref->RemoveFromEffectChildrenList( child );
	}
}

void EveChildRef::SetProceduralContainerVariable( const char* name, float value )
{
	if( m_child )
	{
		m_child->SetProceduralContainerVariable( name, value );
	}
}

void EveChildRef::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}
	if( m_child )
	{
		m_child->UpdateVisibility( updateContext, parentTransform, parentLod );
	}
}

void EveChildRef::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_display && m_child )
	{
		m_child->GetRenderables( renderables );
	}
}

bool EveChildRef::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool success = false;
	Vector4 bSphere( 0.f, 0.f, 0.f, -1.f );
	if( m_child && m_child->GetBoundingSphere( bSphere ) )
	{
		BoundingSphereSetOrUpdate( bSphere, sphere, success );
		success = true;
	}
	return success;
}

void EveChildRef::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_child )
	{
		m_child->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildRef::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_display && m_child )
	{
		m_child->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildRef::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_child )
	{
		EveChildUpdateParams newParams = params;
		newParams.isVisible &= m_display;
		newParams.childParent = this;
		newParams.localToWorldTransform = m_worldTransform;

		m_child->UpdateSyncronous( updateContext, newParams );
	}
}

void EveChildRef::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform = params.localToWorldTransform;

	UpdateTransform( localToWorldTransform );

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;
	newParams.localToWorldTransform = m_worldTransform;

	if( m_child )
	{
		m_child->UpdateAsyncronous( updateContext, newParams );
	}
}

void EveChildRef::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildRef::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( m_child.p ) )
	{
		owner->PlayCurveSet( name, rangeName );
	}
}

void EveChildRef::StopCurveSet( const std::string& name )
{
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( m_child.p ) )
	{
		owner->StopCurveSet( name );
	}
}

void EveChildRef::UpdateCurveSet( const std::string& name, Be::Time time )
{
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( m_child.p ) )
	{
		owner->UpdateCurveSet( name, time );
	}
}

float EveChildRef::GetCurveSetDuration( const std::string& name ) const
{
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( m_child.p ) )
	{
		owner->GetCurveSetDuration( name );
	}
	return 0.f;
}

float EveChildRef::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( m_child.p ) )
	{
		owner->GetRangeDuration( name, rangeName );
	}
	return 0.f;
}

void EveChildRef::PlayAllCurveSets()
{
	if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( m_child.p ) )
	{
		owner->PlayAllCurveSets();
	}
}

void EveChildRef::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( m_child )
	{
		return m_child->SetShaderOption( name, value );
	}
}

void EveChildRef::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildRef::ChangeLOD( Tr2Lod lod )
{
	if( m_child )
	{
		m_child->ChangeLOD( lod );
	}
};

void EveChildRef::SetControllerVariable( const char* name, float value )
{
	if( m_child )
	{
		m_child->SetControllerVariable( name, value );
	}
};

void EveChildRef::HandleControllerEvent( const char* name )
{
	if( m_child )
	{
		m_child->HandleControllerEvent( name );
	}
};

void EveChildRef::SetInheritProperties( const Color* colorSet )
{
	if( m_child )
	{
		if( IEveInheritPropertiesOwnerPtr child = BlueCastPtr( m_child ) )
		{
			child->SetInheritProperties( colorSet );
		}
	}
};

void EveChildRef::StartControllers()
{
	if( m_child )
	{
		m_child->StartControllers();
	}
};

void EveChildRef::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( m_child.p ) )
	{
		renderable->GetDebugOptions( options );
	}
}

void EveChildRef::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}
	if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( m_child.p ) )
	{
		renderable->RenderDebugInfo( renderer );
	}
}

bool EveChildRef::LoadChild()
{
	// unregister the old child
	UnRegisterComponents();

	CCP_LOG( "Loading child red file %s", m_resPath.c_str() );
	m_child = BeResMan->LoadObject<IEveSpaceObjectChild>( m_resPath.c_str() );
	if( !m_child )
	{
		CCP_LOGERR( "Red file %s is invalid or not an Eve Child type.", m_resPath.c_str() );
		return false;
	}

	RegisterComponents();

	return true;
}

ITr2AudEmitterPtr EveChildRef::FindSoundEmitter( const char* name )
{
	if( auto owner = dynamic_cast<ITr2SoundEmitterOwner*>( m_child.p ) )
	{
		return owner->FindSoundEmitter( name );
	}
	return nullptr;
}