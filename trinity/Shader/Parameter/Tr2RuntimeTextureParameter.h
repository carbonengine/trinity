////////////////////////////////////////////////////////////
//
//    Created:   September 2017
//    Copyright: CCP 2017
//
#pragma once

#include "include/ITriEffectParameter.h"

BLUE_DECLARE_INTERFACE( ITr2TextureProvider );

BLUE_CLASS( Tr2RuntimeTextureParameter ): public ITriEffectResourceParameter, public INotify
{
public:
	Tr2RuntimeTextureParameter( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool OnModified( Be::Var* value ) override;

	virtual bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL& resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const;
	virtual bool ApplyUav(
		Tr2ResourceSetDescriptionAL& resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const;
	virtual const char* GetParameterName() const;
	virtual void RebuildEffectHandles( Tr2Shader* effectRes );
	virtual unsigned GetHashValue( unsigned startingHash ) const;

	void Create( const BlueSharedString& name, ITr2TextureProvider* texture, uint32_t uavMipLevel = 0 );

	void SetTextureProvider( ITr2TextureProvider* texture );
	ITr2TextureProviderPtr GetTextureProvider() const;
	void SetUavMipLevel( uint32_t mipLevel );
private:
	void OnAddedToMaterial( Tr2Material* material ) override;
	void OnRemovedFromMaterial( Tr2Material* material ) override;

	BlueSharedString m_name;
	ITr2TextureProviderPtr m_texture;
	std::vector<Tr2Material*> m_materials;
	Tr2EffectResource::Type m_resourceType;
	uint32_t m_uavMipLevel;
};

TYPEDEF_BLUECLASS( Tr2RuntimeTextureParameter );