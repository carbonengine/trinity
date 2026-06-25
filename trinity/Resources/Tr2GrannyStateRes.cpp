// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#if GSTATE_ENABLED

#include "Tr2GrannyStateRes.h"
#include "Resources/TriGrannyRes.h"
#include "Utilities/GeometryUtils.h"


Tr2GrannyStateRes::Tr2GrannyStateRes( IRoot* lockobj ) :
	m_characterFile( nullptr ),
	m_data( nullptr ),
	m_dataSize( 0 ),
	m_memoryUsage( 0 ),
	m_anim_bound( false ),
	m_callbackData()
{
}


Tr2GrannyStateRes::~Tr2GrannyStateRes()
{
	Cleanup();
	if( m_characterFile )
	{
		GrannyFreeFile( m_characterFile );
		m_characterFile = NULL;
	}
}

bool Tr2GrannyStateRes::IsMemoryUsageKnown()
{
	return !IsLoading();
}

size_t Tr2GrannyStateRes::GetMemoryUsage()
{
	if( m_characterFile )
	{
		return m_dataSize + sizeof( m_characterFile );
	}
	else
	{
		return 1024;
	}
}


BlueAsyncRes::LoadingResult Tr2GrannyStateRes::DoLoad()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_dataStream->LockData( &m_data, 0 ) )
	{
		return LR_FAILED;
	}

	m_dataSize = m_dataStream->GetSize();

	{
		if( m_characterFile )
		{
			GrannyFreeFile( m_characterFile );
			m_characterFile = NULL;
		}

		CCP_STATS_ZONE( "Tr2GrannyStateRes::DoLoad reading Granny file" );
		m_characterFile = ProtectedGrannyReadEntireFileFromMemory( m_path.c_str(), (uint32_t)m_dataSize, m_data );
	}
	if( !m_characterFile )
	{
		return LR_FAILED;
	}

	return LR_SUCCESS;
}


std::string GetFullAnimPath( std::string SourceFilenameString, std::string dir_path )
{
	auto last_delimiter_pos = dir_path.find_last_of( "/\\" );
	dir_path.erase( last_delimiter_pos, dir_path.length() - last_delimiter_pos );

	while( SourceFilenameString.substr( 0, 2 ) == ".." )
	{
		SourceFilenameString.erase( 0, 3 );
		last_delimiter_pos = dir_path.find_last_of( "/\\" );
		dir_path.erase( last_delimiter_pos, dir_path.length() - last_delimiter_pos );
	}

	if( SourceFilenameString.substr( 0, 1 ) == "." )
	{
		SourceFilenameString.erase( 0, 2 );
	}

	dir_path += "/";
	dir_path += SourceFilenameString;
	SourceFilenameString = dir_path;

	std::replace( SourceFilenameString.begin(), SourceFilenameString.end(), '\\', '/' );
	return SourceFilenameString;
}


granny_file_info* GStateAnimationBindingCallback( gstate_character_info* BindingInfo,
												  char const* SourceFilename,
												  void* UserData )
{
	auto callbackData = *( static_cast<GStateBindingCallbackData*>( UserData ) );
	std::map<std::string, TriGrannyResPtr>* gStateAnimFilesPtr = callbackData.anim_map_pointer;


	// Animation granny files are relative to the parent directory of the .gsf file
	std::string SourceFilenameString = SourceFilename;
	std::string dir_path = callbackData.gsf_path;

	SourceFilenameString = GetFullAnimPath( SourceFilenameString, dir_path );

	TriGrannyResPtr result_granny_res = nullptr;

	for( auto it = gStateAnimFilesPtr->begin(); it != gStateAnimFilesPtr->end(); it++ )
	{

		std::string iter_file_name = it->first;
		std::replace( iter_file_name.begin(), iter_file_name.end(), '\\', '/' );

		if( iter_file_name == SourceFilenameString )
		{
			result_granny_res = it->second;
		}
	}

	if( result_granny_res != nullptr )
	{
		return result_granny_res->ValidateFileInfo();
	}

	CCP_LOGERR( "GState Binding Step: '%s' is required by the GState file and is not loaded", SourceFilenameString.c_str() );

	return nullptr;
}


bool Tr2GrannyStateRes::IsAllAnimGood()
{
	bool all_anim_good = true;

	auto anim_files = GetGStateAnimFileRefPaths();
	if( anim_files.size() != m_gStateAnimFiles.size() )
	{
		return false;
	}

	for( auto it = m_gStateAnimFiles.begin(); it != m_gStateAnimFiles.end(); it++ )
	{
		all_anim_good = all_anim_good && it->second->IsGood();
	}

	return all_anim_good;
}


bool Tr2GrannyStateRes::DoPrepare()
{
	LoadAnimResources();
	return true;
}

bool Tr2GrannyStateRes::IsFullyLoaded() const
{
	return m_anim_bound;
}

gstate_character_info* Tr2GrannyStateRes::GetCharacterInfo() const
{
	if( m_characterFile )
	{
		return GStateGetCharacterInfo( m_characterFile );
	}
	return NULL;
}


void Tr2GrannyStateRes::RebuildCachedData( BlueAsyncRes* p )
{
	for( auto it = m_gStateAnimFiles.begin(); it != m_gStateAnimFiles.end(); it++ )
	{
		if( p == it->second )
		{
			if( it->second && !it->second->GetGrannyFile() )
			{
				CCP_LOGERR( "'%s' not found or not a valid Granny file", it->first.c_str() );
			}
			continue;
		}
	}

	if( IsAllAnimGood() )
	{
		m_callbackData.gsf_path = CW2A( m_path.c_str() );
		m_callbackData.anim_map_pointer = &m_gStateAnimFiles;

		gstate_character_info* character_info = GetCharacterInfo();

		if( !GStateBindCharacterFileReferences( character_info, static_cast<gstate_file_ref_callback*>( GStateAnimationBindingCallback ), static_cast<void*>( &m_callbackData ) ) )
		{
			CCP_LOGERR( "'%ls' Granny State file refers to invalid or unavailable animation.", m_path.c_str() );
		}

		m_anim_bound = true;
	}
}

void Tr2GrannyStateRes::ReleaseCachedData( BlueAsyncRes* p )
{
	Cleanup();
}


void Tr2GrannyStateRes::Cleanup()
{
	m_anim_bound = false;
	if( !m_gStateAnimFiles.empty() )
	{
		for( auto it = m_gStateAnimFiles.begin(); it != m_gStateAnimFiles.end(); it++ )
		{
			it->second->RemoveNotifyTarget( this );
			it->second.Unlock();
		}
	}
	m_gStateAnimFiles.clear();
}


const std::vector<std::string> Tr2GrannyStateRes::GetGStateAnimFileRefPaths() const
{
	std::vector<std::string> path_list;

	gstate_character_info* character_info = GetCharacterInfo();
	granny_int32x num_anim_sets = GStateGetNumAnimationSets( character_info );

	for( int Idx = 0; Idx < num_anim_sets; Idx++ )
	{
		granny_int32x num_source_files = GStateGetNumSourceFileReferencesFromSetIndex( character_info, Idx );

		for( int ref_idx = 0; ref_idx < num_source_files; ref_idx++ )
		{
			path_list.push_back( std::string( GStateGetSourceFileReferenceFromIndex( character_info, Idx, ref_idx ) ) );
		}
	}

	return path_list;
}



void Tr2GrannyStateRes::LoadAnimResources()
{
	m_anim_bound = false;
	auto file_list = GetGStateAnimFileRefPaths();
	for( auto it = file_list.begin(); it != file_list.end(); ++it )
	{
		std::string anim_res_path = GetFullAnimPath( *it, static_cast<std::string>( CW2A( m_path.c_str() ) ) );
		LoadAnimResPath( anim_res_path );
	}
}


void Tr2GrannyStateRes::LoadAnimResPath( const std::string& val )
{
	auto it = m_gStateAnimFiles.find( val );
	if( it != m_gStateAnimFiles.end() )
	{
		it->second->RemoveNotifyTarget( this );
		it->second.Unlock();
	}

	TriGrannyResPtr granny_res;
	BeResMan->GetResource( val.c_str(), "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&granny_res );

	m_gStateAnimFiles[val] = granny_res;

	if( granny_res )
	{
		granny_res->AddNotifyTarget( this );
	}
}

#endif
