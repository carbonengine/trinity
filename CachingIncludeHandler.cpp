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

// --------------------------------------------------------------------------------------
// Description:
//   CachingIncludeHandler default constructor
// --------------------------------------------------------------------------------------
CachingIncludeHandler::CachingIncludeHandler()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   CachingIncludeHandler destructor
// --------------------------------------------------------------------------------------
CachingIncludeHandler::~CachingIncludeHandler()
{
	for( FileFromPath::iterator it = m_fileFromPath.begin(); it != m_fileFromPath.end(); ++it )
	{
		free( it->second.data );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ID3DXInclude interface. Fetches requested include file contents either
//   from cache or from file.
// Arguments:
//   includeType - Type of include (local or system), parameter unused
//   fileName - File name to include (can be relative to parent path)
//   parentData - Contents of the parent file
//   data - (out) Contents of the included file
//   bytes - (out) Size of contents of the included file
// Return Value:
//   S_OK if file contents was extracted
//   E_FAIL on error
// --------------------------------------------------------------------------------------
HRESULT CachingIncludeHandler::Open( D3DXINCLUDE_TYPE includeType, 
									 LPCSTR fileName, 
									 LPCVOID parentData, 
									 LPCVOID *data, 
									 UINT *bytes )
{
	FILETIME unused;
	return Open( includeType, fileName, parentData, data, bytes, m_rootPath.c_str(), unused );
}

// --------------------------------------------------------------------------------------
// Description:
//   A custom variant of ID3DXInclude::Open method that allows getting included file 
//   modification time and use custom file root. This function is used by 
//   MTimeIncludeHandler.
// Arguments:
//   includeType - Type of include (local or system), parameter unused
//   fileName - File name to include (can be relative to parent path)
//   parentData - Contents of the parent file
//   data - (out) Contents of the included file
//   bytes - (out) Size of contents of the included file
//   rootPath - Custom root path for resolving relative paths
//   mtime - (out) Included file modification time
// Return Value:
//   S_OK if file contents was extracted
//   E_FAIL on error
// See also:
//   MTimeIncludeHandler
// --------------------------------------------------------------------------------------
HRESULT CachingIncludeHandler::Open( D3DXINCLUDE_TYPE, 
									 LPCSTR fileName, 
									 LPCVOID parentData, 
									 LPCVOID *data, 
									 UINT *bytes,
									 LPCSTR rootPath,
									 FILETIME& mtime )
{
	MutexScope scope( m_CS );

	char fullPath[MAX_PATH];
	if( PathIsRelative( fileName ) )
	{
		char parentPath[MAX_PATH];
		if( parentData == NULL )
		{
			snprintf( parentPath, sizeof( parentPath ), "%s", rootPath );
		}
		else
		{
			PathFromFile::iterator parent = m_pathFromFile.find( reinterpret_cast<const char*>( parentData ) - 1 );
			if( parent != m_pathFromFile.end() )
			{
				snprintf( parentPath, sizeof( parentPath ), "%s", parent->second.c_str() );
			}
			else
			{
				parentPath[0] = 0;
			}
		}
#if _WIN32
		if( !PathAppend( parentPath, fileName ) )
		{
			return E_FAIL;
		}
		if( !PathCanonicalize( fullPath, parentPath ) )
		{
			return E_FAIL;
		}
#else
        // TODO MACOS
		snprintf( fullPath, sizeof( fullPath ), "%s/%s", parentPath, fileName );
		// size_t len = strlen( parentPath );
		// errno_t result = strcpy_s( parentPath + len, MAX_PATH - len, fileName );
		// if( result != 0 )
		// {
		// 	return E_FAIL;
		// }

		// TODO: Make the path canonical. (How?)
		// realpath( parentPath, fullPath );
		// errno_t result = strcpy_s( fullPath, MAX_PATH, parentPath );
		// if( result != 0 )
		// {
		// 	return E_FAIL;
		// }
#endif
	}
	else
	{
		snprintf( fullPath, sizeof( fullPath ), "%s", fileName );
	}
	FileFromPath::iterator fileFromPath = m_fileFromPath.find( fullPath );
	if( fileFromPath != m_fileFromPath.end() )
	{
		*bytes = UINT( fileFromPath->second.size );
		*data = reinterpret_cast<char*>( fileFromPath->second.data ) + 1;
		mtime = fileFromPath->second.modifiedTime;
		return S_OK;
	}

	FileInfo info;

#if _WIN32
	HANDLE file = CreateFile( fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( file == INVALID_HANDLE_VALUE )
	{
		return E_FAIL;
	}

	if( !GetFileTime( file, nullptr, nullptr, &info.modifiedTime ) )
	{
		info.modifiedTime.dwHighDateTime = (DWORD)-1;
		info.modifiedTime.dwLowDateTime = (DWORD)-1;
	}
	mtime = info.modifiedTime;

	unsigned fileSize = GetFileSize( file, NULL );
#else
	FILE* file = fopen( fullPath, "rb" );
	if( !file )
	{
		return E_FAIL;
	}

	unsigned fileSize = 0;
	struct stat buf;
	if( stat( fullPath, &buf ) == 0 )
	{
		mtime.dwHighDateTime = buf.st_mtimespec.tv_sec;
		mtime.dwLowDateTime = buf.st_mtimespec.tv_nsec;
		fileSize = buf.st_size;
	}
	else
	{
		mtime.dwHighDateTime = (DWORD)-1;
		mtime.dwLowDateTime = (DWORD)-1;
	}
#endif

	info.size = fileSize;
	info.data = malloc( info.size + 2 );
	if( info.data == NULL )
	{
#if _WIN32
		CloseHandle( file );
#else
		fclose( file );
#endif
		return E_FAIL;
	}
	*reinterpret_cast<char*>( info.data ) = '\n';
	DWORD bytesRead;
#if _WIN32
	if( !ReadFile( file, reinterpret_cast<char*>( info.data ) + 1, fileSize, &bytesRead, NULL ) )
	{
		CloseHandle( file );
		free( info.data );
		return E_FAIL;
	}
	*( reinterpret_cast<char*>( info.data ) + fileSize + 1 ) = 0;
	CloseHandle( file );
	m_fileFromPath[fullPath] = info;
	const char* filename = PathFindFileName( fullPath );
	m_pathFromFile[info.data] = std::string( fullPath, filename - fullPath );
#else
	bytesRead = fread( reinterpret_cast<char*>( info.data ) + 1, 1, fileSize, file );
	if( bytesRead != fileSize )
	{
		fclose( file );
		free( info.data );
		return E_FAIL;
	}
	*( reinterpret_cast<char*>( info.data ) + fileSize + 1 ) = 0;
	fclose( file );
	m_fileFromPath[fullPath] = info;
	char storage[MAXPATHLEN];
	const char* dirName = dirname_r( fullPath, storage );
	m_pathFromFile[info.data] = dirName;
#endif
	*bytes = UINT( info.size );
	*data = reinterpret_cast<char*>( info.data ) + 1;
	return S_OK;
}

HRESULT CachingIncludeHandler::AddPrefix( const char* fileName, const char* prefix, LPCVOID *outData, UINT *bytes )
{
	MutexScope scope( m_CS );

	char fullPath[MAX_PATH];
	if( PathIsRelative( fileName ) )
	{
		char parentPath[MAX_PATH];
		parentPath[0] = 0;
#if _WIN32
		if( !PathAppend( parentPath, fileName ) )
		{
			return E_FAIL;
		}
		if( !PathCanonicalize( fullPath, parentPath ) )
		{
			return E_FAIL;
		}
#else
        // TODO MACOS
		snprintf( fullPath, sizeof( fullPath ), "%s/%s", parentPath, fileName );
		// size_t len = strlen( parentPath );
		// errno_t result = strcpy_s( parentPath + len, MAX_PATH - len, fileName );
		// if( result != 0 )
		// {
		// 	return E_FAIL;
		// }

		// TODO: Make the path canonical. (How?)
		// errno_t result = strcpy_s( fullPath, MAX_PATH, parentPath );
		// if( result != 0 )
		// {
		// 	return E_FAIL;
		// }
#endif
	}
	else
	{
		snprintf( fullPath, sizeof( fullPath ), "%s", fileName );
	}
	FileFromPath::iterator fileFromPath = m_fileFromPath.find( fullPath );
	if( fileFromPath == m_fileFromPath.end() )
	{
		return E_FAIL;
	}

	auto &info = fileFromPath->second;
	auto length = strlen( prefix );
	auto data = malloc( info.size + length + 2 );
	if( data == NULL )
	{
		return E_FAIL;
	}
	*reinterpret_cast<char*>( data ) = '\n';
	memcpy( static_cast<char*>( data ) + 1, prefix, length );
	memcpy( static_cast<char*>( data ) + 1 + length, static_cast<char*>( info.data ) + 1, info.size + 1 );
	info.size += length;

	m_pathFromFile[data] = m_pathFromFile[info.data];
	m_pathFromFile.erase( info.data );
	free( info.data );
	info.data = data;

	if( outData )
	{
		*outData = static_cast<char*>( data ) + 1;
	}
	if( bytes )
	{
		*bytes = UINT( info.size );
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ID3DXInclude interface. Supposed to free contents of a previously opened
//   include file. Does nothing since we cache all included file contents.
// Arguments:
//   data - Contents of the included file
// Return Value:
//   S_OK always
// --------------------------------------------------------------------------------------
HRESULT CachingIncludeHandler::Close( LPCVOID )
{
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a full file path for a relative include path.
// Arguments:
//   fileName - File path relative to the "parent" file (also might be a full path)
//   parentData - Contents of the "parent" file (returned by previous call to Open)
//   fullPath - (out) Full path to fileName. Needs to be pre-allocated.
// Return Value:
//   S_OK if path constructed successfully
//   E_FAIL on error
// --------------------------------------------------------------------------------------
HRESULT CachingIncludeHandler::GetFullPathName( LPCSTR fileName, LPCVOID parentData, LPSTR fullPath )
{
	MutexScope scope( m_CS );

	if( PathIsRelative( fileName ) )
	{
		char parentPath[MAX_PATH];
		if( parentData == NULL )
		{
			snprintf( parentPath, sizeof( parentPath ), "%s", m_rootPath.c_str() );
		}
		else
		{
			PathFromFile::iterator parent = m_pathFromFile.find( reinterpret_cast<const char*>( parentData ) - 1 );
			if( parent != m_pathFromFile.end() )
			{
				snprintf( parentPath, sizeof( parentPath ), "%s", parent->second.c_str() );
			}
			else
			{
				parentPath[0] = 0;
			}
		}
#if _WIN32
		if( !PathAppend( parentPath, fileName ) )
		{
			return E_FAIL;
		}
		if( !PathCanonicalize( fullPath, parentPath ) )
		{
			return E_FAIL;
		}
#else
        // TODO MACOS
		snprintf( fullPath, MAX_PATH, "%s/%s", parentPath, fileName );
		// size_t len = strlen( parentPath );
		// errno_t result = strcpy_s( parentPath + len, MAX_PATH - len, fileName );
		// if( result != 0 )
		// {
		// 	return E_FAIL;
		// }

		// TODO: Make the path canonical. (How?)
		// errno_t result = strcpy_s( fullPath, MAX_PATH, parentPath );
		// if( result != 0 )
		// {
		// 	return E_FAIL;
		// }
#endif
	}
	else
	{
		snprintf( fullPath, MAX_PATH, "%s", fileName );
	}
	return S_OK;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns path to the "root", entry point file.
// Arguments:
//   shaderPath - path to the "root", entry point file
// --------------------------------------------------------------------------------------
void CachingIncludeHandler::SetRootPath( const char* shaderPath )
{
#if _WIN32
	const char* filename = PathFindFileName( shaderPath );
	m_rootPath = std::string( shaderPath, filename - shaderPath );
#else
	char storage[MAXPATHLEN];
	const char* dirName = dirname_r( shaderPath, storage );
	m_rootPath = dirName;
#endif
}
