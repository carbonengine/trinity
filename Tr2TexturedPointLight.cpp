#include "StdAfx.h"
#include "Tr2TexturedPointLight.h"
#include "Resources/TriTextureRes.h"


Tr2TexturedPointLight::Tr2TexturedPointLight( IRoot* lockobj )
	:Tr2PointLight( lockobj )
{
	m_isDynamic = true;
}

bool Tr2TexturedPointLight::Initialize()
{
	if( !m_texturePath.empty() )
	{
		BeResMan->GetResource( m_texturePath, L"", m_texture );
	}
	return true;
}

void Tr2TexturedPointLight::SetTexturePath( std::wstring path )
{
	m_texturePath = path;
	m_texture = nullptr;
	BeResMan->GetResource( m_texturePath, L"", m_texture );
}

bool Tr2TexturedPointLight::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_texturePath ) )
	{
		SetTexturePath( m_texturePath );
	}
	return true;
}

void Tr2TexturedPointLight::UpdateLight()
{
	if( m_texture )
	{
		m_color = m_texture->GetAverageColor();
	}
}