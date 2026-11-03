// Copyright © 2026 CCP ehf.

#pragma once
#include "ITr2TextureProvider.h"
#include "Tr2VoxelData.h"

/**
* @brief Texture composed from voxel data. Reacts to changes in data and updates the texture accordingly.
*/
class Tr2VoxelTexture : public ITr2TextureProvider,
						public INotify
{
public:
	EXPOSE_TO_BLUE();
	Tr2VoxelTexture( IRoot* lockobj = nullptr );
	bool OnModified( Be::Var* value ) override;

	/**
	*	@brief Gets the texture.
	*/
	Tr2TextureAL* GetTexture() override;

	/**
	*	@brief Gets the texture change event.
	*/
	OnTextureChangeEvent& OnTextureChange() override;

	/**
	*	@brief Sets the voxel data associated with this texture.
	*/
	void SetVoxelData( Tr2VoxelData* voxelData );

	/**
	*	@brief Gets the voxel data associated with this texture.
	*/
	Tr2VoxelData* GetVoxelData() const;

	/**
	*	@brief Returns whether the texture is valid.
	*/
	bool IsValid() const;

	/**
	*	@brief Gets the width of the texture.
	*/
	uint32_t GetWidth() const;

	/**
	*	@brief Gets the height of the texture.
	*/
	uint32_t GetHeight() const;

	/**
	*	@brief Gets the depth of the texture.
	*/
	uint32_t GetDepth() const;

	/**
	*	@brief Gets the mip count of the texture.
	*/
	uint32_t GetMipCount() const;

	/**
	*	@brief Gets the format of the texture.
	*/
	Tr2RenderContextEnum::PixelFormat GetFormat() const;

	/**
	*	@brief Gets the type of the texture.
	*/
	Tr2RenderContextEnum::TextureType GetType() const;

private:
	/**
	* @brief Rebuilds the texture from the voxel data. This function is called when the voxel data or the channel is set.
	*/
	void RebuildTexture();

	/**
	*	@brief Creates a texture from the voxel data.
	*	It creates a new texture when the voxel data and selected channel are valid. Otherwise, it returns an empty invalid texture.
	*/
	Tr2TextureAL CreateTexture();

	/**
	*	@brief Populates the texture with the voxel data.
	*	When the voxel data is modified, this function is called to update the texture with the new data.
	*/
	void PopulateTextureData();

	/**
	*	@brief Called when the voxel data is modified.
	*	@param modifiedChannels The channels that were modified.
	*/
	void OnVoxelDataChanged( Tr2VoxelData::ModifiedChannels modifiedChannels );

	// The voxel data associated with this texture.
	Tr2VoxelDataPtr m_voxelData;

	// The texture associated with this voxel data.
	Tr2TextureAL m_texture;

	// The texture change callback event, which is broadcast when the texture is modified.
	OnTextureChangeEvent m_onTextureChange;

	// The voxel data channel to use for the texture.
	size_t m_channel = 0;
};

TYPEDEF_BLUECLASS( Tr2VoxelTexture );