// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "EveChildFogVolume.h"
#include "Tr2Renderer.h"


EveChildFogVolume::EveChildFogVolume( IRoot* lockobj ) :
	PARENTLOCK( m_volumes ),
	m_intensity( 1.f ),
	m_boundingSphere( Vector3( 0.0, 0.0, 0.0 ), 0.0 )
{

	//Set some default values
	m_settings.thickness = 1.0f;
	m_settings.lightDirectionality = 0.5f;
	m_settings.environmentIntensity = 1.0f;
	m_settings.environmentDirectionality = 0.75f;
	m_settings.fogColor = Color( 1.0f, 1.0f, 1.0f, 1.0f );
	m_settings.backgroundVisibility = 0.0f;

	m_settings.godRayNoiseIntensity = 0.0f;
	m_settings.godRayNoiseFrequency = 15.0f;
	m_settings.godRayNoiseAnimationSpeed = 0.0f;

	m_settings.fogNoiseIntensity = 0.0f;
	m_settings.fogNoiseFrequency = 15.0f;
	m_settings.fogNoiseMovementSpeed = Vector3( 0.0f, 0.0f, 0.0f );

	// The proper value is set in the UpdateAsyncronous method
	m_settings.intensity = 0.0f;
}

void EveChildFogVolume::RebuildBoundingSphere()
{
	m_boundingSphere = CcpMath::Sphere( Vector3( 0.0, 0.0, 0.0 ), 0.0 );

	for( auto& volume : m_volumes )
	{
		auto volumeSphere = volume->GetBoundingSphere();

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

void EveChildFogVolume::RegisterComponents()
{
	GetComponentRegistry()->RegisterComponent<ITr2FroxelFogSettings>( this );
}

void EveChildFogVolume::UnRegisterComponents()
{
	GetComponentRegistry()->UnRegisterComponent<ITr2FroxelFogSettings>( this );
}

/////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObjectChild
const char* EveChildFogVolume::GetName() const
{
	return m_name.c_str();
}

void EveChildFogVolume::SetName( const char* name )
{
	m_name = BlueSharedString( name );
}

void EveChildFogVolume::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform, Tr2Lod parentLod )
{
}

bool EveChildFogVolume::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere.GetXYZ() = m_boundingSphere.center;
	sphere.w = m_boundingSphere.radius;

	return true;
}

void EveChildFogVolume::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
}

void EveChildFogVolume::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	UpdateTransformFromParent( params );
	RebuildBoundingSphere();

	// global postprocess volumes have no volumes, so they are always on
	if( m_volumes.empty() )
	{
		m_settings.intensity = m_intensity;
	}
	else
	{
		m_settings.intensity = 0.0f;

		Matrix inverseWorldTransform = Inverse( m_worldTransform );
		Vector3 cameraInObjectSpace = Transform( Tr2Renderer::GetViewPosition(), inverseWorldTransform ).GetXYZ();

		// check first if the camera position is within the environment bounding box
		if( m_boundingSphere.IsPointInside( cameraInObjectSpace ) )
		{
			// Now find the intensity within the volumes
			for( auto& volume : m_volumes )
			{
				m_settings.intensity = std::max( m_settings.intensity, volume->GetIntensity( cameraInObjectSpace ) );
				if( m_settings.intensity == 1.0f )
				{
					// early exit
					break;
				}
			}
			m_settings.intensity *= m_intensity;
		}
	}
}

void EveChildFogVolume::UpdateTransformFromParent( const EveChildUpdateParams& params )
{
	Matrix localToWorldTransform = params.localToWorldTransform;

	UpdateTransform( localToWorldTransform );
}

void EveChildFogVolume::GetLocalToWorldTransform( Matrix& transform ) const
{
}

void EveChildFogVolume::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	// call base class's setup
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

bool EveChildFogVolume::IsAlwaysOn() const
{
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool EveChildFogVolume::Initialize()
{
	RebuildBoundingSphere();
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void EveChildFogVolume::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Volumes" );
	options.insert( "Bounding Sphere" );
}

void EveChildFogVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( this, "Volumes" ) )
	{
		for( auto& volume : m_volumes )
		{
			volume->RenderDebugInfo( renderer, m_worldTransform );
		}
	}

	if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.center ) * m_worldTransform, m_boundingSphere.radius, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}

ITr2FroxelFogSettings::FroxelFogSettings* EveChildFogVolume::GetFroxelFogSettings()
{
	return &m_settings;
}
