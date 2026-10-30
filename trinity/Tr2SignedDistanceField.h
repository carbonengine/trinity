// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2VoxelData.h"


/**
 * @brief Signed distance field representation of a voxel grid. Provides methods to compute and access the signed distance values at each voxel cell.
 * The field is constructed from either a supplied SDF channel or computed from the occupancy channel of the voxel data.
 */
class Tr2SignedDistanceField
{
public:
	struct LinearTransform
	{
		float scale = 1.f;
		float offset = 0.f;
		float Apply( float value ) const
		{
			return value * scale + offset;
		}
	};

	/**
	 * @brief Constructs a signed distance field from the supplied SDF channel, applying the specified linear transform to the values. Expects
	 * the SDF channel to be of type Float or normalized Int8 (single component). If the channel is not of the expected type, the field will be empty.
	 * @param sdf The SDF channel snapshot to construct the field from.
	 * @param transform The linear transform to apply to the SDF values.
	 * @param cancel An atomic flag to signal cancellation of the operation.
	 */
	Tr2SignedDistanceField( const Tr2VoxelData::PayloadSnapshot& sdf, const LinearTransform& transform, const std::atomic<bool>& cancel );

	/**
	 * @brief Constructs a signed distance field from the supplied occupancy channel.
	 * @param occupancy The occupancy channel snapshot to construct the field from.
	 * @param cancel An atomic flag to signal cancellation of the operation.
	 */
	Tr2SignedDistanceField( const Tr2VoxelData::PayloadSnapshot& occupancy, const std::atomic<bool>& cancel );

	bool IsValid() const
	{
		return !m_sdf.empty();
	}

	float GetValue( size_t index ) const;
	float GetValue( int32_t x, int32_t y, int32_t z ) const;

private:
	Tr2VoxelData::Dimensions m_dimensions;
	std::vector<float> m_sdf;

	using Int3 = std::array<int32_t, 3>;
	static constexpr std::array<std::array<int, 2>, 3> s_axisUV = { { { 1, 2 }, { 2, 0 }, { 0, 1 } } };
	static constexpr int32_t s_sdfSearchRadius = 4; // surface band radius for the chamfer distance transform
	static constexpr float s_sdfClamp = 3.0; // encode clamp in cells
	static constexpr float s_deepEmpty = s_sdfClamp; // field value sampled outside the grid
	static constexpr float s_signFloor = 0.12; // min |field| kept at an in-grid cell centre after the low-pass
	static constexpr float s_blurAlpha = 0.15; // sign-preserving low-pass kernel [a/2, 1-a, a/2] of the extraction field

	Int3 GetGridSize() const;
	std::vector<float> ComputeSdf( const Tr2VoxelData::PayloadSnapshot& occupancyChannel, const std::atomic<bool>& cancel ) const;
	std::vector<float> CreateFieldFromOccupancy( const Tr2VoxelData::PayloadSnapshot& occupancyChannel, const std::atomic<bool>& cancel ) const;
};
