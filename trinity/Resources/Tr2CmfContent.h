// Copyright © 2026 CCP ehf.

#pragma once

#include <cmf/cmf.h>

class Tr2CmfContents
{
public:
	Tr2CmfContents() = default;

	/**
	 * @brief Reads the contents of a .cmf file from the provided stream. The stream is expected to be positioned at the start of the file, and the filename is used for error reporting purposes only.
	 * @param stream Input stream to read the .cmf file from.
	 * @param filename Filename of the .cmf file, used for error reporting.
	 */
	explicit Tr2CmfContents( IBlueStream& stream, const char* filename );

	/**
	 * @brief Returns true if the .cmf file was successfully read and parsed, false otherwise.
	 */
	operator bool() const;
	/**
	 * @brief Gets a pointer to the structured data of the loaded .cmf file. This data is expected to be valid as long as the Tr2CmfContents object is alive.
	 * @return A pointer to the structured data object, or nullptr if the .cmf file failed to load or did not contain valid data.
	 */
	const cmf::Data* GetData() const;

	/**
	 * @brief Returns a pointer to the raw data of the section at the specified index. The returned pointer is valid as long as the Tr2CmfContents object is alive.
	 * The function will perform decompression if the section is compressed, and will return the uncompressed data. That is the reason why the function is not const.
	 * The function may return nullptr if the index is out of bounds, if the file failed to load, or the section was unloaded using UnloadGpuBuffers call.
	 * @param index Index of the section to retrieve.
	 */
	const void* GetSection( uint32_t index );

	/**
	 * @brief Returns a pointer to the data pointed by the provided buffer view, taking view's offset into account. The returned pointer is valid as long as the Tr2CmfContents object is alive.
	 * The function will perform decompression if the section is compressed, and will return the uncompressed data. That is the reason why the function is not const.
	 * The function may return nullptr if the index is out of bounds, if the file failed to load, or the section was unloaded using UnloadGpuBuffers call.
	 * @param view Buffer view describing the section to retrieve.
	 */
	const void* GetViewData( const cmf::BufferView& view );

	/**
	 * @brief Unloads the GPU buffer sections of the .cmf file. This is intended to be called once GPU buffer data has been uploaded to GPU.
	 */
	void UnloadGpuBuffers();

private:
	struct Section
	{
		cmf::Section section;
		std::unique_ptr<uint8_t[]> data;
	};
	std::vector<Section> m_sections;
};
