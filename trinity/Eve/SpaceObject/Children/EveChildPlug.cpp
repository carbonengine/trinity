// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveChildPlug.h"

#include "Utilities/BoundingSphere.h"
#include "Eve/EveUpdateContext.h"
#include "Tr2ExternalParameter.h"
#include "Controllers/ITr2Controller.h"


EveChildPlug::EveChildPlug( IRoot* lockobj ) :
	PARENTLOCK( m_objects ),
	PARENTLOCK( m_externalParameters ),
	PARENTLOCK( m_controllers ),
	m_worldTransform( IdentityMatrix() ),
	m_display( true )
{
	m_controllers.SetNotify( this );
}

EveChildPlug::~EveChildPlug()
{
	for( auto& controller : m_controllers )
	{
		controller->Unlink( UnlinkReason::DELETING );
	}
}

bool EveChildPlug::Initialize()
{
	for( auto& controller : m_controllers )
	{
		if( !controller->IsLinked() )
		{
			controller->Link( *GetRawRoot() );
		}
	}
	return true;
}

void EveChildPlug::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_controllers && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( ITr2ControllerPtr controller = BlueCastPtr( value ) )
			{
				controller->Link( *GetRawRoot() );
				for( auto it = begin( m_controllerVariables ); it != end( m_controllerVariables ); ++it )
				{
					controller->SetVariable( it->first.c_str(), it->second );
				}
			}
			break;
		case BELIST_REMOVED:
			if( ITr2ControllerPtr controller = BlueCastPtr( value ) )
			{
				controller->Unlink();
			}
			break;
		case BELIST_UNLOADSTART:
			for( auto& controller : m_controllers )
			{
				controller->Unlink();
			}
			break;
		default:
			break;
		}
	}
	else if( list == &m_objects && ( event & BELIST_LOADING ) == 0 )
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

bool EveChildPlug::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_display ) )
	{
		ReRegister();
	}
	return true;
}

// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildPlug::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->Register( registry );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//    Unregisters itself and its children with the scene registration container.
// --------------------------------------------------------------------------------
void EveChildPlug::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->UnRegister( registry );
			}
		}
	}
}

const char* EveChildPlug::GetName() const
{
	return m_name.c_str();
}

void EveChildPlug::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildPlug::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->UpdateVisibility( updateContext, parentTransform, parentLod );
	}
}

void EveChildPlug::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->GetRenderables( renderables );
	}
}

bool EveChildPlug::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool success = false;
	Vector4 bSphere( 0.f, 0.f, 0.f, -1.f );
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( ( *it )->GetBoundingSphere( bSphere ) )
		{
			BoundingSphereSetOrUpdate( bSphere, sphere, success );
			success = true;
		}
	}
	return success;
}

void EveChildPlug::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildPlug::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_display )
	{
		return;
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildPlug::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->UpdateSyncronous( updateContext, newParams );
	}
}

void EveChildPlug::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_worldTransform = params.localToWorldTransform;

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->UpdateAsyncronous( updateContext, newParams );
	}
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Update( params.controllerUpdateFrequency );
	}
}

void EveChildPlug::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildPlug::ChangeLOD( Tr2Lod lod )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->ChangeLOD( lod );
	}
}

void EveChildPlug::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		IEveSpaceObjectChild* child = *it;
		child->SetShaderOption( name, value );
	}
}

void EveChildPlug::PlayAllCurveSets()
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->PlayAllCurveSets();
		}
	}
}

void EveChildPlug::StopAllCurveSets()
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->StopAllCurveSets();
		}
	}
}

void EveChildPlug::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->PlayCurveSet( name, rangeName );
		}
	}
}

void EveChildPlug::StopCurveSet( const std::string& name )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->StopCurveSet( name );
		}
	}
}

void EveChildPlug::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->UpdateCurveSet( name, time );
		}
	}
}

float EveChildPlug::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			maxDuration = max( maxDuration, owner->GetCurveSetDuration( name ) );
		}
	}

	return maxDuration;
}

float EveChildPlug::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	float maxDuration = 0.f;
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			maxDuration = max( maxDuration, owner->GetRangeDuration( name, rangeName ) );
		}
	}

	return maxDuration;
}

void EveChildPlug::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
}

void EveChildPlug::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->GetDebugOptions( options );
		}
	}
}

void EveChildPlug::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( renderer );
		}
	}
}

void EveChildPlug::SetControllerVariable( const char* name, float value )
{
	auto found = find_if( begin( m_controllerVariables ), end( m_controllerVariables ), [name]( auto& x ) { return x.first == name; } );
	if( found == end( m_controllerVariables ) )
	{
		m_controllerVariables.push_back( { name, value } );
	}
	else
	{
		found->second = value;
	}
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->SetVariable( name, value );
	}
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		( *it )->SetControllerVariable( name, value );
	}
}

void EveChildPlug::HandleControllerEvent( const char* name )
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->HandleEvent( name );
	}
}

void EveChildPlug::StartControllers()
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Start();
	}
}

IEveSpaceObjectChildPtr EveChildPlug::GetEffectChildByName( const char* name ) const
{
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		auto child = *it;
		if( strcmp( child->GetName(), name ) == 0 )
		{
			return child;
		}
	}
	return nullptr;
}

void EveChildPlug::AddToEffectChildrenList( IEveSpaceObjectChild* child )
{
	if( IsInRegistry() )
	{
		if( EveEntityPtr entity = BlueCastPtr( child->GetRootObject() ) )
		{
			entity->Register( GetComponentRegistry() );
		}
	}
	m_objects.Append( child->GetRootObject() );
}

void EveChildPlug::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
	auto index = m_objects.FindKey( child );
	if( index >= 0 )
	{
		if( IsInRegistry() )
		{
			if( EveEntityPtr entity = BlueCastPtr( m_objects[index] ) )
			{
				entity->UnRegister( GetComponentRegistry() );
			}
		}
		m_objects.Remove( index );
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Adds an external parameter to the external parameters list.
// --------------------------------------------------------------------------------
void EveChildPlug::AddExternalParameter( Tr2ExternalParameter* externalParameter )
{
	m_externalParameters.Append( externalParameter->GetRawRoot() );
}

const PTr2ExternalParameterVector& EveChildPlug::GetExternalParameters() const
{
	return m_externalParameters;
}

void EveChildPlug::SetInheritProperties( const Color* colorSet )
{
	if( !m_inheritProperties )
	{
		m_inheritProperties.CreateInstance();
	}
	m_inheritProperties->SetProperties( colorSet );
}

ITr2AudEmitterPtr EveChildPlug::FindSoundEmitter( const char* name )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2SoundEmitterOwner*>( *it ) )
		{
			auto emitter = owner->FindSoundEmitter( name );
			if( emitter != nullptr )
			{
				return emitter;
			}
		}
	}
	return nullptr;
}
