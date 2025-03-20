#include "StdAfx.h"
#include "EveChildLightingOverride.h"
#include "Tr2Renderer.h"

IEveLightingOverride::Overrides IEveLightingOverride::Overrides::operator*( float rhs ) const
{
	Overrides result = *this;
	result.sunColor *= rhs;
	result.sunIntensity *= rhs;
	result.backgroundIntensity *= rhs;
	result.reflectionIntensity *= rhs;
	return result;
}

IEveLightingOverride::Overrides IEveLightingOverride::Overrides::operator + ( const Overrides& rhs ) const
{
	Overrides result = *this;
	result.sunColor += rhs.sunColor;
	result.sunIntensity += rhs.sunIntensity;
	result.backgroundIntensity += rhs.backgroundIntensity;
	result.reflectionIntensity += rhs.reflectionIntensity;
	return result;
}


EveChildLightingOverride::EveChildLightingOverride( IRoot* lockobj ) :
	PARENTLOCK( m_volumes ),
	m_intensity( 1.0f ),
	m_boundingSphere( Vector3( 0.0, 0.0, 0.0 ), 0.0 )
{
	m_overrides.value.sunColor = Color( 1, 1, 1, 1 );
	m_overrides.value.sunIntensity = 1;
	m_overrides.value.backgroundIntensity = 1;
	m_overrides.value.reflectionIntensity = 1;
}

EveChildLightingOverride::OverrideInfo EveChildLightingOverride::GetOverrides() const 
{
	return m_overrides;
}

void EveChildLightingOverride::RegisterComponents()
{
	GetComponentRegistry()->RegisterComponent<IEveLightingOverride>( this );
}

void EveChildLightingOverride::UnRegisterComponents()
{
	GetComponentRegistry()->UnRegisterComponent<IEveLightingOverride>( this );
}

void EveChildLightingOverride::RebuildBoundingSphere()
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

const char* EveChildLightingOverride::GetName() const
{
	return m_name.c_str();
}

void EveChildLightingOverride::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

bool EveChildLightingOverride::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere.GetXYZ() = m_boundingSphere.center;
	sphere.w = m_boundingSphere.radius;

	return true;
}

void EveChildLightingOverride::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
}

void EveChildLightingOverride::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
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
		localToWorldTransform = IdentityMatrix();
	}
	UpdateTransform( localToWorldTransform );
	RebuildBoundingSphere();

	if( m_volumes.empty() )
	{
		m_overrides.intensity = 1.0f;
	}
	else
	{
		m_overrides.intensity = 0.0f;

		Matrix inverseWorldTransform = Inverse( m_worldTransform );
		Vector3 cameraInObjectSpace = Transform( Tr2Renderer::GetViewPosition(), inverseWorldTransform ).GetXYZ();

		// check first if the camera position is within the environment bounding box
		if( m_boundingSphere.IsPointInside( cameraInObjectSpace ) )
		{
			// Now find the intensity within the volumes
			for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
			{
				m_overrides.intensity = std::max( m_overrides.intensity, ( *volume )->GetIntensity( cameraInObjectSpace ) );
				if( m_overrides.intensity == 1.0f )
				{
					// early exit
					break;
				}
			}
		}
	}
	m_overrides.intensity *= m_intensity;
}

void EveChildLightingOverride::GetLocalToWorldTransform( Matrix& transform ) const
{
}

void EveChildLightingOverride::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

bool EveChildLightingOverride::IsAlwaysOn() const
{
	return true;
}

bool EveChildLightingOverride::Initialize()
{
	RebuildBoundingSphere();
	return true;
}


void EveChildLightingOverride::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Volumes" );
	options.insert( "Bounding Sphere" );
}

void EveChildLightingOverride::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( this, "Volumes" ) )
	{
		for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
		{
			( *volume )->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.center ) * m_worldTransform, m_boundingSphere.radius, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}
