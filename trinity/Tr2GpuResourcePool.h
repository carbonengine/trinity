// Copyright © 2026 CCP ehf.

#pragma once

#include "Tr2DeviceResource.h"
#include <variant>


struct TextureSize2D
{
	uint32_t width = 0;
	uint32_t height = 0;

	TextureSize2D() = default;
	TextureSize2D( uint32_t w, uint32_t h ) :
		width( w ),
		height( h )
	{
	}
	TextureSize2D( const Tr2BitmapDimensions& desc ) :
		width( desc.GetWidth() ),
		height( desc.GetHeight() )
	{
	}

	bool operator==( const TextureSize2D& other ) const
	{
		return width == other.width && height == other.height;
	}
	bool operator!=( const TextureSize2D& other ) const
	{
		return !operator==( other );
	}
	TextureSize2D operator*( float scale ) const
	{
		return TextureSize2D{ std::max( uint32_t( 1 ), uint32_t( float( width ) * scale ) ), std::max( uint32_t( 1 ), uint32_t( float( height ) * scale ) ) };
	}
};


class Tr2GpuResourcePool : public Tr2DeviceResource
{
public:
	template <typename T>
	struct GpuResource
	{
		T resource;
		uint64_t lastAccessFrame = 0;
		int32_t lockCount = 0;
		std::string name;
	};

	template <typename T>
	class GpuResourceHandle
	{
	public:
		GpuResourceHandle() = default;
		GpuResourceHandle( const GpuResourceHandle& other );
		GpuResourceHandle( GpuResourceHandle&& );
		GpuResourceHandle& operator=( const GpuResourceHandle& other );
		GpuResourceHandle& operator=( GpuResourceHandle&& );
		~GpuResourceHandle();

		[[nodiscard]] const T& Get() const;
		operator const T&() const;
		const T* operator->() const;
		bool IsValid() const;

	private:
		friend class Tr2GpuResourcePool;
		explicit GpuResourceHandle( const std::shared_ptr<GpuResource<T>>& rec );

		std::shared_ptr<GpuResource<T>> m_resourceRec;
		T m_resource;
	};

	using Texture = GpuResourceHandle<Tr2TextureAL>;
	using Buffer = GpuResourceHandle<Tr2BufferAL>;

	explicit Tr2GpuResourcePool( Tr2GpuResourcePool* outer = nullptr );
	Tr2GpuResourcePool( const Tr2GpuResourcePool& ) = delete;
	Tr2GpuResourcePool& operator=( const Tr2GpuResourcePool& ) = delete;
	~Tr2GpuResourcePool();


	using TextureInitializer = std::variant<std::function<void( Tr2TextureAL& texture, Tr2RenderContextAL& renderContext )>, Tr2SubresourceData*>;
	[[nodiscard]] Texture GetTempTexture( const char* name, uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat pixelFormat, Tr2GpuUsage::Type gpuUsage );
	[[nodiscard]] Texture GetTempTexture( const char* name, const TextureSize2D& size, Tr2RenderContextEnum::PixelFormat pixelFormat, Tr2GpuUsage::Type gpuUsage );
	[[nodiscard]] Texture GetTempTexture( const char* name, const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage );
	[[nodiscard]] Texture GetPersistentTexture( const char* name, uint32_t width, uint32_t height, Tr2RenderContextEnum::PixelFormat pixelFormat, Tr2GpuUsage::Type gpuUsage, const TextureInitializer& initializer );
	[[nodiscard]] Texture GetPersistentTexture( const char* name, const Tr2BitmapDimensions& desc, Tr2GpuUsage::Type gpuUsage, const TextureInitializer& initializer );

	using BufferInitializer = std::variant<std::function<void( Tr2BufferAL& buffer, Tr2RenderContextAL& renderContext )>, const void*>;
	[[nodiscard]] Buffer GetTempBuffer( const char* name, const Tr2BufferDescriptionAL& desc );
	[[nodiscard]] Buffer GetPersistentBuffer( const char* name, const Tr2BufferDescriptionAL& desc, const BufferInitializer& initializer );

	void SetDebugMode( bool enable );
	bool GetDebugMode() const;

	void ClearUnusedResources( uint64_t frameThreshold = 3 );
	static void ClearAllUnusedResources( uint64_t frameThreshold = 3 );

	void DebugGetAllTempTextures( std::vector<Tr2TextureAL>& out ) const;

private:
	using TextureRec = GpuResource<Tr2TextureAL>;
	using BufferRec = GpuResource<Tr2BufferAL>;

	void ReleaseResources( TriStorage s ) override;
	bool OnPrepareResources() override;

	Tr2GpuResourcePool* m_outer = nullptr;
	std::vector<std::shared_ptr<TextureRec>> m_tempTextures;
	std::vector<std::shared_ptr<TextureRec>> m_persistentTextures;
	std::vector<std::shared_ptr<BufferRec>> m_tempBuffers;
	std::vector<std::shared_ptr<BufferRec>> m_persistentBuffers;
	bool m_debugMode = false;
};

Tr2GpuResourcePool& GetGlobalGpuResourcePool();


template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>::GpuResourceHandle( const GpuResourceHandle& other ) :
	m_resourceRec( other.m_resourceRec ),
	m_resource( other.m_resource )
{
	if( m_resourceRec )
	{
		m_resourceRec->lockCount++;
	}
}

template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>::GpuResourceHandle( GpuResourceHandle&& other ) :
	m_resourceRec( other.m_resourceRec ),
	m_resource( other.m_resource )
{
	other.m_resourceRec = nullptr;
	other.m_resource = {};
}

template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>& Tr2GpuResourcePool::GpuResourceHandle<T>::operator=( const GpuResourceHandle& other )
{
	if( this != &other )
	{
		if( m_resourceRec )
		{
			m_resourceRec->lockCount--;
		}
		m_resourceRec = other.m_resourceRec;
		m_resource = other.m_resource;
		if( m_resourceRec )
		{
			m_resourceRec->lockCount++;
		}
	}
	return *this;
}

template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>& Tr2GpuResourcePool::GpuResourceHandle<T>::operator=( GpuResourceHandle&& other )
{
	if( this != &other )
	{
		if( m_resourceRec )
		{
			m_resourceRec->lockCount--;
		}
		m_resourceRec = other.m_resourceRec;
		m_resource = other.m_resource;
		other.m_resourceRec = nullptr;
		other.m_resource = {};
	}
	return *this;
}

template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>::~GpuResourceHandle()
{
	if( m_resourceRec )
	{
		m_resourceRec->lockCount--;
	}
}

template <typename T>
const T& Tr2GpuResourcePool::GpuResourceHandle<T>::Get() const
{
	return m_resource;
}

template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>::operator const T&() const
{
	return Get();
}

template <typename T>
const T* Tr2GpuResourcePool::GpuResourceHandle<T>::operator->() const
{
	return &Get();
}

template <typename T>
bool Tr2GpuResourcePool::GpuResourceHandle<T>::IsValid() const
{
	return m_resourceRec != nullptr;
}

template <typename T>
Tr2GpuResourcePool::GpuResourceHandle<T>::GpuResourceHandle( const std::shared_ptr<Tr2GpuResourcePool::GpuResource<T>>& rec ) :
	m_resourceRec( rec ),
	m_resource( rec->resource )
{
	m_resourceRec->lockCount++;
}
