// Copyright © 2022 CCP ehf.

#include "StdAfx.h"
#include "Tr2TextureArray.h"

Tr2TextureArray::Tr2TextureArray( IRoot* )
{
}

void Tr2TextureArray::SetExpectedElementDimensions( const Tr2BitmapDimensions& dimensions )
{
	if( !m_texture.IsValid() )
	{
		m_dimensions = dimensions;
	}
}

Tr2TextureArrayElement Tr2TextureArray::AddElement( const ImageIO::HostBitmap& bitmap )
{
	if( !bitmap.IsValid() )
	{
		return Tr2TextureArrayElement();
	}

	if( m_dimensions.GetType() != Tr2RenderContextEnum::TEX_TYPE_INVALID )
	{
		if( m_dimensions.GetType() != bitmap.GetType() ||
			m_dimensions.GetFormat() != bitmap.GetFormat() ||
			m_dimensions.GetWidth() != bitmap.GetWidth() ||
			m_dimensions.GetHeight() != bitmap.GetHeight() ||
			m_dimensions.GetDepth() != bitmap.GetDepth() ||
			m_dimensions.GetTrueMipCount() != bitmap.GetTrueMipCount() )
		{
			return Tr2TextureArrayElement();
		}
	}

	uint32_t index = uint32_t( m_elements.size() );

	for( size_t i = 0; i < m_elements.size(); ++i )
	{
		if( !m_elements[i].IsValid() )
		{
			index = uint32_t( i );
			break;
		}
	}
	if( index == m_elements.size() )
	{
		m_elements.push_back( ImageIO::HostBitmap() );
	}
	m_elements[index].CreateFromBitmapDimensions( bitmap );
	memcpy( m_elements[index].GetRawData(), bitmap.GetRawData(), bitmap.GetRawDataSize() );

	m_dimensions = Tr2BitmapDimensions(
		bitmap.GetType(),
		bitmap.GetFormat(),
		bitmap.GetWidth(),
		bitmap.GetHeight(),
		bitmap.GetDepth(),
		bitmap.GetTrueMipCount(),
		uint32_t( ( m_elements.size() + m_increment - 1 ) / m_increment * m_increment ) );

	Tr2TextureAL newTexture = CreateTexture();
	if( !newTexture.IsValid() )
	{
		m_elements[index] = ImageIO::HostBitmap();
		return Tr2TextureArrayElement();
	}
	m_texture = newTexture;
	m_onTextureChange();

	Tr2TextureArrayElement element;
	element.m_data.reset( new Tr2TextureArrayElement::Data );
	element.m_data->m_array = this;
	element.m_data->m_index = index;

	return element;
}

Tr2TextureAL* Tr2TextureArray::GetTexture()
{
	return &m_texture;
}

Tr2TextureArray::OnTextureChangeEvent& Tr2TextureArray::OnTextureChange()
{
	return m_onTextureChange;
}

uint32_t Tr2TextureArray::GetElementCount() const
{
	return uint32_t( m_elements.size() );
}

const Tr2BitmapDimensions& Tr2TextureArray::GetDimensions() const
{
	return m_dimensions;
}

uint32_t Tr2TextureArray::GetWidth() const
{
	return m_dimensions.GetWidth();
}

uint32_t Tr2TextureArray::GetHeight() const
{
	return m_dimensions.GetHeight();
}

uint32_t Tr2TextureArray::GetArraySize() const
{
	return m_dimensions.GetArraySize();
}

Tr2RenderContextEnum::PixelFormat Tr2TextureArray::GetFormat() const
{
	return m_dimensions.GetFormat();
}

uint32_t Tr2TextureArray::GetMipCount() const
{
	return m_dimensions.GetTrueMipCount();
}


void Tr2TextureArray::RemoveElement( uint32_t index )
{
	m_elements[index] = ImageIO::HostBitmap();
}

void Tr2TextureArray::ReleaseResources( TriStorage )
{
}

bool Tr2TextureArray::OnPrepareResources()
{
	if( !m_texture.IsValid() && !m_elements.empty() )
	{
		m_texture = CreateTexture();
		m_onTextureChange();
	}
	return true;
}

Tr2TextureAL Tr2TextureArray::CreateTexture() const
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	std::vector<Tr2SubresourceData> initialData;
	Tr2SubresourceData filler;
	bool validFiller = false;
	auto mipCount = m_dimensions.GetTrueMipCount();
	for( auto& element : m_elements )
	{
		if( element.IsValid() )
		{
			filler = { element.GetMipRawData( 0 ),
					   element.GetMipPitch( 0 ),
					   element.GetMipSize( 0 ) };
			validFiller = true;
			for( uint32_t i = 0; i < mipCount; ++i )
			{
				initialData.push_back( { element.GetMipRawData( i ), element.GetMipPitch( i ), element.GetMipSize( i ) } );
			}
		}
		else
		{
			for( uint32_t i = 0; i < mipCount; ++i )
			{
				initialData.push_back( { nullptr, 0, 0 } );
			}
		}
	}
	for( uint32_t i = uint32_t( m_elements.size() ); i < m_dimensions.GetArraySize(); ++i )
	{
		for( uint32_t i = 0; i < mipCount; ++i )
		{
			initialData.push_back( { nullptr, 0, 0 } );
		}
	}

	Tr2TextureAL newTexture;
	if( validFiller )
	{
		for( auto& data : initialData )
		{
			if( !data.m_sysMem )
			{
				data = filler;
			}
		}
		newTexture.Create( m_dimensions, Tr2MsaaDesc(), m_gpuUsage, m_cpuUsage, initialData.data(), renderContext );
	}
	else
	{
		newTexture.Create( m_dimensions, Tr2MsaaDesc(), m_gpuUsage, m_cpuUsage, nullptr, renderContext );
	}

	return newTexture;
}


Tr2TextureArrayElement::operator bool() const
{
	return m_data != nullptr;
}

bool Tr2TextureArrayElement::IsValid() const
{
	return m_data != nullptr;
}

uint32_t Tr2TextureArrayElement::GetElementIndex() const
{
	return m_data != nullptr ? m_data->m_index : 0;
}

Tr2TextureArray* Tr2TextureArrayElement::GetArray() const
{
	return m_data != nullptr ? m_data->m_array : nullptr;
}

Tr2TextureAL Tr2TextureArrayElement::GetTexture() const
{
	if( m_data )
	{
		return *m_data->m_array->GetTexture();
	}
	return Tr2TextureAL();
}


Tr2TextureArrayElement::Data::~Data()
{
	m_array->RemoveElement( m_index );
}
