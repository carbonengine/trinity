// Copyright © 2015 CCP ehf.

#include "StdAfx.h"
#include "EveChildContainer.h"

#include "Utilities/BoundingSphere.h"
#include "Curves/TriCurveSet.h"
#include "Tr2Renderer.h"
#include "Eve/EveUpdateContext.h"
#include "TriObserverLocal.h"
#include "Tr2LightManager.h"
#include "Lights/Tr2PointLight.h"
#include "Controllers/ITr2Controller.h"
#include "Controllers/Tr2Controller.h"
#include "Controllers/Tr2ControllerFloatVariable.h"
#include "Eve/SpaceObject/Utils/fxAttributes/IEveFxAttribute.h"
#include "ITr2SoundEmitterOwner.h"


EveChildContainer::EveChildContainer( IRoot* lockobj ) :
	EveChildTransform(),
	EveEntity( lockobj ),
	PARENTLOCK( m_objects ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_transformModifiers ),
	PARENTLOCK( m_controllers ),
	PARENTLOCK( m_fxAttributes ),
	PARENTLOCK( m_attachments ),
	m_displayFilter( SHADER_ALL ),
	m_worldVelocity( 0, 0, 0 ),
	m_display( true ),
	m_updateOnDisplay( true ),
	m_mute( false ),
	m_isAlwaysOn( false ),
	m_isPlacementRoot( false ),
	m_ownerMaxSpeed( 0 ),
	m_origin( SPACE ),
	m_activationStrength( 1.0f )
{
	m_controllers.SetNotify( this );
	m_objects.SetNotify( this );
	m_attachments.SetNotify( this );
	m_lights.SetNotify( this );

	memset( &m_vsData, 0, sizeof( EveSpaceObjectVSData ) );
	memset( &m_psData, 0, sizeof( EveSpaceObjectPSData ) );
	m_vsData.shipData.y = 1.f;
	m_vsData.shipData.w = 1.f;
	m_psData.shipData.y = 1.f;
	m_psData.shipData.w = 1.f;
	m_psData.screenSize = Vector4( 0.5f, 0.5f, 0.5f, 1.f );
}

EveChildContainer::~EveChildContainer()
{
	for( auto& controller : m_controllers )
	{
		controller->Unlink( UnlinkReason::DELETING );
	}
}

bool EveChildContainer::Initialize()
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

bool EveChildContainer::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_display ) || IsMatch( val, m_displayFilter ) )
	{
		ReRegister();
	}
	else if( IsMatch( val, m_mute ) )
	{
		MuteChildren();
	}

	return true;
}

void EveChildContainer::SetMute( bool isMute )
{
	if( isMute != m_mute )
	{
		m_mute = isMute;
		MuteChildren();
	}
}

void EveChildContainer::MuteChildren()
{
	for( auto it : m_objects )
	{
		it->SetMute( m_mute );
	}
	for( auto it : m_observers )
	{
		it->SetMute( m_mute );
	}
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

	if( list == &m_attachments && ( event & BELIST_LOADING ) == 0 )
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

	if( list == &m_objects && ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
	{
		if( m_inheritProperties )
		{
			if( IEveInheritPropertiesOwnerPtr obj = BlueCastPtr( value ) )
			{
				obj->SetInheritProperties( m_inheritProperties->GetProperties() );
			}
		}
	}

	if( list == &m_lights && ( event & BELIST_EVENTMASK ) == BELIST_INSERTED )
	{
		if( m_inheritProperties )
		{
			if( IEveInheritPropertiesOwnerPtr light = BlueCastPtr( value ) )
			{
				light->SetInheritProperties( m_inheritProperties->GetProperties() );
			}
		}
	}

	if( list == &m_lights )
	{
		auto maskedEvent = event & BELIST_EVENTMASK;
		if( ( maskedEvent == BELIST_UNLOADSTART ) || ( ( maskedEvent == BELIST_REMOVED ) && m_lights.empty() ) )
		{
			auto registry = this->GetComponentRegistry();
			if( registry )
			{
				registry->UnRegisterComponent<ITr2LightOwner>( this );
			}
		}
		else if( ( maskedEvent == BELIST_INSERTED ) && m_lights.size() == 1 )
		{
			auto registry = this->GetComponentRegistry();
			if( registry )
			{
				registry->RegisterComponent<ITr2LightOwner>( this );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//    Registers itself and its children with the scene registration container.
//    This is so we don't have to traverse the tree every frame
// --------------------------------------------------------------------------------
void EveChildContainer::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display && IsUpdating() )
	{
		if( !m_lights.empty() )
		{
			registry->RegisterComponent<ITr2LightOwner>( this );
		}

		for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->Register( registry );
			}
		}

		for( auto& it : m_attachments )
		{
			if( EveEntityPtr entity = BlueCastPtr( it ) )
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
void EveChildContainer::UnRegisterComponents()
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

		for( auto& it : m_attachments )
		{
			if( EveEntityPtr entity = BlueCastPtr( it ) )
			{
				entity->UnRegister( registry );
			}
		}
	}
}

void EveChildContainer::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); ++it )
	{
		IEveSpaceObjectChild* child = *it;
		child->SetShaderOption( name, value );
	}

	for( auto& attachment : m_attachments )
	{
		attachment->SetShaderOption( name, value );
	}
}

bool EveChildContainer::IsAlwaysOn() const
{
	return m_isAlwaysOn;
}


// use: Check if the childs should be rendered on the current shader settings
bool EveChildContainer::IsRendering() const
{

	TR2SHADERMODEL settings = Tr2Renderer::GetShaderModel();

	if( settings == TR2SM_AUTHORING && m_displayFilter != ONLY_REFLECTIONS )
	{
		return true;
	}

	switch( m_displayFilter )
	{
	case SHADER_LOW:
		return settings == TR2SM_3_0_LO;
	case SHADER_LOWMID:
		return settings <= TR2SM_3_0_HI;
	case SHADER_MED:
		return settings == TR2SM_3_0_HI;
	case SHADER_HIGHMID:
		return settings >= TR2SM_3_0_HI;
	case SHADER_HIGH:
		return settings == TR2SM_3_0_DEPTH;
	case SHADER_ALL:
		return true;
	case ONLY_REFLECTIONS:
		return false;
	}
	return false;
}

bool EveChildContainer::IsUpdating() const
{
	return ( m_display || !m_updateOnDisplay ) && ( IsRendering() || m_displayFilter == ONLY_REFLECTIONS );
}

const char* EveChildContainer::GetName() const
{
	return m_name.c_str();
}

void EveChildContainer::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildContainer::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

	if( !IsUpdating() )
	{
		return;
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->UpdateVisibility( updateContext, parentTransform, parentLod );
	}

	if( HasRenderables() )
	{
		size_t boneCount = 0;
		const Float4x3* bones = nullptr;
		if( m_animationOwner && m_animationOwner->GetAnimationController() )
		{
			Tr2GrannyAnimationUtils::GetBoneList( m_animationOwner->GetAnimationController(), bones, boneCount );
		}

		for( auto it = begin( m_attachments ); it != end( m_attachments ); ++it )
		{
			( *it )->UpdateVisibility( updateContext, m_worldTransform, bones, boneCount );
		}
	}
}

void EveChildContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display || !m_hasUpdated )
	{
		return;
	}
	if( !IsRendering() )
	{
		return;
	}

	if( HasRenderables() )
	{
		renderables.push_back( this );
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->GetRenderables( renderables );
	}
}

bool EveChildContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( !IsUpdating() )
	{
		return false;
	}

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

void EveChildContainer::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
	for( auto& it : m_attachments )
	{
		it->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildContainer::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_display || !m_hasUpdated )
	{
		return;
	}
	if( !IsRendering() )
	{
		return;
	}
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
	if( !m_attachments.empty() )
	{
		size_t boneCount = 0;
		const Float4x3* bones = nullptr;

		if( m_animationOwner && m_animationOwner->GetAnimationController() )
		{
			Tr2GrannyAnimationUtils::GetBoneList( m_animationOwner->GetAnimationController(), bones, boneCount );
		}
		for( auto& it : m_attachments )
		{
			it->AddToQuadRenderer( quadRenderer, m_worldTransform, 1.0, 0.0, bones, boneCount );
		}
	}
}

void EveChildContainer::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( !IsUpdating() )
	{
		return;
	}

	m_ownerMaxSpeed = params.ownerMaxSpeed;

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;
	newParams.localToWorldTransform = m_worldTransform;

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->UpdateSyncronous( updateContext, newParams );
	}
	for( auto it = m_observers.begin(); it != m_observers.end(); it++ )
	{
		( *it )->Update( m_worldTransform );
	}
}

void EveChildContainer::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	m_boneOffsets.AdvanceFrame();

	if( !IsUpdating() )
	{
		return;
	}
	if( m_isPlacementRoot && updateContext.GetTaskGroup() )
	{
		updateContext.GetTaskGroup()->run( [this, &updateContext, params] {
			CCP_STATS_ZONE( "Placement Child UpdateAsyncronous" );
			this->DoUpdateAsyncronous( updateContext, params );
		} );
	}
	else
	{
		DoUpdateAsyncronous( updateContext, params );
	}
}

void EveChildContainer::DoUpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	Matrix lastWorldTransform = m_worldTransform;
	Matrix localToWorldTransform = params.localToWorldTransform;

	UpdateTransform( localToWorldTransform );

	auto boneCount = params.boneCount;
	auto bones = params.bones;

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Update( params.controllerUpdateFrequency );
	}

	if( m_animationOwner && m_animationOwner->GetAnimationController() )
	{
		Tr2GrannyAnimationUtils::GetBoneList( m_animationOwner->GetAnimationController(), bones, boneCount );
	}

	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); it++ )
	{
		m_worldTransform = ( *it )->ApplyTransform( m_worldTransform, boneCount, bones );
	}

	m_activationStrength = params.activationStrength;

	if( HasRenderables() )
	{
		m_perObjectDataVs.InvalidateBufferData();
		m_perObjectDataPs.InvalidateBufferData();

		// need to update the data we get from the parent to be relevant to us!
		if( nullptr != params.spaceObjectParent )
		{
			params.spaceObjectParent->GetPerObjectStructs( m_vsData, m_psData );

			// need to move the clipdata inversely of the translation of the childmesh
			m_vsData.clipData = Vector4( m_vsData.clipData.GetXYZ() - m_translation, m_vsData.clipData.w );
			m_psData.clipSphereCenter = m_psData.clipSphereCenter - m_translation;
		}

		m_vsData.worldTransform = Transpose( m_worldTransform );
		m_vsData.invWorldTransform = Inverse( m_vsData.worldTransform );
		m_vsData.worldTransformLast = Transpose( lastWorldTransform );

		m_psData.worldTransform = m_vsData.worldTransform;
		m_psData.worldTransformLast = m_vsData.worldTransformLast;
		m_psData.invWorldTransform = m_vsData.invWorldTransform;
	}

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;
	newParams.boneCount = boneCount;
	newParams.bones = bones;
	newParams.localToWorldTransform = m_worldTransform;

	if( params.spaceObjectParent )
	{
		if( !params.childParent )
		{
			params.spaceObjectParent->GetWorldVelocity( m_worldVelocity );
			newParams.worldVelocity = m_worldVelocity;
		}
		else
		{
			m_worldVelocity = params.worldVelocity;
		}
	}

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->UpdateAsyncronous( updateContext, newParams );
	}

	if( !m_curveSets.empty() )
	{
		Be::Time time = updateContext.GetTime();
		for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
		{
			( *it )->Update( time, time );
		}
	}

	for( auto it = m_fxAttributes.begin(); it != m_fxAttributes.end(); it++ )
	{
		( *it )->UpdateAsyncronous( updateContext, newParams );
	}

	for( auto it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		( *it )->SetBoneMatrix( bones, boneCount );
	}

	for( auto& attachment : m_attachments )
	{
		attachment->UpdateLights( m_worldTransform, bones, boneCount, m_activationStrength, 0.0 );
	}
	m_hasUpdated = true;
}


void EveChildContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildContainer::ChangeLOD( Tr2Lod lod )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		( *it )->ChangeLOD( lod );
	}
}

void EveChildContainer::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display || !m_hasUpdated )
	{
		return;
	}
	if( !IsRendering() )
	{
		return;
	}
	XMMATRIX worldTransform = m_worldTransform;
	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetX() ),
											   XMVectorAdd( XMVector3LengthEst( m_worldTransform.GetY() ), XMVector3LengthEst( m_worldTransform.GetZ() ) ) ) ) /
		3.f;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, worldTransform, scaling );
	}
}

void EveChildContainer::SetOrigin( Origin origin )
{
	m_origin = origin;
}

void EveChildContainer::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	if( !IsUpdating() )
	{
		return;
	}

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
	for( auto cit = m_objects.begin(); cit != m_objects.end(); cit++ )
	{
		if( auto child = dynamic_cast<ITr2CurveSetOwner*>( *cit ) )
		{
			child->PlayAllCurveSets();
		}
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		( *it )->Play();
	}
}

void EveChildContainer::StopAllCurveSets()
{
	for( auto cit = m_objects.begin(); cit != m_objects.end(); cit++ )
	{
		if( auto child = dynamic_cast<ITr2CurveSetOwner*>( *cit ) )
		{
			child->StopAllCurveSets();
		}
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		( *it )->Stop();
	}
}

void EveChildContainer::StopCurveSet( const std::string& name )
{
	if( !IsUpdating() )
	{
		return;
	}

	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Stop();
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
		if( ( *it )->GetName() == name )
		{
			( *it )->Update( time, time );
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
	if( !IsUpdating() )
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
	if( !IsUpdating() )
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

void EveChildContainer::SetAlwaysOn( bool alwaysOn )
{
	m_isAlwaysOn = alwaysOn;
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
	options.insert( "Lights" );

	for( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->GetDebugOptions( options );
	}

	for( auto& it : m_attachments )
	{
		it->GetDebugOptions( options );
	}
}

void EveChildContainer::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}

	for( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->RenderDebugInfo( renderer );
	}

	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( renderer );
		}
	}

	if( renderer.HasOption( this, "Lights" ) )
	{
		for( auto it = begin( m_lights ); it != end( m_lights ); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if( !m_attachments.empty() )
	{
		size_t boneCount = 0;
		const Float4x3* bones = nullptr;

		if( m_animationOwner && m_animationOwner->GetAnimationController() )
		{
			Tr2GrannyAnimationUtils::GetBoneList( m_animationOwner->GetAnimationController(), bones, boneCount );
		}
		for( auto& it : m_attachments )
		{
			it->RenderDebugInfo( renderer, m_worldTransform, bones, boneCount );
		}
	}
}

void EveChildContainer::AddController( ITr2Controller* controller )
{
	m_controllers.Append( controller );
}

void EveChildContainer::SetControllerVariable( const char* name, float value )
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

void EveChildContainer::HandleControllerEvent( const char* name )
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->HandleEvent( name );
	}
	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		( *it )->HandleControllerEvent( name );
	}
}

void EveChildContainer::StartControllers()
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Start();
	}

	for( auto it = begin( m_objects ); it != end( m_objects ); ++it )
	{
		( *it )->StartControllers();
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
	auto childRoot = child->GetRootObject();
	m_objects.Append( childRoot );

	if( ITr2ControllerOwnerPtr ptr = BlueCastPtr( childRoot ) )
	{
		for( auto it = begin( m_controllerVariables ); it != end( m_controllerVariables ); ++it )
		{
			ptr->SetControllerVariable( it->first.c_str(), it->second );
		}
	}
}

void EveChildContainer::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
	auto index = m_objects.FindKey( child );
	if( index >= 0 )
	{
		if( IsInRegistry() )
		{
			if( EveEntityPtr entity = BlueCastPtr( m_objects.GetAt( index ) ) )
			{
				entity->UnRegister( GetComponentRegistry() );
			}
		}
		m_objects.Remove( index );
	}
}

void EveChildContainer::GetWorldVelocity( Vector3& velocity ) const
{
	velocity = m_worldVelocity;
}

void EveChildContainer::SetInheritProperties( const Color* colorSet )
{
	if( !m_inheritProperties )
	{
		m_inheritProperties.CreateInstance();
	}
	m_inheritProperties->SetProperties( colorSet );

	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		if( IEveInheritPropertiesOwnerPtr cast = BlueCastPtr( *it ) )
		{
			cast->SetInheritProperties( colorSet );
		}
	}

	for( auto it = m_lights.begin(); it != m_lights.end(); it++ )
	{
		if( IEveInheritPropertiesOwnerPtr light = BlueCastPtr( *it ) )
		{
			light->SetInheritProperties( colorSet );
		}
	}
}

ITr2AudEmitterPtr EveChildContainer::FindSoundEmitter( const char* name )
{
	for( auto it = begin( m_observers ); it != end( m_observers ); ++it )
	{
		auto observer = *it;
		if( observer->m_name == name )
		{
			ITr2AudEmitterPtr listener = BlueCastPtr( observer->GetObserver() );
			return listener;
		}
	}

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

void EveChildContainer::AddObserver( TriObserverLocalPtr observer )
{
	m_observers.Append( observer );
}

float EveChildContainer::GetOwnerMaxSpeed() const
{
	return m_ownerMaxSpeed;
}

void EveChildContainer::SetDisplayQualityModifier( DisplayQualityModifier filter )
{
	m_displayFilter = filter;
}

bool EveChildContainer::GetControllerValueByName( const char* name, float& out )
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		if( ITr2ActionControllerPtr controller = BlueCastPtr( *it ) )
		{
			if( auto var = controller->GetFloatVariableByName( name ) )
			{
				out = *var;
				return true;
			}
		}
	}

	return false;
}

void EveChildContainer::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
}

void EveChildContainer::SetProceduralContainerVariable( const char* name, float value )
{
	for( auto it = m_objects.begin(); it != m_objects.end(); it++ )
	{
		auto child = *it;
		child->SetProceduralContainerVariable( name, value );
	}
}

void EveChildContainer::SetAnimationOwner( ITr2GrannyAnimationOwner* animationOwner )
{
	m_animationOwner = animationOwner;
}

void EveChildContainer::AddAttachment( IEveSpaceObjectAttachment* attachment )
{
	m_attachments.Append( attachment );
}

void EveChildContainer::ClearAttachments()
{
	m_attachments.Clear();
}

bool EveChildContainer::HasRenderables() const
{
	return !m_attachments.empty();
}

void EveChildContainer::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{
	if( m_display && reason == TR2RENDERREASON_NORMAL && m_activationStrength != 0.0 )
	{
		for( auto& it : m_attachments )
		{
			it->GetBatches( batches, batchType, perObjectData, reason );
		}
	}
}

bool EveChildContainer::HasTransparentBatches()
{
	return false;
}

float EveChildContainer::GetSortValue()
{
	return 0.0;
}

uint32_t EveChildContainer::GetPerObjectDataSize( Tr2RenderContextEnum::ShaderType shaderType ) const
{
	if( shaderType == Tr2RenderContextEnum::VERTEX_SHADER )
	{
		return sizeof( m_vsData );
	}
	else if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		return sizeof( m_psData );
	}
	return 0;
}

void EveChildContainer::UpdatePerObjectBuffer( Tr2RenderContextEnum::ShaderType shaderType, uint32_t size, void* data )
{
	if( shaderType == Tr2RenderContextEnum::PIXEL_SHADER )
	{
		memcpy( data, &m_psData, sizeof( m_psData ) );
	}
	else
	{
		memcpy( data, &m_vsData, sizeof( m_vsData ) );
	}
}

Tr2PerObjectData* EveChildContainer::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	if( m_animationOwner )
	{
		auto animation = m_animationOwner->GetAnimationController();
		if( animation && animation->IsInitialized() )
		{
			auto boneCount = uint32_t( animation->GetMeshBoneCount() );
			m_vsData.boneOffsets[2] = boneCount;
			m_boneOffsets.UploadTransforms( Tr2RingBuffer::GetInstance<Float4x3>(), reinterpret_cast<const Float4x3*>( animation->GetMeshBoneMatrixList() ), boneCount );
		}
	}
	m_vsData.boneOffsets[0] = m_boneOffsets.GetCurrentFrameOffset();
	m_vsData.boneOffsets[1] = m_boneOffsets.GetPreviousFrameOffset();

	Tr2PerObjectDataWithPersistentBuffers<EveChildContainer>* perObjectData = accumulator->Allocate<Tr2PerObjectDataWithPersistentBuffers<EveChildContainer>>();
	if( !perObjectData )
	{
		return NULL;
	}
	perObjectData->Initialize( this, &m_perObjectDataVs, &m_perObjectDataPs );

	return perObjectData;
}

void EveChildContainer::SetIsPlacementRoot( bool isPlacementRoot )
{
	m_isPlacementRoot = isPlacementRoot;
}

bool EveChildContainer::Empty() const
{
	return m_objects.empty() && m_lights.empty() && m_attachments.empty() && m_controllers.empty() &&
		m_curveSets.empty() && m_transformModifiers.empty() && m_observers.empty();
}