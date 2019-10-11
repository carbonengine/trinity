#include "StdAfx.h"
#include "EveBoxVolume.h"
#include "ITr2Renderable.h"
#include "Tr2Renderer.h"

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

void EveBoxVolume::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& parentTransform )
{
	Vector3 min, max;
	max = Vector3( 0.5f, 0.5f, 0.5f );
	min = Vector3( -0.5f, -0.5f, -0.5f );

	renderer.DrawBox( this, m_boxTransform * parentTransform, min, max, Tr2DebugRenderer::Wireframe, 0xff555555 );
	renderer.DrawBox( this, m_centerTransform * parentTransform, min, max, Tr2DebugRenderer::Wireframe, 0xff777777 );
}

Vector4 EveBoxVolume::GetBoundingSphere() const
{
	return Vector4( m_position, Length( m_scaling ) * 0.5f );
}

float EveBoxVolume::GetIntensity( Vector3 cameraPosition )
{
	// This is probably wrong, need test cases!
	Vector3 cameraInVolumeSpace = Transform( cameraPosition, m_inverseBoxTransform ).GetXYZ();
	float distFromCenterSq = LengthSq( cameraInVolumeSpace );
	float radiusSq = LengthSq( m_scaling ) * 0.5f;
	if (distFromCenterSq > radiusSq)
	{
		return 0;
	}

	float maxOuterSize = max( m_scaling.x, max( m_scaling.y, m_scaling.z ) );
	float maxInnerSize = max( m_innerScaling.x, max( m_innerScaling.y, m_innerScaling.z ) );

	float distFromInnerBox = (maxOuterSize - maxInnerSize) * 0.5f;

	float xDist = max( abs( cameraInVolumeSpace.x - m_position.x ) - m_innerScaling.x * 0.5f, 0.0f );
	float yDist = max( abs( cameraInVolumeSpace.y - m_position.y ) - m_innerScaling.y * 0.5f, 0.0f );
	float zDist = max( abs( cameraInVolumeSpace.z - m_position.z ) - m_innerScaling.z * 0.5f, 0.0f );

	float distSq = xDist * xDist + yDist * yDist + zDist * zDist;

	return 1.0f - distSq / (distFromInnerBox * distFromInnerBox);

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

	if (m_notifyParent && (IsMatch( val, m_position ) || IsMatch( val, m_scaling )))
	{
		m_notifyParentFunc();
	}
	if (IsMatch( val, m_position ) || IsMatch( val, m_rotation ) || IsMatch( val, m_scaling ) || IsMatch( val, m_innerScaling ))
	{
		m_boxTransform = TransformationMatrix( m_scaling, m_rotation, m_position );
		m_centerTransform = TransformationMatrix( m_innerScaling, m_rotation, m_position + m_centerOffset );

		m_inverseBoxTransform = Inverse( TransformationMatrix( Vector3( 1.0, 1.0, 1.0 ), m_rotation, m_position ) );
		m_inverseCenterTransform = Inverse( m_centerTransform );
	}
	return true;
}