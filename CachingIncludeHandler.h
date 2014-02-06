////////////////////////////////////////////////////////////
//
//    Created:   November 2011
//    Copyright: CCP 2011
//

#pragma once
#ifndef CachingIncludeHandler_h
#define CachingIncludeHandler_h

// --------------------------------------------------------------------------------------
// Description:
//   CachingIncludeHandler is an include handler for D3D effect compiler. It caches 
//   contents of included files, so the next time they are requested it returns cached 
//   contents. This object can be accessed from different threads. 
// --------------------------------------------------------------------------------------
class CachingIncludeHandler: public ID3DXInclude
{
public:
	CachingIncludeHandler();
	~CachingIncludeHandler();

    HRESULT __stdcall Open( D3DXINCLUDE_TYPE includeType, 
							LPCSTR fileName, 
							LPCVOID parentData, 
							LPCVOID *data, 
							UINT *bytes );
    HRESULT __stdcall Close( LPCVOID data );

	HRESULT GetFullPathName( LPCSTR fileName, 
							 LPCVOID parentData,
							 LPSTR fullFileName );
	void SetRootPath( const char* shaderPath );

	HRESULT __stdcall Open( D3DXINCLUDE_TYPE includeType, 
							LPCSTR fileName, 
							LPCVOID parentData, 
							LPCVOID *data, 
							UINT *bytes,
							LPCSTR rootPath,
							FILETIME& mtime );
private:
	// Cached file contents
	struct FileInfo
	{
		void* data;
		unsigned size;
		FILETIME modifiedTime;
	};
	typedef std::map<LPCVOID, std::string> PathFromFile;
	typedef std::map<std::string, FileInfo> FileFromPath;

	// Mapping from file path to file contents
	PathFromFile m_pathFromFile;
	// Mapping from file contents to file path (for resolving relative paths)
	FileFromPath m_fileFromPath;
	// Path of the entry point file
	std::string m_rootPath;
	// Critical section for shared data
	CRITICAL_SECTION m_CS;
};

#endif // CachingIncludeHandler_h