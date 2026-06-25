// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2GpuResourcePool.h"


namespace
{
bool CompatibleDescriptions( const Tr2BufferDescriptionAL& a, const Tr2BufferDescriptionAL& b )
{
	return a.count == b.count && a.stride == b.stride && a.format == b.format && a.gpuUsage == b.gpuUsage && a.cpuUsage == b.cpuUsage;
}

std::vector<Tr2GpuResourcePool*>& AllGpuResourcePools()
{
	static std::vector<Tr2GpuResourcePool*> s_allPools;
	return s_allPools;
}

template <typename T>
void AppendName( T& record, const char* name )
{
	if( record->name.find( name ) == std::string::npos )
	{
		if( !record->name.empty() )
		{
			record->name += ",";
		}
		record->name += name;
		record->resource.SetName( record->name.c_str() );
	}
}

bool IsValid( const Tr2BitmapDimensions& desc )
{
	if( desc.GetFormat() == ImageIO::PIXEL_FORMAT_UNKNOWN )
	{
		return false;
	}
	switch( desc.GetType() )
	{
	case ImageIO::TEX_TYPE_1D:
		return desc.GetWidth() > 0;
	case ImageIO::TEX_TYPE_2D:
		return desc.GetWidth() > 0 && desc.GetHeight() > 0;
	case ImageIO::TEX_TYPE_3D:
		return desc.GetWidth() > 0 && desc.GetHeight() > 0 && desc.GetDepth() > 0;
	case ImageIO::TEX_TYPE_CUBE:
		return desc.GetWidth() > 0 && desc.GetHeight() == desc.GetWidth();
	default:
		return false;
	}
}

std::string ToString( const Tr2BitmapDimensions& desc )
{
	char buf[128];
	switch( desc.GetType() )
	{
	case ImageIO::TEX_TYPE_1D:
		sprintf_s( buf, "1D %u fmt=%u", desc.GetWidth(), (uint32_t)desc.GetFormat() );
		break;
	case ImageIO::TEX_TYPE_2D:
		sprintf_s( buf, "2D %ux%u fmt=%u", desc.GetWidth(), desc.GetHeight(), (uint32_t)desc.GetFormat() );
		break;
	case ImageIO::TEX_TYPE_3D:
		sprintf_s( buf, "3D %ux%ux%u fmt=%u", desc.GetWidth(), desc.GetHeight(), desc.GetDepth(), (uint32_t)desc.GetFormat() );
		break;
	case ImageIO::TEX_TYPE_CUBE:
		sprintf_s( buf, "Cube %ux%u fmt=%u", desc.GetWidth(), desc.GetHeight(), (uint32_t)desc.GetFormat() );
		break;
	default:
		strcpy_s( buf, "Invalid type" );
		break;
	}
	return std::string( buf );
}

}

Tr2GpuResourcePool::Tr2GpuResourcePool( Tr2GpuResourcePool* outer ) :
	m_outer( outer )
{
	AllGpuResourcePools().push_back( this );
}

Tr2GpuResourcePool::~Tr2GpuResourcePool()
{
	auto& allPools = AllGpuResourcePools();
	auto found = std::find( allPools.begin(), allPools.end(), this );
	if( found != allPools.end() )
	{
		allPools.erase( found );
	}
}

Tr2GpuResourcePool::Texture Tr2GpuResourcePool::GetTempTexture( const char* name, uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat pixelFormat, Tr2GpuUsage::Type gpuUsage )
{
	return GetTempTexture( name, Tr2BitmapDimensions( width, height, 1, pixelFormat ), gpuUsage );
}

Tr2GpuResourcePool::Texture Tr2GpuResourcePool::GetTempTexture( const char* name, const TextureSize2D& size, Tr2RenderContextEnum::PixelFormat pixelFormat, Tr2GpuUsage::Type gpuUsage )
{
	return GetTempTexture( name, Tr2BitmapDimensions( size.width, size.height, 1, pixelFormat ), gpuUsage );
}

Tr2GpuResourcePool::Texture Tr2GpuResourcePool::GetTempTexture( const char* name, const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage )
{
	if( m_outer && !m_debugMode )
	{
		return m_outer->GetTempTexture( name, desc, gpuUsage );
	}
	if( !IsValid( desc ) )
	{
		CCP_LOGERR( "Invalid texture description for a temporary texture %s (%s)", name, ToString( desc ).c_str() );
		return {};
	}

	auto found = find_if( begin( m_tempTextures ), end( m_tempTextures ), [&]( const auto& x ) {
		if( m_debugMode )
		{
			return x->lockCount == 0 && x->resource.GetDesc() == desc && x->resource.GetGpuUsage() == gpuUsage && x->name == name;
		}
		return x->lockCount == 0 && x->resource.GetDesc() == desc && x->resource.GetGpuUsage() == gpuUsage;
	} );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( found == end( m_tempTextures ) )
	{
		auto tex = std::make_shared<TextureRec>();
		if( FAILED( tex->resource.Create( desc, gpuUsage, renderContext ) ) )
		{
			CCP_LOGERR( "Failed to create a temporary texture %s (%s usage=%u)", name, ToString( desc ).c_str(), (uint32_t)gpuUsage );
			return Texture();
		}
		tex->resource.SetName( name );
		tex->lastAccessFrame = renderContext.GetRecordingFrameNumber();
		tex->name = name;
		m_tempTextures.push_back( tex );
		return Texture( tex );
	}
	( *found )->lastAccessFrame = renderContext.GetRecordingFrameNumber();
	AppendName( *found, name );
	return Texture( *found );
}

Tr2GpuResourcePool::Texture Tr2GpuResourcePool::GetPersistentTexture( const char* name, uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat pixelFormat, Tr2GpuUsage::Type gpuUsage, const TextureInitializer& initializer )
{
	return GetPersistentTexture( name, Tr2BitmapDimensions( width, height, 1, pixelFormat ), gpuUsage, initializer );
}

Tr2GpuResourcePool::Texture Tr2GpuResourcePool::GetPersistentTexture( const char* name, const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, const TextureInitializer& initializer )
{
	if( !IsValid( desc ) )
	{
		CCP_LOGERR( "Invalid texture description for a persistent texture %s (%s)", name, ToString( desc ).c_str() );
		return {};
	}

	auto found = find_if( begin( m_persistentTextures ), end( m_persistentTextures ), [&]( const auto& x ) {
		return x->resource.GetDesc() == desc && x->resource.GetGpuUsage() == gpuUsage && x->name == name;
	} );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( found == end( m_persistentTextures ) )
	{
		auto tex = std::make_shared<TextureRec>();
		auto initData = std::get_if<Tr2SubresourceData*>( &initializer );
		if( FAILED( tex->resource.Create( desc, gpuUsage, Tr2CpuUsage::NONE, initData ? *initData : nullptr, renderContext ) ) )
		{
			CCP_LOGERR( "Failed to create a persistent texture %s (%s usage=%u)", name, ToString( desc ).c_str(), (uint32_t)gpuUsage );
			return Texture();
		}
		tex->resource.SetName( name );
		if( auto initFunc = std::get_if<std::function<void( Tr2TextureAL & texture, Tr2RenderContextAL & renderContext )>>( &initializer ) )
		{
			( *initFunc )( tex->resource, renderContext );
		}
		tex->lastAccessFrame = renderContext.GetRecordingFrameNumber();
		tex->name = name;
		m_persistentTextures.push_back( tex );
		return Texture( tex );
	}
	( *found )->lastAccessFrame = renderContext.GetRecordingFrameNumber();
	return Texture( *found );
}

Tr2GpuResourcePool::Buffer Tr2GpuResourcePool::GetTempBuffer( const char* name, const Tr2BufferDescriptionAL& desc )
{
	if( m_outer && !m_debugMode )
	{
		return m_outer->GetTempBuffer( name, desc );
	}
	if( desc.count == 0 )
	{
		CCP_LOGERR( "Invalid buffer description for a temporary buffer %s (elements=%u, stride=%u, format=%u)", name, desc.count, desc.stride, (uint32_t)desc.format );
		return {};
	}
	auto found = find_if( begin( m_tempBuffers ), end( m_tempBuffers ), [&]( const auto& x ) {
		if( m_debugMode )
		{
			return x->lockCount == 0 && CompatibleDescriptions( x->resource.GetDesc(), desc ) && x->name == name;
		}
		return x->lockCount == 0 && CompatibleDescriptions( x->resource.GetDesc(), desc );
	} );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( found == end( m_tempBuffers ) )
	{
		auto buf = std::make_shared<BufferRec>();
		if( FAILED( buf->resource.Create( desc, nullptr, renderContext ) ) )
		{
			CCP_LOGERR( "Failed to create a temporary buffer %s (elements=%u, stride=%u, format=%u)", name, desc.count, desc.stride, (uint32_t)desc.format );
			return Buffer();
		}
		buf->resource.SetName( name );
		buf->lastAccessFrame = renderContext.GetRecordingFrameNumber();
		buf->name = name;
		m_tempBuffers.push_back( buf );
		return Buffer( buf );
	}
	( *found )->lastAccessFrame = renderContext.GetRecordingFrameNumber();
	AppendName( *found, name );
	return Buffer( *found );
}

Tr2GpuResourcePool::Buffer Tr2GpuResourcePool::GetPersistentBuffer( const char* name, const Tr2BufferDescriptionAL& desc, const BufferInitializer& initializer )
{
	if( desc.count == 0 )
	{
		CCP_LOGERR( "Invalid buffer description for a persistent buffer %s (elements=%u, stride=%u, format=%u)", name, desc.count, desc.stride, (uint32_t)desc.format );
		return {};
	}
	auto found = find_if( begin( m_persistentBuffers ), end( m_persistentBuffers ), [&]( const auto& x ) {
		return CompatibleDescriptions( x->resource.GetDesc(), desc ) && x->name == name;
	} );
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( found == end( m_persistentBuffers ) )
	{
		auto buf = std::make_shared<BufferRec>();
		auto initData = std::get_if<const void*>( &initializer );
		if( FAILED( buf->resource.Create( desc, initData ? *initData : nullptr, renderContext ) ) )
		{
			CCP_LOGERR( "Failed to create a temporary buffer %s (elements=%u, stride=%u, format=%u)", name, desc.count, desc.stride, (uint32_t)desc.format );
			return Buffer();
		}
		buf->resource.SetName( name );
		if( auto initFunc = std::get_if<std::function<void( Tr2BufferAL & buffer, Tr2RenderContextAL & renderContext )>>( &initializer ) )
		{
			( *initFunc )( buf->resource, renderContext );
		}
		buf->lastAccessFrame = renderContext.GetRecordingFrameNumber();
		buf->name = name;
		m_persistentBuffers.push_back( buf );
		return Buffer( buf );
	}
	( *found )->lastAccessFrame = renderContext.GetRecordingFrameNumber();
	return Buffer( *found );
}

void Tr2GpuResourcePool::SetDebugMode( bool enable )
{
	m_debugMode = enable;
	m_tempTextures.clear();
	m_persistentTextures.clear();
	m_tempBuffers.clear();
	m_persistentBuffers.clear();
}

bool Tr2GpuResourcePool::GetDebugMode() const
{
	return m_debugMode;
}

void Tr2GpuResourcePool::ClearUnusedResources( uint64_t frameThreshold )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();
	auto currentFrame = renderContext.GetRecordingFrameNumber();
	auto RetireResources = [currentFrame, frameThreshold]( auto& vec ) {
		for( auto& resource : vec )
		{
			if( resource->lockCount > 0 )
			{
				CCP_LOGERR( "Tr2GpuResourcePool: resource %s is still locked while clearing unused resources", resource->name.c_str() );
			}
		}
		vec.erase( std::remove_if( begin( vec ), end( vec ), [&]( const auto& x ) {
					   return currentFrame >= frameThreshold + x->lastAccessFrame;
				   } ),
				   end( vec ) );
	};

	RetireResources( m_tempTextures );
	RetireResources( m_persistentTextures );
	RetireResources( m_tempBuffers );
	RetireResources( m_persistentBuffers );
}

void Tr2GpuResourcePool::ClearAllUnusedResources( uint64_t frameThreshold )
{
	for( auto pool : AllGpuResourcePools() )
	{
		pool->ClearUnusedResources( frameThreshold );
	}
}

void Tr2GpuResourcePool::DebugGetAllTempTextures( std::vector<Tr2TextureAL>& out ) const
{
	for( auto& tex : m_tempTextures )
	{
		out.push_back( tex->resource );
	}
}

void Tr2GpuResourcePool::ReleaseResources( TriStorage s )
{
	if( s == TriStorageFlags::TRISTORAGE_ALL )
	{
		m_tempTextures.clear();
		m_persistentTextures.clear();
		m_tempBuffers.clear();
		m_persistentBuffers.clear();
	}
}

bool Tr2GpuResourcePool::OnPrepareResources()
{
	return true;
}


Tr2GpuResourcePool& GetGlobalGpuResourcePool()
{
	static Tr2GpuResourcePool globalPool;
	return globalPool;
}