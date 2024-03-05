////////////////////////////////////////////////////////////
//
//    Created:   November 2021
//    Copyright: CCP 2021
//
#include "Resources/Tr2LodResource.h"
#include "EveChildProceduralContainer.h"
#include "Utilities/BoundingSphere.h"

EveChildProceduralContainer::EveChildProceduralContainer( IRoot* lockobj ) :
	EveChildTransform(),
    PARENTLOCK( m_transformModifiers ),
    m_proceduralContainerVariables( "EveChildContainer::m_proceduralContainerVariables" ),
    m_display( true )
{
}

EveChildProceduralContainer::~EveChildProceduralContainer()
{
}

const char* EveChildProceduralContainer::GetName() const
{
    return m_name.c_str();
}

void EveChildProceduralContainer::SetName( const char* name )
{
    m_name = BlueSharedString( name );
}

bool EveChildProceduralContainer::Initialize()
{
	return true;
}

void EveChildProceduralContainer::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list )
{
}

void EveChildProceduralContainer::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{
	if( !m_display )
	{
		return;
	}

    if ( nullptr != m_selectedObject )
    {
	    m_selectedObject->UpdateVisibility( frustum, parentTransform, parentLod );
    }
}

void EveChildProceduralContainer::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
    if ( m_display && m_selectedObject )
    {
        m_selectedObject->GetRenderables( renderables );
    }
}

bool EveChildProceduralContainer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
    bool success = false;
    Vector4 bSphere( 0.f, 0.f, 0.f, -1.f );
    if ( m_selectedObject && m_selectedObject->GetBoundingSphere( bSphere ) )
    {
        BoundingSphereSetOrUpdate( bSphere, sphere, success );
        success = true;
    }
    return success;
}

void EveChildProceduralContainer::RegisterWithQuadRenderer( Tr2QuadRenderer& quadRenderer )
{
    if ( nullptr != m_selectedObject )
    {
	    m_selectedObject->RegisterWithQuadRenderer( quadRenderer );
    }
}

void EveChildProceduralContainer::AddQuadsToQuadRenderer( const TriFrustum& frustum, Tr2QuadRenderer& quadRenderer ) const
{
	if( !m_display )
	{
		return;
	}
    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->AddQuadsToQuadRenderer(frustum, quadRenderer);
    }
}

void EveChildProceduralContainer::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->UpdateSyncronous(updateContext, newParams);
    }

	if( nullptr != m_selectionMethod && m_selectionMethod->IsSelectedChildModified() )
	{
		ConfigureSelectedObject();
	}
}

void EveChildProceduralContainer::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
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

	for( auto it = m_transformModifiers.begin(); it != m_transformModifiers.end(); it++ )
	{
		m_worldTransform = ( *it )->ApplyTransform( m_worldTransform, params.boneCount, params.bones );
	}

	EveChildUpdateParams newParams = params;
	newParams.isVisible &= m_display;
	newParams.childParent = this;

    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->UpdateAsyncronous(updateContext, newParams);
    }

    if ( nullptr != m_selectionMethod )
    {
        m_selectionMethod->UpdateASyncronous(updateContext, newParams);
    }
}

void EveChildProceduralContainer::ConfigureSelectedObject()
{
    EveChildRefPtr child = m_selectionMethod->GetSelectedChild();
    if( child != nullptr )
    {
        for( auto it = begin( m_proceduralContainerVariables ); it != end( m_proceduralContainerVariables ); ++it )
        {
            child->SetProceduralContainerVariable( it->first.c_str(), it->second );
        }
    }
    m_selectedObject = child;
}

void EveChildProceduralContainer::SetProceduralContainerVariable(const char *name, float value)
{
    m_proceduralContainerVariables[name] = value;
    if( m_selectionMethod )
    {
        m_selectionMethod->SetProceduralMethodVariable( name, value );
    }
}

const char* EveChildProceduralContainer::GetMethodVariableName()
{
    const char* name = "methodUnassigned";

    if( m_selectionMethod )
    {
        name = m_selectionMethod->GetProceduralMethodVariable();
    }

    return name;
}

//  --- other interface functions ---

void EveChildProceduralContainer::GetLocalToWorldTransform( Matrix& transform ) const
{
	transform = m_worldTransform;
}

void EveChildProceduralContainer::ChangeLOD( Tr2Lod lod )
{
    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->ChangeLOD( lod );
    }
}

void EveChildProceduralContainer::PlayCurveSet( const std::string& name, const std::string& rangeName )
{
    if( auto owner = dynamic_cast<ITr2CurveSetOwner*>(  &(*m_selectedObject) ) )
    {
        owner->PlayCurveSet( name, rangeName );
    }
}

void EveChildProceduralContainer::PlayAllCurveSets()
{
    if( auto child = dynamic_cast<ITr2CurveSetOwner*>( &(*m_selectedObject) ) )
    {
        child->PlayAllCurveSets();
    }
}

void EveChildProceduralContainer::StopAllCurveSets()
{
    if( auto child = dynamic_cast<ITr2CurveSetOwner*>( &(*m_selectedObject) ) )
    {
        child->StopAllCurveSets();
    }
}

void EveChildProceduralContainer::StopCurveSet( const std::string& name )
{
    if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( &(*m_selectedObject) ) )
    {
        owner->StopCurveSet( name );
    }
}

void EveChildProceduralContainer::UpdateCurveSet( const std::string& name, Be::Time time )
{
    if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( &(*m_selectedObject) ) )
    {
        owner->UpdateCurveSet( name, time );
    }
}

float EveChildProceduralContainer::GetCurveSetDuration( const std::string& name ) const
{
	float maxDuration = 0.f;

    if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( &(*m_selectedObject) ) )
    {
        maxDuration = max( maxDuration, owner->GetCurveSetDuration( name ) );
    }

	return maxDuration;
}

float EveChildProceduralContainer::GetRangeDuration( const std::string& name, const std::string& rangeName ) const
{
	float maxDuration = 0.f;

    if( auto owner = dynamic_cast<ITr2CurveSetOwner*>( &(*m_selectedObject) ) )
    {
        maxDuration = max( maxDuration, owner->GetRangeDuration( name, rangeName ) );
    }

	return maxDuration;
}

void EveChildProceduralContainer::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

void EveChildProceduralContainer::SetControllerVariable( const char* name, float value )
{
    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->SetControllerVariable( name, value );
    }
}

void EveChildProceduralContainer::HandleControllerEvent( const char* name )
{
    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->HandleControllerEvent(name);
    }
}

void EveChildProceduralContainer::StartControllers()
{
    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->StartControllers();
    }
}

ITr2AudEmitterPtr EveChildProceduralContainer::FindSoundEmitter( const char* name )
{
    if( auto owner = dynamic_cast<ITr2SoundEmitterOwner*>( &(*m_selectedObject) ) )
    {
        auto emitter = owner->FindSoundEmitter( name );
        if( emitter != nullptr )
        {
            return emitter;
        }
    }
	return nullptr;
}

void EveChildProceduralContainer::AddTransformModifier( IEveChildTransformModifier* modifier )
{
	m_transformModifiers.Append( modifier );
}

void EveChildProceduralContainer::SetShaderOption( const BlueSharedString& name, const BlueSharedString& value )
{
    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->SetShaderOption(name, value);
    }
}

void EveChildProceduralContainer::GetLights( Tr2LightManager& lightManager ) const
{
    if( !m_display )
    {
        return;
    }

    if ( nullptr != m_selectedObject )
    {
        m_selectedObject->GetLights(lightManager);
    }
}

void EveChildProceduralContainer::SetInheritProperties( const Color* colorSet )
{
    if ( nullptr != m_selectedObject )
    {
		if( IEveInheritPropertiesOwnerPtr child = BlueCastPtr( m_selectedObject ) )
		{
			child->SetInheritProperties( colorSet );
		}
    }
	//TODO instead set properties on the m_selectionMethod
}

void EveChildProceduralContainer::GetDebugOptions( Tr2DebugRendererOptions& options )
{
    if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( &(*m_selectedObject) ) )
    {
        renderable->GetDebugOptions( options );
    }
    options.insert( "ProceduralVolumes" );
}

void EveChildProceduralContainer::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
    if( !m_display )
    {
        return;
    }

    if( auto renderable = dynamic_cast<ITr2DebugRenderable*>( &(*m_selectedObject) ) )
    {
        renderable->RenderDebugInfo( renderer );
    }

    if( renderer.HasOption( this, "ProceduralVolumes" ) )
    {
        if( m_selectionMethod != nullptr )
        {
            IEveVolumeVector* debugVolumes = m_selectionMethod->GetDebugVolumes();

            for( auto volume = debugVolumes->begin(); volume != debugVolumes->end(); ++volume )
            {
                ( *volume )->RenderDebugInfo( renderer, m_worldTransform );
            }
        }
    }
}