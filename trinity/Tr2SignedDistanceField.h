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
	/**
	 * @brief Linear transform applied to the SDF values during construction. This allows scaling and offsetting the SDF values.
	 */
	struct LinearTransform
	{
		float scale = 1.f;
		float offset = 0.f;

		[[nodiscard]] float Apply( float value ) const
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

	/**
	 * @brief Indicates whether the current object is in a valid state. The SDF may be invalid if the input channels were not of the expected type.
	 * @return true if the object is valid; otherwise, false.
	 */
	[[nodiscard]] bool IsValid() const;

	/**
	 * @brief Gets the value at the specified linear index.
	 * @param index The zero-based position of the value to retrieve.
	 * @return The value at the specified index as a float.
	 */
	[[nodiscard]] float GetValue( size_t index ) const;

	/**
	 * @brief Gets the value at the specified 3D grid coordinates.
	 * @param x The x-coordinate of the value to retrieve.
	 * @param y The y-coordinate of the value to retrieve.
	 * @param z The z-coordinate of the value to retrieve.
	 * @return The value at the specified coordinates as a float.
	 */
	[[nodiscard]] float GetValue( int32_t x, int32_t y, int32_t z ) const;

	/**
	 * @brief Minmod gradient at a grid sample: per axis the one-sided difference of smaller magnitude, zero where they
	 * disagree in sign. Unlike a central difference it does not read across an edge, so it keeps sharp features.
	 */
	[[nodiscard]] Vector3 GetGradient( int32_t x, int32_t y, int32_t z ) const;

	/**
	 * @brief Trilinear interpolation of the field at a point in grid cell space (cell centres at integer coordinates).
	 * Samples outside the grid read as deep empty.
	 * @param point The point in grid cell space to sample.
	 * @return The interpolated value at the specified point.
	 */
	[[nodiscard]] float Sample( const Vector3& point ) const;

	/**
	 * @brief Trilinear interpolation of the sample gradients (GetGradient) at a point in grid cell space.
	 * @param point The point in grid cell space to sample.
	 * @return The interpolated gradient at the specified point.
	 */
	[[nodiscard]] Vector3 SampleGradient( const Vector3& point ) const;

	/**
	 * @brief Updates the current field to be the minimum of itself and another field. The other field must have the same dimensions as the current field.
	 * @param other The other signed distance field to compare with.
	 * @return true if the field was modified; otherwise, false.
	 */
	bool Min( const Tr2SignedDistanceField& other );

	/**
	 * @brief Updates the current field to be the maximum of itself and another field. The other field must have the same dimensions as the current field.
	 * @param other The other signed distance field to compare with.
	 * @return true if the field was modified; otherwise, false.
	 */
	bool Max( const Tr2SignedDistanceField& other );

	/**
	 * @brief Gets the dimensions of the signed distance field grid.
	 * @return A reference to the Dimensions object representing the grid dimensions.
	 */
	[[nodiscard]] const Tr2VoxelData::Dimensions& GetDimensions() const;

	/**
	 * @brief Down-samples the signed distance field by a factor of 2 in each dimension, reducing the resolution of the field. 
	 * The resulting field will have half the width, height, and depth of the original field.
	 */
	void DownSample();

	/**
	 * @brief Checks whether a voxel payload type is a valid SDF channel type.
	 * @param type The payload type value to validate.
	 * @return true if the payload type is valid for an SDF channel; otherwise, false.
	 */
	[[nodiscard]] static bool IsValidSdfChannelType( const Tr2VoxelData::PayloadType& type );

	/**
	 * @brief Checks whether a payload type is a valid occupancy channel type.
	 * @param type The payload type to validate.
	 * @return true if the payload type is a valid occupancy channel type; otherwise, false.
	 */
	[[nodiscard]] static bool IsValidOccupancyChannelType( const Tr2VoxelData::PayloadType& type );

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
	static float GetCornerWeight( const Vector3& t, int32_t corner ); // trilinear weight of a cube corner (bits x, y, z)
	std::vector<float> ComputeSdf( const Tr2VoxelData::PayloadSnapshot& occupancyChannel, const std::atomic<bool>& cancel ) const;
	std::vector<float> CreateFieldFromOccupancy( const Tr2VoxelData::PayloadSnapshot& occupancyChannel, const std::atomic<bool>& cancel ) const;
};
