// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2SignedDistanceField.h"


/**
 * @brief Parameters of the surface cuts (Tr2SurfaceCuts). Lengths are in grid cells.
 */
struct Tr2SurfaceCutParams
{
	uint32_t seed = 0; // seeds facet and cleave placement, so a shape cuts the same way on every build
	float facetSpacing = 3.f; // one facet seed per cube of this size
	float facetDepth = 0.f; // how far below the surface a facet plane sinks; 0 disables facets
	float facetMaxCut = 0.3f; // deepest cut a facet makes
	float cleaveDepth = 0.f; // deepest cleave cap; 0 disables cleaves

	[[nodiscard]] bool HasFacets() const
	{
		return facetDepth > 0.f && facetSpacing > 0.f && facetMaxCut > 0.f;
	}

	[[nodiscard]] bool HasCleaves() const
	{
		return cleaveDepth > 0.f;
	}

	[[nodiscard]] bool IsEnabled() const
	{
		return HasFacets() || HasCleaves();
	}

	// The deepest cut either kind makes
	[[nodiscard]] float GetMaxCut() const
	{
		return std::max( HasFacets() ? facetMaxCut : 0.f, HasCleaves() ? cleaveDepth : 0.f );
	}
};


/**
 * @brief Planar cuts of a signed distance field's surface, which break a smooth shape into flats. Used by Tr2VoxelGeometry
 * to add detail to the mesh.
 *
 * Facets are near-tangent planes on a jittered seed lattice, each through its seed's nearest surface point, slightly
 * tilted and sunk inside. They flatten the surface in patches a few cells across but follow its curve. Cleaves are
 * larger bites anchored on the shape's outermost surface, which break its outline into straight runs and corners.
 * A surface point above a plane within the plane's reach moves down onto it; the deepest cut wins. Placement is a pure
 * function of the field and the seed.
 *
 * Facet planes are built lazily on first use, so an instance must not be shared between threads.
 */
class Tr2SurfaceCuts
{
public:
	/**
	 * @param field The field whose surface is cut. Must outlive this object.
	 * @param dimensions Dimensions of the field's grid.
	 * @param params Cut parameters.
	 */
	Tr2SurfaceCuts( const Tr2SignedDistanceField& field, const Tr2VoxelData::Dimensions& dimensions, const Tr2SurfaceCutParams& params );

	/**
	 * @brief Offset moving a surface point onto the plane that cuts it deepest, or zero where nothing cuts it.
	 * @param point Surface point in grid cell space (cell centres at integer coordinates).
	 * @param facing Unit outward surface normal at the point; planes facing another way leave it alone.
	 */
	[[nodiscard]] Vector3 GetCut( const Vector3& point, const Vector3& facing ) const;

private:
	using Int3 = std::array<int32_t, 3>;

	// The plane is Dot( normal, p ) = offset, with the solid side below
	struct Facet
	{
		Vector3 seed;
		Vector3 normal;
		float offset = 0;
	};

	struct Cleave
	{
		Vector3 anchor;
		Vector3 normal;
		float offset = 0;
		float reach = 0;
	};

	float GetFacetCut( const Vector3& point, const Vector3& facing, Vector3& normal ) const;
	float GetCleaveCut( const Vector3& point, const Vector3& facing, Vector3& normal ) const;
	const Facet* GetFacet( const Int3& cell ) const;
	std::optional<Facet> CreateFacet( const Int3& cell ) const;
	std::vector<Cleave> CreateCleaves() const;
	std::optional<Vector3> FindAnchor( const Vector3& direction ) const;
	std::optional<Vector3> GetSurfaceNormal( const Vector3& point ) const;

	const Tr2SignedDistanceField& m_field;
	const Tr2SurfaceCutParams m_params;
	Vector3 m_center; // grid center, where the cleave anchor search starts
	float m_searchRadius; // grid half diagonal, how far the cleave anchor search runs

	// Facet seed cells covering the grid; each is unbuilt, empty or holds a facet
	enum class FacetState : uint8_t
	{
		Unbuilt,
		Empty,
		Built,
	};
	Int3 m_seedMin;
	Int3 m_seedCount;
	mutable std::vector<FacetState> m_facetStates;
	mutable std::vector<Facet> m_facets;

	std::vector<Cleave> m_cleaves;
};
