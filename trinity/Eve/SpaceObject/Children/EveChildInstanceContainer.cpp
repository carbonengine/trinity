////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//
#include "StdAfx.h"
#include "EveChildInstanceContainer.h"
#include "EveChildContainer.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierAttachToBone.h"
#include "Tr2DebugRenderer.h"


namespace
{
BlueStructureDefinition s_eveChildInstanceTransformStructureDef[] =
	{
		{ "scale", Be::FLOAT32_3, 0 },
		{ "rotation", Be::FLOAT32_4, 12 },
		{ "translation", Be::FLOAT32_3, 28 },
		{ "boneIndex", Be::INT32_1, 40 },
		{ 0 }
	};

EveChildInstanceTransform s_defaultEveChildInstanceTransform;
}


EveChildInstanceTransform::EveChildInstanceTransform() :
	scale( Vector3( 1.f, 1.f, 1.f ) ),
	rotation( IdentityQuaternion() ),
	translation( Vector3() ),
	boneIndex( -1 )
{
}


EveChildInstanceContainer::EveChildInstanceContainer( IRoot* lockobj ) :
	EveChildTransform(),
	PARENTLOCK( m_instances ),
	PARENTLOCK( m_transforms ),
	PARENTLOCK( m_transformModifiers ),
	m_worldVelocity( 0, 0, 0 ),
	m_display( true ),
	m_isAlwaysOn( false ),
	m_disableEditMode( false ),
	m_ownerMaxSpeed( 0 ),
	m_origin( SPACE ),
	m_locatorSetName( "" ),
	m_reset( true )
{
	m_transforms.SetStructureDefinition( s_eveChildInstanceTransformStructureDef );
	m_transforms.SetDefaultValue( &s_defaultEveChildInstanceTransform );
	m_transforms.SetNotify( this );

	m_transformModifiers.SetNotify( this );
}

EveChildInstanceContainer::~EveChildInstanceContainer()
{
}

const char* EveChildInstanceContainer::GetName() const
{
	return m_name.c_str();
}

void EveChildInstanceContainer::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

bool EveChildInstanceContainer::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_source ) || IsMatch( value, m_locatorSetName ) )
	{
		m_reset = true;
	}
	if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}
	return true;
}


void EveChildInstanceContainer::RegisterComponents()
{
	if( IsInRegistry() && m_display )
	{
		for( auto instance = m_instances.begin(); instance != m_instances.end(); ++instance )
		{
			if( EveEntityPtr entity = BlueCastPtr( *instance ) )
			{
				entity->Register( this->GetComponentRegistry() );
			}
		}
	}
}

void EveChildInstanceContainer::UnRegisterComponents()
{
	if( IsInRegistry() )
	{
		for( auto instance = m_instances.begin(); instance != m_instances.end(); ++instance )
		{
			if( EveEntityPtr entity = BlueCastPtr( *instance ) )
			{
				entity->UnRegister( this->GetComponentRegistry() );
			}
		}
	}
}

void EveChildInstanceContainer::SetSourceEffect( IEveSpaceObjectChildPtr sourceEffect )
{
	m_source = sourceEffect;
	m_reset = true;
}

void EveChildInstanceContainer::AddInstanceTransform( const Vector3& scale, const Quaternion& rotation, const Vector3& translation, int32_t boneIndex )
{
	EveChildInstanceTransform* transform = new EveChildInstanceTransform();
	transform->boneIndex = boneIndex;
	transform->rotation = rotation;
	transform->scale = scale;
	transform->translation = translation;

	m_transforms.Append( transform );

	CreateInstance( scale, rotation, translation, boneIndex );
	m_reset = false;
}

void EveChildInstanceContainer::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_transformModifiers )
	{
		m_reset = true;
	}
}


void EveChildInstanceContainer::OnStructureListModified( Event event, const void* item, size_t index, IBlueStructureList* list )
{
	m_reset = true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the locatorset which is supposed to be used for distribution and redistributes
//   the source
// Arguments:
//   locatorSetName - The name of the locator set name
// --------------------------------------------------------------------------------------
void EveChildInstanceContainer::DistributeAcrossLocatorset( const BlueSharedString locatorSetName )
{
	m_locatorSetName = locatorSetName;
	m_reset = true;
}


// --------------------------------------------------------------------------------------
// Description:
//   Creates instances across locator sets and/or the transforms
// Arguments:
//   parent - The actual space object parent
// --------------------------------------------------------------------------------------
void EveChildInstanceContainer::CreateInstances( IEveSpaceObject2* parent )
{
	ClearInstanceList();

	if( !m_source )
	{
		return;
	}

	if( !m_locatorSetName.empty() )
	{
		if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( parent ) )
		{
			auto locators = spaceObject->GetLocatorsForSet( m_locatorSetName );
			if( locators )
			{
				for( auto locator = locators->begin(); locator != locators->end(); ++locator )
				{
					CreateInstance( Vector3( 1.f, 1.f, 1.f ), ( *locator ).direction, ( *locator ).position, ( *locator ).boneIndex );
				}
			}
		}
	}

	for( auto transform = m_transforms.begin(); transform != m_transforms.end(); ++transform )
	{
		CreateInstance( transform->scale, transform->rotation, transform->translation, transform->boneIndex );
	}
}


// --------------------------------------------------------------------------------------
// Description:
//   Creates an instance of the source that is scales, rotated and translated and is connected
//   to the bone (if boneIndex >= 0)
// Arguments:
//   scale - The scale of the instance
//   rotation - The rotation of the instance
//   translation - The translation of the instance
//   boneIndex- The bone that the instance is connected to (optional)
// --------------------------------------------------------------------------------------
void EveChildInstanceContainer::CreateInstance( const Vector3& scale, const Quaternion& rotation, const Vector3& translation, const int32_t boneIndex )
{
	if( m_source == nullptr )
	{
		return;
	}

	EveChildContainerPtr translationParent;
	translationParent.CreateInstance();

	IEveSpaceObjectChildPtr instance;

	if( !BeClasses->CopyTo( m_source, (IRoot**)&instance ) )
	{
		return;
	}
	for( auto tm = m_transformModifiers.begin(); tm != m_transformModifiers.end(); ++tm )
	{
		instance->AddTransformModifier( *tm );
	}
	translationParent->AddToEffectChildrenList( instance );
	translationParent->Setup( &scale, &rotation, &translation, TR2_LOD_LOW );
	translationParent->Initialize();

	if( boneIndex >= 0 )
	{
		EveChildContainerPtr boneParent;
		boneParent.CreateInstance();

		EveChildModifierAttachToBonePtr mod;
		mod.CreateInstance();
		mod->SetBoneIndex( boneIndex );
		boneParent->AddTransformModifier( mod );
		boneParent->AddToEffectChildrenList( translationParent );
		boneParent->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
		boneParent->Register( GetComponentRegistry() );

		m_instances.Append( (IEveSpaceObjectChildPtr)boneParent );
	}
	else
	{
		translationParent->RegisterWithQuadRenderer( *Tr2QuadRenderer::Instance() );
		translationParent->Register( GetComponentRegistry() );
		m_instances.Append( (IEveSpaceObjectChildPtr)translationParent );
	}

	return;
}


// --------------------------------------------------------------------------------------
// Description:
//   A helper method to update a position of an instance
// --------------------------------------------------------------------------------------
void EveChildInstanceContainer::UpdateInstance( const uint32_t index, const Vector3& scale, const Quaternion& rotation, const Vector3& translation )
{
	auto instance = (IEveSpaceObjectChild*)m_instances.GetAt( index );
	if( instance == nullptr )
	{
		return;
	}
	instance->Setup( &scale, &rotation, &translation, Tr2Lod::TR2_LOD_LOW );
}

// --------------------------------------------------------------------------------------
// Description:
//   A helper method to run a function on the source (if the instances are not valid) or
//   on the instances
// Arguments:
//   func - The function to run, takes a single parameter which is an IEveSpaceObjectChild*
// --------------------------------------------------------------------------------------
void EveChildInstanceContainer::RunOnInstances( std::function<void( IEveSpaceObjectChild* )> func ) const
{
	if( m_instances.empty() && m_source && !m_disableEditMode )
	{
		func( m_source );
	}
	else
	{
		for( auto it = m_instances.begin(); it != m_instances.end(); ++it )
		{
			func( *it );
		}
	}
}

void EveChildInstanceContainer::DisableEditMode( bool disable )
{
	m_disableEditMode = disable;
}

void EveChildInstanceContainer::ClearInstanceList()
{
	UnRegisterComponents();
	m_instances.Clear();
}

void EveChildInstanceContainer::PopFront()
{
	if( !m_instances.empty() )
	{
		if( EveEntityPtr front = BlueCastPtr( m_instances[0] ) )
		{
			if( IsInRegistry() )
			{
				front->UnRegister( GetComponentRegistry() );
			}
		}

		m_instances.Remove( 0 );
	}
}

float EveChildInstanceContainer::GetOwnerMaxSpeed() const
{
	return m_ownerMaxSpeed;
}

void EveChildInstanceContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	if( !m_display )
	{
		return;
	}

	RunOnInstances( [&renderables]( IEveSpaceObjectChild* c ) { c->GetRenderables( renderables ); } );
}


void EveChildInstanceContainer::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

	RunOnInstances( [&frustum, &parentTransform, parentLod]( IEveSpaceObjectChild* c ) { c->UpdateVisibility( frustum, parentTransform, parentLod ); } );
}

void EveChildInstanceContainer::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( !m_display )
	{
		return;
	}

	if( m_reset )
	{
		CreateInstances( params.spaceObjectParent );

		m_reset = false;
	}

	m_ownerMaxSpeed = params.ownerMaxSpeed;

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

	RunOnInstances( [&updateContext, &newParams]( IEveSpaceObjectChild* c ) { c->UpdateSyncronous( updateContext, newParams ); } );
}

void EveChildInstanceContainer::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
	m_reset = true;
}

void EveChildInstanceContainer::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( !m_display )
	{
		return;
	}

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

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;
	newParams.localToWorldTransform = localToWorldTransform;

	RunOnInstances( [&updateContext, &newParams]( IEveSpaceObjectChild* c ) { c->UpdateAsyncronous( updateContext, newParams ); } );


	if( params.spaceObjectParent && !params.childParent )
	{
		params.spaceObjectParent->GetWorldVelocity( m_worldVelocity );
	}
}

void EveChildInstanceContainer::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildInstanceContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

bool EveChildInstanceContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( !m_display )
	{
		return true;
	}

	RunOnInstances( [&sphere, &query]( IEveSpaceObjectChild* c ) { c->GetBoundingSphere( sphere, query ); } );
	return true;
}


void EveChildInstanceContainer::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( m_source )
	{
		m_source->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveChildInstanceContainer::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	RunOnInstances( [&frustum, &quadRenderer]( IEveSpaceObjectChild* c ) { c->AddQuadsToQuadRenderer( frustum, quadRenderer ); } );
}

void EveChildInstanceContainer::ChangeLOD( Tr2Lod lod )
{
	RunOnInstances( [&lod]( IEveSpaceObjectChild* c ) { c->ChangeLOD( lod ); } );
}

void EveChildInstanceContainer::GetLights( Tr2LightManager& lightManager ) const
{
	RunOnInstances( [&lightManager]( IEveSpaceObjectChild* c ) { c->GetLights( lightManager ); } );
}

void EveChildInstanceContainer::SetOrigin( Origin origin )
{
	m_origin = origin;
}

void EveChildInstanceContainer::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	RunOnInstances( [&name, &value]( IEveSpaceObjectChild* c ) { c->SetShaderOption( name, value ); } );
}

bool EveChildInstanceContainer::IsAlwaysOn() const
{
	return m_isAlwaysOn;
}

void EveChildInstanceContainer::SetInheritProperties( const Color* colorSet )
{
	if( IEveInheritPropertiesOwnerPtr obj = BlueCastPtr( m_source ) )
	{
		obj->SetInheritProperties( colorSet );
	}
}

void EveChildInstanceContainer::GetWorldVelocity( Vector3& velocity ) const
{
	velocity = m_worldVelocity;
}

IEveSpaceObjectChildPtr EveChildInstanceContainer::GetEffectChildByName( const char* name ) const
{
	// don't really know what to do here...
	return nullptr;
}

void EveChildInstanceContainer::AddToEffectChildrenList( IEveSpaceObjectChild* child )
{
	// do nothing for now
}

void EveChildInstanceContainer::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
	// do nothing for now
}

void EveChildInstanceContainer::SetControllerVariable( const char* name, float value )
{
	RunOnInstances( [&name, &value]( IEveSpaceObjectChild* c ) { c->SetControllerVariable( name, value ); } );
}

void EveChildInstanceContainer::HandleControllerEvent( const char* name )
{
	RunOnInstances( [name]( IEveSpaceObjectChild* c ) { c->HandleControllerEvent( name ); } );
}

void EveChildInstanceContainer::StartControllers()
{
	RunOnInstances( []( IEveSpaceObjectChild* c ) { c->StartControllers(); } );
}

void EveChildInstanceContainer::SetControllerVariableForInstance( const uint32_t index, const char* name, float value )
{
	if( index > m_instances.size() )
	{
		return;
	}

	auto instance = m_instances[index];
	instance->SetControllerVariable( name, value );
}

void EveChildInstanceContainer::HandleControllerEventForInstance( const uint32_t index, const char* name )
{
	if( index > m_instances.size() )
	{
		return;
	}

	auto instance = m_instances[index];
	instance->HandleControllerEvent( name );
}

void EveChildInstanceContainer::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	RunOnInstances( [&name, &rangeName]( IEveSpaceObjectChild* c ) {
		ITr2CurveSetOwnerPtr cso = BlueCastPtr( c->GetRootObject() );
		if( cso != nullptr )
		{
			cso->PlayCurveSet( name, rangeName );
		}
	} );
}

void EveChildInstanceContainer::StopCurveSet( const std::string& name )
{
	RunOnInstances( [&name]( IEveSpaceObjectChild* c ) {
		ITr2CurveSetOwnerPtr cso = BlueCastPtr( c->GetRootObject() );

		if( cso != nullptr )
		{
			cso->StopCurveSet( name );
		}
	} );
}

void EveChildInstanceContainer::UpdateCurveSet( const std::string& name, Be::Time time )
{
	RunOnInstances( [&name, time]( IEveSpaceObjectChild* c ) {
		ITr2CurveSetOwnerPtr cso = BlueCastPtr( c->GetRootObject() );
		if( cso != nullptr )
		{
			cso->UpdateCurveSet( name, time );
		}
	} );
}

float EveChildInstanceContainer::GetCurveSetDuration( const std::string& name ) const
{
	if( m_source )
	{
		if( ITr2CurveSetOwnerPtr cso = BlueCastPtr( m_source->GetRootObject() ) )
		{
			return cso->GetCurveSetDuration( name );
		}
	}
	return 0;
}

float EveChildInstanceContainer::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	if( m_source )
	{
		if( ITr2CurveSetOwnerPtr cso = BlueCastPtr( m_source->GetRootObject() ) )
		{
			return cso->GetRangeDuration( name, rangeName );
		}
	}
	return 0;
}

void EveChildInstanceContainer::PlayAllCurveSets()
{
	RunOnInstances( []( IEveSpaceObjectChild* c ) {
		EveChildContainerPtr cc = BlueCastPtr( c->GetRootObject() );
		if( cc != nullptr )
		{
			cc->PlayAllCurveSets();
		}
	} );
}

void EveChildInstanceContainer::StopAllCurveSets()
{
	RunOnInstances( []( IEveSpaceObjectChild* c ) {
		EveChildContainerPtr cc = BlueCastPtr( c->GetRootObject() );
		if( cc != nullptr )
		{
			cc->StopAllCurveSets();
		}
	} );
}

void EveChildInstanceContainer::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	RunOnInstances( [&options]( IEveSpaceObjectChild* c ) {
		if( auto dr = dynamic_cast<ITr2DebugRenderable*>( c ) )
		{
			dr->GetDebugOptions( options );
		}
	} );
}

void EveChildInstanceContainer::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	RunOnInstances( [&renderer]( IEveSpaceObjectChild* c ) {
		if( auto dr = dynamic_cast<ITr2DebugRenderable*>( c ) )
		{
			dr->RenderDebugInfo( renderer );
		}
	} );
}
