#pragma once

#include "Tr2PointLight.h"


BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2TexturedPointLight ): 
	public Tr2PointLight, 
	public IInitialize, 
	public INotify
{
public:
	Tr2TexturedPointLight( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool Initialize() override;
	bool OnModified( Be::Var* value ) override;

	void SetTexturePath( std::wstring path );
protected:
	void UpdateLight() override;
private:
	std::wstring m_texturePath;
	TriTextureResPtr m_texture;
};

TYPEDEF_BLUECLASS( Tr2TexturedPointLight );