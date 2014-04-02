#include "StdAfx.h"
#include "Tr2Transform.h"
#include "Tr2Mesh.h"
#include "include/TriMath.h"
#include "Curves/TriCurveSet.h"

Tr2Transform::Tr2Transform( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	m_modifier( TR2TM_NONE ),
	m_scaling( 1.0f, 1.0f, 1.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_translation( 0.0f, 0.0f, 0.0f ),
	m_display( true ),
	m_update( true ),
	m_useDistanceBasedScale( false ),
	m_distanceBasedScaleArg1( 0.2f ),
	m_distanceBasedScaleArg2( 0.63f ),
	m_sortValueMultiplier( 1.0f )
{
	D3DXMatrixIdentity( &m_worldTransform );
	D3DXMatrixIdentity( &m_localTransform );
}

void Tr2Transform::Update( Be::Time time )
{
	if( !m_update )
	{
		return;
	}
	
	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Update( TimeAsDouble( time ) );
	}
}

void Tr2Transform::UpdateViewDependentData( const Matrix& parentTransform )
{
	Vector3 finalScale;
	if( m_useDistanceBasedScale )
	{
		Vector3 myPos;
		D3DXVec3TransformCoord( &myPos, &m_translation, &parentTransform );

		const Vector3& camPos = Tr2Renderer::GetViewPosition();
		Vector3 d = myPos - camPos;
		const float dist = D3DXVec3Length( &d );
		const float fov = Tr2Renderer::GetFieldOfView();

		//fovHeight is the thing to multiply with to let the object
		//always keep the same height on screen
		const float fovHeight = sinf(fov/2.0f) * dist ;

		//this is the number we want to reduce it by
		const float scaler = m_distanceBasedScaleArg1/powf( fovHeight, m_distanceBasedScaleArg2 );
		const float scale = scaler*fovHeight;
		
		finalScale = m_scaling * scale;
	}
	else
	{
		finalScale = m_scaling;
	}

	D3DXMatrixTransformation( &m_localTransform, 0, 0, &finalScale, 0, &m_rotation, &m_translation );
	D3DXMatrixMultiply( &m_worldTransform, &m_localTransform, &parentTransform );

	switch( m_modifier )
	{
		case TR2TM_BILLBOARD:
		case TR2TM_SIMPLE_HALO:
			{
				float parentScaleX = D3DXVec3Length( &parentTransform.GetX() );
				float parentScaleY = D3DXVec3Length( &parentTransform.GetY() );
				float parentScaleZ = D3DXVec3Length( &parentTransform.GetZ() );
				finalScale.x *= parentScaleX;
				finalScale.y *= parentScaleY;
				finalScale.z *= parentScaleZ;

				if( m_modifier == TR2TM_SIMPLE_HALO )
				{
					const Vector3& myPos = m_worldTransform.GetTranslation();
					const Vector3& camPos = Tr2Renderer::GetViewPosition();
					Vector3 d = camPos - myPos;

					Vector3 backward, dirToCamNorm;

					float scale = D3DXVec3Dot(
						D3DXVec3Normalize(&dirToCamNorm, &d), 
						D3DXVec3Normalize(&backward, TriVectorRotatedBasisMatrix(&backward, TRITA_Z, &m_worldTransform))
						);

					if( scale < 0.0f )
					{
						scale = 0.0f;
					}

					finalScale *= scale*scale;
				}

				const Matrix& invView = Tr2Renderer::GetInverseViewTransform();
				m_worldTransform._11 = invView._11 * finalScale.x;
				m_worldTransform._12 = invView._12 * finalScale.x;
				m_worldTransform._13 = invView._13 * finalScale.x;
				m_worldTransform._21 = invView._21 * finalScale.y;
				m_worldTransform._22 = invView._22 * finalScale.y;
				m_worldTransform._23 = invView._23 * finalScale.y;
				m_worldTransform._31 = invView._31 * finalScale.z;
				m_worldTransform._32 = invView._32 * finalScale.z;
				m_worldTransform._33 = invView._33 * finalScale.z;
			}
			break;

		case TR2TM_EVE_CAMERA_ROTATION:
			{
				// apply the parent transform ONLY to the translation!
				Vector3 newTranslation;
				D3DXVec3TransformCoord( &newTranslation, &m_translation, &parentTransform );
				D3DXMatrixTransformation( &m_localTransform, 0, 0, &finalScale, 0, &m_rotation, &newTranslation );
				Vector3 temp( m_localTransform._41, m_localTransform._42, m_localTransform._43 );
				D3DXMatrixMultiply( &m_worldTransform , &m_localTransform, &Tr2Renderer::GetInverseViewTransform() );
				m_worldTransform._41 = temp.x;
				m_worldTransform._42 = temp.y;
				m_worldTransform._43 = temp.z;

			}
			break;
		case TR2TM_EVE_CAMERA_ROTATION_ALIGNED:
		case TR2TM_EVE_BOOSTER:
		case TR2TM_EVE_SIMPLE_HALO:
			{
				Matrix translationTransform;
				D3DXMatrixTranslation( &translationTransform, m_translation.x, m_translation.y, m_translation.z );

				D3DXMatrixMultiply( &m_worldTransform, &translationTransform, &parentTransform );

				const Vector3& myPos = m_worldTransform.GetTranslation();
				const Vector3& camPos = Tr2Renderer::GetViewPosition();
				Vector3 d = camPos - myPos;

				Vector3 camFwd = d;
				Matrix parentT;
				D3DXMatrixTranspose( &parentT, &parentTransform );
				TriVectorRotateMatrix(&camFwd, &camFwd, &parentT);


				float lengthSq = D3DXVec3LengthSq( &parentTransform.GetX() );
				camFwd.x /= lengthSq;
				lengthSq = D3DXVec3LengthSq( &parentTransform.GetY() );
				camFwd.y /= lengthSq;
				lengthSq = D3DXVec3LengthSq( &parentTransform.GetZ() );
				camFwd.z /= lengthSq;

				float distCenter = D3DXVec3Length( &camFwd );
				D3DXVec3Normalize( &camFwd, &camFwd );

				const Matrix& viewMatrix = Tr2Renderer::GetViewTransform();
				Vector3 right( viewMatrix._11, viewMatrix._21, viewMatrix._31 );
				TriVectorRotateMatrix(&right, &right, &parentT);
				
				Vector3 up;
				D3DXVec3Cross( &up, &camFwd, &right );
				D3DXVec3Normalize( &up, &up );

				Matrix alignMat;
				TriMatrixChangeBase( &alignMat, &camFwd, &up );
				TriMatrixRotate( &alignMat, &m_rotation, &alignMat );


				if( m_modifier == TR2TM_EVE_SIMPLE_HALO )
				{
					Vector3 forward, dirToCamNorm;

					D3DXVec3Normalize( &forward, TriVectorRotatedBasisMatrix( &forward, TRITA_Z, &m_worldTransform ) );
					Vector3 backward = -forward;

					float scale = D3DXVec3Dot( D3DXVec3Normalize( &dirToCamNorm, &d ), &backward );

					if( scale < 0.0f )
					{
						scale = 0.0f;
					}

					Matrix scalingTransform;
					D3DXMatrixScaling( &scalingTransform, m_scaling.x * scale, m_scaling.y * scale, m_scaling.z * scale );

					D3DXMatrixMultiply( &m_worldTransform, &alignMat, &m_worldTransform );
					D3DXMatrixMultiply( &m_worldTransform, &scalingTransform, &m_worldTransform );
				}
				else if( m_modifier == TR2TM_EVE_BOOSTER )
				{
					float radius = 0.5f;
					float B = sqrtf(distCenter*distCenter - radius*radius);
					float scale =  B/distCenter;
					float trans = -radius*radius/(distCenter*scale);

					scale *= m_scaling.x;

					Matrix scalingTransform;
					D3DXMatrixScaling( &scalingTransform, scale, scale, scale );

					Matrix translationTransform;
					D3DXMatrixTranslation( &translationTransform, 0.0f, 0.0f, trans );

					D3DXMatrixMultiply( &m_worldTransform, &scalingTransform, &m_worldTransform );
					D3DXMatrixMultiply( &m_worldTransform, &alignMat, &m_worldTransform );
					D3DXMatrixMultiply( &m_worldTransform, &translationTransform, &m_worldTransform );
				}
				else // TR2TM_EVE_CAMERA_ROTATION_ALIGNED
				{
					D3DXMatrixMultiply( &m_worldTransform, &alignMat, &m_worldTransform );

					Matrix scalingTransform;
					D3DXMatrixScaling( &scalingTransform, finalScale.x, finalScale.y, finalScale.z );

					D3DXMatrixMultiply( &m_worldTransform, &scalingTransform, &m_worldTransform );
				}
			}
			break;

		case TR2TM_LOOK_AT_CAMERA:
			{
				Matrix invView;
				const Vector3& pos = m_worldTransform.GetTranslation();
				const Vector3& camPos = Tr2Renderer::GetViewPosition();
				Vector3 up( 0.0f, 1.0f, 0.0f );
				D3DXMatrixLookAtRH( &invView, &camPos, &pos, &up );
				D3DXMatrixTranspose( &invView, &invView );

				float parentScaleX = D3DXVec3Length( &parentTransform.GetX() );
				float parentScaleY = D3DXVec3Length( &parentTransform.GetY() );
				float parentScaleZ = D3DXVec3Length( &parentTransform.GetZ() );
				finalScale.x *= parentScaleX;
				finalScale.y *= parentScaleY;
				finalScale.z *= parentScaleZ;

				m_worldTransform._11 = invView._11 * finalScale.x;
				m_worldTransform._12 = invView._12 * finalScale.x;
				m_worldTransform._13 = invView._13 * finalScale.x;
				m_worldTransform._21 = invView._21 * finalScale.y;
				m_worldTransform._22 = invView._22 * finalScale.y;
				m_worldTransform._23 = invView._23 * finalScale.y;
				m_worldTransform._31 = invView._31 * finalScale.z;
				m_worldTransform._32 = invView._32 * finalScale.z;
				m_worldTransform._33 = invView._33 * finalScale.z;
			}
			break;

		case TR2TM_TRANSLATE_WITH_CAMERA:
			{
				Vector3& pos = m_worldTransform.GetTranslation();
				pos = Tr2Renderer::GetViewPosition();
			}
			break;

		default:
			break;
	}
}

bool Tr2Transform::HasTransparentBatches()
{
	if( m_display && m_mesh )
	{
		return !(m_mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT )->empty());
	}

	return false;
}

void Tr2Transform::GetBatches( ITriRenderBatchAccumulator* batches,
							   TriBatchType batchType,
							   const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( batchType ), perObjectData );
	}
}

void Tr2Transform::GetShadowBatches( ITriRenderBatchAccumulator* batches, const Tr2PerObjectData* perObjectData )
{
	if( m_display && m_mesh )
	{
		m_mesh->GetBatches( batches, m_mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), perObjectData );
	}
}

float Tr2Transform::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = D3DXVec3Length( &d );
	return distance * m_sortValueMultiplier;
}
