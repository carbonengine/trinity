////////////////////////////////////////////////////////////
//
//    Created:   June 2015
//    Copyright: CCP 2015
//
#include "StdAfx.h"
#include "EveChildContainer.h"

#include "Utilities/BoundingSphere.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Renderer.h"
#include "Eve/EveUpdateContext.h"
#include "TriObserverLocal.h"
#include "Tr2LightManager.h"
#include "Tr2PointLight.h"
#include "Controllers/ITr2Controller.h"


EveChildContainer::EveChildContainer( IRoot* lockobj ) :
	EveChildTransform(),
	PARENTLOCK( m_objects ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_transformModifiers ),
	PARENTLOCK( m_controllers ),
	m_controllerVariables( "EveChildContainer::m_controllerVariables" ),
	m_worldVelocity( 0, 0, 0 ),
	m_display( true ),
	m_hideOnLowQuality( false )
{
	m_controllers.SetNotify( this );
}

EveChildContainer::~EveChildContainer()
{
}

bool EveChildContainer::Initialize()
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Link( *GetRawRoot() );
	}
	return true;
}

void EveChildContainer::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
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
		default:
			break;
		}
	}
}

void EveChildContainer::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	for ( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		IEveSpaceObjectChild *child = *it;
		child->SetShaderOption( name, value );
	}
}

const char* EveChildContainer::GetName() const
{
	return m_name.c_str();
}

void EveChildContainer::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildContainer::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateVisibility( frustum, parentTransform, parentLod );
	}
}

void EveChildContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->GetRenderables( renderables );
	}
}

bool EveChildContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return false;
	}

	bool success = false;
	Vector4 bSphere( 0.f, 0.f, 0.f, -1.f );
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( (*it)->GetBoundingSphere( bSphere ) )
		{
			BoundingSphereSetOrUpdate( bSphere, sphere, success );
			success = true;
		}
	}
	return success;
}

void EveChildContainer::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildContainer::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if (!m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildContainer::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateSyncronous( updateContext, newParams );
	}
	for( auto it = m_observers.begin(); it != m_observers.end(); it++ )
	{
		(*it)->Update( m_worldTransform );
	}
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Update();
	}
}

void EveChildContainer::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	Matrix localToWorldTransform;
	if( params.childParent )
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if( params.spaceObjectParent )
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else 
	{
		return;
	}

	UpdateTransform( localToWorldTransform );
	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); it++ )
	{
		m_worldTransform = (*it)->ApplyTransform( m_worldTransform, params.boneCount, params.bones );
	}

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->UpdateAsyncronous( updateContext, newParams );
	}
	
	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Update( time, time );
	}

	if( params.spaceObjectParent && !params.childParent )
	{
		params.spaceObjectParent->GetWorldVelocity( m_worldVelocity );
	}
}


void EveChildContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildContainer::ChangeLOD( Tr2Lod lod )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		(*it)->ChangeLOD( lod );
	}
}

void EveChildContainer::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}
	XMMATRIX worldTransform = m_worldTransform;
	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetX() ), 
		XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetY() ), XMVector3LengthEst( m_worldTransform.GetZ() ) ) ) ) / 3.f;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, worldTransform, scaling );
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		( *it )->GetLights( lightManager );
	}
}

void EveChildContainer::SetOrigin( Origin origin ) 
{
	m_origin = origin;
}

void EveChildContainer::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			if( rangeName.empty() )
			{
				( *it )->ResetTimeRange();
				( *it )->Play();
			}
			else
			{
				( *it )->PlayTimeRange( rangeName.c_str() );
			}
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->PlayCurveSet( name, rangeName );
		}
	}
}

void EveChildContainer::PlayAllCurveSets()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Play();
	}
}

void EveChildContainer::StopAllCurveSets()
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Stop();
	}
}

void EveChildContainer::StopCurveSet( const std::string& name )
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return;
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Stop();
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->StopCurveSet( name );
		}
	}
}

void EveChildContainer::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( (*it)->GetName() == name )
		{
			(*it)->Update( time, time );
		}
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			owner->UpdateCurveSet( name, time );
		}
	}
}

float EveChildContainer::GetCurveSetDuration( const std::string& name ) const
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return 0.f;
	}

	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetMaxCurveDuration() );
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			maxDuration = max( maxDuration, owner->GetCurveSetDuration( name ) );
		}
	}

	return maxDuration;
}

float EveChildContainer::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	if( m_hideOnLowQuality && Tr2Renderer::IsLowQuality() )
	{
		return 0.f;
	}

	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetRangeDuration( rangeName.c_str() ) );
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *it ) )
		{
			maxDuration = max( maxDuration, owner->GetRangeDuration( name, rangeName ) );
		}
	}

	return maxDuration;
}

void EveChildContainer::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildContainer::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->GetDebugOptions( options );
		}
	}
}

void EveChildContainer::RenderDebugInfo( Tr2DebugRenderer& renderer )
{
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( renderer );
		}
	}
}

void EveChildContainer::SetControllerVariable( const char* name, float value )
{
	m_controllerVariables[name] = value;
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->SetVariable( name, value );
	}
}

void EveChildContainer::StartControllers()
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Start();
	}
}

IEveSpaceObjectChildPtr EveChildContainer::GetEffectChildByName( const char* name ) const
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

void EveChildContainer::AddToEffectChildrenList( IEveSpaceObjectChild* child )
{
	m_objects.Append( child->GetRootObject() );
}

void EveChildContainer::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
	auto index = m_objects.FindKey( child );
	if( index >= 0 )
	{
		m_objects.Remove( index );
	}
}

void EveChildContainer::GetWorldVelocity( Vector3& velocity ) const
{
	velocity = m_worldVelocity;
}

void EveChildContainer::SetInheritProperties( const Color* colorSet )
{
	if ( !m_inheritProperties )
	{
		m_inheritProperties.CreateInstance();
	}
	m_inheritProperties->SetProperties( colorSet );
}

ITr2SoundEmitter* EveChildContainer::FindSoundEmitter( const char* name )
{
	for( auto it = begin( m_observers ); it != end( m_observers ); ++it )
	{
		auto observer = *it;
		if( observer->m_name == name )
		{
			ITr2SoundEmitterPtr listener = BlueCastPtr( observer->GetObserver() );
			return listener;
		}
	}
	return nullptr;
}