// Copyright © 2023 CCP ehf.

#ifndef TR2EFFECTRES_H
#define TR2EFFECTRES_H


#include "ITr2EffectValue.h"
#include "Tr2DeviceResource.h"
#include "ITriReroutable.h"
#include "Shader/Tr2EffectDescription.h"

class TriVariable;
struct ITriEffectResourceParameter;

BLUE_DECLARE( Tr2EffectRes );
BLUE_DECLARE( Tr2Shader );

struct Tr2ShaderPermutation
{
	BlueSharedString name;
	std::vector<BlueSharedString> options;
	size_t defaultOption;
	std::string description;
	uint8_t type;
};

BLUE_CLASS( Tr2EffectRes ) :
	public BlueAsyncRes,
	public ICacheable,
	public Tr2DeviceResource
{
public:
	EXPOSE_TO_BLUE();

	Tr2EffectRes( IRoot* lockobj = NULL );
	~Tr2EffectRes();

	Tr2ShaderPtr GetShader( const Tr2ShaderOption* options, size_t count );


	//////////////////////////////////////////////////////////////////////////
	// ICacheable
	bool IsMemoryUsageKnown();
	size_t GetMemoryUsage();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	void ReleaseResources( TriStorage s );
#if TRINITYDEV
	virtual void GetDescription( std::string & desc );
#endif
private:
	bool OnPrepareResources();

	// Provide the functions that do the actual work of loading and preparing.
	// The async management itself is done in BlueAsyncRes.
	virtual LoadingResult DoLoad();
	virtual bool DoPrepare();

#if BLUE_WITH_PYTHON
	PyObject* GetPermutationDescription();
#endif

protected:
	// Per-permutation compiled file information
	struct FileRecord
	{
		uint32_t index;
		// Compiled code offset into the file
		uint32_t offset;
		// Compiled code size
		uint32_t size;
	};

	CcpMallocBuffer m_data;
	uint32_t m_version;

	const char* m_stringTable;
	size_t m_stringTableSize;

	const FileRecord* m_offsets;
	uint32_t m_offsetCount;

	TrackableStdVector<Tr2ShaderPermutation> m_permutations;
	TrackableStdUnorderedMap<uint32_t, Tr2ShaderPtr> m_shaders;

	friend class Tr2EffectStateManager;
};

TYPEDEF_BLUECLASS_WR_SHUTDOWN( Tr2EffectRes );


void ModifyGlobalEffectOptions( const std::vector<Tr2ShaderOption>& changes );
const std::vector<Tr2ShaderOption>& GetGlobalEffectOptions();


#endif
