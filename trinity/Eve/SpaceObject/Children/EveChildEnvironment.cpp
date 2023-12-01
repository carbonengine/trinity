#include "StdAfx.h"
#include "EveChildEnvironment.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "TriUtil.h"
#include "Utilities/BoundingSphere.h"


EveChildEnvironment::EveChildEnvironment( IRoot* lockobj ) : 
	PARENTLOCK( m_volumes ),
	PARENTLOCK( m_exclusionVolumes ),
	m_environmentIntensity( 0.0f ),
	m_boundingSphere( 0.0, 0.0, 0.0, 0.0 )
{
	m_volumes.SetNotify( this );
}

EveChildEnvironment::~EveChildEnvironment()
{

}

void EveChildEnvironment::RebuildBoundingSphere()
{
	m_boundingSphere = Vector4( 0.0, 0.0, 0.0, 0.0 );
	for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
	{
		Vector4 bs = ( *volume )->GetBoundingSphere();
		if( volume == m_volumes.begin() )
		{ 
			m_boundingSphere = bs;
		}
		else
		{
			BoundingSphereUpdate( bs, m_boundingSphere );
		}
	}
}

void EveChildEnvironment::SetAsDirty() 
{
	m_isDirty = true;
}

/////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObjectChild
const char* EveChildEnvironment::GetName() const
{
	return m_name.c_str();
}

void EveChildEnvironment::SetName( const char* name )
{
	m_name = BlueSharedString(name);
}

void EveChildEnvironment::UpdateVisibility( const TriFrustum& frustum, const Matrix& parentTransform, Tr2Lod parentLod )
{

}

void EveChildEnvironment::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	// here be the fog quads
}

bool EveChildEnvironment::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;

	return true;
}

void EveChildEnvironment::UpdateSyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{

}

void EveChildEnvironment::UpdateAsyncronous( EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	if( m_isDirty )
	{
		m_isDirty = false;
		RebuildBoundingSphere();
	}

	UpdateTransformFromParent( params );

	if( m_volumes.size() == 0 )
	{
		m_environmentIntensity = 1.0f;
	}
	else
	{
		m_environmentIntensity = 0.0f;

		Matrix inverseWorldTransform = Inverse( m_worldTransform );
		Vector3 cameraInObjectSpace = Transform( Tr2Renderer::GetViewPosition(), inverseWorldTransform ).GetXYZ();

		// check first if the camera position is within the environment bounding box
		if( LengthSq( cameraInObjectSpace - m_boundingSphere.GetXYZ() ) <= m_boundingSphere.w * m_boundingSphere.w )
		{
			// Now find the intensity within the volumes
			for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
			{
				m_environmentIntensity = std::max( m_environmentIntensity, ( *volume )->GetIntensity( cameraInObjectSpace ) );
				if( m_environmentIntensity == 1.0f )
				{
					// early exit
					break;
				}
			}
		}
	}
}

void EveChildEnvironment::UpdateTransformFromParent( const EveChildUpdateParams& params )
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

void EveChildEnvironment::GetLocalToWorldTransform( Matrix& transform ) const
{

}

//void EveChildEnvironment::ChangeLOD( Tr2Lod lod ) {};
//void GetLights( Tr2LightManager& lightManager ) const {};

void EveChildEnvironment::Setup( const Vector3* scale, const Quaternion* rotation, const Vector3* translation, Tr2Lod lowestLodVisible )
{
	// call base class's setup
	EveChildTransform::Setup( scale, rotation, translation, lowestLodVisible );
}

bool EveChildEnvironment::IsAlwaysOn() const
{
	return true;
}


/////////////////////////////////////////////////////////////////////////////////////
// ITr2Renderable
bool EveChildEnvironment::HasTransparentBatches()
{
	return false;
}

void EveChildEnvironment::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData, Tr2RenderReason reason )
{

}

float EveChildEnvironment::GetSortValue()
{
	return 0.0f;
}

Tr2PerObjectData* EveChildEnvironment::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool EveChildEnvironment::Initialize()
{
	for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
	{
		( *volume )->RegisterForChanges( std::bind( &EveChildEnvironment::SetAsDirty, this) );
	}

	RebuildBoundingSphere();
	return true;
}


void EveChildEnvironment::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( theList != &m_volumes )
	{
		return;
	}

	switch (event & BELIST_EVENTMASK)
	{
	case BELIST_INSERTED:
		if( IEveVolumePtr volume = BlueCastPtr( value ) )
		{
			volume->RegisterForChanges( std::bind( &EveChildEnvironment::SetAsDirty, this ) );
		}
	case BELIST_REMOVED:
		m_isDirty = true;
		break;
	default:
		break;
	};
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2DebugRenderable
void EveChildEnvironment::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Volumes" );
	options.insert( "ExclusionVolumes" );
	options.insert( "Bounding Sphere" );
}

void EveChildEnvironment::RenderDebugInfo( ITr2DebugRenderer2& renderer )
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
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.GetXYZ() ) * m_worldTransform, m_boundingSphere.w, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}