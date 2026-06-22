// Copyright © 2022 CCP ehf.

#pragma once

#include "ITr2TextureProvider.h"
#include "Tr2DeviceResource.h"


BLUE_DECLARE( Tr2TextureArray );


class Tr2TextureArrayElement
{
public:
	operator bool() const;
	bool IsValid() const;
	uint32_t GetElementIndex() const;
	Tr2TextureArray* GetArray() const;
	Tr2TextureAL GetTexture() const;

private:
	struct Data
	{
		~Data();

		Tr2TextureArrayPtr m_array;
		uint32_t m_index;
	};

	std::shared_ptr<Data> m_data;

	friend class Tr2TextureArray;
};


BLUE_CLASS( Tr2TextureArray ) :
	public ITr2TextureProvider,
	public Tr2DeviceResource
{
public:
	Tr2TextureArray( IRoot* = nullptr );

	EXPOSE_TO_BLUE();

	void SetExpectedElementDimensions( const Tr2BitmapDimensions& dimensions );
	Tr2TextureArrayElement AddElement( const ImageIO::HostBitmap& bitmap );

	Tr2TextureAL* GetTexture() override;
	OnTextureChangeEvent& OnTextureChange() override;

	uint32_t GetElementCount() const;
	const Tr2BitmapDimensions& GetDimensions() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetArraySize() const;
	Tr2RenderContextEnum::PixelFormat GetFormat() const;
	uint32_t GetMipCount() const;

	void ReleaseResources( TriStorage ) override;

protected:
	bool OnPrepareResources() override;

private:
	void RemoveElement( uint32_t index );
	Tr2TextureAL CreateTexture() const;

	friend struct Tr2TextureArrayElement::Data;

private:
	std::vector<ImageIO::HostBitmap> m_elements;
	Tr2TextureAL m_texture;
	Tr2BitmapDimensions m_dimensions;
	uint32_t m_increment = 16;
	Tr2GpuUsage::Type m_gpuUsage = Tr2GpuUsage::SHADER_RESOURCE;
	Tr2CpuUsage::Type m_cpuUsage = Tr2CpuUsage::NONE;
	OnTextureChangeEvent m_onTextureChange;
};

TYPEDEF_BLUECLASS( Tr2TextureArray );