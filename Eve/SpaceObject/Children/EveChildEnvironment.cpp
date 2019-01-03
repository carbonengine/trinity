#include "StdAfx.h"
#include "EveChildEnvironment.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"
#include "TriUtil.h"
#include "Utilities/BoundingSphere.h"


EveSphereVolume::EveSphereVolume( IRoot* lockobj ) :
	m_position( 0, 0, 0 ),
	m_centerOffset( 0, 0, 0 ),
	m_radius( 0 ),
	m_innerRadius( 0 ),
	m_notifyParent( false )
{
}

EveSphereVolume::~EveSphereVolume()
{
}

void EveSphereVolume::RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform )
{
	renderer.DrawSphere( this, TranslationMatrix( m_position ) * parentTransform, m_radius, 20, Tr2DebugRenderer::Wireframe, 0xff555555 );
	renderer.DrawSphere( this, TranslationMatrix( m_position + m_centerOffset ) * parentTransform, max(m_innerRadius, 1.0f), 20, Tr2DebugRenderer::Wireframe, 0xff777777 );
}

Vector4 EveSphereVolume::GetBoundingSphere() const
{
	return Vector4( m_position, m_radius );
}

float EveSphereVolume::GetIntensity( Vector3 cameraPosition )
{
	// are we inside of the outer radius?
	float radiusSq = m_radius * m_radius;
	if( LengthSq( cameraPosition - m_position ) > radiusSq )
	{
		return 0;
	}

	Vector3 line = cameraPosition - m_position - m_centerOffset;
	float distanceToInnerRadiusSq = LengthSq( line );
	float innerRadiusSq = m_innerRadius * m_innerRadius;
	if( distanceToInnerRadiusSq <  innerRadiusSq )
	{
		return 1;
	}

	return 1.0f - distanceToInnerRadiusSq / (radiusSq - innerRadiusSq);
	/*
	// TODO make math good! 

	// find the intersection line between the camera position and the center offset
	// determening the intensity on how far we are from that line to the inner radius
	line = m_centerOffset - cameraPosition;
	Vector3 centerToOffset = m_centerOffset - m_position;

	// project the center to the line
	Vector3 centerProj = m_centerOffset + Dot( centerToOffset, line ) / Dot( line, line ) * line;

	//
	//   Cam --x----- CO
	//         |
	//         |
	//         P
	//
		
	float centerProjToBoundsSq = radiusSq - LengthSq( centerProj - m_position );
			
	float cameraPosToCenterProjSq = LengthSq( centerProj - cameraPosition );
	float centerOffsetToCenterProjSq = LengthSq( centerProj - m_centerOffset ) - innerRadiusSq;

	float totalDistance = centerOffsetToCenterProjSq + centerProjToBoundsSq;
	float cameraDistance = LengthSq(cameraPosition - m_centerOffset) - innerRadiusSq;

	return 1.0f - cameraDistance / totalDistance;
	*/
}

void EveSphereVolume::RegisterForChanges( std::function<void()> NotifyParent )
{
	m_notifyParentFunc = NotifyParent;
	m_notifyParent = true;
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveSphereVolume::OnModified( Be::Var* val )
{
	m_innerRadius = min( m_innerRadius, m_radius );

	if( m_notifyParent && ( IsMatch( val, m_position ) || IsMatch( val, m_radius ) ) )
	{
		m_notifyParentFunc();
	}
	return true;
}

EveBoxVolume::EveBoxVolume( IRoot* lockobj ) :
	m_position( 0, 0, 0 ),
	m_centerOffset( 0, 0, 0 ),
	m_scaling( 0, 0, 0 ),
	m_innerScaling( 0, 0, 0 ),
	m_rotation( 0, 0, 0, 1 ),
	m_notifyParent( false )
{
}

EveBoxVolume::~EveBoxVolume()
{
}

void EveBoxVolume::RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& parentTransform )
{
	Vector3 min, max;
	max = Vector3( 0.5f, 0.5f, 0.5f );
	min = Vector3( -0.5f, -0.5f, -0.5f ); 
	
	renderer.DrawBox( this, m_boxTransform * parentTransform, min, max, Tr2DebugRenderer::Wireframe, 0xff555555);
	renderer.DrawBox( this, m_centerTransform * parentTransform , min, max, Tr2DebugRenderer::Wireframe, 0xff777777 );
}

Vector4 EveBoxVolume::GetBoundingSphere() const
{
	return Vector4( m_position, Length(m_scaling) * 0.5f );
}

float EveBoxVolume::GetIntensity( Vector3 cameraPosition )
{
	// This is probably wrong, need test cases!
	Vector3 cameraInVolumeSpace = Transform( cameraPosition, m_inverseBoxTransform );
	float distFromCenterSq = LengthSq( cameraInVolumeSpace );
	float radiusSq = LengthSq( m_scaling ) * 0.5f;
	if( distFromCenterSq > radiusSq )
	{
		return 0;
	}
	
	float maxOuterSize = max( m_scaling.x, max( m_scaling.y, m_scaling.z ) );
	float maxInnerSize = max( m_innerScaling.x, max( m_innerScaling.y, m_innerScaling.z ) );

	float distFromInnerBox = (maxOuterSize - maxInnerSize) * 0.5f;
	
	float xDist = max( abs( cameraInVolumeSpace.x - m_position.x ) - m_innerScaling.x * 0.5f, 0.0f );
	float yDist = max( abs( cameraInVolumeSpace.y - m_position.y ) - m_innerScaling.y * 0.5f, 0.0f );
	float zDist = max( abs( cameraInVolumeSpace.z - m_position.z ) - m_innerScaling.z * 0.5f, 0.0f );

	float distSq =  xDist * xDist + yDist * yDist + zDist * zDist;
	
	return 1.0f - distSq / ( distFromInnerBox * distFromInnerBox );
	
}

void EveBoxVolume::RegisterForChanges( std::function<void()> NotifyParent )
{
	m_notifyParentFunc = NotifyParent;
	m_notifyParent = true;
}

//////////////////////////////////////////////////////////////////////////
// INotify
bool EveBoxVolume::OnModified( Be::Var* val )
{
	m_innerScaling.x = min( m_innerScaling.x, m_scaling.x );
	m_innerScaling.y = min( m_innerScaling.y, m_scaling.y );
	m_innerScaling.z = min( m_innerScaling.z, m_scaling.z );
		
	if( m_notifyParent && ( IsMatch( val, m_position ) || IsMatch( val, m_scaling ) ) )
	{
		m_notifyParentFunc();
	}
	if( IsMatch( val, m_position ) || IsMatch( val, m_rotation ) || IsMatch( val, m_scaling ) || IsMatch( val, m_innerScaling ) )
	{
		m_boxTransform = TransformationMatrix( m_scaling, m_rotation, m_position );
		m_centerTransform = TransformationMatrix( m_innerScaling, m_rotation, m_position + m_centerOffset );

		m_inverseBoxTransform = Inverse( TransformationMatrix( Vector3( 1.0, 1.0, 1.0 ), m_rotation, m_position ) );
		m_inverseCenterTransform = Inverse( m_centerTransform );
	}
	return true;
}


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
		Vector3 cameraInObjectSpace = Transform( Tr2Renderer::GetViewPosition(), inverseWorldTransform );

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

void EveChildEnvironment::GetBatches( ITriRenderBatchAccumulator* batches, TriBatchType batchType, const Tr2PerObjectData* perObjectData )
{

}

void EveChildEnvironment::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
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
	options.insert( "Bounding Sphere" );
}

void EveChildEnvironment::RenderDebugInfo( Tr2DebugRenderer& renderer )
{
	//if (renderer.HasOption( this, "Volumes" ))
	{
		for( auto volume = m_volumes.begin(); volume != m_volumes.end(); ++volume )
		{
			(*volume)->RenderDebugInfo( renderer, m_worldTransform );
		}
	}
	//if( renderer.HasOption( this, "Bounding Sphere" ) )
	{
		renderer.DrawSphere( this, TranslationMatrix( m_boundingSphere.GetXYZ() ) * m_worldTransform, m_boundingSphere.w, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
	}
}