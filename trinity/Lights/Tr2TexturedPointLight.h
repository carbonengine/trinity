// Copyright © 2023 CCP ehf.

#pragma once

#include "Tr2PointLight.h"


BLUE_DECLARE( TriTextureRes );

BLUE_CLASS( Tr2TexturedPointLight ) :
	public Tr2PointLight
{
public:
	Tr2TexturedPointLight( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool Initialize() override;
	bool OnModified( Be::Var * value ) override;
	void SetLightData( LightData & data ) override;
	void SetSaturation( float saturation );

protected:
	void Update() override;

private:
	void SetTexturePath( std::wstring path );
	TriTextureResPtr m_texture;
	float m_saturation;
};

TYPEDEF_BLUECLASS( Tr2TexturedPointLight );