// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2VoxelTexture.h"

namespace
{

/**
 * @brief Returns the pixel formats corresponding to the given Tr2VoxelDataType. The returned array contains pixel formats for 1, 2 and 4 elements respectively.
 * @param type The Tr2VoxelDataType to get pixel formats for.
 * @param normalized Whether the pixel formats should be normalized (for integer types).
 * @return An array of pixel formats corresponding to the given Tr2VoxelDataType.
 */
std::array<Tr2RenderContextEnum::PixelFormat, 3> GetDataTypePixelFormats( Tr2VoxelDataType type, bool normalized )
{
	using namespace Tr2RenderContextEnum;
	constexpr std::array<PixelFormat, 3> invalidFormat = { PIXEL_FORMAT_UNKNOWN, PIXEL_FORMAT_UNKNOWN, PIXEL_FORMAT_UNKNOWN };
	switch( type )
	{
	case Tr2VoxelDataType::Boolean:
		return { PIXEL_FORMAT_R8_UINT, PIXEL_FORMAT_R8G8_UINT, PIXEL_FORMAT_R8G8B8A8_UINT };
	case Tr2VoxelDataType::Float:
		return { PIXEL_FORMAT_R32_FLOAT, PIXEL_FORMAT_R32G32_FLOAT, PIXEL_FORMAT_R32G32B32A32_FLOAT };
	case Tr2VoxelDataType::Float16:
		return { PIXEL_FORMAT_R16_FLOAT, PIXEL_FORMAT_R16G16_FLOAT, PIXEL_FORMAT_R16G16B16A16_FLOAT };
	case Tr2VoxelDataType::Int8:
		if( normalized )
		{
			return { PIXEL_FORMAT_R8_SNORM, PIXEL_FORMAT_R8G8_SNORM, PIXEL_FORMAT_R8G8B8A8_SNORM };
		}
		return { PIXEL_FORMAT_R8_SINT, PIXEL_FORMAT_R8G8_SINT, PIXEL_FORMAT_R8G8B8A8_SINT };
	case Tr2VoxelDataType::UInt8:
		if( normalized )
		{
			return { PIXEL_FORMAT_R8_UNORM, PIXEL_FORMAT_R8G8_UNORM, PIXEL_FORMAT_R8G8B8A8_UNORM };
		}
		return { PIXEL_FORMAT_R8_UINT, PIXEL_FORMAT_R8G8_UINT, PIXEL_FORMAT_R8G8B8A8_UINT };
	case Tr2VoxelDataType::Int16:
		if( normalized )
		{
			return { PIXEL_FORMAT_R16_SNORM, PIXEL_FORMAT_R16G16_SNORM, PIXEL_FORMAT_R16G16B16A16_SNORM };
		}
		return { PIXEL_FORMAT_R16_SINT, PIXEL_FORMAT_R16G16_SINT, PIXEL_FORMAT_R16G16B16A16_SINT };
	case Tr2VoxelDataType::UInt16:
		if( normalized )
		{
			return { PIXEL_FORMAT_R16_UNORM, PIXEL_FORMAT_R16G16_UNORM, PIXEL_FORMAT_R16G16B16A16_UNORM };
		}
		return { PIXEL_FORMAT_R16_UINT, PIXEL_FORMAT_R16G16_UINT, PIXEL_FORMAT_R16G16B16A16_UINT };
	case Tr2VoxelDataType::Int32:
		if( normalized )
		{
			return invalidFormat;
		}
		return { PIXEL_FORMAT_R32_SINT, PIXEL_FORMAT_R32G32_SINT, PIXEL_FORMAT_R32G32B32A32_SINT };
	case Tr2VoxelDataType::UInt32:
		if( normalized )
		{
			return invalidFormat;
		}
		return { PIXEL_FORMAT_R32_UINT, PIXEL_FORMAT_R32G32_UINT, PIXEL_FORMAT_R32G32B32A32_UINT };
	}
	return invalidFormat;
}

Tr2RenderContextEnum::PixelFormat GetPixelFormatForVoxelDataType( const Tr2VoxelData::PayloadType& payloadType )
{
	auto pixelFormats = GetDataTypePixelFormats( payloadType.type, payloadType.normalized );
	switch( payloadType.elementCount )
	{
	case 1:
		return pixelFormats[0];
	case 2:
		return pixelFormats[1];
	case 4:
		return pixelFormats[2];
	default:
		return Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_UNKNOWN;
	}
}
}

Tr2VoxelTexture::Tr2VoxelTexture( IRoot* )
{
}

bool Tr2VoxelTexture::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_channel ) )
	{
		RebuildTexture();
	}
	return true;
}

Tr2TextureAL* Tr2VoxelTexture::GetTexture()
{
	if( m_texture.IsValid() )
	{
		return &m_texture;
	}
	return nullptr;
}

Tr2VoxelTexture::OnTextureChangeEvent& Tr2VoxelTexture::OnTextureChange()
{
	return m_onTextureChange;
}

void Tr2VoxelTexture::SetVoxelData( Tr2VoxelData* voxelData )
{
	if( m_voxelData == voxelData )
	{
		return;
	}
	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.UnregisterListener( this );
	}
	m_voxelData = voxelData;

	if( m_voxelData )
	{
		m_voxelData->m_onDataChanged.RegisterListener<Tr2VoxelTexture, &Tr2VoxelTexture::OnVoxelDataChanged>( this );
	}

	RebuildTexture();
}

Tr2VoxelData* Tr2VoxelTexture::GetVoxelData() const
{
	return m_voxelData;
}

void Tr2VoxelTexture::OnVoxelDataChanged( Tr2VoxelData::ModifiedChannels modifiedChannels )
{
	if( m_texture.IsValid() && modifiedChannels.IsChannelModified( m_channel ) )
	{
		PopulateTextureData();
	}
}

void Tr2VoxelTexture::RebuildTexture()
{
	// Empty when there is no voxel data or the channel can't be a texture
	m_texture = CreateTexture();
	if( m_texture.IsValid() )
	{
		PopulateTextureData();
	}
	m_onTextureChange.Broadcast();
}

Tr2TextureAL Tr2VoxelTexture::CreateTexture()
{
	auto texture = Tr2TextureAL();

	if( !m_voxelData )
	{
		return texture;
	}

	if( m_channel >= m_voxelData->GetPayloadChannelCount() )
	{
		CCP_LOGERR( "Invalid channel index %zu for voxel data with %zu channels", m_channel, m_voxelData->GetPayloadChannelCount() );
		return texture;
	}

	auto voxelDims = m_voxelData->GetDimensions();
	auto payloadType = m_voxelData->GetPayloadType( m_channel );
	auto pixelFormat = GetPixelFormatForVoxelDataType( payloadType );

	if( pixelFormat == Tr2RenderContextEnum::PixelFormat::PIXEL_FORMAT_UNKNOWN )
	{
		CCP_LOGERR( "Unsupported voxel data type (%s x%d%s) for texture creation",
					Tr2VoxelDataTypeName( payloadType.type ),
					payloadType.elementCount,
					payloadType.normalized ? ", normalized" : "" );
		return texture;
	}

	auto dims = Tr2BitmapDimensions(
		Tr2RenderContextEnum::TextureType::TEX_TYPE_3D,
		pixelFormat,
		voxelDims.width,
		voxelDims.height,
		voxelDims.depth,
		1 );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	auto result = texture.Create( dims, Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, renderContext );

	if( FAILED( result ) )
	{
		CCP_LOGERR( "Failed to create texture" );
	}
	return texture;
}

void Tr2VoxelTexture::PopulateTextureData()
{
	auto snapshot = m_voxelData->GetPayloadSnapshot( m_channel );
	if( snapshot.m_data == nullptr )
	{
		CCP_LOGERR( "Failed to get payload snapshot for channel %zu", m_channel );
		return;
	}

	auto voxelDims = m_voxelData->GetDimensions();
	const auto& payloadType = snapshot.m_type;

	// The source pitches are passed explicitly, so the backend can use its own row alignment
	uint32_t pitch = voxelDims.width * Tr2RenderContextEnum::GetBytesPerPixel( m_texture.GetFormat() );
	uint32_t slicePitch = pitch * voxelDims.height;

	const uint8_t* source = snapshot.m_data;
	std::vector<uint8_t> unpacked;
	if( payloadType.type == Tr2VoxelDataType::Boolean )
	{
		// Byte n of the texture data is element n of the payload
		unpacked.resize( size_t( slicePitch ) * voxelDims.depth );
		for( size_t n = 0; n < unpacked.size(); ++n )
		{
			unpacked[n] = Tr2VoxelData::GetPackedBit( snapshot.m_data, n );
		}
		source = unpacked.data();
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( FAILED( m_texture.UpdateSubresource( Tr2TextureSubresource( 0 ), source, pitch, slicePitch, renderContext ) ) )
	{
		CCP_LOGERR( "Failed to update texture data" );
	}
}

bool Tr2VoxelTexture::IsValid() const
{
	return m_texture.IsValid();
}

uint32_t Tr2VoxelTexture::GetWidth() const
{
	return m_texture.GetWidth();
}

uint32_t Tr2VoxelTexture::GetHeight() const
{
	return m_texture.GetHeight();
}

uint32_t Tr2VoxelTexture::GetDepth() const
{
	return m_texture.GetDepth();
}

uint32_t Tr2VoxelTexture::GetMipCount() const
{
	return m_texture.GetMipCount();
}

Tr2RenderContextEnum::PixelFormat Tr2VoxelTexture::GetFormat() const
{
	return m_texture.GetFormat();
}

Tr2RenderContextEnum::TextureType Tr2VoxelTexture::GetType() const
{
	return m_texture.GetType();
}
