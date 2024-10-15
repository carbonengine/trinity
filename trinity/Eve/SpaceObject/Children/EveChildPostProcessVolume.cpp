#include "StdAfx.h"
#include "EveChildPostProcessVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "TriUtil.h"
#include "Utilities/BoundingSphere.h"


EveChildPostProcessVolume::EveChildPostProcessVolume( IRoot* lockobj ) : 
	PARENTLOCK( m_volumes ),
	PARENTLOCK( m_exclusionVolumes ),
	m_boundingSphere( Vector3( 0.0, 0.0, 0.0), 0.0 )
{
	m_postProcessAttributes.CreateInstance();
}

EveChildPostProcessVolume::~EveChildPostProcessVolume()
{
}

void EveChildPostProcessVolume::RebuildBoundingSphere()
{
	CCP_STATS_ZONE( __FUNCTION__ );

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
	CCP_STATS_ZONE( __FUNCTION__ );

	UpdateTransformFromParent( params );

	RebuildBoundingSphere();

	// global postprocess volumes have no volumes, so they are always on
	if( m_volumes.size() == 0 )
	{
		m_postProcessAttributes->intensity = 1.0f;
	}
	else
	{
		m_postProcessAttributes->intensity = 0.0f;

		Matrix inverseWorldTransform = Inverse( m_worldTransform );
		Vector3 cameraInObjectSpace = Transform( Tr2Renderer::GetViewPosition(), inverseWorldTransform ).GetXYZ();

		// check first if the camera position is within the environment bounding box
		if( m_boundingSphere.IsPointInside( cameraInObjectSpace ) )
		{
			// Now find the intensity within the volumes
			for( const auto& volume : m_volumes )
			{
				m_postProcessAttributes->intensity = std::max( m_postProcessAttributes->intensity, volume->GetIntensity( cameraInObjectSpace ) );
				if( m_postProcessAttributes->intensity == 1.0f )
				{
					// early exit
					break;
				}
			}

			if( m_postProcessAttributes->intensity != 0.0f )
			{
				// check if the camera is within an exclusion volume
				float negativeIntensity = 0.0f;
				for( const auto& volume : m_exclusionVolumes )
				{
					negativeIntensity = std::max( negativeIntensity, volume->GetIntensity( cameraInObjectSpace ) );
					if( negativeIntensity == 1.0f )
					{
						// early exit
						break;
					}
				}
				m_postProcessAttributes->intensity = std::max( 0.0f, m_postProcessAttributes->intensity - negativeIntensity );
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
	RebuildBoundingSphere();
	return true;
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
			(*volume)->RenderDebugInfo( renderer, m_worldTransform, 0xFFFFFFFF );
		}
	}

	if (renderer.HasOption( this, "ExclusionVolumes" ))
	{
		for (auto volume = m_exclusionVolumes.begin(); volume != m_exclusionVolumes.end(); ++volume)
		{
			( *volume )->RenderDebugInfo( renderer, m_worldTransform, 0xFFFF3333 );
		}
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.center ) * m_worldTransform, m_boundingSphere.radius, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}

Tr2PostProcessAttributes* EveChildPostProcessVolume::GetPostProcessAttributes()
{
	return m_postProcessAttributes;
}
