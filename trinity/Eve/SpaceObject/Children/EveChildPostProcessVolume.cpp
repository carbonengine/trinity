#include "StdAfx.h"
#include "EveChildPostProcessVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "TriUtil.h"
#include "Utilities/BoundingSphere.h"


EveChildPostProcessVolume::EveChildPostProcessVolume( IRoot* lockobj ) : 
	PARENTLOCK( m_volumes ),
	PARENTLOCK( m_exclusionVolumes ),
	m_boundingSphere( Vector3( 0.0, 0.0, 0.0), 0.0 ),
	m_rebuildBoundingSphereRequired( true )
{
	m_volumes.SetNotify( this );
}

EveChildPostProcessVolume::~EveChildPostProcessVolume()
{

}

void EveChildPostProcessVolume::RebuildBoundingSphere()
{
	m_boundingSphere.center *= 0.0f;
	m_boundingSphere.radius *= 0.0f;

	for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
	{
		auto volumeSphere = ( *volume )->GetBoundingSphere();
	
		// this code is hijacked from carbon-math, it should live there but I don't want to touch carbon-math for now
		if( !volumeSphere.IsInitialized() )
		{
			continue;
		}
		// if sphere is not initialized, just copy it
		// also if the sphere we are including in this sphere, then also copy it
		if( !m_boundingSphere.IsInitialized() || volumeSphere.IsSphereInside( m_boundingSphere ) )
		{
			m_boundingSphere = volumeSphere;
			continue;
		}
		// do not update if is inside
		if( m_boundingSphere.IsSphereInside( volumeSphere ) )
		{
			continue;
		}

		// extend sphere
		Vector3 delta = volumeSphere.center - m_boundingSphere.center;
		float deltaLen = Length( delta );

		m_boundingSphere.center += 0.5f * ( 1.f + ( volumeSphere.radius - m_boundingSphere.radius ) / deltaLen ) * delta;
		m_boundingSphere.radius = 0.5f * ( m_boundingSphere.radius + volumeSphere.radius + deltaLen );
	}
}

void EveChildPostProcessVolume::FlagBoundingSphereRebuildRequired()
{
	m_rebuildBoundingSphereRequired = true;
}

void EveChildPostProcessVolume::RegisterComponents()
{
	GetComponentRegistry()->RegisterComponent<ITr2PostProcessOwner>( this );
}

void EveChildPostProcessVolume::UnRegisterComponents()
{
	GetComponentRegistry()->UnRegisterComponent<ITr2PostProcessOwner>( this );
}

/////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObjectChild
const char* EveChildPostProcessVolume::GetName() const
{
	return m_name.c_str();
}

void EveChildPostProcessVolume::SetName( const char* name )
{
	m_name = BlueSharedString(name);
}

void EveChildPostProcessVolume::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{

}

bool EveChildPostProcessVolume::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere.GetXYZ() = m_boundingSphere.center;
	sphere.w = m_boundingSphere.radius;

	return true;
}

void EveChildPostProcessVolume::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{

}

void EveChildPostProcessVolume::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	UpdateTransformFromParent( params );

	if( m_rebuildBoundingSphereRequired )
	{
		m_rebuildBoundingSphereRequired = false;
		RebuildBoundingSphere();
	}

	// global postprocess volumes have no volumes, so they are always on
	if( m_volumes.size() == 0 )
	{
		m_postProcessAttributeOverrides.intensity = 1.0f;
	}
	else
	{
		m_postProcessAttributeOverrides.intensity = 0.0f;

		Matrix inverseWorldTransform = Inverse( m_worldTransform );
		Vector3 cameraInObjectSpace = Transform( Tr2Renderer::GetViewPosition(), inverseWorldTransform ).GetXYZ();

		// check first if the camera position is within the environment bounding box
		if( m_boundingSphere.IsPointInside( cameraInObjectSpace ) )
		{
			// Now find the intensity within the volumes
			for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
			{
				m_postProcessAttributeOverrides.intensity = std::max( m_postProcessAttributeOverrides.intensity, ( *volume )->GetIntensity( cameraInObjectSpace ) );
				if( m_postProcessAttributeOverrides.intensity == 1.0f )
				{
					// early exit
					break;
				}
			}
		}
	}
}

void EveChildPostProcessVolume::UpdateTransformFromParent( const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform;
	if (params.childParent)
	{
		params.childParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else if (params.spaceObjectParent)
	{
		params.spaceObjectParent->GetLocalToWorldTransform( localToWorldTransform );
	}
	else
	{
		return;
	}

	UpdateTransform( localToWorldTransform );
}

void EveChildPostProcessVolume::GetLocalToWorldTransform( Matrix& transform ) const
{

}

void EveChildPostProcessVolume::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	// call base class's setup
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

bool EveChildPostProcessVolume::IsAlwaysOn() const
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool EveChildPostProcessVolume::Initialize()
{
	for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
	{
		( *volume )->RegisterForChanges( std::bind( &EveChildPostProcessVolume::FlagBoundingSphereRebuildRequired, this ) );
	}

	RebuildBoundingSphere();
	return true;
}


void EveChildPostProcessVolume::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( theList != &m_volumes )
	{
		return;
	}

	m_rebuildBoundingSphereRequired = true;
	switch (event & BELIST_EVENTMASK)
	{
	case BELIST_INSERTED:
		if( IEveVolumePtr volume = BlueCastPtr( value ) )
		{
			volume->RegisterForChanges( std::bind( &EveChildPostProcessVolume::FlagBoundingSphereRebuildRequired, this ) );
		}
	default:
		break;
	};
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void EveChildPostProcessVolume::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Volumes" );
	options.insert( "ExclusionVolumes" );
	options.insert( "Bounding Sphere" );
}

void EveChildPostProcessVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if (renderer.HasOption( this, "Volumes" ))
	{
		for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
		{
			(*volume)->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if (renderer.HasOption( this, "ExclusionVolumes" ))
	{
		for (auto volume = m_exclusionVolumes.begin(); volume != m_exclusionVolumes.end(); ++volume)
		{
			(*volume)->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.center ) * m_worldTransform, m_boundingSphere.radius, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}

PostProcess::Attributes& EveChildPostProcessVolume::GetPostProcessAttributes()
{
	return m_postProcessAttributeOverrides;
}
