#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorProbeVolume.h"
#include "TriLineSet.h"
#include "Tr2InteriorCell.h"

// Define a workaround for crashes in Geo::VConstruct when it's not inlined (compiler error?)
#define GEO_VCONSTRUCT( x, y, z, w ) (_mm_setr_ps((x), (y), (z), (w)))
//#define GEO_VCONSTRUCT( x, y, z, w ) (Geo::VConstruct((x), (y), (z), (w)))

// ------------------------------------------------------------------------------------------------------
Tr2InteriorProbeVolume::Tr2InteriorProbeVolume( IRoot* lockobj ) :
	m_drawBoundingBox( false ),
	m_bDrawLightProbes( false ),
	m_sphereProbes( false ),
	m_validProbes( false ),
	m_drawProbeCulling( false ),
	m_parentCell( NULL )
{
	D3DXMatrixIdentity( &m_transform );
	m_lightProbeResX = 2;
	m_lightProbeResY = 2;
	m_lightProbeResZ = 2;

	m_volumeBox = GEO_NEW(GeoEngine::NonAAVolume);

	BuildLightVolume( m_lightProbeResX, m_lightProbeResY, m_lightProbeResZ);
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorProbeVolume::~Tr2InteriorProbeVolume()
{
	using namespace GeoEngine;
	GEO_DELETE(NonAAVolume, m_volumeBox);
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorProbeVolume::Initialize()
{
	BuildLightVolume( 
		m_lightProbeResX,
		m_lightProbeResY,
		m_lightProbeResZ );

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetParentCell( Tr2InteriorCell* cell )
{
	m_parentCell = cell;
}

// ------------------------------------------------------------------------------------------------------
const Matrix& Tr2InteriorProbeVolume::GetParentTransform() const
{
	return m_parentCell ? m_parentCell->GetWorldTransform() : Tr2Renderer::GetIdentityTransform();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorProbeVolume::GetLocalBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	minBounds = Vector3( -0.5f, -0.5f, -0.5f );
	maxBounds = Vector3( 0.5f, 0.5f, 0.5f );

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorProbeVolume::GetWorldBoundingBox( Vector3& min, Vector3& max ) const
{
	min = Vector3( -0.5f, -0.5f, -0.5f );
	max = Vector3( 0.5f, 0.5f, 0.5f );

	BoundingBoxTransform( min, max, m_transform );

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetTransform( const Matrix &transform )
{
	m_transform = transform;

	BuildLightVolume( 
		m_lightProbeResX,
		m_lightProbeResY,
		m_lightProbeResZ );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetPosition( const Vector3& pos )
{
	m_transform._41 = pos.x;
	m_transform._42 = pos.y;
	m_transform._43 = pos.z;

	BuildLightVolume( 
		m_lightProbeResX,
		m_lightProbeResY,
		m_lightProbeResZ );
}

// ------------------------------------------------------------------------------------------------------
const Quaternion Tr2InteriorProbeVolume::GetRotation( void ) const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpRotation;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetRotation( const Quaternion& rotQuat )
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	
	
	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &tmpScale, NULL, &rotQuat, &tmpTranslation );

	BuildLightVolume( 
		m_lightProbeResX,
		m_lightProbeResY,
		m_lightProbeResZ );
}

// ------------------------------------------------------------------------------------------------------
const Vector3 Tr2InteriorProbeVolume::GetScaling( void ) const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	
	
	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpScale;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetScaling( const Vector3& scaleVec )
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &scaleVec, NULL, &tmpRotation, &tmpTranslation );

	BuildLightVolume( 
		m_lightProbeResX,
		m_lightProbeResY,
		m_lightProbeResZ );
}

// ------------------------------------------------------------------------------------------------------
int Tr2InteriorProbeVolume::GetResolutionX() const
{
	return m_lightProbeResX;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetResolutionX(int resultionX)
{
	BuildLightVolume( 
		resultionX,
		m_lightProbeResY,
		m_lightProbeResZ );
}

// ------------------------------------------------------------------------------------------------------
int Tr2InteriorProbeVolume::GetResolutionY() const
{
	return m_lightProbeResY;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetResolutionY(int resolutionY)
{
	BuildLightVolume( 
		m_lightProbeResX,
		resolutionY,
		m_lightProbeResZ );
}

// ------------------------------------------------------------------------------------------------------
int Tr2InteriorProbeVolume::GetResolutionZ() const
{
	return m_lightProbeResZ;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetResolutionZ(int resolutionZ)
{
	BuildLightVolume( 
		m_lightProbeResX,
		m_lightProbeResY,
		resolutionZ );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::RenderDebugInfo( TriLineSetPtr lines ) const
{
	const Matrix& worldTransform = GetParentTransform();

	if( m_drawBoundingBox )
	{
		Vector3 min, max;
		GetLocalBoundingBox(min, max);

		Vector3 minA(max.x, min.y, min.z);
		Vector3 minB(min.x, max.y, min.z); 
		Vector3	minC(max.x, max.y, min.z); 
		Vector3	maxA(max.x, min.y, max.z); 
		Vector3	maxB(min.x, max.y, max.z); 
		Vector3	maxC(min.x, min.y, max.z);

		Matrix transform = m_transform * worldTransform;

		D3DXVec3TransformCoord( &minA, &minA, &transform );
		D3DXVec3TransformCoord( &minB, &minB, &transform );
		D3DXVec3TransformCoord( &minC, &minC, &transform );
		D3DXVec3TransformCoord( &maxA, &maxA, &transform );
		D3DXVec3TransformCoord( &maxB, &maxB, &transform );
		D3DXVec3TransformCoord( &maxC, &maxC, &transform );
		D3DXVec3TransformCoord( &min, &min, &transform );
		D3DXVec3TransformCoord( &max, &max, &transform );

		lines->Add( min, 0x800000ff, minA, 0x800000ff );
		lines->Add( min, 0x800000ff, minB, 0x800000ff );
		lines->Add( minC, 0x800000ff, minB, 0x800000ff );
		lines->Add( minA, 0x800000ff, minC, 0x800000ff );

		lines->Add( max, 0x800000ff, maxA, 0x800000ff );
		lines->Add( max, 0x800000ff, maxB, 0x800000ff );
		lines->Add( maxC, 0x800000ff, maxB, 0x800000ff );
		lines->Add( maxA, 0x800000ff, maxC, 0x800000ff );
		
		lines->Add( min, 0x800000ff, maxC, 0x800000ff );
		lines->Add( max, 0x800000ff, minC, 0x800000ff );
		lines->Add( minB, 0x800000ff, maxB, 0x800000ff );
		lines->Add( minA, 0x800000ff, maxA, 0x800000ff );
	}

	// light probe positions
	if( m_bDrawLightProbes )
	{
		// Draw boxes for probe positions
		for( unsigned int i = 0; i < m_lightProbes.size(); ++i )
		{
			Vector3 pos = m_lightProbes[i];
			D3DXVec3TransformCoord( &pos, &pos, &worldTransform );
			lines->AddBox( Vector3(pos.x - 0.05f, pos.y - 0.05f, pos.z - 0.05f), Vector3(pos.x + 0.05f, pos.y + 0.05f, pos.z + 0.05f), 0xffffffff );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::BuildLightVolume( int xRes, int yRes, int zRes )
{
	m_lightProbeResX = std::max( xRes, 2 );
	m_lightProbeResY = std::max( yRes, 2 );
	m_lightProbeResZ = std::max( zRes, 2 );

	m_lightProbes.clear();

	Vector3 axis = GetScaling();
	Geo::v128 s = GEO_VCONSTRUCT(axis.x, axis.y, axis.z, 0.0f);
	Vector3 position = Vector3( -0.5f, -0.5f, -0.5f );
	D3DXVec3TransformCoord( &position, &position, &m_transform );
	Geo::v128 pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 0.0f );


	Vector3 x( 1.0f, 0.0f, 0.0f );
	Vector3 y( 0.0f, 1.0f, 0.0f );
	Vector3 z( 0.0f, 0.0f, 1.0f );
	D3DXVec3TransformNormal( &x, &x, &m_transform );
	D3DXVec3TransformNormal( &y, &y, &m_transform );
	D3DXVec3TransformNormal( &z, &z, &m_transform );
	x /= axis.x;
	y /= axis.y;
	z /= axis.z;

	Geo::v128 mat[3];
	mat[0] = GEO_VCONSTRUCT( x.x, x.y, x.z, 0.0f );
	mat[1] = GEO_VCONSTRUCT( y.x, y.y, y.z, 0.0f );
	mat[2] = GEO_VCONSTRUCT( z.x, z.y, z.z, 0.0f );

	GeoEngine::NonAABoundingBox boundingBox;
	boundingBox.Initialise( mat, pos, s );

	m_volumeBox->Initialise( &boundingBox, m_lightProbeResX, m_lightProbeResY, m_lightProbeResZ );

	for( int z = 0; z < m_lightProbeResZ; ++z )
	{
		for( int y = 0; y < m_lightProbeResY; ++y )
		{
			for( int x = 0; x < m_lightProbeResX; ++x )
			{
				Vector3 sample( 
					(float( x ) + 0.5f) / float( m_lightProbeResX ) - 0.5f,
					(float( y ) + 0.5f) / float( m_lightProbeResY ) - 0.5f,
					(float( z ) + 0.5f) / float( m_lightProbeResZ ) - 0.5f );
				D3DXVec3TransformCoord( &sample, &sample, &m_transform );
				m_lightProbes.push_back( sample );
			}
		}
	}

	m_validProbes = false;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetSphereProbeVisualization( bool visualize )
{
	m_sphereProbes = visualize;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorProbeVolume::GetSphereProbeVisualization() const
{
	return m_sphereProbes;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::SetValidProbes( bool validProbes )
{
	m_validProbes = validProbes;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorProbeVolume::HasValidProbes() const
{
	return m_validProbes;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorProbeVolume::GetProbePositions ( const Vector3& position, Vector3 *probes, float *weights ) const
{
	// Find the closest sample		
	Geo::s32 voxIDs[GeoEngine::VOL_INTERP_TRI];
	Geo::v128 pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 1.0f );
	if( !m_volumeBox->GetNearestSamples( voxIDs, weights, pos, GeoEngine::VOL_INTERP_TRI ) )
	{
		return;
	}

	for (int i = 0; i < GeoEngine::VOL_INTERP_TRI; ++i)
	{
		probes[i] = m_lightProbes[voxIDs[i]];
	}
}

// ------------------------------------------------------------------------------------------------------
float Tr2InteriorProbeVolume::GetMixWeight( const Vector3& position ) const
{
	Geo::v128 pos;
	pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 1.0f );
	Geo::v128 localPos = m_volumeBox->MapToLocalUnitCube( pos );
	Vector3 localPosition( Geo::VGetX(localPos), Geo::VGetY(localPos), Geo::VGetZ(localPos) );

	float mixWeight = 0.0f;

	if (localPosition.x < 0.0f)
	{
		mixWeight = std::max( mixWeight, -localPosition.x );
	}
	else if (localPosition.x > 1.0f)
	{
		mixWeight = std::max( mixWeight, localPosition.x - 1 );
	}
	if (localPosition.y < 0.0f)
	{
		mixWeight = std::max( mixWeight, -localPosition.y );
	}
	else if (localPosition.y > 1.0f)
	{
		mixWeight = std::max( mixWeight, localPosition.y - 1 );
	}
	if (localPosition.z < 0.0f)
	{
		mixWeight = std::max( mixWeight, -localPosition.z );
	}
	else if (localPosition.z > 1.0f)
	{
		mixWeight = std::max( mixWeight, localPosition.z - 1 );
	}

	mixWeight *= std::max( std::max( m_lightProbeResX, m_lightProbeResY ), m_lightProbeResZ );

	mixWeight = 1.0f - std::min( mixWeight, 1.0f );

	return mixWeight;
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the object in its
//   local coordinate space.
// Return value:
//   AABB for the object in its local coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorProbeVolume::GetBoundingBoxInLocalSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetLocalBoundingBox( result.m_min, result.m_max );
	return result;
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns AABB for the object in the
//   world coordinate space.
// Return value:
//   AABB for the object in the world coordinate space
// -------------------------------------------------------------
AxisAlignedBoundingBox Tr2InteriorProbeVolume::GetBoundingBoxInWorldSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.f, 0.f, 0.f ), Vector3( 0.f, 0.f, 0.f ) );
	GetWorldBoundingBox( result.m_min, result.m_max );
	return result;
}

#endif
