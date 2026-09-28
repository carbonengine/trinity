// Copyright © 2026 CCP ehf.

#pragma once

#include "EveSpaceObjectChild.h"


BLUE_CLASS_IMPL( EveChildPartData );
/**
 * @brief Persistent state of a modular space object: seed faction/race and per-part transforms and bounds.
 * Stored as an effect child so the state travels with the object. All editing goes through EveModularObjectModifier.
 */
class EveChildPartData : public EveSpaceObjectChild
{
public:
	EveChildPartData( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	PartTag GetUnusedPartID() const;

	/// @brief Fallbacks for EveModularObjectModifier::AddHull when it is called with an empty faction or race name.
	std::string m_faction;
	std::string m_race;

	struct PartData
	{
		PartTag partId;
		Vector3 position;
		Quaternion rotation;
		Vector3 scale;
		CcpMath::Sphere boundingSphere; ///< In the modular object's local space.
	};
	std::vector<PartData> m_parts;
};

TYPEDEF_BLUECLASS( EveChildPartData );