/* 
	*************************************************************************

	TriTextureParameter.h

	Created:   March 2015
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
#if !defined( _TriTextureParameter_H_)
#define _TriTextureParameter_H_

#include "include/ITriEffectParameter.h"
#include "Shader/Tr2EffectDescription.h"

BLUE_DECLARE( Tr2Shader );
BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE( TriTextureParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriTextureParameter );
BLUE_DECLARE( TriTextureRes );


BLUE_CLASS( TriTextureParameter ):
	public ITriEffectTextureParameter,
	public IInitialize,
	public INotify,
	public ICopierCustomAssignment
{

public:
	TriTextureParameter(IRoot* lockobj = NULL);
	~TriTextureParameter();

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader* effectRes );

	//////////////////////////////////////////////////////////////////////////
	// ITriEffectTextureParameter
	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL& resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	virtual bool ApplyUav(
		Tr2ResourceSetDescriptionAL& resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const;
	unsigned GetHashValue( unsigned startingHash ) const;

	void UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities ) override;
	void EnableTextureLoding( const std::array<float, UV_SET_MAX_COUNT>& uvDensityScale ) override;
	void DisableTextureLoding() override;

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

	// access resource
	void SetResource( ITr2TextureProvider* newRes );
	
	virtual ITr2TextureProvider* GetResource() const;
	
	// access strings
	void SetParameterName( const BlueSharedString& name );
	const wchar_t* GetResourcePath() const;
	void SetResourcePath( const char* resourcePath );

private:
	Tr2ShaderPtr m_cachedEffect;

	BlueSharedString m_name;

	ITr2TextureProviderPtr m_resource;
	mutable ITr2TextureProviderPtr m_lowResResource;
	TriTextureResPtr m_textureRes;
	Tr2EffectResource::Type m_resourceType;

	uint32_t m_uavMipLevel;

	std::array<float, UV_SET_MAX_COUNT> m_uvDensityScale;

	bool m_isUsedByEffect;
	bool m_textureLodEnabled;

protected:
	std::string m_resourcePath;
};

TYPEDEF_BLUECLASS(TriTextureParameter);
#endif 
