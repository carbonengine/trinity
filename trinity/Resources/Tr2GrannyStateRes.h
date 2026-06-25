// Copyright © 2023 CCP ehf.

#pragma once

#ifndef Tr2GrannyStateRes_h
#define Tr2GrannyStateRes_h

#if GSTATE_ENABLED

#if WITH_GRANNY
#include "gstate.h"
#endif

BLUE_DECLARE( TriGrannyRes );
BLUE_DECLARE( Tr2GrannyStateRes );

struct GStateBindingCallbackData
{
	std::string gsf_path;
	std::map<std::string, TriGrannyResPtr>* anim_map_pointer;
	GStateBindingCallbackData() :
		gsf_path( "" ),
		anim_map_pointer( nullptr )
	{
	}
};

std::string GetFullAnimPath( std::string SourceFilenameString, std::string dir_path );

BLUE_CLASS( Tr2GrannyStateRes ) :
	public BlueAsyncRes,
	public ICacheable,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();
	Tr2GrannyStateRes( IRoot* lockobj = NULL );
	~Tr2GrannyStateRes();

	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	//
	//////////////////////////////////////////////////////////////////////////

	granny_file* GetCharacterFile() const
	{
		return m_characterFile;
	}
	gstate_character_info* GetCharacterInfo() const;
	const std::vector<std::string> GetGStateAnimFileRefPaths() const;

	bool IsFullyLoaded() const;

	//////////////////////////////////////////////////////////////////////////
	// IAsyncLoadedResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes * p );
	void RebuildCachedData( BlueAsyncRes * p );
	void Cleanup();

protected:
	virtual LoadingResult DoLoad();
	bool DoPrepare();

private:
	size_t m_dataSize;
	size_t m_memoryUsage;
	void* m_data;
	bool m_anim_bound;
	GStateBindingCallbackData m_callbackData;
	granny_file* m_characterFile;
	std::map<std::string, TriGrannyResPtr> m_gStateAnimFiles;
	void LoadAnimResPath( const std::string& val );
	void LoadAnimResources();
	bool IsAllAnimGood();
	void WaitForAndBindAnimResources();
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2GrannyStateRes );

#endif

#endif