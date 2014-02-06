////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#include "stdafx.h"
#include "CachingIncludeHandler.h"

// --------------------------------------------------------------------------------------
// Description:
//   CachingIncludeHandler default constructor
// --------------------------------------------------------------------------------------
CachingIncludeHandler::CachingIncludeHandler()
{
	InitializeCriticalSection( &m_CS );
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
	DeleteCriticalSection( &m_CS );
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
HRESULT CachingIncludeHandler::Open( D3DXINCLUDE_TYPE includeType, 
									 LPCSTR fileName, 
									 LPCVOID parentData, 
									 LPCVOID *data, 
									 UINT *bytes,
									 LPCSTR rootPath,
									 FILETIME& mtime )
{
	EnterCriticalSection( &m_CS );
	char fullPath[MAX_PATH];
	if( PathIsRelative( fileName ) )
	{
		char parentPath[MAX_PATH];
		if( parentData == NULL )
		{
			strcpy_s( parentPath, MAX_PATH, rootPath );
		}
		else
		{
			PathFromFile::iterator parent = m_pathFromFile.find( reinterpret_cast<const char*>( parentData ) - 1 );
			if( parent != m_pathFromFile.end() )
			{
				strcpy_s( parentPath, MAX_PATH, parent->second.c_str() );
			}
			else
			{
				parentPath[0] = 0;
			}
		}
		if( !PathAppend( parentPath, fileName ) )
		{
			LeaveCriticalSection( &m_CS );
			return E_FAIL;
		}
		if( !PathCanonicalize( fullPath, parentPath ) )
		{
			LeaveCriticalSection( &m_CS );
			return E_FAIL;
		}
	}
	else
	{
		strcpy_s( fullPath, MAX_PATH, fileName );
	}
	FileFromPath::iterator fileFromPath = m_fileFromPath.find( fullPath );
	if( fileFromPath != m_fileFromPath.end() )
	{
		*bytes = fileFromPath->second.size;
		*data = reinterpret_cast<char*>( fileFromPath->second.data ) + 1;
		mtime = fileFromPath->second.modifiedTime;
		LeaveCriticalSection( &m_CS );
		return S_OK;
	}
	HANDLE file = CreateFile( fullPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL );
	if( file == INVALID_HANDLE_VALUE )
	{
		LeaveCriticalSection( &m_CS );
		return E_FAIL;
	}

	FileInfo info;

	if( !GetFileTime( file, nullptr, nullptr, &info.modifiedTime ) )
	{
		info.modifiedTime.dwHighDateTime = (DWORD)-1;
		info.modifiedTime.dwLowDateTime = (DWORD)-1;
	}
	mtime = info.modifiedTime;

	unsigned fileSize = GetFileSize( file, NULL );

	info.size = fileSize;
	info.data = malloc( info.size + 2 );
	if( info.data == NULL )
	{
		CloseHandle( file );
		LeaveCriticalSection( &m_CS );
		return E_FAIL;
	}
	*reinterpret_cast<char*>( info.data ) = '\n';
	DWORD bytesRead;
	if( !ReadFile( file, reinterpret_cast<char*>( info.data ) + 1, fileSize, &bytesRead, NULL ) )
	{
		CloseHandle( file );
		free( info.data );
		LeaveCriticalSection( &m_CS );
		return E_FAIL;
	}
	*( reinterpret_cast<char*>( info.data ) + fileSize + 1 ) = 0;
	CloseHandle( file );
	m_fileFromPath[fullPath] = info;
	const char* filename = PathFindFileName( fullPath );
	m_pathFromFile[info.data] = std::string( fullPath, filename - fullPath );
	*bytes = info.size;
	*data = reinterpret_cast<char*>( info.data ) + 1;
	LeaveCriticalSection( &m_CS );
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
HRESULT CachingIncludeHandler::Close( LPCVOID data )
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
	EnterCriticalSection( &m_CS );
	if( PathIsRelative( fileName ) )
	{
		char parentPath[MAX_PATH];
		if( parentData == NULL )
		{
			strcpy_s( parentPath, MAX_PATH, m_rootPath.c_str() );
		}
		else
		{
			PathFromFile::iterator parent = m_pathFromFile.find( reinterpret_cast<const char*>( parentData ) - 1 );
			if( parent != m_pathFromFile.end() )
			{
				strcpy_s( parentPath, MAX_PATH, parent->second.c_str() );
			}
			else
			{
				parentPath[0] = 0;
			}
		}
		if( !PathAppend( parentPath, fileName ) )
		{
			LeaveCriticalSection( &m_CS );
			return E_FAIL;
		}
		if( !PathCanonicalize( fullPath, parentPath ) )
		{
			LeaveCriticalSection( &m_CS );
			return E_FAIL;
		}
	}
	else
	{
		strcpy_s( fullPath, MAX_PATH, fileName );
	}
	LeaveCriticalSection( &m_CS );
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
	const char* filename = PathFindFileName( shaderPath );
	m_rootPath = std::string( shaderPath, filename - shaderPath );
}
