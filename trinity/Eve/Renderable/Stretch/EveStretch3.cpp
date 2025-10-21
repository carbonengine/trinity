////////////////////////////////////////////////////////////
//
//    Created:   March 2020
//    Copyright: CCP 2020
//

#include "StdAfx.h"

#include "EveStretch3.h"
#include "TriFloat.h"
#include "Eve/EveUpdateContext.h"
#include "Include/TriMath.h"
#include "Utilities/BoundingSphere.h"
#include "Curves/TriCurveSet.h"
#include "Tr2DynamicBinding.h"

#include "Lights/Tr2PointLight.h"
#include "Controllers/ITr2Controller.h"
#include "Eve/SpaceObject/Children/TransformModifiers/EveChildModifierStretch.h"
#include "Eve/SpaceObject/Children/IEveSpaceObjectChild.h"

#include "Audio/Tr2AudioStretchAuto.h"
#include "Audio/Tr2AudioStretchBase.h"

EveStretch3::EveStretch3( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	PARENTLOCK( m_controllers ),
	PARENTLOCK( m_dynamicBindings ),
	m_display( true ),
	m_update( true ),
	m_sourcePosition( 0.0f, 0.0f, 0.0f ),
	m_destinationPosition( 0.0f, 0.0f, 0.0f ),
	m_startTime( 0 ),
	m_destObjectScale( 1.0f ),
	m_sourceMatrix( IdentityMatrix() ),
	m_isMuzzleEffect( false ),
	m_stretchState( STRETCH_STATE_UNDEFINED )
{
	m_length.CreateInstance();
	m_moveProgression.CreateInstance();

	m_controllers.SetNotify( this );
	m_dynamicBindings.SetNotify( this );
}

bool EveStretch3::Initialize()
{
	if( m_stretchObject )
	{
		m_stretchModifier.CreateInstance();
	}
	if( m_dest && m_stretchModifier )
	{
		m_stretchModifier->SetDest( m_dest );
	}

	for( auto& controller : m_controllers )
	{
		if( !controller->IsLinked() )
		{
			controller->Link( *GetRawRoot() );
		}
	}

	InitializeBindings();

	return true;
}

IEveSpaceObject2* EveStretch3::GetSourceSpaceObject() const
{
	return m_sourceSpaceObject;		
}

void EveStretch3::SetSourceSpaceObject( IEveSpaceObject2* spaceObject )
{
	m_sourceSpaceObject = spaceObject;
	InitializeBindings();
}

IEveSpaceObject2* EveStretch3::GetDestSpaceObject() const
{
	return m_destSpaceObject;
}

void EveStretch3::SetDestSpaceObject( IEveSpaceObject2* spaceObject )
{
	m_destSpaceObject = spaceObject;
	InitializeBindings();
}


void EveStretch3::InitializeBindings()
{
	for( auto binding = m_dynamicBindings.begin(); binding != m_dynamicBindings.end(); ++binding )
	{
		( *binding )->SetOwner( this );
		( *binding )->Link();
	}
}

void EveStretch3::Rebind( bool onlyUpdateBindings )
{
	for( auto binding = begin( m_dynamicBindings ); binding != end( m_dynamicBindings ); ++binding )
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

void EveStretch3::RunOnComponents( std::function<void( IEveSpaceObjectChild* )> func ) const
{
	if( m_sourceObject )
	{
		func( m_sourceObject );
	}

	if( m_destObject )
	{
		func( m_destObject );
	}

	if( m_stretchObject )
	{
		func( m_stretchObject );
	}

	if( m_moveObject )
	{
		func( m_moveObject );
	}
}

float EveStretch3::RunOnComponentsGetMax( std::function<float( IEveSpaceObjectChild* )> func ) const
{
	float ret = -1.f;
	if( m_sourceObject )
	{
		ret = max( ret, func( m_sourceObject ) );
	}

	if( m_destObject )
	{
		ret = max( ret, func( m_destObject ) );
	}

	if( m_stretchObject )
	{
		ret = max( ret, func( m_stretchObject ) );
	}

	if( m_moveObject )
	{
		ret = max( ret, func( m_moveObject ) );
	}
	return ret;
}

std::unordered_map<std::string, IRoot*> EveStretch3::GetParameterMap() const
{
	std::unordered_map<std::string, IRoot*> parameterMap;

	for( auto curveset = m_curveSets.begin(); curveset != m_curveSets.end(); ++curveset )
	{
		auto c = ( *curveset );
		parameterMap[c->GetName().c_str()] = c->GetRawRoot();
	}

	parameterMap["Owner"] = this->GetRawRoot();
	if( !!m_sourceSpaceObject )
	{
		parameterMap["SourceSpaceObject"] = m_sourceSpaceObject;
	}
	if( !!m_destSpaceObject )
	{
		parameterMap["DestSpaceObject"] = m_destSpaceObject;
	}

	if( m_sourceObject )
	{
		parameterMap["SourceObject"] = m_sourceObject->GetRootObject();
	}
	if( m_destObject )
	{
		parameterMap["DestObject"] = m_destObject->GetRootObject();
	}
	if( m_moveObject )
	{
		parameterMap["MoveObject"] = m_moveObject->GetRootObject();
	}
	if( m_stretchObject )
	{
		parameterMap["StretchObject"] = m_stretchObject->GetRootObject();
	}

	return parameterMap;
}

IEveSpaceObjectChild* EveStretch3::GetSourceObject()
{
	return m_sourceObject;
}

void EveStretch3::SetSourceObject( IEveSpaceObjectChild* sourceObject )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_sourceObject ) )
	{
		entity->UnRegister( registry );
	}
	m_sourceObject = sourceObject;
	if( EveEntityPtr entity = BlueCastPtr( m_sourceObject ) )
	{
		entity->Register( registry );
	}
}

IEveSpaceObjectChild* EveStretch3::GetDestObject()
{
	return m_destObject;
}

void EveStretch3::SetDestObject( IEveSpaceObjectChild* destObject )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_destObject ) )
	{
		entity->UnRegister( registry );
	}
	m_destObject = destObject;
	if( EveEntityPtr entity = BlueCastPtr( m_destObject ) )
	{
		entity->Register( registry );
	}
}

IEveSpaceObjectChild* EveStretch3::GetStretchObject()
{
	return m_stretchObject;
}

void EveStretch3::SetStretchObject( IEveSpaceObjectChild* stretchObject )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_stretchObject ) )
	{
		entity->UnRegister( registry );
	}
	m_stretchObject = stretchObject;
	if( m_stretchObject )
	{
		m_stretchModifier.CreateInstance();
		if( m_dest )
		{
			m_stretchModifier->SetDest( m_dest );
		}
	}
	else
	{
		m_stretchModifier = nullptr;
	}
	if( EveEntityPtr entity = BlueCastPtr( m_stretchObject ) )
	{
		entity->Register( registry );
	}
}

IEveSpaceObjectChild* EveStretch3::GetMoveObject()
{
	return m_moveObject;
}

void EveStretch3::SetMoveObject( IEveSpaceObjectChild* moveObject )
{
	auto registry = GetComponentRegistry();
	if( EveEntityPtr entity = BlueCastPtr( m_moveObject ) )
	{
		entity->UnRegister( registry );
	}
	m_moveObject = moveObject;
	if( EveEntityPtr entity = BlueCastPtr( m_moveObject ) )
	{
		entity->Register( registry );
	}
}

bool EveStretch3::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_dest ) )
	{
		if( m_dest == nullptr )
		{
			m_stretchModifier = nullptr;
		}
		else
		{
			if( m_stretchModifier == nullptr )
			{
				m_stretchModifier.CreateInstance();
			}
			m_stretchModifier->SetDest( m_dest );
		}
	}
	else if( IsMatch( value, m_display ) )
	{
		ReRegister();
	}

	return true;
}

void EveStretch3::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
	if( list == &m_controllers && ( event & BELIST_LOADING ) == 0 )
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
	if( list == &m_dynamicBindings && ( event & BELIST_LOADING ) == 0 )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( Tr2DynamicBindingPtr dynamicBinding = BlueCastPtr( value ) )
			{
				dynamicBinding->SetOwner( this );
				dynamicBinding->Link();
			}
			break;
		case BELIST_REMOVED:
			if( Tr2DynamicBindingPtr dynamicBinding = BlueCastPtr( value ) )
			{
				dynamicBinding->SetOwner( nullptr );
			}
			break;
		default:
			break;
		}
	}
}


void EveStretch3::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_update )
	{
		return;
	}

	if( m_stretchState == STRETCH_STATE_STARTING )
	{
		StartControllers();
		SetControllerVariable( "FiringDelay", m_delay );
		SetControllerVariable( "IsFiring", 1 );
		m_stretchState = STRETCH_STATE_STARTED;
	}
	else if( m_stretchState == STRETCH_STATE_STOPPING )
	{
		SetControllerVariable( "IsFiring", 0 );
		m_stretchState = STRETCH_STATE_UNDEFINED;
	}

	Be::Time time = updateContext.GetTime();

	for( auto binding = m_dynamicBindings.begin(); binding != m_dynamicBindings.end(); ++binding )
	{
		( *binding )->Update( time );
	}

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Update();
	}

	if( m_source )
	{
		m_source->Update( &m_sourcePosition, time );
	}

	if( m_dest )
	{
		m_dest->Update( &m_destinationPosition, time );
	}

	Vector3 directionVec( m_sourcePosition - m_destinationPosition );
	m_length->m_value = Length( directionVec );

	EveChildUpdateParams params;
	params.spaceObjectParent = !m_sourceSpaceObject ? static_cast<IEveSpaceObject2*>( this ) : (IEveSpaceObject2*)m_sourceSpaceObject;
	params.childParent = nullptr;
	params.boneCount = 0;
	params.bones = nullptr;
	params.isVisible = m_display;

	if( m_sourceObject )
	{
		m_sourceObject->UpdateSyncronous( updateContext, params );
	}

	if( m_stretchObject )
	{
		m_stretchObject->UpdateSyncronous( updateContext, params );
	}

	if( m_moveObject )
	{
		params.localToWorldTransform = TranslationMatrix( directionVec * m_moveProgression->m_value );

		m_moveObject->UpdateSyncronous( updateContext, params );
	}

	// reset the space object parent
	params.spaceObjectParent = nullptr;

	if( m_destObject )
	{
		params.localToWorldTransform = TranslationMatrix( m_destinationPosition );
		params.spaceObjectParent = !m_destSpaceObject ? static_cast<IEveSpaceObject2*>( this ) : (IEveSpaceObject2*)m_destSpaceObject;
		m_destObject->UpdateSyncronous( updateContext, params );
	}
}

void EveStretch3::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
	if( !m_update )
	{
		return;
	}

	Be::Time time = updateContext.GetTime();
	if( m_startTime == 0 )
	{
		m_startTime = time;
	}

	for( auto curve = m_curveSets.begin(); curve != m_curveSets.end(); ++curve )
	{
		( *curve )->Update( TimeAsDouble( time - m_startTime ) );
	}
	EveChildUpdateParams params;
	params.spaceObjectParent = nullptr;
	params.childParent = nullptr;
	params.boneCount = 0;
	params.bones = nullptr;
	params.isVisible = m_display;

	Vector3 directionVec( m_sourcePosition - m_destinationPosition );
	Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
	Matrix rotationMatrix = IdentityMatrix();
	TriQuaternionArcFromForward( &rotation, &directionVec );
	rotationMatrix = RotationMatrix( rotation );

	Matrix sourceMatrix = m_isMuzzleEffect ? m_sourceMatrix : rotationMatrix * TranslationMatrix( m_sourcePosition );

	if( m_sourceObject )
	{
		params.localToWorldTransform = sourceMatrix;
		m_sourceObject->UpdateAsyncronous( updateContext, params );
	}

	if( m_stretchObject )
	{
		if( m_stretchModifier )
		{
			m_stretchModifier->SetDestPosition( m_destinationPosition );
			params.localToWorldTransform = m_stretchModifier->ApplyTransform( TranslationMatrix( m_sourcePosition ), 0, nullptr );
		}
		else
		{
			params.localToWorldTransform = TranslationMatrix( m_sourcePosition );
		}

		m_stretchObject->UpdateAsyncronous( updateContext, params );
	}
	if( m_moveObject )
	{
		Vector3 movedPosition = Lerp( m_sourcePosition, m_destinationPosition, m_moveProgression->m_value );
		params.localToWorldTransform = rotationMatrix * TranslationMatrix( movedPosition );

		m_moveObject->UpdateAsyncronous( updateContext, params );
	}
	if( m_destObject )
	{
		// make the rotation matrix face source
		auto oneEighty = Matrix(
			-1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0 );
		params.localToWorldTransform = ScalingMatrix( Vector3( 1, 1, 1 ) * m_destObjectScale ) * oneEighty * rotationMatrix * TranslationMatrix( m_destinationPosition );
		m_destObject->UpdateAsyncronous( updateContext, params );
	}

	if( auto tmp = dynamic_cast<Tr2AudioStretchBase*>( m_audio.p ) )
	{
		tmp->Update( m_sourcePosition, m_destinationPosition );
	}
}

void EveStretch3::UpdateEffectAsync( const EveUpdateContext& updateContext )
{
	UpdateAsyncronous( updateContext );
}

void EveStretch3::UpdateEffectSync( const EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
}

void EveStretch3::StartMoving()
{
}

void EveStretch3::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
	if( !m_display )
	{
		return;
	}

	auto sourceMatrix = TranslationMatrix( m_sourcePosition );
	auto destMatrix = TranslationMatrix( m_destinationPosition );
	Vector3 directionVec = Normalize( m_sourcePosition - m_destinationPosition );
	float scalingLength = m_length->m_value;

	if( m_sourceObject )
	{
		m_sourceObject->UpdateVisibility( updateContext, sourceMatrix, TR2_LOD_HIGH );
	}

	if( m_destObject )
	{
		m_destObject->UpdateVisibility( updateContext, destMatrix, TR2_LOD_HIGH );
	}

	if( m_stretchObject )
	{
		// We need to figure out a better way of doing this, maybe we need to be able to pass in
		// an inverse modifier to update this correctly
		// Currently this will almost always be visible, because of the stretch
		// If we can not make it stretched, then we could make it invisible sooner
		m_stretchObject->UpdateVisibility( updateContext, parentTransform, TR2_LOD_HIGH );
	}

	if( m_moveObject )
	{
		Vector3 movedPostition = Lerp( m_sourcePosition, m_destinationPosition, m_moveProgression->m_value );
		Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
		TriQuaternionArcFromForward( &rotation, &directionVec );
		auto moveMatrix = TransformationMatrix( Vector3( 1, 1, 1 ), rotation, movedPostition );

		m_moveObject->UpdateVisibility( updateContext, moveMatrix, TR2_LOD_HIGH );
	}
}

void EveStretch3::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	GetRenderables( renderables, nullptr );
}

void EveStretch3::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}

	RunOnComponents( [&renderables]( IEveSpaceObjectChild* c ) { c->GetRenderables( renderables ); } );
}

void EveStretch3::SetDisplay( bool display )
{
	m_display = display;
	ReRegister();
}

bool EveStretch3::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	bool valid = false;

	Vector4 v;
	if( m_destObject )
	{
		if( m_destObject->GetBoundingSphere( v, query ) )
		{
			sphere = v;
			valid = true;
		}
	}
	if( m_sourceObject )
	{
		if( m_sourceObject->GetBoundingSphere( v, query ) )
		{
			BoundingSphereSetOrUpdate( v, sphere, valid );
			valid = true;
		}
	}
	if( m_stretchObject )
	{
		if( m_stretchObject->GetBoundingSphere( v, query ) )
		{
			BoundingSphereSetOrUpdate( v, sphere, valid );
			valid = true;
		}
	}
	if( m_moveObject )
	{
		if( m_moveObject->GetBoundingSphere( v, query ) )
		{
			BoundingSphereSetOrUpdate( v, sphere, valid );
			valid = true;
		}
	}

	return valid;
}

float EveStretch3::GetCurveDuration()
{
	float maxDuration = 0;
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		maxDuration = std::max( maxDuration, ( *it )->GetMaxCurveDuration() / ( *it )->GetTimeScale() );
	}

	return maxDuration;
}

void EveStretch3::StartFiring( float delay )
{
	// can't change the controllers since this could be called asyncronously
	m_delay = delay;
	m_stretchState = STRETCH_STATE_STARTING;
}

void EveStretch3::StopFiring()
{
	// can't change the controllers since this could be called asyncronously
	m_stretchState = STRETCH_STATE_STOPPING;
}

void EveStretch3::SetFiringTransform( const Matrix& source, const Vector3& dest )
{
	// this means the muzzle effect should not be rotated
	m_isMuzzleEffect = true;

	m_dest = nullptr;
	m_source = nullptr;

	m_sourcePosition = source.GetTranslation();
	m_destinationPosition = dest;
	m_sourceMatrix = source;
}

void EveStretch3::SetFiringTransform( const Vector3& source, const Vector3& dest )
{
	// this means we need to calculate the direction to the destination
	m_isMuzzleEffect = false;

	m_dest = nullptr;
	m_source = nullptr;
	m_sourcePosition = source;
	m_destinationPosition = dest;
	m_sourceMatrix = TranslationMatrix( source );
}

void EveStretch3::DisplayEndPoints( bool displaySource, bool displayDest )
{
}

void EveStretch3::RegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry && m_display )
	{
		RunOnComponents( 
			[this]( IEveSpaceObjectChild* c ) 
			{ 
				if( EveEntityPtr entity = BlueCastPtr( c ) )
				{
					entity->Register( GetComponentRegistry() );
				}
			} 
		);
	}
}

void EveStretch3::UnRegisterComponents()
{
	auto registry = this->GetComponentRegistry();
	if( registry )
	{
		RunOnComponents( 
			[this]( IEveSpaceObjectChild* c ) 
			{ 
				if( EveEntityPtr entity = BlueCastPtr( c ) )
				{
					entity->UnRegister( GetComponentRegistry() );
				}
			} 
		);
	}
}

void EveStretch3::GetBindingRoots( std::unordered_map<std::string, IRoot*>& variables )
{
	ITr2ControllerOwner::GetBindingRoots( variables );

	variables["Source"] = m_sourceObject;
	variables["Dest"] = m_destObject;
	variables["Stretch"] = m_stretchObject;
	variables["Move"] = m_moveObject;
	variables["SourceSpaceObject"] = m_sourceSpaceObject;
	variables["DestSpaceObject"] = m_destSpaceObject;
}


void EveStretch3::SetControllerVariable( const char* name, float value )
{
	RunOnComponents( [name, value]( IEveSpaceObjectChild* c ) { c->SetControllerVariable( name, value ); } );

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->SetVariable( name, value );
	}
}


void EveStretch3::HandleControllerEvent( const char* name )
{
	RunOnComponents( [name]( IEveSpaceObjectChild* c ) { c->HandleControllerEvent( name ); } );

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->HandleEvent( name );
	}
}

void EveStretch3::StartControllers()
{
	RunOnComponents( []( IEveSpaceObjectChild* c ) { c->StartControllers(); } );

	for( auto it = begin( m_controllers ); it != end( m_controllers ); ++it )
	{
		( *it )->Start();
	}
}


void EveStretch3::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
	if( !m_display )
	{
		return;
	}
	RunOnComponents( [&quadRenderer]( IEveSpaceObjectChild* c ) { c->RegisterWithQuadRenderer( quadRenderer ); } );
}

void EveStretch3::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer )
{
	if( !m_display )
	{
		return;
	}

	RunOnComponents( [&frustum, &quadRenderer]( IEveSpaceObjectChild* c ) { c->AddQuadsToQuadRenderer( frustum, quadRenderer ); } );
}

void EveStretch3::UpdateModelCenterWorldPosition( Vector3& position, Be::Time t )
{
}

void EveStretch3::GetModelCenterWorldPosition( Vector3& position ) const
{
}

bool EveStretch3::GetLocalBoundingBox( Vector3& min, Vector3& max )
{
	return false;
}

void EveStretch3::GetLocalToWorldTransform( Matrix& transform ) const
{
}

void EveStretch3::SetDestObjectScale( float scale )
{
	m_destObjectScale = scale;
}

void EveStretch3::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Lights" );
	RunOnComponents( [&options]( IEveSpaceObjectChild* c ) {
		auto debugRenderable = dynamic_cast<ITr2DebugRenderable*>( c );
		if( debugRenderable )
		{
			debugRenderable->GetDebugOptions( options );
		}
	} );

	if( auto tmp = dynamic_cast<Tr2AudioStretchBase*>( m_audio.p ) )
	{
		tmp->GetDebugOptions( options );
	}
}

void EveStretch3::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !m_display )
	{
		return;
	}
	RunOnComponents( [&renderer]( IEveSpaceObjectChild* c ) {
		auto debugRenderable = dynamic_cast<ITr2DebugRenderable*>( c );
		if( debugRenderable )
		{
			debugRenderable->RenderDebugInfo( renderer );
		}
	} );

	if( auto tmp = dynamic_cast<Tr2AudioStretchBase*>( m_audio.p ) )
	{
		tmp->RenderDebugInfo( renderer );
	}
}

void EveStretch3::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
	if( !m_display )
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

	RunOnComponents( [name, rangeName]( IEveSpaceObjectChild* c ) {
		auto curveSetOwner = dynamic_cast<ITr2CurveSetOwner*>( c );
		if( curveSetOwner )
		{
			curveSetOwner->PlayCurveSet( name, rangeName );
		}
	} );
}

void EveStretch3::StopCurveSet( const std::string& name )
{
	if( !m_display )
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

	RunOnComponents( [name]( IEveSpaceObjectChild* c ) {
		auto curveSetOwner = dynamic_cast<ITr2CurveSetOwner*>( c );
		if( curveSetOwner )
		{
			curveSetOwner->StopCurveSet( name );
		}
	} );
}

void EveStretch3::UpdateCurveSet( const std::string& name, Be::Time time )
{
	for( auto it = m_curveSets.begin(); it != m_curveSets.end(); it++ )
	{
		if( ( *it )->GetName() == name )
		{
			( *it )->Update( time, time );
		}
	}

	RunOnComponents( [name, time]( IEveSpaceObjectChild* c ) {
		auto curveSetOwner = dynamic_cast<ITr2CurveSetOwner*>( c );
		if( curveSetOwner )
		{
			curveSetOwner->UpdateCurveSet( name, time );
		}
	} );
}

float EveStretch3::GetCurveSetDuration( const std::string& name ) const
{
	if( !m_display )
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
	float componentMax = RunOnComponentsGetMax( [name]( IEveSpaceObjectChild* c ) -> float {
		auto curveSetOwner = dynamic_cast<ITr2CurveSetOwner*>( c );
		if( curveSetOwner )
		{
			return curveSetOwner->GetCurveSetDuration( name );
		}
		return 0.0f;
	} );

	return max( maxDuration, componentMax );
}

float EveStretch3::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	if( !m_display )
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

	float componentMax = RunOnComponentsGetMax( [name, rangeName]( IEveSpaceObjectChild* c ) -> float {
		auto curveSetOwner = dynamic_cast<ITr2CurveSetOwner*>( c );
		if( curveSetOwner )
		{
			return curveSetOwner->GetRangeDuration( name, rangeName );
		}
		return 0.0f;
	} );

	return max( maxDuration, componentMax );
}

ITr2AudEmitterPtr EveStretch3::FindSoundEmitter( const char* name )
{
	if( auto tmp = dynamic_cast<Tr2AudioStretchBase*>( m_audio.p ) )
	{
		return tmp->FindEmitterByName( name );
	}
	return nullptr;
}
