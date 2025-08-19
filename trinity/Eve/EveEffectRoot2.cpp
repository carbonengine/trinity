#include "StdAfx.h"
#include "EveEffectRoot2.h"

#include "Utilities/BoundingSphere.h"
#include "TriFrustum.h"
#include "Lights/Tr2PointLight.h"
#include "Tr2LightManager.h"
#include "Controllers/ITr2Controller.h"
#include "Curves/TriCurveSet.h"
#include "Eve/EveUpdateContext.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"

extern float g_eveSpaceObjectResourceUnloadingTimeThreshold;

EveEffectRoot2::EveEffectRoot2( IRoot* lockobj ) :
	PARENTLOCK( m_observers ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_externalParameters ),
	PARENTLOCK( m_effectChildren ),
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_controllers ),
	m_boundingSphere( 0, 0, 0, 0 ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_estimatedSize( 0.0f ),
	m_display( true ),
	m_mute( false ),
	m_startTime( 0 ),
	m_effectDuration( -1 ),
	m_lodLevel( TR2_LOD_HIGH ),
	m_dynamicLODSelection( false ),
	m_changeLOD( true ),
	m_secondaryLightingSphereRadiusLocal( 0.5f ),
	m_secondaryLightingSphereRadiusWorld( 0.5f ),
	m_secondaryLightingEmissiveColor( 0.f, 0.f, 0.f, 0.f )
{
	m_controllers.SetNotify( this );
	m_effectChildren.SetNotify( this );
}

bool EveEffectRoot2::Initialize()
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

bool EveEffectRoot2::OnModified( Be::Var* val )
{
	if( IsMatch( val, m_display ) )
	{
		ReRegister();
	}
	else if( IsMatch( val, m_mute ) )
	{
		SetMute( val );
	}
	return true;
}

void EveEffectRoot2::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
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
	else if( list == &m_effectChildren && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( IEveSpaceObjectChildPtr child = BlueCastPtr( value ) )
			{
				for( auto it = begin( m_controllerVariables ); it != end( m_controllerVariables ); ++it )
				{
					child->SetControllerVariable( it->first.c_str(), it->second );
				}
				child->StartControllers();
			}
			if( IsInRegistry() )
			{
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->Register( GetComponentRegistry() );
				}
			}
			break;
		case BELIST_REMOVED:
			if( IsInRegistry() )
			{
				if( EveEntityPtr entity = BlueCastPtr( value ) )
				{
					entity->UnRegister( GetComponentRegistry() );
				}
			}
			break;
		case BELIST_UNLOADSTART:
			if( IsInRegistry() )
			{
				for( ssize_t i = 0; i < list->GetSize(); ++i )
				{
					if( EveEntityPtr entity = BlueCastPtr( list->GetAt(i) ) )
					{
						entity->UnRegister( GetComponentRegistry() );
					}
				}			
			}
		default:
			break;
		}
	}
}

void EveEffectRoot2::UpdateSyncronous( const EveUpdateContext& updateContext )
{	
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateWorldTransform( updateContext.GetTime() );

	m_localTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
	m_lastUpdateMatrix = m_localTransform * m_worldTransform;
	m_secondaryLightingSphereRadiusWorld = m_secondaryLightingSphereRadiusLocal * ( m_scaling.x + m_scaling.y + m_scaling.z ) / 3.f;

	for( TriObserverLocalVector::iterator it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		(*it)->Update( m_lastUpdateMatrix );
	}

	if( !m_effectChildren.empty() )
	{
		Matrix worldTransform;
		GetLocalToWorldTransform( worldTransform );
		EveChildUpdateParams params;
		params.spaceObjectParent = this;
		params.childParent = nullptr;
		params.boneCount = 0;
		params.bones = nullptr;
		params.localToWorldTransform = worldTransform;
		params.isVisible = m_display;

		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
		{
			( *ecIt )->UpdateSyncronous( updateContext, params );
		}
	}
}

void EveEffectRoot2::UpdateAsyncronous( const EveUpdateContext& updateContext ) 
{	
	UpdateControllers();

	Be::Time time = updateContext.GetTime();
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		(*it)->Update( time, time );
	}

	if( !m_effectChildren.empty() )
	{
		Matrix worldTransform;
		GetLocalToWorldTransform( worldTransform );
		EveChildUpdateParams params;
		params.spaceObjectParent = this;
		params.childParent = nullptr;
		params.boneCount = 0;
		params.bones = nullptr;
		params.localToWorldTransform = worldTransform;
		params.isVisible = m_display;

		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
		{
			( *ecIt )->UpdateAsyncronous( updateContext, params );
		}
	}
}

void EveEffectRoot2::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	if( m_dynamicLODSelection )
	{
		Vector4 boundingSphere;
		GetBoundingSphere( boundingSphere );
		BoundingSphereTransform( m_worldTransform, boundingSphere );
		
		if( updateContext.GetFrustum().IsSphereVisible( &boundingSphere ) )
		{
			m_estimatedSize = updateContext.GetFrustum().GetPixelSizeAccross( &boundingSphere );
		}

		Tr2Lod oldLod = m_lodLevel;
		m_lodLevel = TR2_LOD_LOW;

		if( m_estimatedSize >= updateContext.GetMediumDetailThreshold() )
		{
			m_lodLevel = TR2_LOD_HIGH;
		}
		else if( m_estimatedSize >= updateContext.GetLowDetailThreshold() )
		{
			m_lodLevel = TR2_LOD_MEDIUM;
		}

		m_changeLOD |= oldLod != m_lodLevel;
	}
	
	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
	{
		(*ecIt)->UpdateVisibility( updateContext, parentTransform, m_lodLevel );
	}
}

void EveEffectRoot2::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors ) 
{
	if( !m_display )
	{
		return;
	}

	if( m_changeLOD )
	{
		m_changeLOD = false;

		for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt ) 
		{
			(*ecIt)->ChangeLOD( m_lodLevel );
		}
	}

	for( auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt )
	{
		(*ecIt)->GetRenderables( renderables );
	}
}


void EveEffectRoot2::UpdateControllers() 
{
	for ( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		(*it)->Update();
	}
}

bool EveEffectRoot2::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{ 
	sphere = m_boundingSphere;
	return true;
};


void EveEffectRoot2::UpdateWorldTransform( Be::Time time )
{	
	Quaternion rotation;
	Vector3 translation;

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, time );
	}
	else
	{
		translation = Vector3( 0.0f, 0.0f, 0.0f );
	}

	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, time );
	}
	else
	{
		rotation = Quaternion( 0.0f, 0.0f, 0.0f, 1.0f );
	}

	
	if( m_modelRotation )
	{
		Quaternion modelRotation;
		m_modelRotation->Update( &modelRotation, time );
		rotation = modelRotation * rotation;
	}

	m_worldTransform = RotationMatrix( rotation ) * TranslationMatrix( translation );
	
	if( m_modelTranslation )
	{
		Vector3 modelTranslation;
		m_modelTranslation->Update( &modelTranslation, time );
		modelTranslation = TransformCoord( modelTranslation, m_worldTransform );
		m_worldTransform.GetTranslation() = modelTranslation;
	}
}


void EveEffectRoot2::UpdateModelCenterWorldPosition( Vector3 &position, Be::Time t )
{
	// This version of the function should perform an update on the model / ball position
	Matrix currentTransform;
	UpdateWorldTransform( t );
	currentTransform = TransformationMatrix( m_scaling, m_rotation, m_translation );
	currentTransform = currentTransform * m_worldTransform;

	position = TransformCoord( m_boundingSphere.GetXYZ(), currentTransform );
}

void EveEffectRoot2::GetModelCenterWorldPosition( Vector3 &position ) const
{
	// This version of the function does not perform an update on the object
	position = TransformCoord( m_boundingSphere.GetXYZ(), m_lastUpdateMatrix );
}


bool EveEffectRoot2::GetLocalBoundingBox( Vector3 &min, Vector3 &max )
{
	// If possible, return an AABB in local coordinates
	return false;
}

void EveEffectRoot2::GetLocalToWorldTransform( Matrix &transform ) const
{
	// Get the local to world transform
	transform = m_lastUpdateMatrix;
}

void EveEffectRoot2::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->RegisterWithQuadRenderer( quadRenderer );
	}
}

void EveEffectRoot2::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if (!m_display )
	{
		return;
	}
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->AddQuadsToQuadRenderer( frustum, quadRenderer );
	}
}

void EveEffectRoot2::GetLights( Tr2LightManager& lightManager ) const
{
	if( !m_display )
	{
		return;
	}

	XMMATRIX worldTransform = m_lastUpdateMatrix;
	float scaling = XMVectorGetX( XMVectorAdd( XMVector3LengthEst( m_lastUpdateMatrix.GetX() ), 
		XMVectorAdd( XMVector3LengthEst( m_lastUpdateMatrix.GetY() ), XMVector3LengthEst( m_lastUpdateMatrix.GetZ() ) ) ) ) / 3.f;
	for( auto it = std::begin( m_lights ); it != std::end( m_lights ); ++it )
	{
		( *it )->AddLight( lightManager, worldTransform, scaling );
	}
	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		( *it )->GetLights( lightManager );
	}
}


void EveEffectRoot2::AddLight( Tr2Light* light )
{
	m_lights.Append( light->GetRawRoot() );
}

void EveEffectRoot2::ClearLights()
{
	m_lights.Clear();
}

// --------------------------------------------------------------------------------
// Description:
//   Called by all children. It is similar to what spaceobjects send to vs/ps
// --------------------------------------------------------------------------------
void EveEffectRoot2::GetPerObjectStructs( EveSpaceObjectVSData& vsData, EveSpaceObjectPSData& psData ) const
{
	// vs
	memset( &vsData, 0, sizeof( EveSpaceObjectVSData ) );
	// activation
	vsData.shipData.y = 1.f;
	// boundingsphere
	vsData.shipData.w = 1.f;

	// ps
	memset( &psData, 0, sizeof( EveSpaceObjectPSData ) );
	// activation
	psData.shipData.y = 1.f;
	// boundingsphere
	psData.shipData.w = 1.f;
}

void EveEffectRoot2::RegisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	static const Color s_noAlbedoColor( 0.f, 0.f, 0.f, 0.f );
	manager.RegisterSecondaryLightSource( 
		&m_worldTransform.GetTranslation(), 
		&m_secondaryLightingSphereRadiusWorld, 
		&s_noAlbedoColor, 
		&m_secondaryLightingEmissiveColor );
}

void EveEffectRoot2::UnregisterSecondaryLightSource( Tr2ShLightingManager& manager )
{
	manager.UnregisterSecondaryLightSource( &m_worldTransform.GetTranslation() );
}


void EveEffectRoot2::RegisterComponents( )
{
	auto registry = GetComponentRegistry();
	if( registry && m_display )
	{
		for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->Register( registry );
			}
		}
	}	
}

void EveEffectRoot2::UnRegisterComponents(  )
{
	auto registry = GetComponentRegistry();
	if( registry )
	{
		for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
		{
			if( EveEntityPtr entity = BlueCastPtr( *it ) )
			{
				entity->UnRegister( registry );
			}
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Plays all curve sets.
// --------------------------------------------------------------------------------
void EveEffectRoot2::Start()
{
	// play curvesets owned by this effect root
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		( *it )->Play();
	}

	// play curvesets on children owned by this effect root
	for( auto cit = m_effectChildren.begin(); cit != m_effectChildren.end(); cit++ )
	{
		if( auto child = dynamic_cast<ITr2CurveSetOwner*>( *cit ) )
		{
			child->PlayAllCurveSets();
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Stops all "top level" curve sets.
// --------------------------------------------------------------------------------
void EveEffectRoot2::Stop()
{
	// stop curvesets owned by this effect root
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		( *it )->Stop();
	}

	// stop curvesets on children owned by this effect root
	for( auto cit = m_effectChildren.begin(); cit != m_effectChildren.end(); cit++ )
	{
		if( auto child = dynamic_cast<ITr2CurveSetOwner*>( *cit ) )
		{
			child->StopAllCurveSets();
		}
	}
}


// --------------------------------------------------------------------------------
// Description:
//   Is mostly used for effects, so no damage locators at all!
// --------------------------------------------------------------------------------
unsigned int EveEffectRoot2::GetDamageLocatorCount() const
{
	return 0;
}

bool EveEffectRoot2::GetDamageLocatorPosition( Vector3* out, int index, bool inWorldSpace )
{
	*out = m_worldTransform.GetTranslation();
	return true;
}

bool EveEffectRoot2::GetDamageLocatorDirection( Vector3* out, int index, bool inWorldSpace )
{
	*out = Vector3( 0.f, 1.f, 0.f );
	return true;
}

bool EveEffectRoot2::GetImpactPosition( Vector3& out, int locator, const Vector3& posPrev, const Vector3& posNow, float epsilon )
{
	GetDamageLocatorPosition( &out, locator, true );
	return LengthSq( posNow - out ) < epsilon;
}

bool EveEffectRoot2::HasImpactConfigurationShield() const
{
	return false;
}

int EveEffectRoot2::GetClosestDamageLocatorIndex( const Vector3* position )
{
	return 0;
}

int EveEffectRoot2::GetGoodDamageLocatorIndex( const Vector3 &position )
{
	return 0;
}

float EveEffectRoot2::GetRadius() const
{
	return m_boundingSphere.w;
}

// -----------------------------------------------------------------------------
// Description:
//   Create an impact effect on this object
//   Is empty for transforms!
// -----------------------------------------------------------------------------
int EveEffectRoot2::CreateImpact( int damageLocatorIndex, const Vector3& direction, float lifeTime, float size )
{
	return -1;
}

// -----------------------------------------------------------------------------
// Description:
//   Update the effect on this object
//   Is empty for transforms!
// -----------------------------------------------------------------------------
bool EveEffectRoot2::UpdateImpact( Vector3& out, const Vector3& direction, int impactIndex )
{
	return false;
}

// -----------------------------------------------------------------------------
Vector3 EveEffectRoot2::GetWorldPosition()
{
	return m_worldTransform.GetTranslation();
}

Quaternion EveEffectRoot2::GetWorldRotation()
{
	return Normalize( m_rotation * RotationQuaternion( m_worldTransform ) );
}

void EveEffectRoot2::GetMissPosition( const Vector3* hit, const Vector3* source, Vector3* out )
{
	GetDamageLocatorPosition(out, -1, true );
	
	if( hit && source ) 
	{
		Vector3 local( *hit - *out );
		Vector3 dir = Normalize( *hit - *source );
		
		local -= dir * Dot( dir, local );

		local = Normalize( local );
		const Vector3 off = local * m_boundingSphere.w * 1.125f;
		*out += off;
	}
}

// -----------------------------------------------------------------------------
PIEveSpaceObjectChildVector& EveEffectRoot2::GetChildren()
{ 
	return m_effectChildren; 
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::SetTransform( const Matrix& transform )
{
	Decompose( m_scaling, m_rotation, m_translation, transform );
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::PlayCurveSet( const std::string& name, const std::string& rangeName )
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
	for( auto childIt = m_effectChildren.begin(); childIt != m_effectChildren.end(); childIt++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *childIt ) )
		{
			owner->PlayCurveSet( name, rangeName );
		}
	}
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::StopCurveSet( const std::string& name )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Stop();
		}
	}
	for( auto childIt = m_effectChildren.begin(); childIt != m_effectChildren.end(); childIt++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *childIt ) )
		{
			owner->StopCurveSet( name );
		}
	}
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Update( time, time );
		}
	}
	for( auto childIt = m_effectChildren.begin(); childIt != m_effectChildren.end(); childIt++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *childIt ) )
		{
			owner->UpdateCurveSet( name, time );
		}
	}
}

// -----------------------------------------------------------------------------
float EveEffectRoot2::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetMaxCurveDuration() );
		}
	}
	for( auto childIt = m_effectChildren.begin(); childIt != m_effectChildren.end(); childIt++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *childIt ) )
		{
			maxDuration = max( maxDuration, owner->GetCurveSetDuration( name ) );
		}
	}
	return maxDuration;
}

// -----------------------------------------------------------------------------
float EveEffectRoot2::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	float maxDuration = 0.f;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			maxDuration = max( maxDuration, ( *it )->GetRangeDuration( rangeName.c_str() ) );
		}
	}
	for( auto childIt = m_effectChildren.begin(); childIt != m_effectChildren.end(); childIt++ )
	{
		if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( *childIt ) )
		{
			maxDuration = max( maxDuration, owner->GetRangeDuration( name, rangeName ) );
		}
	}
	return maxDuration;
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Bounding Sphere" );
	options.insert( "Lights" );

	for ( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->GetDebugOptions( options );
	}

	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->GetDebugOptions( options );
		}
	}
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if (renderer.HasOption( GetRawRoot(), "Bounding Sphere" ))
	{
		renderer.DrawSphere( this, m_boundingSphere.GetXYZ(), GetBoundingSphereRadius(), 8, Tr2DebugRenderer::Wireframe, 0xffff00ff );
	}

	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( *it ) )
		{
			renderable->RenderDebugInfo( renderer );
		}
	}

	if( renderer.HasOption( GetRawRoot(), "Lights" ) )
	{
		for( auto it = begin( m_lights ); it != end( m_lights ); ++it )
		{
			( *it )->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	for ( auto it = m_observers.begin(); it != m_observers.end(); ++it )
	{
		( *it )->RenderDebugInfo( renderer );
	}
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::SetControllerVariable( const char* name, float value )
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
	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		( *it )->SetControllerVariable( name, value );
	}
}

void EveEffectRoot2::HandleControllerEvent( const char* name )
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->HandleEvent( name );
	}
	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		( *it )->HandleControllerEvent( name );
	}
}

// -----------------------------------------------------------------------------
void EveEffectRoot2::StartControllers()
{
	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Start();
	}
	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		( *it )->StartControllers();
	}
}

// --------------------------------------------------------------------------------
IEveSpaceObjectChildPtr EveEffectRoot2::GetEffectChildByName( const char* name ) const
{
	for( auto it = begin( m_effectChildren ); it != end( m_effectChildren ); ++it )
	{
		auto child = *it;
		if( strcmp( child->GetName(), name ) == 0 )
		{
			return child;
		}
	}
	return nullptr;
}

// --------------------------------------------------------------------------------
void EveEffectRoot2::AddToEffectChildrenList( IEveSpaceObjectChild* child )
{
	if( IsInRegistry() && m_display )
	{
		if( EveEntityPtr entity = BlueCastPtr( child->GetRootObject() ) )
		{
			entity->Register( GetComponentRegistry() );
		}
	}
	m_effectChildren.Append( child->GetRootObject() );
}

// --------------------------------------------------------------------------------
void EveEffectRoot2::RemoveFromEffectChildrenList( IEveSpaceObjectChild* child )
{
	auto index = m_effectChildren.FindKey( child );
	if( index >= 0 )
	{
		if( IsInRegistry() )
		{
			if( EveEntityPtr entity = BlueCastPtr( m_effectChildren[index] ) )
			{
				entity->UnRegister( GetComponentRegistry() );
			}
		}
		m_effectChildren.Remove( index );
	}
}

void EveEffectRoot2::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
	for ( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); ++it )
	{
		IEveSpaceObjectChild *child = *it;
		child->SetShaderOption( name, value );
	}
}

ITr2AudEmitterPtr EveEffectRoot2::FindSoundEmitter( const char* name )
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

	for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); it++ )
	{
		if( auto owner = dynamic_cast<ITr2SoundEmitterOwner*>( *it ) )
		{
			auto emitter = owner->FindSoundEmitter( name );
			if ( emitter != nullptr )
			{
				return emitter;
			}
		}
	}
	return nullptr;
}

void EveEffectRoot2::AddObserver( TriObserverLocalPtr observer )
{
	m_observers.Append( observer );
}

void EveEffectRoot2::SetMute( bool isMute )
{
	for( auto it : m_effectChildren )
	{
		it->SetMute( m_mute );
	}
	for( auto it : m_observers )
	{
		it->SetMute( m_mute );
	}
}

void EveEffectRoot2::FreezeHighDetailMesh()
{
	m_lodLevel = TR2_LOD_HIGH;
	for (auto ecIt = m_effectChildren.begin(); ecIt != m_effectChildren.end(); ++ecIt)
	{
		(*ecIt)->ChangeLOD(m_lodLevel);
	}
}

void EveEffectRoot2::SetProceduralContainerVariable( const char *name, float value )
{
    for( auto it = m_effectChildren.begin(); it != m_effectChildren.end(); it++ )
    {
        auto child = *it;
        child->SetProceduralContainerVariable( name, value );
    }
}