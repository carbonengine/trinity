////////////////////////////////////////////////////////////
//
//    Created:   March 2023
//    Copyright: CCP 2023
//

#pragma once

#include "include/ITriEffectParameter.h"

BLUE_DECLARE( Tr2TextureAnimation );


BLUE_CLASS( Tr2TextureAnimationParameter ) :
	public ITriEffectResourceParameter, public INotify
{
public:
	Tr2TextureAnimationParameter( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool OnModified( Be::Var * value ) override;

	bool CopyToResourceSet(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex,
		ResourceFlags flags ) const override;
	bool ApplyUav(
		Tr2ResourceSetDescriptionAL & resourceDesc,
		Tr2RenderContextEnum::ShaderType stage,
		uint32_t registerIndex ) const override;
	const char* GetParameterName() const override;
	void RebuildEffectHandles( Tr2Shader * effectRes ) override;
	unsigned GetHashValue( unsigned startingHash ) const override;

	Tr2TextureAL GetTexture() const;

private:
	void OnAddedToMaterial( Tr2Material * material ) override;
	void OnRemovedFromMaterial( Tr2Material * material ) override;

	BlueSharedString m_name;
	BlueSharedString m_channel;
	Tr2TextureAnimationPtr m_animation;
	std::vector<Tr2Material*> m_materials;
	Tr2EffectResource::Type m_resourceType;
};

TYPEDEF_BLUECLASS( Tr2TextureAnimationParameter );
