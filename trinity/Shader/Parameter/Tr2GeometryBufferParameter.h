// Copyright © 2012 CCP ehf.

#pragma once

#include "include/ITriEffectParameter.h"

BLUE_DECLARE_INTERFACE( ITr2GpuBuffer );
BLUE_DECLARE( Tr2Shader );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GeometryBufferParameter is an effect parameter class that can be used to provide
//   buffers (Tr2UavBuffer or geometry) to effects.
// See Also:
//   ITriEffectResourceParameter
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GeometryBufferParameter ) :
	public ITriEffectResourceParameter,
	public IInitialize,
	public INotify
{
public:
	Tr2GeometryBufferParameter( IRoot* lockobj = NULL );
	~Tr2GeometryBufferParameter();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader * effectRes );
	unsigned GetHashValue( unsigned startingHash ) const;

	//////////////////////////////////////////////////////////////////////////
	// ITriEffectResourceParameter
	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	virtual bool ApplyUav(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize();

	bool IsValid() const;

	void SetGpuBuffer( ITr2GpuBuffer * buffer );
	ITr2GpuBufferPtr GetGpuBuffer() const;

	BlueSharedString m_name;

protected:
	// Path to geometry resource
	std::wstring m_resourcePath;
	// Mesh index in geometry resource
	int32_t m_meshIndex;

	// GPU buffer
	ITr2GpuBufferPtr m_gpuBuffer;

	// If the parameter used
	bool m_isUsedByEffect;

	// Owner effect
	Tr2ShaderPtr m_cachedEffect;

public:
	EXPOSE_TO_BLUE();
};

BLUE_CLASS_ALLOW_DELAYED_DELETE( Tr2GeometryBufferParameter );
TYPEDEF_BLUECLASS( Tr2GeometryBufferParameter );