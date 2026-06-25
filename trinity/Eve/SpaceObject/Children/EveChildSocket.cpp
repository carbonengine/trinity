// Copyright © 2019 CCP ehf.

#include "StdAfx.h"
#include "EveChildSocket.h"

#include <algorithm>

#include "EveChildPlug.h"
#include "Utilities/BoundingSphere.h"
#include "Eve/EveUpdateContext.h"
#include "Tr2ExternalParameter.h"
#include "SocketParameters/EveSocketParameter.h"


EveChildSocket::EveChildSocket( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	m_plug(),
	m_name(),
	m_plugResPath(),
	m_display( true )
{
}

EveChildSocket::~EveChildSocket()
{
}

const char* EveChildSocket::GetPlugResPath() const
{
	return m_plugResPath.c_str();
}

void EveChildSocket::SetPlugResPath( const char* resPath )
{
	if( m_plugResPath != resPath )
	{
		m_plugResPath = resPath;
		LoadChild();
	}
}

void EveChildSocket::Reload()
{
	Initialize();
}

bool Contains( std::string str, const char* term )
{
	transform( str.begin(), str.end(), str.begin(), ::tolower );
	return str.find( term ) != std::string::npos;
}

bool EveChildSocket::AddParameterForExternal( Tr2ExternalParameter& externalParam )
{
	auto destination = externalParam.GetDestinationEntry();
	if( !destination || !externalParam.IsValid() )
	{
		CCP_LOGWARN( "EveChildSocket: destinationObject is not set for one of the plug's external parameters." );
		return false;
	}

	EveSocketParameterBindingBasePtr ptr;
	switch( destination->mType )
	{
	case Be::BYTE:
	case Be::BOOL:
		ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterBool>::Class() );
		break;

	case Be::SHORT:
	case Be::LONG:
	case Be::INT64:
		ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterInt>::Class() );
		break;

	case Be::FLOAT:
	case Be::DOUBLE:
		ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterFloat>::Class() );
		break;

	case Be::FLOATARRAY:
		if( destination->GetFloatArraySize() == 2 )
		{
			ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterVector2>::Class() );
		}
		else if( destination->GetFloatArraySize() == 3 )
		{
			ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterVector3>::Class() );
		}
		else if( destination->GetFloatArraySize() == 4 )
		{
			if( Contains( destination->mName, "color" ) )
			{
				ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterColor>::Class() );
			}
			else
			{
				ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterVector4>::Class() );
			}
		}
		else
		{
			CCP_LOGWARN( "EveChildSocket: Unsupported value type." );
		}
		break;

	case Be::STDSTRING:
	case Be::STDWSTRING:
	case Be::CHARARRAY:
	case Be::CSTRING:
	case Be::WCSTRING:
		if( Contains( destination->mName, "path" ) )
		{
			ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterFilePath>::Class() );
		}
		else
		{
			ptr.CreateInstance( BlueClassTypeTraits<EveSocketParameterString>::Class() );
		}
		break;

	default:
		CCP_LOGWARN( "EveChildSocket: Unsupported value type." );
		break;
	}

	if( ptr )
	{
		ptr->SetName( externalParam.GetName() );
		ptr->BindToExternalParameter( externalParam );
		ptr->SetValueToDefault();
		m_parameters.Append( ptr );

		return true;
	}
	else
	{
		return false;
	}
}

void EveChildSocket::BindParameters()
{
	if( m_plug )
	{
		// clear out old bindings
		for( auto paramIt = begin( m_parameters ); paramIt != end( m_parameters ); ++paramIt )
		{
			( *paramIt )->ClearBindings();
		}

		// Attach all the new external params
		const PTr2ExternalParameterVector& externalParams = m_plug->GetExternalParameters();
		for( auto it = begin( externalParams ); it != end( externalParams ); ++it )
		{
			bool paramBound = false;
			for( auto paramIt = begin( m_parameters ); paramIt != end( m_parameters ); ++paramIt )
			{
				if( ( *paramIt )->BindToExternalParameter( **it ) )
				{
					paramBound = true;
					break;
				}
			}
			// No appropriate SocketParameter found, so make a new one if possible.
			if( !paramBound )
			{
				AddParameterForExternal( **it );
			}
		}
	}
}

void EveChildSocket::Propogate()
{
	if( m_plug )
	{
		for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
		{
			( *it )->Propagate();
		}
	}
}

bool EveChildSocket::Initialize()
{
	LoadChild();
	BindParameters();
	Propogate();

	return true;
}

bool EveChildSocket::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_plugResPath ) )
	{
		Initialize();
	}
	if( IsMatch( value, m_display ) )
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
void EveChildSocket::RegisterComponents()
{
	if( IsInRegistry() && m_plug != nullptr && m_display )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_plug ) )
		{
			entity->Register( this->GetComponentRegistry() );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//    Unregisters itself and its children with the scene registration container.
// --------------------------------------------------------------------------------
void EveChildSocket::UnRegisterComponents()
{
	if( IsInRegistry() && m_plug )
	{
		if( EveEntityPtr entity = BlueCastPtr( m_plug ) )
		{
			entity->UnRegister( this->GetComponentRegistry() );
		}
	}
}


const char* EveChildSocket::GetName() const
{
	return m_name.c_str();
}

void EveChildSocket::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

IEveSpaceObjectChildPtr EveChildSocket::GetEffectChildByName( const char* name ) const
{
	if( m_plug )
	{
		return m_plug->GetEffectChildByName( name );
	}
	return nullptr;
}

void EveChildSocket::AddToEffectChildrenList( IEveSpaceObjectChild* child )
{
}

void EveChildSocket::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
}

void EveChildSocket::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}
	if( m_plug )
	{
		m_plug->UpdateVisibility( updateContext, parentTransform, parentLod );
	}
}

void EveChildSocket::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( m_display && m_plug )
	{
		m_plug->GetRenderables( renderables );
	}
}

bool EveChildSocket::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool success = false;
	Vector4 bSphere( 0.f, 0.f, 0.f, -1.f );
	if( m_plug && m_plug->GetBoundingSphere( bSphere ) )
	{
		BoundingSphereSetOrUpdate( bSphere, sphere, success );
		success = true;
	}
	return success;
}

void EveChildSocket::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_plug )
	{
		m_plug->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildSocket::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( m_display && m_plug )
	{
		m_plug->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveChildSocket::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_plug )
	{
		for( auto it = begin( m_parameters ); it != end( m_parameters ); ++it )
		{
			( *it )->Propagate();
		}

		EveChildUpdateParams newParams = params;
		newParams.isVisible &= m_display;
		newParams.childParent = this;
		newParams.localToWorldTransform = m_worldTransform;

		m_plug->UpdateSyncronous( updateContext, newParams );
	}
}

void EveChildSocket::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform = params.localToWorldTransform;

	UpdateTransform( localToWorldTransform );

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;
	newParams.localToWorldTransform = m_worldTransform;

	if( m_plug )
	{
		m_plug->UpdateAsyncronous( updateContext, newParams );
	}
}

void EveChildSocket::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildSocket::PlayAllCurveSets()
{
	if( m_plug )
	{
		m_plug->PlayAllCurveSets();
	}
}

void EveChildSocket::StopAllCurveSets()
{
	if( m_plug )
	{
		m_plug->StopAllCurveSets();
	}
}

void EveChildSocket::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	if( m_plug )
	{
		m_plug->PlayCurveSet( name, rangeName );
	}
}

void EveChildSocket::StopCurveSet( const std::string& name )
{
	if( m_plug )
	{
		m_plug->StopCurveSet( name );
	}
}

void EveChildSocket::UpdateCurveSet( const std::string& name, Be::Time time )
{
	if( m_plug )
	{
		m_plug->UpdateCurveSet( name, time );
	}
}

float EveChildSocket::GetCurveSetDuration( const std::string& name ) const
{
	if( m_plug )
	{
		return m_plug->GetCurveSetDuration( name );
	}
	return 0.f;
}

float EveChildSocket::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	if( m_plug )
	{
		return m_plug->GetRangeDuration( name, rangeName );
	}
	return 0.f;
}

void EveChildSocket::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	if( m_plug )
	{
		return m_plug->SetShaderOption( name, value );
	}
}

void EveChildSocket::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildSocket::ChangeLOD( Tr2Lod lod )
{
	if( m_plug )
	{
		m_plug->ChangeLOD( lod );
	}
};

void EveChildSocket::SetControllerVariable( const char* name, float value )
{
	if( m_plug )
	{
		m_plug->SetControllerVariable( name, value );
	}
};

void EveChildSocket::HandleControllerEvent( const char* name )
{
	if( m_plug )
	{
		m_plug->HandleControllerEvent( name );
	}
};

void EveChildSocket::SetInheritProperties( const Color* colorSet )
{
	if( m_plug )
	{
		m_plug->SetInheritProperties( colorSet );
	}
};

void EveChildSocket::StartControllers()
{
	if( m_plug )
	{
		m_plug->StartControllers();
	}
};

void EveChildSocket::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	if( !m_plug )
	{
		return;
	}
	if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( &*m_plug ) )
	{
		renderable->GetDebugOptions( options );
	}
}

void EveChildSocket::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display || !m_plug )
	{
		return;
	}
	if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( &*m_plug ) )
	{
		renderable->RenderDebugInfo( renderer );
	}
}

bool EveChildSocket::LoadChild()
{
	// unregister the current child
	UnRegisterComponents();

	CCP_LOG( "Loading child red file %s", m_plugResPath.c_str() );
	m_plug = BeResMan->LoadObject<EveChildPlug>( m_plugResPath.c_str() );
	if( !m_plug )
	{
		CCP_LOGERR( "Red file %s is invalid or not an Eve Child type.", m_plugResPath.c_str() );
		return false;
	}

	RegisterComponents();

	return true;
}

ITr2AudEmitterPtr EveChildSocket::FindSoundEmitter( const char* name )
{
	if( m_plug )
	{
		return m_plug->FindSoundEmitter( name );
	}
	return nullptr;
}
