////////////////////////////////////////////////////////////
//
//    Created:   May 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorPortalSocket.h"
#include "Tr2InteriorPhysicalPortal.h"
#include "Tr2InteriorCell.h"
#include "Resources/TriGeometryRes.h"
#include "TriLineSet.h"

Tr2InteriorPortalSocket::Tr2InteriorPortalSocket( IRoot* lockobj )
:	m_name("PortalSocket"),
	m_portal( NULL ),
	m_cell( NULL ),
	m_minBounds( -1.0f, -1.0f, -1.0f ),
	m_maxBounds( 1.0f, 1.0f, 1.0f ),
	m_scale( 1.0f ),
	m_normalCell( 0.0f, 0.0f, 0.0f ),
	m_normalLocal( 0.0f, 0.0f, 0.0f ),
	m_instanceInSystemIdx( -1 ),
	m_validProbe( false ),
	m_probeOffset( 0.8f ),
	m_emissiveColor( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_geometryGuid( 0 ),
	m_texturePosition( 0.f, 0.f )
{
	D3DXMatrixIdentity( &m_transform );
}

Tr2InteriorPortalSocket::~Tr2InteriorPortalSocket()
{
}

bool Tr2InteriorPortalSocket::Initialize()
{
	m_normalCell = m_normalLocal = Vector3( 0.0f, 0.0f, 0.0f );
	SetHasValidProbe( false );
	return true;
}

bool Tr2InteriorPortalSocket::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_minBounds ) || 
		IsMatch( value, m_maxBounds ) || 
		IsMatch( value, m_transform ) )
	{
		m_normalCell = m_normalLocal = Vector3( 0.0f, 0.0f, 0.0f );
		SetHasValidProbe( false );
	}
	if( IsMatch( value, m_probeOffset ) )
	{
		m_validProbe = false;
	}

	return true;
}

// -------------------------------------------------------------
// Description:
//   Returns parent cell world transform matrix or identity
//   matrix if there is no parent cell.
// Return Value:
//   Parent cell transform matrix
// -------------------------------------------------------------
const Matrix& Tr2InteriorPortalSocket::GetParentTransform() const
{
	return m_cell ? m_cell->GetWorldTransform() : Tr2Renderer::GetIdentityTransform();
}

// -------------------------------------------------------------
// Description:
//   Set L2 SH coefficients for lighting coming into the portal socket.
//   It is called by containing cell after it calculates SH coeficients
//   for all portal socket probes.
//   The function passes its arguments to the containing portal to be
//   transfered to the opposite portal socket.
// Arguments:
//   redMat, greenMat, blueMat - L2 SH coeficients for lighting
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetInputColor( const Matrix& redMat, const Matrix& greenMat, const Matrix& blueMat )
{
	if( m_portal )
	{
		m_portal->SetOutputSocketColor( this, redMat, greenMat, blueMat );
	}
}

// -------------------------------------------------------------
// Description:
//   Calculates SH probe position for the portal socket. The probe
//   position is offseted from socket's center along its "normal".
//   This function is called by the cell when rebuilding Enlighten.
// Arguments:
//   position (out) - position of the SH probe in parent cell space
// Return Value:
//   true If the socket needs SH probe to be computed
//   false If the socket doesn't need SH probe to be computed
// -------------------------------------------------------------
bool Tr2InteriorPortalSocket::GetInputProbePosition( Geo::v128& position )
{
	Vector3 minBounds, maxBounds;
	GetLocalBoundingBox( minBounds, maxBounds );
	Vector3 center = ( minBounds + maxBounds ) / 2.0f;
	D3DXVec3TransformCoord( &center, &center, &GetTransform() );
	center += GetNormalCell() * m_probeOffset;
	position = Geo::VConstruct( center.x, center.y, center.z, 1.0f );

	return true;
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// -------------------------------------------------------------
// Description:
//   Creates packed Enlighten geometry (a plane rectangle) to act
//   as emissive geomtry to output lighting from the portal.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   outputPixelSize - the size of pixel in parent cell space
// Return Value:
//   Packed geometry object or NULL if the socket doesn't use 
//   emissive geometry.
// -------------------------------------------------------------
Enlighten::IPrecompPackedGeometry* Tr2InteriorPortalSocket::GetEnlightenPackedGeometry( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog, float outputPixelSize )
{
	if( m_packedGeometry )
	{
		return m_packedGeometry.GetPtr();
	}

	Vector3 minBounds, maxBounds;
	GetLocalBoundingBox( minBounds, maxBounds );

	Enlighten::IPrecompInputMesh* precomputeMesh = Enlighten::IPrecompInputMesh::Create();
	Enlighten::PrecompMeshProperties precomputeMeshProperties;
	precomputeMeshProperties.m_IsDirectLightingMesh = true;		// we can illuminate it
	precomputeMeshProperties.m_IsIndirectLightingMesh = true;	// it receives radiosity
	precomputeMeshProperties.m_IsTargetMesh = true;				// it doesn't do any mesh simplification (from the sample)

	Enlighten::PrecompInputVertex verts[4];
	Vector3 pos;
	if( abs( GetNormalLocal().z ) > 0.0f )
	{
		float side = GetNormalLocal().z < 0.0f ? maxBounds.z : minBounds.z;
		pos = Vector3( minBounds.x, minBounds.y, side );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[0].m_Position ), &pos, &GetTransform() );
		pos = Vector3( minBounds.x, maxBounds.y, side );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[1].m_Position ), &pos, &GetTransform() );
		pos = Vector3( maxBounds.x, maxBounds.y, side );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[2].m_Position ), &pos, &GetTransform() );
		pos = Vector3( maxBounds.x, minBounds.y, side );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[3].m_Position ), &pos, &GetTransform() );
	}
	else if( abs( GetNormalLocal().y ) > 0.0f )
	{
		float side = GetNormalLocal().y < 0.0f ? maxBounds.y : minBounds.y;
		pos = Vector3( minBounds.x, side, minBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[0].m_Position ), &pos, &GetTransform() );
		pos = Vector3( minBounds.x, side, maxBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[1].m_Position ), &pos, &GetTransform() );
		pos = Vector3( maxBounds.x, side, maxBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[2].m_Position ), &pos, &GetTransform() );
		pos = Vector3( maxBounds.x, side, minBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[3].m_Position ), &pos, &GetTransform() );
	}
	else
	{
		float side = GetNormalLocal().x < 0.0f ? maxBounds.x : minBounds.x;
		pos = Vector3( side, minBounds.y, minBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[0].m_Position ), &pos, &GetTransform() );
		pos = Vector3( side, minBounds.y, maxBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[1].m_Position ), &pos, &GetTransform() );
		pos = Vector3( side, maxBounds.y, maxBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[2].m_Position ), &pos, &GetTransform() );
		pos = Vector3( side, maxBounds.y, minBounds.z );
		D3DXVec3TransformCoord( reinterpret_cast<Vector3*>( &verts[3].m_Position ), &pos, &GetTransform() );
	}

	verts[0].m_Normal = reinterpret_cast<const Geo::GeoVector3&>( GetNormalCell() );
	verts[1].m_Normal = verts[0].m_Normal;
	verts[2].m_Normal = verts[0].m_Normal;
	verts[3].m_Normal = verts[0].m_Normal;

	verts[0].m_ChartUV = verts[0].m_AlbedoUV = Geo::GeoPoint2( 0.0f, 0.0f );
	verts[1].m_ChartUV = verts[1].m_AlbedoUV = Geo::GeoPoint2( 0.0f, 0.0f );
	verts[2].m_ChartUV = verts[2].m_AlbedoUV = Geo::GeoPoint2( 0.0f, 0.0f );
	verts[3].m_ChartUV = verts[3].m_AlbedoUV = Geo::GeoPoint2( 0.0f, 0.0f );

	precomputeMesh->AddVertices( verts, verts + 4 );

	Enlighten::PrecompInputFace faces[2];
	Vector3 sideA = reinterpret_cast<Vector3&>( verts[1].m_Position ) - reinterpret_cast<Vector3&>( verts[0].m_Position );
	Vector3 sideB = reinterpret_cast<Vector3&>( verts[2].m_Position ) - reinterpret_cast<Vector3&>( verts[0].m_Position );
	Vector3 cross;
	if( D3DXVec3Dot( D3DXVec3Cross( &cross, &sideA, &sideB ), &GetNormalCell() ) > 0 )
	{
		faces[0].m_Indices[0] =  0;
		faces[0].m_Indices[1] =  1;
		faces[0].m_Indices[2] =  2;
		faces[1].m_Indices[0] =  0;
		faces[1].m_Indices[1] =  2;
		faces[1].m_Indices[2] =  3;
	}
	else
	{
		faces[0].m_Indices[0] =  0;
		faces[0].m_Indices[1] =  2;
		faces[0].m_Indices[2] =  1;
		faces[1].m_Indices[0] =  0;
		faces[1].m_Indices[1] =  3;
		faces[1].m_Indices[2] =  2;
	}
	faces[0].m_AlbedoId = faces[1].m_AlbedoId = 0;

	precomputeMesh->AddFaces( faces, faces + 2 );

	Enlighten::IPrecompInputGeometry* geom = NULL;

	if( m_geometryGuid == 0 )
	{
		// Assign guid if it was not already read/assigned from python
		m_geometryGuid = TriGeometryRes::s_currentEnlightenGeometryGuid++;
	}

	geom = Enlighten::IPrecompInputGeometry::Create( Geo::GeoGuid::Create( 1, m_geometryGuid ) );
	geom->AddMesh( precomputeMesh, &precomputeMeshProperties );

	const float enlightenRadiosityPerPixelSurfaceArea = pow(outputPixelSize,2.0f);
	geom->SetRadiosityPerPixelSurfaceArea( enlightenRadiosityPerPixelSurfaceArea );

	Enlighten::IPrecompPackedGeometry* packedGeometry = NULL;
	CCP_LOG("Packing Portal Socket Geometry.");
	Geo::s32 packResult = pPrecompute->PackGeometry( geom, &prog, packedGeometry );

	if( !packedGeometry || (packResult != 0) )
	{
		if( packResult == -1 )
		{
			CCP_LOGERR( "PackGeometry failed on portal socket - Invalid mesh input" );
		}
		else if( packResult == -2 )
		{
			CCP_LOGERR( "PackGeometry failed on portal socket - Invalid mesh properties" );
		}
		else if( packResult == -3 )
		{
			CCP_LOGERR( "PackGeometry failed on portal socket - Precompute failure" );
		}
		else if( packResult == -4 )
		{
			CCP_LOGERR( "PackGeometry failed on portal socket - Build empty" );
		}

		return NULL;
	}

	m_packedGeometry.Delete();
	m_packedGeometry = Geo::GeoAutoReleasePtr<Enlighten::IPrecompPackedGeometry>( packedGeometry );
	return packedGeometry;
}
#endif

// -------------------------------------------------------------
// Description:
//   Sets an index of socket's packed geometry in Enlighten system
//   during Enlighten build.
//   The index is used later to assign emissive color to the geometry.
// Arguments:
//   index - index of socket's packed geometry in Enlighten system
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetInstanceInSystemIdx( unsigned int index )
{
	m_instanceInSystemIdx = index;
}

// -------------------------------------------------------------
// Description:
//   Returns cached index of socket's packed geometry in Enlighten system.
//   The index is set with SetInstanceInSystemIdx method during Enlighten
//   build.
// Return Value:
//   Index of socket's packed geometry in Enlighten system
// -------------------------------------------------------------
unsigned int Tr2InteriorPortalSocket::GetInstanceInSystemIdx() const
{
	return m_instanceInSystemIdx;
}

// -------------------------------------------------------------
// Description:
//   Returns socket's emissive geometry color.
// Return Value:
//   Color of socket's emissive geometry
// -------------------------------------------------------------
const Color& Tr2InteriorPortalSocket::GetEmissiveColor()
{
	return m_emissiveColor;
}

// -------------------------------------------------------------
// Description:
//   Assigns a position of pixel in Enlighten albedo texture that
//   represents albedo/emissive color for this portal socket.
//   This function is called during Enlighten precompute.
// Arguments:
//   position - Enlighten albedo texture pixel position
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetTexturePixelPosition( const Vector2& position )
{
	m_texturePosition = position;
}

// -------------------------------------------------------------
// Description:
//   Returns a position of pixel in Enlighten albedo texture that
//   represents albedo/emissive color for this portal socket.
// Return Value:
//   Enlighten albedo texture pixel position
// -------------------------------------------------------------
const Vector2& Tr2InteriorPortalSocket::GetTexturePixelPosition() const
{
	return m_texturePosition;
}

// -------------------------------------------------------------
// Description:
//   Stores a pointer to the containing cell. Is called by 
//   Tr2InteriorCell when portal socket is added or removed
//   from the list of sockets.
// Arguments:
//   cell - Cell that contains this socket (or NULL when socket
//          is removed from cell portalSockets list)
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::AddToCell( Tr2InteriorCell* cell )
{
	SetHasValidProbe( false );
	m_cell = cell;
}

// -------------------------------------------------------------
// Description:
//   Stores a pointer to the portal that links this socket. Is 
//   called by Tr2InteriorPhysicalSocket when this socket is 
//   assigned to it. 
// Arguments:
//   portal - Portal that links this socket
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::AddToPortal( Tr2InteriorPhysicalPortal* portal )
{
	m_portal = portal;
}

// -------------------------------------------------------------
// Description:
//   Returns cell that contains this socket.
// Return Value:
//   Cell that contains this socket
// -------------------------------------------------------------
Tr2InteriorCell* Tr2InteriorPortalSocket::GetCell() const
{
	return m_cell;
}

// -------------------------------------------------------------
// Description:
//   Returns transformation matrix from socket's local
//   to parent cell coordinate system 
// Return Value:
//   Transformation matrix from local to parent cell CS
// -------------------------------------------------------------
const Matrix& Tr2InteriorPortalSocket::GetTransform() const
{
	return m_transform;
}

// -------------------------------------------------------------
// Description:
//   Sets transformation matrix from socket's local
//   to parent cell coordinate system and ivalidate cached packed
//   geometry, normal vectors and valid SH probe flag.
// Arguments:
//   transform - Transformation matrix from local to parent cell CS
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetTransform( const Matrix& transform )
{
	m_transform = transform;
	m_normalCell = m_normalLocal = Vector3( 0.0f, 0.0f, 0.0f );
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	m_packedGeometry.Delete();
#endif
	SetHasValidProbe( false );
}

// -------------------------------------------------------------
// Description:
//   Returns transformation matrix from socket's local
//   to world coordinate system 
// Return Value:
//   Transformation matrix from local to world CS
// -------------------------------------------------------------
Matrix Tr2InteriorPortalSocket::GetWorldTransform() const
{
	if( m_cell )
	{
		return m_transform * m_cell->GetWorldTransform();
	}
	else
	{
		return m_transform;
	}
}

// -------------------------------------------------------------
// Description:
//   Returns bounding box min and max corner points in socket's
//   local coordinate system.
// Arguments:
//   minBounds (out) - Coordinates of socket's min bounding box corner in local CS
//   maxBounds (out) - Coordinates of socket's max bounding box corner in local CS
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::GetLocalBoundingBox( Vector3& minBounds, Vector3& maxBounds )
{
	minBounds = m_minBounds;
	maxBounds = m_maxBounds;
}

// -------------------------------------------------------------
// Description:
//   Assignes new bounding box for the socket.
// Arguments:
//   minBounds - Coordinates of socket's min bounding box corner in local CS
//   maxBounds - Coordinates of socket's max bounding box corner in local CS
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetLocalBoundingBox( const Vector3& minBounds, const Vector3& maxBounds )
{
	m_minBounds = minBounds;
	m_maxBounds = maxBounds;
	m_normalCell = m_normalLocal = Vector3( 0.0f, 0.0f, 0.0f );
	SetHasValidProbe( false );
}

// -------------------------------------------------------------
// Description:
//   Assigns color to emissive geometry or light source based on
//   provided SH coefficients. Is called be the portal with
//   parameters coming from the opposite portal socket.
// Arguments:
//   redMat, greenMat, blueMat - L2 SH coeficients for lighting
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetOutputColor( const Matrix& redMat, const Matrix& greenMat, const Matrix& blueMat )
{
	Vector4 normal( -GetNormalCell(), 1.0f );

	Vector4 tmp;

	m_emissiveColor.r = std::max( D3DXVec4Dot( &normal, D3DXVec4Transform( &tmp, &normal, &redMat ) ) * m_scale / D3DX_PI, 0.0f );
	m_emissiveColor.g = std::max( D3DXVec4Dot( &normal, D3DXVec4Transform( &tmp, &normal, &greenMat ) ) * m_scale / D3DX_PI, 0.0f );
	m_emissiveColor.b = std::max( D3DXVec4Dot( &normal, D3DXVec4Transform( &tmp, &normal, &blueMat ) ) * m_scale / D3DX_PI, 0.0f );
	m_emissiveColor.a = 1.0f;
}

// -------------------------------------------------------------
// Description:
//   Modifies valid SH probe flag. The probe becomes valid after
//   SH probe volumes are rebuilt by the cell. Any manipulation
//   on socket's transform or bounding box invalidates this flag.
// Arguments:
//   validProbe - True iff the probe is valid
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::SetHasValidProbe( bool validProbe )
{
	m_validProbe = validProbe;
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	if( !m_validProbe )
	{
		m_packedGeometry.Delete();
	}
#endif
}

// -------------------------------------------------------------
// Description:
//   Returns the flag indicating if the socket's SH probe is valid.
// Return Value:
//   true If the socket has valid SH probe
//   false If the socket does not have valid SH probe
// -------------------------------------------------------------
bool Tr2InteriorPortalSocket::HasValidProbe() const
{
	return m_validProbe;
}

// -------------------------------------------------------------
// Description:
//   Renders socket's bounding box, SH probe and surface/light position for debugging.
// -------------------------------------------------------------
void Tr2InteriorPortalSocket::RenderDebugInfo( TriLineSetPtr lines )
{
	const Matrix& parentTransform = m_cell ? m_cell->GetWorldTransform() : Tr2Renderer::GetIdentityTransform();
	Matrix worldTransform = m_transform * parentTransform;

	lines->AddOrientedBox( worldTransform, m_minBounds, m_maxBounds, 0x80ffff00 );

	Geo::v128 probe;
	if( GetInputProbePosition( probe ) )
	{
		Vector3 probePos( Geo::VGetX( probe ), Geo::VGetY( probe ), Geo::VGetZ( probe ) );
		D3DXVec3TransformCoord( &probePos, &probePos, &parentTransform );
		lines->AddBox( Vector3(probePos.x - 0.05f, probePos.y - 0.05f, probePos.z - 0.05f), Vector3(probePos.x + 0.05f, probePos.y + 0.05f, probePos.z + 0.05f), 0x80ffff00 );
	}

	if( abs( GetNormalLocal().z ) > 0.0f )
	{
		float side = GetNormalLocal().z < 0.0f ? m_maxBounds.z : m_minBounds.z;

		Vector3 pos1 = Vector3( m_minBounds.x, m_minBounds.y, side );
		D3DXVec3TransformCoord( &pos1, &pos1, &worldTransform );
		Vector3 pos2 = Vector3( m_maxBounds.x, m_maxBounds.y, side );
		D3DXVec3TransformCoord( &pos2, &pos2, &worldTransform );
		lines->Add( pos1, 0x80ffff00, pos2, 0x80ffff00 );

		pos1 = Vector3( m_minBounds.x, m_maxBounds.y, side );
		D3DXVec3TransformCoord( &pos1, &pos1, &worldTransform );
		pos2 = Vector3( m_maxBounds.x, m_minBounds.y, side );
		D3DXVec3TransformCoord( &pos2, &pos2, &worldTransform );
		lines->Add( pos1, 0x80ffff00, pos2, 0x80ffff00 );
	}
	else if( abs( GetNormalLocal().y ) > 0.0f )
	{
		float side = GetNormalLocal().y < 0.0f ? m_maxBounds.y : m_minBounds.y;

		Vector3 pos1 = Vector3( m_minBounds.x, side, m_minBounds.z );
		D3DXVec3TransformCoord( &pos1, &pos1, &worldTransform );
		Vector3 pos2 = Vector3( m_maxBounds.x, side, m_maxBounds.z );
		D3DXVec3TransformCoord( &pos2, &pos2, &worldTransform );
		lines->Add( pos1, 0x80ffff00, pos2, 0x80ffff00 );

		pos1 = Vector3( m_minBounds.x, side, m_maxBounds.z );
		D3DXVec3TransformCoord( &pos1, &pos1, &worldTransform );
		pos2 = Vector3( m_maxBounds.x, side, m_minBounds.z );
		D3DXVec3TransformCoord( &pos2, &pos2, &worldTransform );
		lines->Add( pos1, 0x80ffff00, pos2, 0x80ffff00 );
	}
	else
	{
		float side = GetNormalLocal().x < 0.0f ? m_maxBounds.x : m_minBounds.x;

		Vector3 pos1 = Vector3( side, m_minBounds.y, m_minBounds.z );
		D3DXVec3TransformCoord( &pos1, &pos1, &worldTransform );
		Vector3 pos2 = Vector3( side, m_maxBounds.y, m_maxBounds.z );
		D3DXVec3TransformCoord( &pos2, &pos2, &worldTransform );
		lines->Add( pos1, 0x80ffff00, pos2, 0x80ffff00 );

		pos1 = Vector3( side, m_minBounds.y, m_maxBounds.z );
		D3DXVec3TransformCoord( &pos1, &pos1, &worldTransform );
		pos2 = Vector3( side, m_maxBounds.y, m_minBounds.z );
		D3DXVec3TransformCoord( &pos2, &pos2, &worldTransform );
		lines->Add( pos1, 0x80ffff00, pos2, 0x80ffff00 );
	}
}

// -------------------------------------------------------------
// Description:
//   Calculates and caches socket's "normal" vector. The normal
//   vector is perpendicular to the sockets bounding box side with
//   the largest surface and points towards containing cell center.
// Return Value:
//   Socket's "normal" vector in its local CS
// -------------------------------------------------------------
const Vector3& Tr2InteriorPortalSocket::GetNormalLocal()
{
	if( m_cell != NULL && m_normalLocal.x == 0.0f && m_normalLocal.y == 0.0f && m_normalLocal.z == 0.0f )
	{
		Vector3 size = m_maxBounds - m_minBounds;
		size.x = abs( size.x );
		size.y = abs( size.y );
		size.z = abs( size.z );

		Vector3 cellMin, cellMax;
		if( !m_cell->GetBoundingBox( cellMin, cellMax ) )
		{
			return m_normalLocal;
		}
		Vector3 cellCenter = ( cellMin + cellMax ) / 2.0f;
		Vector3 portalCenter = ( m_maxBounds + m_minBounds ) / 2.0f;
		D3DXVec3TransformCoord( &portalCenter, &portalCenter, &m_transform );
		Vector3 direction = cellCenter - portalCenter;

		if( size.x * size.y > size.x * size.z && size.x * size.y > size.y * size.z )
		{
			m_normalLocal = Vector3( 0, 0, 1.0f );
		}
		else if( size.x * size.z > size.x * size.y && size.x * size.z > size.y * size.z )
		{
			m_normalLocal = Vector3( 0, 1.0f, 0 );
		}
		else
		{
			m_normalLocal = Vector3( 1.0f, 0, 0 );
		}
		D3DXVec3TransformNormal( &m_normalCell, &m_normalLocal, &m_transform );
		if( D3DXVec3Dot( &direction, &m_normalCell ) < 0.0f )
		{
			m_normalCell = -m_normalCell;
			m_normalLocal = -m_normalLocal;
		}
	}
	return m_normalLocal;
}

// -------------------------------------------------------------
// Description:
//   Returns socket's "normal" vector in cell coordinate system.
// Return Value:
//   Socket's "normal" vector in parent cell CS
// SeeAlso:
//   GetNormalLocal
// -------------------------------------------------------------
const Vector3& Tr2InteriorPortalSocket::GetNormalCell()
{
	GetNormalLocal();
	return m_normalCell;
}

#endif
