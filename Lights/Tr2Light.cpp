////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2Light.h"
#include "Include/TriMath.h"

LightData::LightData() :
	position( 0, 0, 0 ),
	color( 0, 0, 0, 1 ),
	brightness( 1.0f ),
	noiseAmplitude( 0.0f ),
	noiseFrequency( 1.0f ),
	noiseOctaves( 1 ),
	radius( 0.0f ),
	innerRadius( 0.0f ),
	rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	innerAngle( 0.0f ),
	outerAngle( 0.0f ),
	texturePath( L"" )
{
}


Tr2Light::Tr2Light( IRoot* lockobj ) :
	m_isDynamic( false ),
	m_type( UNDEFINED_LIGHT ),
	m_name( "" ),
	m_brightnessMultiplier( 1.f )
{
	m_startTime = BeOS->GetCurrentFrameTime();
	m_lightData = LightData();
}

void Tr2Light::SetLightData( LightData& baseData )
{
	m_lightData = baseData;
}

void Tr2Light::SetBrightnessMultiplier( float multi )
{
	m_brightnessMultiplier = multi;
}

void Tr2Light::ChangeLightColor( Color c )
{
	m_lightData.color = c;
}

void Tr2Light::AddLight( Tr2LightManager& lightManager, CXMMATRIX transform, float scale )
{
	if( m_isDynamic )
	{
		this->Update();
	}
	
	Tr2LightManager::PerLightData data;

	float brightness = m_lightData.brightness * m_brightnessMultiplier;
	if( m_lightData.noiseAmplitude != 0.f )
	{
		float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() - m_startTime ) * m_lightData.noiseFrequency, 2.f, 2.f, m_lightData.noiseOctaves ) );
		brightness *= ( ( noise + 1.0f ) / 2.0f ) * m_lightData.noiseAmplitude;
	}
	data.color = ( Vector4( m_lightData.color ) * brightness ).GetXYZ();
	data.radius = m_lightData.radius * scale;
	data.innerRadius = m_lightData.innerRadius * scale;
	data.position = Vector3( XMVector3TransformCoord( m_lightData.position, transform ) );
	float outerAngle = 2.0f + cos(TRI_2PI * m_lightData.outerAngle / 360.0f); // we do this so we always have a direction, if we have a spotlight

	switch( m_type )
	{
	case SPOT_LIGHT:
		// rotate the direction
		data.direction = Transform( Vector4( 0.0, 0.0, -outerAngle, 0.0 ), RotationMatrix( m_lightData.rotation ) * transform ).GetXYZ();
		data.innerAngle = cos(TRI_2PI * m_lightData.innerAngle / 360.0f);
		break;
	default:
		data.direction = Vector3( 0.f, 0.f, 0.f );
		data.innerAngle = 0.0f;
		break;
	}

	lightManager.AddLight( data );
}
	

void Tr2Light::GetLight( Vector3& position, float& radius, Color& color )
{
	position = m_lightData.position;
	radius = m_lightData.radius;
	float brightness = m_lightData.brightness;
	if( m_lightData.noiseAmplitude != 0.f )
	{
		float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() ) * m_lightData.noiseFrequency, 2.f, 2.f, m_lightData.noiseOctaves ) );
		brightness *= ( ( noise + 1.0f ) / 2.0f ) * m_lightData.noiseAmplitude;
	}
	color = m_lightData.color * brightness;
}


// INotify
bool Tr2Light::OnModified( Be::Var* value )
{
	return true;
}

void Tr2Light::Update()
{

}

void Tr2Light::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix )
{

}
