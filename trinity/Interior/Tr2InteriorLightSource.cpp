// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2InteriorLightSource.h"
#include "Tr2InteriorConstantBufferFormats.h"
#include "TriDebugResourceHelper.h"

#include "Utilities/BoundingSphere.h"
#include "Tr2AtlasTexture.h"
#include "Tr2KelvinColor.h"
#include "Curves/TriCurveSet.h"
#include "Include/TriMath.h"
#include "Shader/Tr2Effect.h"
#include "TriViewport.h"

BLUE_DEFINE_INTERFACE( ITr2InteriorLight );

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource default constructor
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::Tr2InteriorLightSource( IRoot* lockobj ) :
	PARENTLOCK( m_curveSets ),
	m_name(),
	m_position( 0.f, 0.f, 0.f ),
	m_radius( 1.f ),
	m_color( 1.f, 1.f, 1.f, 1.f ),
	m_falloff( 1.f ),
	m_specularIntensity( 1.f ),
	m_coneAlphaOuter( 180.f ),
	m_coneAlphaInner( 180.f ),
	m_coneDirection( 0.f, -1.f, 0.f ),
	m_primaryLighting( true ),
	m_worldBoundingBox( Vector3( -1.f, -1.f, -1.f ), Vector3( 1.f, 1.f, 1.f ) ),
	m_useKelvinColor( false ),
	m_unitToWorldTransform( IdentityMatrix() ),
	m_viewProjection( IdentityMatrix() )
{
	m_kelvinColor.CreateInstance();
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2InteriorLightSource destructor.
// --------------------------------------------------------------------------------------
Tr2InteriorLightSource::~Tr2InteriorLightSource()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from IInitialize interface.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::Initialize()
{
	m_worldBoundingBox = AxisAlignedBoundingBox( m_position - Vector3( m_radius, m_radius, m_radius ), m_position + Vector3( m_radius, m_radius, m_radius ) );
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Inherited from INotify interface.  Allows the light source to respond to parameter
//   changes generated in Python.  If the light position changes, the regions of influence
//   are updated with the new transform matrix & the light is flagged as 'dirty', forcing
//   a new round of light-cell intersection tests on the next scene Update. The light is
//   flagged as dirty.
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2InteriorLightSource::OnModified( Be::Var* value )
{
	// Update the regions of influce if the position changes
	if( IsMatch( value, m_position ) )
	{
		m_worldBoundingBox = AxisAlignedBoundingBox( m_position - Vector3( m_radius, m_radius, m_radius ), m_position + Vector3( m_radius, m_radius, m_radius ) );
	}
	else if( IsMatch( value, m_radius ) ||
			 IsMatch( value, m_coneAlphaOuter ) ||
			 IsMatch( value, m_coneDirection ) )
	{
		m_worldBoundingBox = AxisAlignedBoundingBox( m_position - Vector3( m_radius, m_radius, m_radius ), m_position + Vector3( m_radius, m_radius, m_radius ) );
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Copies the lighting parameters into the per-object data.
// Arguments:
//   lightData - The per-object light data to populate
// --------------------------------------------------------------------------------------
void Tr2InteriorLightSource::PopulateLightData( Tr2InteriorPerObjectLightData* lightData ) const
{
	// just put this in struct
	lightData->position = m_position;
	lightData->radius = std::max( m_radius, 0.0f ); // when radius<0 the light is treated as box light in forward rendering
	if( m_useKelvinColor )
	{
		Color k = m_kelvinColor->AsRGB();
		lightData->color = Vector3( k.r, k.g, k.b );
	}
	else
	{
		lightData->color = Vector3( m_color.r, m_color.g, m_color.b );
	}

	lightData->color = TriGammaToLinear( lightData->color );

	lightData->pointLightFalloff = m_falloff;
	lightData->shadow0Influence = 0.f;
	lightData->shadow1Influence = 0.f;

	// Spot light values (always populate them, they are basicly pointlights with
	// some non-default values...).  First apply some limits
	float innerAngle = m_coneAlphaInner;
	float outerAngle = m_coneAlphaOuter;
	if( innerAngle + 1.f > outerAngle )
	{
		innerAngle = outerAngle - 1.f;
	}
	// if this is not a spotlight, force full 360degrees sphere
	if( !IsSpotLight() )
	{
		outerAngle = innerAngle = 360.f;
	}

	lightData->coneCosAlphaOuter = cosf( XMConvertToRadians( outerAngle ) );
	lightData->coneCosAlphaInner = cosf( XMConvertToRadians( innerAngle ) );
	lightData->spotDirection = XMVector3Normalize( m_coneDirection );
}

// -------------------------------------------------------------
// Description:
//   Per-frame update method. Updates curve sets.
// Arguments:
//   time - Current system time.
// -------------------------------------------------------------
void Tr2InteriorLightSource::Update( Be::Time time )
{
	for( auto it = m_curveSets.cbegin(); it != m_curveSets.cend(); ++it )
	{
		( *it )->Update( TimeAsDouble( time ) );
	}
}

bool Tr2InteriorLightSource::IsInFrustum( const TriFrustum& frustum, Matrix& objectToWorld ) const
{
	if( !m_primaryLighting )
	{
		return false;
	}
	objectToWorld = TranslationMatrix( m_position );
	return frustum.IsBoxVisible( m_worldBoundingBox.m_min, m_worldBoundingBox.m_max );
}

void Tr2InteriorLightSource::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Lights" );
}

void Tr2InteriorLightSource::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( renderer.HasOption( GetRawRoot(), "Lights" ) )
	{
		renderer.DrawSphere( this, m_position, 0.05f, 10, Tr2DebugRenderer::Wireframe, 0xff333333 );
		if( IsSpotLight() )
		{
			float coneRadius = m_radius * tanf( XMConvertToRadians( m_coneAlphaOuter ) );
			Vector3 focal = m_position + Normalize( m_coneDirection ) * m_radius;
			renderer.DrawCone( this, focal, m_position, coneRadius, 8, Tr2DebugRenderer::Wireframe, 0xff444444 );
		}
		else
		{
			renderer.DrawSphere( this, m_position, m_radius, 12, Tr2DebugRenderer::Wireframe, 0xff444444 );
		}
	}
}
