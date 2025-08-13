#pragma once

#include "IEveSpaceObjectChild.h"
#include "EveChildTransform.h"
#include "Tr2DebugRenderer.h"
#include "TriRenderBatch.h"
#include "Tr2Mesh.h"
#include "Tr2PerObjectData.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/Utils/EveDistributionMethods/IEveDistributionMethod.h"
#include "EveChildMesh.h"


BLUE_DECLARE_INTERFACE( IEveDistributionMethod );

BLUE_CLASS( EveChildInstanceMeshRenderer ):
	public EveChildMesh
{
public:
	EXPOSE_TO_BLUE();

	EveChildInstanceMeshRenderer( IRoot* lockobj = NULL );

	bool GetBoundingSphere( Vector4 & sphere, BoundingSphereQuery query ) const override;
	bool IsVisible( const EveUpdateContext& updateContext ) const override;
	void UpdateSyncronous( const EveUpdateContext & updateContext, const EveChildUpdateParams& params ) override;
	void UpdateAsyncronous( const EveUpdateContext & updateContext, const EveChildUpdateParams& params ) override;
	void ConfigureInstanceData() const;
	uint32_t GetNumberOfEntities() const;

	enum RotationalConstraints
	{
		NONE = 0,
		BILLBOARD = 1,
		BILLBOARD_WITH_Z_LOCKED = 2
	};

protected:
	void UpdateGeometryResource( const PlacementDataWithIdentifierStructureList& placements, size_t size );
	void UpdateBoundingSphere( const PlacementDataWithIdentifierStructureList& placements );

	Vector3 m_staticOffsetTranslation;
	Vector3 m_staticOffsetScale;
	Quaternion m_staticOffsetRotation;
	RotationalConstraints m_rotationConstraint;
	uint32_t m_lastEntityCount;

private:
	struct PerInstanceData
	{
		Vector4 transform0;
		Vector4 transform1;
		Vector4 transform2;
		Vector4 lastTransform0;
		Vector4 lastTransform1;
		Vector4 lastTransform2;
		int32_t boneIndex;
	};

	void UpdateInstanceData( std::vector<PerInstanceData> instances ) const;

	unsigned m_totalObjectCount;
	Vector4 m_boundingSphere;
	IEveDistributionMethodPtr m_distribution;
};

TYPEDEF_BLUECLASS( EveChildInstanceMeshRenderer );
