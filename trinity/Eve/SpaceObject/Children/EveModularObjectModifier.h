// Copyright © 2026 CCP ehf.

#pragma once

#include "EveChildPartData.h"
#include "../../SpaceObjectFactory/EveSOF.h"


BLUE_CLASS_IMPL( EveModularObjectModifier );
/**
 * @brief Transient edit session for a modular space object. Reads and writes the object's EveChildPartData and
 * holds no persistent state itself. Object-level culling volumes only update on ApplyBounds(), which the
 * destructor also calls as a fallback.
 */
class EveModularObjectModifier : public IRoot
{
public:
	using SpaceObjectType = EveSpaceObject2;

	EXPOSE_TO_BLUE();

	void Create( SpaceObjectType* object, EveSOF* sof );
	~EveModularObjectModifier();

	/// @brief Returned by AddHull/AddChild when the part could not be built. Never a valid tag.
	static constexpr EveSpaceObjectChild::PartTag INVALID_PART_TAG = 0xFFFFFFFF;

	/**
	 * @brief Builds a SOF hull as a new part at the given transform. Empty factionName/raceName fall back to
	 * the seed faction/race stored in EveChildPartData (set by CreateModularObject).
	 * @return The new part's tag, or INVALID_PART_TAG if the hull could not be built.
	 */
	EveSpaceObjectChild::PartTag AddHull( const char* hullName, const char* factionName, const char* raceName, const Vector3& position, const Quaternion& rotation, const Vector3& scale );

	/**
	 * @brief Loads a space object child resource and adds it as a new part at the given transform.
	 * @return The new part's tag, or INVALID_PART_TAG if the resource could not be loaded.
	 */
	EveSpaceObjectChild::PartTag AddChild( const char* resPath, const Vector3& position, const Quaternion& rotation, const Vector3& scale );

	BlueStdResult Remove( EveSpaceObjectChild::PartTag partId );

	/**
	 * @brief Recomputes the object's bounding sphere and shape ellipsoid from the current parts.
	 * Cheap; call after a batch of edits to keep culling volumes in sync. The destructor also calls it.
	 */
	void ApplyBounds();

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

/**
 * @brief Creates an empty modular space object together with an open edit session for it.
 * factionName/raceName seed the AddHull fallbacks.
 */
std::pair<IEveSpaceObject2Ptr, EveModularObjectModifierPtr> CreateModularObject( EveSOF* sof, const char* factionName, const char* raceName );

/// @brief Opens an edit session on an existing modular space object.
EveModularObjectModifierPtr ModifyModularObject( EveModularObjectModifier::SpaceObjectType* object, EveSOF* sof );

/// @brief Wrapper for EveModularObjectModifier::INVALID_PART_TAG because blue doesn't support exporting const variables.
EveSpaceObjectChild::PartTag GetInvalidPartTag();