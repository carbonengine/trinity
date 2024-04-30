////////////////////////////////////////////////////////////////////////////////
//
// Created:		February 2022
// Copyright:	CCP 2022
//

#include "StdAfx.h"
#include "Tr2TextureLodManager.h"
#include "TriSettingsRegistrar.h"
#include "TriTextureRes.h"


CCP_STATS_DECLARE( textureLodGpuSize, "Trinity/TextureLod/GpuSize", false, CST_MEMORY, "Total size of Tr2TextureLod textures in GPU memory" );
CCP_STATS_DECLARE( textureLodCpuSize, "Trinity/TextureLod/CpuSize", false, CST_MEMORY, "Total size of Tr2TextureLod textures in CPU memory" );
CCP_STATS_DECLARE( textureLodUploadSize, "Trinity/TextureLod/UploadSize", true, CST_MEMORY, "Size of Tr2TextureLod data uploaded to GPU" );

namespace
{
uint32_t s_gpuMemoryBudget = 300 * 1024 * 1024;
uint32_t s_cpuMemoryBudget = 200 * 1024 * 1024;
uint32_t s_lodChanges = 4;
uint32_t s_loadRequests = 2;

uint32_t GetTextureSize( const Tr2BitmapDimensions& desc )
{
	uint32_t size = 0;
	for( uint32_t i = 0; i < desc.GetTrueMipCount(); ++i )
	{
		size += desc.GetMipSize( i );
	}
	return size * std::max( 1u, desc.GetArraySize() );
}

}

TRI_REGISTER_SETTING( "textureLodGpuBudget", s_gpuMemoryBudget );
TRI_REGISTER_SETTING( "textureLodCpuBudget", s_cpuMemoryBudget );
TRI_REGISTER_SETTING( "textureLodChanges", s_lodChanges );
TRI_REGISTER_SETTING( "textureLoadRequests", s_loadRequests );




Tr2TextureLodManager::Stats::Stats() :
	gpuMemoryUsed( 0 ),
	gpuMemoryAllocated( 0 ),
	cpuMemoryUsed( 0 ),
	cpuMemoryAllocated( 0 ),
	gpuUploadSize( 0 )
{
}


Tr2TextureLodManager::Tr2TextureLodManager() :
	m_gpuMemorySize( 0 ),
	m_cpuMemorySize( 0 ),
	m_lowDetailVtaFiles( false )
{
	BeOS->RegisterForTicks( this, nullptr );
}

Tr2TextureLodManager& Tr2TextureLodManager::Instance()
{
	static CTr2TextureLodManager* s_manager = new CTr2TextureLodManager();
	return *s_manager;
}

void Tr2TextureLodManager::OnTick( Be::Time, Be::Time, void* )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto& it : m_textures )
	{
		it.first->UpdateLodRequest( it.second, *this );
		if( it.second.mipChange == 0 )
		{
			it.second.frameNumber = 0;
		}
	}

	std::sort( begin( m_textures ), end( m_textures ), []( const auto& a, const auto& b ) {
		if( a.second.mipChange < b.second.mipChange )
		{
			return true;
		}
		if( a.second.mipChange == b.second.mipChange )
		{
			return a.second.frameNumber > b.second.frameNumber;
		}

		return false;
	} );
	uint32_t creates = 0;
	uint32_t loads = 0;
	for( auto& it : m_textures )
	{
		if( it.second.mipChange == 0 )
		{
			continue;
		}
		if( it.second.mipChange >= 0 && !NeedToTrimGpuTexture() )
		{
			break;
		}
		if( creates >= s_lodChanges && loads >= s_loadRequests )
		{
			break;
		}
		if( it.second.cachedInRam )
		{
			if( creates < s_lodChanges )
			{
				++creates;
				it.first->ProcessLodRequest( it.second, *this );
				it.second.frameNumber = 0;
			}
		}
		else
		{
			if( loads < s_loadRequests )
			{
				++loads;
				it.first->ProcessLodRequest( it.second, *this );
				it.second.frameNumber = 0;
			}
		}
	}
}

void Tr2TextureLodManager::RegisterTexture( TriTextureRes* texture )
{
	m_textures.push_back( { texture, {} } );
}

void Tr2TextureLodManager::UnregisterTexture( TriTextureRes* texture )
{
	m_textures.erase(
		std::remove_if( begin( m_textures ), end( m_textures ), [texture]( const auto& x ) {
			return x.first == texture;
		} ) );
}

bool Tr2TextureLodManager::CanUploadToGpu( const Tr2BitmapDimensions& desc ) const
{
	return true;
}

bool Tr2TextureLodManager::NeedToTrimGpuTexture() const
{
	return m_gpuMemorySize > s_gpuMemoryBudget;
}

bool Tr2TextureLodManager::NeedToTrimCpuTexture() const
{
	return m_cpuMemorySize > s_cpuMemoryBudget;
}

void Tr2TextureLodManager::GpuTextureCreated( const Tr2BitmapDimensions& desc )
{
	auto size = GetTextureSize( desc );
	m_gpuMemorySize += size;
	CCP_STATS_SET( textureLodGpuSize, m_gpuMemorySize );
	CCP_STATS_ADD( textureLodUploadSize, size );
}

void Tr2TextureLodManager::GpuTextureDestroyed( const Tr2BitmapDimensions& desc )
{
	m_gpuMemorySize += static_cast<uint32_t>( -int32_t( GetTextureSize( desc ) ) );
	CCP_STATS_SET( textureLodGpuSize, m_gpuMemorySize );
}

void Tr2TextureLodManager::CpuTextureCreated( const Tr2BitmapDimensions& desc )
{
	m_cpuMemorySize += GetTextureSize( desc );
	CCP_STATS_SET( textureLodCpuSize, m_cpuMemorySize );
}

void Tr2TextureLodManager::CpuTextureDestroyed( const Tr2BitmapDimensions& desc )
{
	m_cpuMemorySize += static_cast<uint32_t>( -int32_t( GetTextureSize( desc ) ) );
	CCP_STATS_SET( textureLodCpuSize, m_cpuMemorySize );
}


Tr2TextureLodManager::Stats Tr2TextureLodManager::GetStats() const
{
	return m_currentStats;
}

std::vector<TriTextureRes*> Tr2TextureLodManager::GetManagedTextures() const
{
	std::vector<TriTextureRes*> textures;
	textures.reserve( m_textures.size() );
	for( auto& t : m_textures )
	{
		textures.push_back( t.first );
	}
	return textures;
}

void Tr2TextureLodManager::SetUseLowResVtaFilesSetting( bool setting )
{
	if( setting != m_lowDetailVtaFiles )
	{
		m_lowDetailVtaFiles = setting;
		for( auto& t : m_textures )
		{
			auto path = t.first->GetPath();
			auto length = wcslen( path );
			if( length > 4 && wcscmp( path + length - 4, L".vta" ) == 0 )
			{
				t.first->Reload();
			}
		}
	}
}

bool Tr2TextureLodManager::GetUseLowResVtaFilesSetting() const
{
	return m_lowDetailVtaFiles;
}
