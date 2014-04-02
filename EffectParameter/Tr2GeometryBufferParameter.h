////////////////////////////////////////////////////////////
//
//    Created:   October 2012
//    Copyright: CCP 2012
//

#pragma once

#include "include/ITriEffectParameter.h"

BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );
BLUE_DECLARE_INTERFACE( ITr2ShaderState );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GeometryBufferParameter is an effect parameter class that can be used to provide
//   buffers (Tr2UavBuffer or geometry) to effects.
// See Also:
//   ITriEffectResourceParameter
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GeometryBufferParameter ):
	public ITriEffectResourceParameter,
	public IInitialize,
	public INotify
{
public:
	Tr2GeometryBufferParameter(IRoot* lockobj = NULL);
	~Tr2GeometryBufferParameter();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	const char* GetParameterName() const;
	virtual bool IsZeroOrNull( void ) const;
	void RebuildEffectHandles( ITr2ShaderState* effectRes );

	//////////////////////////////////////////////////////////////////////////
	// ITriEffectResourceParameter
	void ReloadResources();
	void LoadResources();
	void UnloadResources();
	void* GetResourcePointer() const;
	bool IsPrepared() const;
	void CopyValueToEffect(		Tr2RenderContextEnum::ShaderType inputType, 
								unsigned char* destHandle, 
								size_t resourceFlags,
								Tr2RenderContext &renderContext ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize();

	bool IsValid() const;

	std::string m_name;
protected:
	// Path to geometry resource
	std::wstring m_resourcePath;
	// Mesh index in geometry resource
	int	m_meshIndex;

	// GPU buffer
	ITr2GpuBufferPtr m_gpuBuffer;

	// If the parameter used
	bool m_isUsedByEffect;

	// Owner effect
	ITr2ShaderStatePtr m_cachedEffect;
public:
	EXPOSE_TO_BLUE();
};

BLUE_CLASS_ALLOW_DELAYED_DELETE( Tr2GeometryBufferParameter );
TYPEDEF_BLUECLASS( Tr2GeometryBufferParameter );