////////////////////////////////////////////////////////////
//
//    Created:   May 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "EveMultiEffect.h"
#include "EveMultiEffectParameter.h"
#include "Tr2DynamicBinding.h"
#include "Controllers/ITr2Controller.h"
#include "Eve/EveUpdateContext.h"
#include "Curves/TriCurveSet.h"

EveMultiEffect::EveMultiEffect( IRoot* lockobj ) : 
	PARENTLOCK( m_parameters ),
	PARENTLOCK( m_bindings ),
	PARENTLOCK( m_externalParameters ),
	PARENTLOCK( m_controllers ),
	PARENTLOCK( m_curveSets )
{
	m_bindings.SetNotify( this );
	m_parameters.SetNotify( this );
	m_controllers.SetNotify( this );
}

std::unordered_map<std::string, IRoot*> EveMultiEffect::GetParameterMap() const
{
	std::unordered_map<std::string, IRoot*> parameterMap;
	for( auto parameter = m_parameters.begin(); parameter != m_parameters.end(); ++parameter )
	{
		auto p = ( *parameter );
		parameterMap[p->GetName().c_str()] = p->GetParameterObject();
	}

	for( auto curveset = m_curveSets.begin(); curveset != m_curveSets.end(); ++curveset )
	{
		auto c = ( *curveset );
		parameterMap[c->GetName().c_str()] = c->GetRawRoot();
	}
	parameterMap["Owner"] = this->GetRawRoot();
	return parameterMap;
}

void EveMultiEffect::Rebind( bool onlyUpdateBindings )
{
	for( auto binding = m_bindings.begin(); binding != m_bindings.end(); ++binding )
	{
		( *binding )->Link();
		const Be::Time time = 0;
		( *binding )->Update( time );  
	}

	if( onlyUpdateBindings )
	{
		return;
	}

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Link( *GetRawRoot() );
	}
}

bool EveMultiEffect::Initialize()
{
	for( auto p = m_parameters.begin(); p != m_parameters.end(); ++p )
	{
		( *p )->SetOwner( this );
	}
	for( auto b = m_bindings.begin(); b != m_bindings.end(); ++b )
	{
		( *b )->SetOwner( this );
	}

	Rebind();
	return true;
}

void EveMultiEffect::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_parameters )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( EveMultiEffectParameterPtr parameter = BlueCastPtr( value ) )
			{
				parameter->SetOwner( this );
			}
			break;
		case BELIST_REMOVED:
			if( EveMultiEffectParameterPtr parameter = BlueCastPtr( value ) )
			{
				parameter->SetOwner( nullptr );
			}
			break;
		default:
			break;
		}
	}
	else if( list == &m_bindings )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( Tr2DynamicBindingPtr binding = BlueCastPtr( value ) )
			{
				binding->SetOwner( this );
			}
			break;
		case BELIST_REMOVED:
			if( Tr2DynamicBindingPtr binding = BlueCastPtr( value ) )
			{
				binding->SetOwner( nullptr );
			}
			break;
		default:
			break;
		}
	}
	else if( list == &m_controllers && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( ITr2ControllerPtr controller = BlueCastPtr( value ) )
			{
				controller->Link( *GetRawRoot() );
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

	Rebind();
}

bool EveMultiEffect::SetParameter( BlueSharedString parameterName, IRoot* object )
{
	for( auto param = m_parameters.begin(); param != m_parameters.end(); ++param )
	{
		if( ( *param )->GetName() == parameterName ) 
		{
			( *param )->SetParameterObject( object );
			
			Rebind();
			return true;
		}
	}
	return false;
}

EveMultiEffectParameter* EveMultiEffect::GetParameterByName( BlueSharedString parameterName )
{
	for( auto param = m_parameters.begin(); param != m_parameters.end(); ++param )
	{
		if( (*param)->GetName() == parameterName )
		{
			return *param;
		}
	}
	return nullptr;
}

// -----------------------------------------------------------------------------
void EveMultiEffect::SetControllerVariable( const char* name, float value )
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->SetVariable( name, value );
	}
}

// -----------------------------------------------------------------------------
void EveMultiEffect::HandleControllerEvent( const char* name )
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->HandleEvent( name );
	}
}

// -----------------------------------------------------------------------------
void EveMultiEffect::StartControllers()
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Start();
	}
}

void EveMultiEffect::GetBindingRoots( std::unordered_map<std::string, IRoot*>& variables )
{
	ITr2ControllerOwner::GetBindingRoots( variables );
	for( auto param = begin( m_parameters ); param != end( m_parameters ); ++param )
	{
		variables[ ( *param )->GetName().c_str() ] = ( *param )->GetParameterObject();
	}
}


// -----------------------------------------------------------------------------
void EveMultiEffect::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
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
}

// -----------------------------------------------------------------------------
void EveMultiEffect::StopCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Stop();
		}
	}
}

// -----------------------------------------------------------------------------
void EveMultiEffect::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Update( time, time );
		}
	}
}

// -----------------------------------------------------------------------------
float EveMultiEffect::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetMaxCurveDuration() );
		}
	}
	return maxDuration;
}

// -----------------------------------------------------------------------------
float EveMultiEffect::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetRangeDuration( rangeName.c_str() ) );
		}
	}
	return maxDuration;
}


/////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObject2
void EveMultiEffect::UpdateSyncronous( EveUpdateContext& updateContext ) 
{
	Be::Time time = updateContext.GetTime();

	// Update all space objects
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		( *it )->Update( time, time );
	}

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Update();
	}

	for( auto binding = m_bindings.begin(); binding != m_bindings.end(); ++binding )
	{
		( *binding )->Update( time );
	}
}


void EveMultiEffect::UpdateAsyncronous( EveUpdateContext& updateContext ) {}
void EveMultiEffect::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform ) {}
void EveMultiEffect::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors ) {}
bool EveMultiEffect::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const { return false; }
void EveMultiEffect::GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const {}
void EveMultiEffect::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t ) {}
void EveMultiEffect::GetModelCenterWorldPosition( Vector3 &position ) const {}
bool EveMultiEffect::GetLocalBoundingBox( Vector3 &min, Vector3 &max ) { return false; }
void EveMultiEffect::GetLocalToWorldTransform( Matrix &transform ) const {}
void EveMultiEffect::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer ) {}
void EveMultiEffect::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) {}