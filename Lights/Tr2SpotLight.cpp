////////////////////////////////////////////////////////////
//
//    Created:   February 2019
//    Copyright: CCP 2019
//

#include "StdAfx.h"
#include "Tr2SpotLight.h"
#include "Tr2DebugRenderer.h"

#include "Tr2LightManager.h"
#include "Include/TriMath.h"

Tr2SpotLight::Tr2SpotLight( IRoot* lockobj ):
	Tr2Light( lockobj )
{
	m_type = SPOT_LIGHT;
}

void Tr2SpotLight::RenderDebugInfo( Tr2DebugRenderer& renderer, const Matrix& worldMatrix )
{
	auto baseColor = m_lightData.color * m_lightData.brightness;
	baseColor.a = 0.1;
	auto selectedColor = baseColor + Color( 0.0, 0.0, 0.0, 0.1 );

	Vector3 end = Transform( Vector3( 0.0, 0.0, m_lightData.radius ), RotationMatrix( m_lightData.rotation ) ).GetXYZ();
	end = Transform( end + m_lightData.position, worldMatrix ).GetXYZ();
	Vector3 start = Transform( m_lightData.position, worldMatrix );
	float outerConeRadius = tan( TRI_2PI * m_lightData.outerAngle / 360.f ) * m_lightData.radius;
	float innerConeRadius = tan( TRI_2PI * m_lightData.innerAngle / 360.f ) * m_lightData.radius;
	renderer.DrawCone( this, end, start, outerConeRadius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
	renderer.DrawCone( this, end, start, innerConeRadius, 10, Tr2DebugRenderer::Solid, Tr2DebugColor( selectedColor, baseColor ) );
}