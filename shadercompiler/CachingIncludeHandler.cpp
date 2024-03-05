////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CachingIncludeHandler.h"

#if !_WIN32
#include <libgen.h>
#include <sys/param.h>
#include <sys/stat.h>


// TODO MACOS: This function is just a stub. Make sure it covers all cases.
namespace
{
    BOOL PathIsRelative( LPCSTR pszPath )
    {
        return pszPath[0] != '/';
    }
}
#endif

namespace
{
std::string GetDirPath( const char* filepath )
{
#if _WIN32
	const char* filename = PathFindFileName( filepath );
	return std::string( filepath, filename - filepath );
#else
	char storage[MAXPATHLEN];
	const char* dirName = dirname_r( filepath, storage );
	return dirName;
#endif
}

std::optional<std::string> JoinPath( const char* path1, const char* path2 )
{
#if _WIN32
	char p1[MAX_PATH];
	strcpy_s( p1, path1 );
	if( !PathAppend( p1, path2 ) )
	{
		return std::nullopt;
	}
	for( auto p = p1; *p; ++p )
	{
		if( *p == '/' )
		{
			*p = '\\';
		}
	}
	char fullPath[MAX_PATH];
	if( !PathCanonicalize( fullPath, p1 ) )
	{
		return std::nullopt;
	}
	return fullPath;
#else
	// TODO MACOS
	return std::string( path1 ) + "/" + path2;
#endif
}

}


CachingIncludeHandler::CachingIncludeHandler()
{
}

CachingIncludeHandler::~CachingIncludeHandler()
{
	for( FileFromPath::iterator it = m_fileFromPath.begin(); it != m_fileFromPath.end(); ++it )
	{
		free( (void*)( it->second.data - 1 ) );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Loads an included file.
// Arguments:
//   fileName - File name to include (can be relative to parent path)
//   parentData - Contents of the parent file as returned by a previous call to Open;
//     used for resolving relative paths
//   rootPath - Custom root path for resolving relative paths
// --------------------------------------------------------------------------------------
std::optional<CachingIncludeHandler::IncludedFile> CachingIncludeHandler::Open( const char* fileName, const char* parentData, const char* rootPath )
{
	std::lock_guard scope( m_cs );

	std::string fullPath;
	if( PathIsRelative( fileName ) )
	{
		std::string parentPath;
		if( parentData == nullptr )
		{
			parentPath = rootPath ? rootPath : m_rootPath;
		}
		else
		{
			auto parent = m_pathFromFile.find( parentData - 1 );
			if( parent != m_pathFromFile.end() )
			{
				parentPath = parent->second;
			}
			else
			{
				parentPath = "";
			}
		}
		if( auto joined = JoinPath( parentPath.c_str(), fileName ) )
		{
			fullPath = *joined;
		}
		else
		{
			return std::nullopt;
		}
	}
	else
	{
		fullPath = fileName;
	}
	auto fileFromPath = m_fileFromPath.find( fullPath );
	if( fileFromPath != m_fileFromPath.end() )
	{
		return fileFromPath->second;
	}

	IncludedFile info;
	info.fullPath = fullPath;

	FILE* file = nullptr;
	fopen_s( &file, fullPath.c_str(), "rb" );
	if( !file )
	{
		return std::nullopt;
	}

	size_t fileSize = 0;
	struct stat buf;
	if( stat( fullPath.c_str(), &buf ) == 0 )
	{
		info.modifiedTime = buf.st_mtime;
		fileSize = buf.st_size;
	}
	else
	{
		info.modifiedTime = -1;
	}

	info.size = fileSize;
	auto fileData = malloc( info.size + 2 );
	if( fileData == nullptr )
	{
		fclose( file );
		return std::nullopt;
	}
	*static_cast<char*>( fileData ) = '\n';

	auto bytesRead = fread( static_cast<char*>( fileData ) + 1, 1, fileSize, file );
	if( bytesRead != fileSize )
	{
		fclose( file );
		free( fileData );
		return std::nullopt;
	}
	*( static_cast<char*>( fileData ) + fileSize + 1 ) = 0;
	fclose( file );

	info.data = static_cast<const char*>( fileData ) + 1;
	m_fileFromPath[fullPath] = info;
	m_pathFromFile[fileData] = GetDirPath( fullPath.c_str() );
	return info;
}

std::optional<CachingIncludeHandler::IncludedFile> CachingIncludeHandler::AddPrefix( const char* fileName, const char* prefix )
{
	std::lock_guard scope( m_cs );

	std::string fullPath;
	if( PathIsRelative( fileName ) )
	{
		if( auto joined = JoinPath( "", fileName ) )
		{
			fullPath = *joined;
		}
		else
		{
			return std::nullopt;
		}
	}
	else
	{
		fullPath = fileName;
	}
	FileFromPath::iterator fileFromPath = m_fileFromPath.find( fullPath );
	if( fileFromPath == m_fileFromPath.end() )
	{
		return std::nullopt;
	}

	auto &info = fileFromPath->second;
	auto length = strlen( prefix );
	auto data = malloc( info.size + length + 2 );
	if( data == NULL )
	{
		return std::nullopt;
	}
	*static_cast<char*>( data ) = '\n';
	memcpy( static_cast<char*>( data ) + 1, prefix, length );
	memcpy( static_cast<char*>( data ) + 1 + length, info.data, info.size + 1 );
	info.size += length;

	m_pathFromFile[data] = m_pathFromFile[info.data - 1];
	m_pathFromFile.erase( info.data - 1 );
	free( (void*)( info.data - 1 ) );
	info.data = static_cast<const char*>( data ) + 1;

	return info;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns path to the "root", entry point file.
// Arguments:
//   shaderPath - path to the "root", entry point file
// --------------------------------------------------------------------------------------
void CachingIncludeHandler::SetRootPath( const char* shaderPath )
{
	std::lock_guard scope( m_cs );
	m_rootPath = GetDirPath( shaderPath );
}
