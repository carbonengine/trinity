// Copyright © 2011 CCP ehf.

#pragma once

// --------------------------------------------------------------------------------------
// Description:
//   CachingIncludeHandler is an include handler for D3D effect compiler. It caches
//   contents of included files, so the next time they are requested it returns cached
//   contents. This object can be accessed from different threads.
// --------------------------------------------------------------------------------------
class CachingIncludeHandler
{
public:
	CachingIncludeHandler();
	~CachingIncludeHandler();

	struct IncludedFile
	{
		std::string fullPath;
		const char* data;
		size_t size;
		time_t modifiedTime;
	};

	enum IncludeType
	{
		IncludeLocal,
		IncludeSystem
	};

	std::optional<IncludedFile> Open( const char* fileName, IncludeType includeType, const char* parentData = nullptr, const char* rootPath = nullptr );

	void SetRootPath( const char* shaderPath );

	std::optional<IncludedFile> AddPrefix( const char* fileName, const char* prefix );

	void AddSystemIncludePath( const char* path );

private:
	typedef std::map<const void*, std::string> PathFromFile;
	typedef std::map<std::string, IncludedFile> FileFromPath;

	// Mapping from file path to file contents
	PathFromFile m_pathFromFile;
	// Mapping from file contents to file path (for resolving relative paths)
	FileFromPath m_fileFromPath;
	// Path of the entry point file
	std::string m_rootPath;
	std::vector<std::string> m_systemIncludePaths;

	std::mutex m_cs;
};
