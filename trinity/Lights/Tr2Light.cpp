////////////////////////////////////////////////////////////
//
//    Created:   March 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2Light.h"
#include "Include/TriMath.h"
#include "Resources/Tr2LightProfileRes.h"


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
	texturePath( L"" ),
	boneIndex( -1 ),
	flags( Tr2LightManager::FLAG_DEFAULT )
{
}


Tr2Light::Tr2Light( IRoot* lockobj ) :
	m_isDynamic( false ),
	m_type( UNDEFINED_LIGHT ),
	m_name( "" ),
	m_brightnessMultiplier( 1.f ),
	m_boneTransform( IdentityMatrix() )
{
	m_startTime = BeOS->GetCurrentFrameTime();
	m_lightData = LightData();
}

void Tr2Light::SetLightData( LightData& baseData )
{
	m_lightData = baseData;
}

void Tr2Light::SetBoneMatrix( const granny_matrix_3x4* bones, size_t boneCount ) 
{
	if( m_lightData.boneIndex >= 0 && m_lightData.boneIndex < boneCount )
	{
		Matrix m = IdentityMatrix();
		TriMatrixCopyFrom3x4( &m, &bones[m_lightData.boneIndex] );
		m_boneTransform = m;
	}
}


void Tr2Light::SetBrightnessMultiplier( float multi )
{
	m_brightnessMultiplier = multi;
}

void Tr2Light::ChangeLightColor( Color c )
{
	m_lightData.color = c;
}

void Tr2Light::AddLight( Tr2LightManager& lightManager, CXMMATRIX transform, float scale, const granny_matrix_3x4* bones, size_t boneCount )
{
	if( m_isDynamic )
	{
		this->Update();
	}

	if( !Tr2LightManager::AreLightFlagsValid( m_lightData.flags ) )
	{
		return;
	}

	SetBoneMatrix( bones, boneCount );
	XMMATRIX lightTransform = XMMatrixMultiply( m_boneTransform, transform );

	Tr2LightManager::PerLightData data;

	float brightness = m_lightData.brightness * m_brightnessMultiplier;
	if( m_lightData.noiseAmplitude != 0.f )
	{
		float noise = float( PerlinNoise1D( TimeAsDouble( BeOS->GetCurrentFrameTime() - m_startTime ) * m_lightData.noiseFrequency, 2.f, 2.f, m_lightData.noiseOctaves ) );
		brightness *= ( ( noise + 1.0f ) / 2.0f ) * m_lightData.noiseAmplitude;
	}
	data.color = ( Vector4( m_lightData.color ) * brightness ).GetXYZ();
	data.radius = m_lightData.radius * scale;
	data.innerRadius = Float_16( m_lightData.innerRadius * scale );
	int16_t profile = 0;
	if( m_lightProfile )
	{
		profile = m_lightProfile->GetTextureIndex() + 1;
	}
	data.flags = m_lightData.flags | ( profile << 4 );
	data.position = Vector3( XMVector3TransformCoord( m_lightData.position, lightTransform ) );

	Matrix lightRotation = RotationMatrix( m_lightData.rotation ) * lightTransform;
	data.direction = Vector3_16( Normalize( Transform( Vector4( 0.0, 0.0, -1, 0.0 ), lightRotation ).GetXYZ() ) );
	switch( m_type )
	{
	case SPOT_LIGHT:
		// rotate the direction
		data.outerAngle = Float_16( cos( TRI_2PI * m_lightData.outerAngle / 360.0f ) );
		data.innerAngle = Float_16( cos(TRI_2PI * m_lightData.innerAngle / 360.0f) );
		break;
	default:
		data.outerAngle = Float_16( 0.0f );
		data.innerAngle = Float_16( 0.0f );
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

bool Tr2Light::Initialize()
{
	Tr2LightProfileResPtr profile;
	if( !m_lightProfilePath.empty() )
	{
		BeResMan->GetResource( m_lightProfilePath, L"lp", profile );
	}
	m_lightProfile = profile;
	return true;
}

// INotify
bool Tr2Light::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_lightProfilePath ) )
	{
		Tr2LightProfileResPtr profile;
		if( !m_lightProfilePath.empty() )
		{
			BeResMan->GetResource( m_lightProfilePath, L"lp", profile );
		}
		m_lightProfile = profile;
	}
	return true;
}

const LightData& Tr2Light::GetLightData() const
{
	return m_lightData;
}

float Tr2Light::GetBrightnessMultiplier() const
{
	return m_brightnessMultiplier;
}

void Tr2Light::Update()
{

}

void Tr2Light::RenderDebugInfo( ITr2DebugRenderer2& renderer, const Matrix& worldMatrix, const granny_matrix_3x4* bones, size_t boneCount )
{

}
