// Copyright © 2006 CCP ehf.

#pragma once
#if !defined( _TriTextureParameter_H_ )
#define _TriTextureParameter_H_

#include "include/ITriEffectParameter.h"
#include "Shader/Tr2EffectDescription.h"

BLUE_DECLARE( Tr2Shader );
BLUE_DECLARE_INTERFACE( ITr2TextureProvider );
BLUE_DECLARE( TriTextureParameter );
BLUE_CLASS_ALLOW_DELAYED_DELETE( TriTextureParameter );
BLUE_DECLARE( TriTextureRes );


BLUE_CLASS( TriTextureParameter ) :
	public ITriEffectTextureParameter,
	public IInitialize,
	public INotify,
	public ICopierCustomAssignment
{

public:
	TriTextureParameter( IRoot* lockobj = NULL );
	~TriTextureParameter();

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITriEffectParameter
	const char* GetParameterName() const;
	void RebuildEffectHandles( Tr2Shader * effectRes );

	//////////////////////////////////////////////////////////////////////////
	// ITriEffectTextureParameter
	void CopyValueToEffect(
		Tr2RenderContextEnum::ShaderType inputType,
		unsigned char* destHandle,
		size_t size,
		Tr2RenderContext& renderContext ) const override;
	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	virtual bool ApplyUav(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const;
	void AddUsedTexture( Tr2BindlessResourcesAL & usedTextures ) const override;
	unsigned GetHashValue( unsigned startingHash ) const;
	bool SupportsDirtyNotification() const override;

	void UsedWithScreenSize( float screenSize, float worldRadius, const std::vector<float>& uvDensities ) override;
	void EnableTextureLoding( const std::array<float, UV_SET_MAX_COUNT>& uvDensityScale ) override;
	void DisableTextureLoding() override;

	void OnAddedToMaterial( Tr2Material * material ) override;
	void OnRemovedFromMaterial( Tr2Material * material ) override;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	/////////////////////////////////////////////////////////////////////////////////////
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ICopierCustomAssignment
	/////////////////////////////////////////////////////////////////////////////////////
	virtual bool AssignTo(
		ICopierCustomAssignment * other,
		ICopier * copier );

	// access resource
	void SetResource( ITr2TextureProvider * newRes );

	virtual ITr2TextureProvider* GetResource() const;

	// access strings
	void SetParameterName( const BlueSharedString& name );
	const wchar_t* GetResourcePath() const;
	void SetResourcePath( const char* resourcePath );

private:
	void OnTextureChanged();
	void CacheTexture();

	Tr2ShaderPtr m_cachedEffect;

	BlueSharedString m_name;

	ITr2TextureProviderPtr m_resource;
	mutable ITr2TextureProviderPtr m_lowResResource;
	TriTextureResPtr m_textureRes;
	Tr2EffectResource::Type m_resourceType;
	Tr2RenderContextEnum::ColorSpace m_bindlessColorSpace;

	uint32_t m_uavMipLevel;

	std::array<float, UV_SET_MAX_COUNT> m_uvDensityScale;
	std::vector<Tr2Material*> m_materials;

	Tr2TextureAL m_cachedTexture;
	uint32_t m_cachedSrvIndex[2];

	bool m_isUsedByEffect;
	bool m_textureLodEnabled;

protected:
	std::string m_resourcePath;
};

TYPEDEF_BLUECLASS( TriTextureParameter );
#endif
