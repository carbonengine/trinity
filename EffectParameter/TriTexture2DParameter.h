/* 
	*************************************************************************

	TriTexture2DParameter.h

	Created:   March 2006
	OS:        Win32
	Project:   Trinity

	Description:   
		
		Under DX9, this parameter acts like a cut down version of TriTexture, 
	loading texture resources.

		Under DX10, this parameter will load any form of buffer object resource,
	primarily textures, but also eventually instance data etc.

	Dependencies:

		DirectX 9.0, Probably more, ytbd.

	(c) CCP 2006

	*************************************************************************
*/
#pragma once
#if !defined( _TriTexture2DParameter_H_)
#define _TriTexture2DParameter_H_

#include "include/ITriEffectParameter.h"

BLUE_DECLARE_INTERFACE( ITr2ShaderState );
BLUE_DECLARE( TriTextureRes );
BLUE_DECLARE( TriTexture2DParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriTexture2DParameter );

BLUE_CLASS( TriTexture2DParameter ):
	public ITriEffectResourceParameter,
	public IInitialize,
	public INotify,
	public ICopierCustomAssignment,
	public Tr2DeviceResource
{

public:
	TriTexture2DParameter(IRoot* lockobj = NULL);
	~TriTexture2DParameter();

	EXPOSE_TO_BLUE();

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
	void CopyValueToEffect(	Tr2RenderContextEnum::ShaderType inputType, 
							unsigned char* destHandle, 
							size_t isSRGB,
							Tr2RenderContext &renderContext ) const;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ICopierCustomAssignment
	/////////////////////////////////////////////////////////////////////////////////////
	virtual bool AssignTo(
		ICopierCustomAssignment* other,
		ICopier* copier
		);

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
private:
	bool OnPrepareResources();
public:				
	void ReleaseResources( TriStorage s );

	// access resource
	void SetResource( TriTextureRes* newRes );
	TriTextureRes* GetResource();
	// access strings
	void SetParameterName( const char* name );
	const char* GetResourcePath() const;
	void SetResourcePath( const char* resourcePath );

protected:
	// this is the index of the current override samplerstate-block
	unsigned int m_overrideSamplerStateIx;
	// update overriding
	void BuildOverrideSamplerStateIx(void);
	// override the effect's sampler state
	bool m_useOverrides;
	Tr2RenderContextEnum::TextureAddressMode m_overrideAddressUMode;
	Tr2RenderContextEnum::TextureAddressMode m_overrideAddressVMode;
	Tr2RenderContextEnum::TextureAddressMode m_overrideAddressWMode;
	Tr2RenderContextEnum::TextureFilter m_overrideFilterMode;
	Tr2RenderContextEnum::TextureFilter m_overrideMipFilterMode;
	float m_overrideMipmapLodBias;
	unsigned int m_overrideMaxMipLevel;
	unsigned int m_overrideMaxAnisotropy;
	bool m_overrideSrgb;

private:
	ITr2ShaderStatePtr m_cachedEffect;

	BlueSharedString m_name;
	std::string m_resourcePath;

	TriTextureResPtr m_resource;

	bool m_isUsedByEffect;
};

TYPEDEF_BLUECLASS(TriTexture2DParameter);
#endif 
