// Copyright © 2026 CCP ehf.

#pragma once

#include "EveSpaceObjectChild.h"
#include "../../SpaceObjectFactory/EveSOF.h"


BLUE_CLASS_IMPL( EveChildPartData );
class EveChildPartData : public EveSpaceObjectChild
{
public:
	EveChildPartData( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	PartTag GetUnusedPartID() const;

	std::string m_faction;
	std::string m_race;

	struct PartData
	{
		PartTag partId;
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
		CcpMath::Sphere boundingSphere;
	};
	std::vector<PartData> m_parts;
};

TYPEDEF_BLUECLASS( EveChildPartData );


BLUE_CLASS_IMPL( EveModularObjectModifier );
class EveModularObjectModifier : public IRoot
{
public:
	using SpaceObjectType = EveSpaceObject2;

	EXPOSE_TO_BLUE();

	void Create( SpaceObjectType* object, EveSOF* sof );
	~EveModularObjectModifier();

	static constexpr EveSpaceObjectChild::PartTag INVALID_PART_TAG = 0xFFFFFFFF;

	EveSpaceObjectChild::PartTag AddHull( const char* hullName, const char* factionName, const char* raceName, const Vector3& position, const Quaternion& rotation, const Vector3& scale );
	EveSpaceObjectChild::PartTag AddChild( const char* resPath, const Vector3& position, const Quaternion& rotation, const Vector3& scale );
	BlueStdResult Remove( EveSpaceObjectChild::PartTag partId );

	BlueStdResult SetTransform( EveSpaceObjectChild::PartTag partId, const Vector3& position, const Quaternion& rotation, Vector3 scale );
	BlueStdResult GetPosition( EveSpaceObjectChild::PartTag partId, Vector3& position ) const;
	BlueStdResult GetRotation( EveSpaceObjectChild::PartTag partId, Quaternion& rotation ) const;
	BlueStdResult GetScale( EveSpaceObjectChild::PartTag partId, Vector3& scale ) const;

private:
	EveChildPartData::PartData* FindPartData( EveSpaceObjectChild::PartTag partId ) const;
	EveSpaceObjectChild::PartTag AllocatePartId() const;
	void UpdateImpactOverlayLocatorCount() const;

	BluePtr<SpaceObjectType> m_object;
	EveChildPartDataPtr m_data;
	EveChildInstancedMeshesPtr m_instancedMeshes;
	EveSOFPtr m_sof;
};

TYPEDEF_BLUECLASS( EveModularObjectModifier );

std::pair<IEveSpaceObject2Ptr, EveModularObjectModifierPtr> CreateModularObject( EveSOF* sof, const char* factionName, const char* raceName );
EveModularObjectModifierPtr ModifyModularObject( EveModularObjectModifier::SpaceObjectType* object, EveSOF* sof );

// Wrapper function because blue doesn't support exporting const variables
EveSpaceObjectChild::PartTag GetInvalidPartTag();
