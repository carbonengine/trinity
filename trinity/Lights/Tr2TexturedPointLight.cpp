#include "StdAfx.h"
#include "Tr2TexturedPointLight.h"
#include "Resources/TriTextureRes.h"


Tr2TexturedPointLight::Tr2TexturedPointLight( IRoot* lockobj ) :
	Tr2PointLight( lockobj )
{
	m_isDynamic = true;
	m_type = POINT_LIGHT;
}

bool Tr2TexturedPointLight::Initialize()
{
	if( !m_lightData.texturePath.empty() )
	{
		BeResMan->GetResource( m_lightData.texturePath, L"", m_texture );
	}
	return Tr2PointLight::Initialize();
}

void Tr2TexturedPointLight::SetLightData( LightData& data )
{
	Tr2PointLight::SetLightData( data );
	SetTexturePath( m_lightData.texturePath );
}

void Tr2TexturedPointLight::SetTexturePath( std::wstring path )
{
	m_texture = nullptr;
	BeResMan->GetResource( path, L"", m_texture );
}

bool Tr2TexturedPointLight::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_lightData.texturePath ) )
	{
		SetTexturePath( m_lightData.texturePath );
	}
	return Tr2PointLight::OnModified(value);
}

void Tr2TexturedPointLight::Update()
{
	if( m_texture )
	{
		m_lightData.color = m_texture->GetAverageColor();
	}
}