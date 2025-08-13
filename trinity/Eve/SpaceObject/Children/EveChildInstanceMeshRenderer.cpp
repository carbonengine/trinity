#include "StdAfx.h"
#include "EveChildInstanceMeshRenderer.h"
#include "Eve/EveUpdateContext.h"
#include "Resources/TriGeometryRes.h"
#include "TriFrustum.h"
#include "Tr2Renderer.h"
#include "TriMath.h"
#include "TransformModifiers/EveChildModifierTransformCommon.h"
#include "Eve/EveConstantBufferFormats.h"
#include "Tr2RuntimeInstanceData.h"
#include "Tr2InstancedMesh.h"


EveChildInstanceMeshRenderer::EveChildInstanceMeshRenderer( IRoot* lockobj ) :
	m_staticOffsetTranslation( 0.f, 0.f, 0.f ),
	m_staticOffsetScale( 1.f, 1.f, 1.f ),
	m_staticOffsetRotation( 0.f, 0.f, 0.f, 1.f ),
	m_totalObjectCount( 0 ),
	m_rotationConstraint( RotationalConstraints::NONE ),
	m_lastEntityCount( 0 )
{
}

bool EveChildInstanceMeshRenderer::IsVisible( const EveUpdateContext& updateContext ) const
{
	if( m_boundingSphere.w != 0 )
	{
		auto& frustum = updateContext.GetFrustum();
		if( frustum.IsSphereVisible( m_boundingSphere.GetXYZ(), m_boundingSphere.w ) )
		{
			return frustum.GetPixelSizeAccrossEst( m_boundingSphere.GetXYZ(), m_boundingSphere.w ) >= updateContext.GetVisibilityThreshold();
		}
	}
	return false;
}

void EveChildInstanceMeshRenderer::UpdateBoundingSphere( const PlacementDataWithIdentifierStructureList& placements )
{
	if( m_mesh == nullptr || m_distribution == nullptr )
	{
		return;
	}

	float baseMeshSize = 0.f;

	if( m_mesh->GetGeometryResource() != nullptr )
	{
		if( ( m_mesh->GetGeometryResource() )->IsGood() )
		{
			TriGeometryResMeshData* meshData = m_mesh->GetGeometryResource()->GetMeshData( m_mesh->GetMeshIndex() );
			baseMeshSize = meshData->m_boundingSphere.w;	
		}
	}

	Vector3 center = m_distribution->GetPlacementDataCenter();
	float longestDistance = 0.f;
	float largestScale = 0.f;

	for( size_t index = 0; index < m_totalObjectCount; index++ )
	{
		Vector3 pointPos = placements[index].initialTranslation + placements[index].additionalTranslation;
		float dist = LengthSq( pointPos - center ); // calculate squared for efficiency 
		Vector3 pointScale = placements[index].initialScale * placements[index].additionalScale; 
		float longestAxis = max( max( pointScale.x, pointScale.y ), pointScale.z ) ;
		largestScale = max( longestAxis, largestScale );
		longestDistance = max( dist, longestDistance );
	}
	longestDistance = sqrt( longestDistance ); // Get real distance from center

	m_boundingSphere = Vector4( center, longestDistance + largestScale * baseMeshSize );
}

bool EveChildInstanceMeshRenderer::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	if( m_mesh == nullptr || m_distribution == nullptr )
	{
		return false;
	}

	sphere = m_boundingSphere;
	return true;
}

void EveChildInstanceMeshRenderer::UpdateSyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveChildMesh::UpdateSyncronous( updateContext, params );

	if( m_distribution != nullptr )
	{
		m_distribution->UpdateSyncronous( updateContext, params );

		const PlacementDataWithIdentifierStructureList& placements = *m_distribution->GetPlacementData();

		uint32_t currentEntityCount = this->GetNumberOfEntities();
		bool updateCount = m_lastEntityCount != currentEntityCount;
		m_lastEntityCount = currentEntityCount;

		if( Tr2InstancedMeshPtr mesh = BlueCastPtr( m_mesh ) )
		{
			if( !mesh->GetInstanceGeometryResource() )
			{
				ConfigureInstanceData();
				UpdateGeometryResource( placements, currentEntityCount );
				UpdateBoundingSphere( placements );
			}
			else
			{
				bool alwaysUpdate = m_distribution->GetHasDynamicMovement() || m_rotationConstraint != EveChildInstanceMeshRenderer::NONE;
				if( updateCount || alwaysUpdate )
				{
					UpdateGeometryResource( placements, currentEntityCount );
					UpdateBoundingSphere( placements );
				}
			}
		}
	}
}

uint32_t EveChildInstanceMeshRenderer::GetNumberOfEntities() const
{
	if( m_distribution != nullptr )
	{
		return uint32_t( m_distribution->GetNumberOfPlacements() );
	}
	return 0;
}


void EveChildInstanceMeshRenderer::UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params )
{
	EveChildMesh::UpdateAsyncronous( updateContext, params );

	if( m_distribution != nullptr )
	{
		m_distribution->UpdateAsyncronous( updateContext, params );
	}
}


void EveChildInstanceMeshRenderer::UpdateGeometryResource( const PlacementDataWithIdentifierStructureList& placements, size_t size )
{
	m_totalObjectCount = uint32_t( size );

	if( !m_isVisible || !m_display || m_totalObjectCount == 0 )
	{
		return;
	}

	std::vector<PerInstanceData> instances;
	instances.reserve( m_totalObjectCount );

	std::vector<Matrix> instanceTransforms;  // for raytracing
	instanceTransforms.reserve( m_totalObjectCount );

	Vector3 position, scaling;
	const Vector3 up( 0.f, 1.f, 0.f );
	Quaternion rotation;
	Vector3 camPos = Tr2Renderer::GetViewPosition();

	for( size_t index = 0; index < m_totalObjectCount; index++ )
	{
		PerInstanceData instanceData;

		// rotation from placement
		rotation = placements[index].additionalRotation * placements[index].initialRotation;

		// rotate the static offset to allign with the placement
		TriVectorRotateQuaternion( &position, &m_staticOffsetTranslation, &rotation );

		// combine all translation data
		position += placements[index].initialTranslation + placements[index].additionalTranslation;

		scaling = Vector3( placements[index].initialScale.x * placements[index].additionalScale.x,
						   placements[index].initialScale.y * placements[index].additionalScale.y,
						   placements[index].initialScale.z * placements[index].additionalScale.z );

		scaling *= m_staticOffsetScale;

		if( m_rotationConstraint == EveChildInstanceMeshRenderer::BILLBOARD )
		{
			Quaternion objUpToCamera;
			Vector3 objDir;
			Vector3 angleToCamera = Normalize( camPos - TransformCoord( position, m_worldTransform ));
			Quaternion originRotation = Inverse( RotationQuaternion( m_worldTransform ) );
			TriVectorRotateQuaternion( &objDir, &up, &originRotation );

			Vector3 right = Normalize( Cross( up, objDir ) );
			Vector3 objUp = Cross( objDir, right );

			if( right == Vector3( 0.f, 0.f, 0.f ) )
			{
				objUp = Vector3( 0.f, 0.f, -objDir.y );
			}

			// calculate Roll adjustment to make billboarding move less for an orbiting camera
			float angle = XM_PIDIV2 - atan2f( angleToCamera.x, angleToCamera.z ) * 0.5f;
			Quaternion rollAdjustment( 0.f, 0.0f, cos( angle ), sin( angle ) );

			TriQuaternionRotationArc( &rotation, &up, &objUp );
			TriQuaternionRotationArc( &objUpToCamera, &angleToCamera, &up );

			rotation = rollAdjustment * rotation * Inverse( objUpToCamera );
		}
		else if( m_rotationConstraint == EveChildInstanceMeshRenderer::BILLBOARD_WITH_Z_LOCKED )
		{
			Vector3 angleToCamera = camPos - TransformCoord( position, m_worldTransform );
			Quaternion modification;
			Matrix rotMatrix = RotationMatrix( rotation ) * m_worldTransform;
			Vector4 vec = rotMatrix * Vector4( angleToCamera, 0.f );
			float rot = atan2f( vec.x, vec.z );
			Matrix result = RotationMatrix( up, rot );
			modification = RotationQuaternion( result );
			rotation = modification * rotation;
		}
		else
		{
			Vector3 meshRotation( 0.f, 0.f, 1.f );
			TriVectorRotateQuaternion( &meshRotation, &up, &rotation );
			meshRotation = Normalize( meshRotation ) * -1.f;
			TriQuaternionArcFromForward( &rotation, &meshRotation );
		}

		rotation = m_staticOffsetRotation * rotation;

		Matrix m = TransformationMatrix( scaling, rotation, position );
		Matrix mLast = TransformationMatrix( scaling, rotation, position + placements[index].translationFrameDelta );
		m = Transpose( m );
		mLast = Transpose( mLast );

		instanceData.transform0 = *reinterpret_cast<Vector4*>( &m.GetX() );
		instanceData.transform1 = *reinterpret_cast<Vector4*>( &m.GetY() );
		instanceData.transform2 = *reinterpret_cast<Vector4*>( &m.GetZ() );

		instanceData.lastTransform0 = *reinterpret_cast<Vector4*>( &mLast.GetX() );
		instanceData.lastTransform1 = *reinterpret_cast<Vector4*>( &mLast.GetY() );
		instanceData.lastTransform2 = *reinterpret_cast<Vector4*>( &mLast.GetZ() );

		instanceData.boneIndex = placements[index].boneIndex;

		instances.push_back( instanceData );
		instanceTransforms.push_back( m );
	}

	UpdateInstanceData( instances );
	SetInstanceTransforms( instanceTransforms );
}


void EveChildInstanceMeshRenderer::ConfigureInstanceData() const
{
	if( Tr2InstancedMeshPtr mesh = BlueCastPtr( m_mesh ) )
	{
		Tr2VertexDefinition instanceDef;
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 0 ); // transform0
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 1 ); // transform1
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 2 ); // transform2
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 3 ); // lastTransform0
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 4 ); // lastTransform1
		instanceDef.Add( Tr2VertexDefinition::FLOAT32_4, Tr2VertexDefinition::TEXCOORD, 5 ); // lastTransform2
		instanceDef.Add( Tr2VertexDefinition::BYTE_4, Tr2VertexDefinition::TEXCOORD, 6 ); // boneindex

		Tr2RuntimeInstanceDataPtr instanceData;
		instanceData.CreateInstance();
		instanceData->SetLayout( instanceDef );
		mesh->SetInstanceGeometryRes( instanceData );
	}
}

void EveChildInstanceMeshRenderer::UpdateInstanceData( std::vector<PerInstanceData> instances ) const
{
	if( Tr2InstancedMeshPtr mesh = BlueCastPtr( m_mesh ) )
	{
		auto geometryResource = mesh->GetInstanceGeometryResource();
		
		if( !geometryResource->IsInstanceDataReady() )
		{
			ConfigureInstanceData();
			geometryResource = mesh->GetInstanceGeometryResource();
		}

		if( Tr2RuntimeInstanceDataPtr geoRes = BlueCastPtr( geometryResource ) )
		{
			auto dest = geoRes->GetData( unsigned( instances.size() ) );
			memcpy( dest, &instances[0], sizeof( instances[0] ) * instances.size() );
			geoRes->UpdateData();

			float maxScale = 0;
			CcpMath::AxisAlignedBox aabb;
			for( auto& instance : instances )
			{
				aabb.Include( Vector3( instance.transform0.w, instance.transform1.w, instance.transform2.w ) );
				maxScale = std::max( maxScale, LengthSq( instance.transform0.GetXYZ() ) );
				maxScale = std::max( maxScale, LengthSq( instance.transform1.GetXYZ() ) );
				maxScale = std::max( maxScale, LengthSq( instance.transform2.GetXYZ() ) );
			}
			geoRes->SetBoundingBox( aabb );
			mesh->SetDynamicScaledBounds( sqrt( maxScale ) );
		}
	}
}