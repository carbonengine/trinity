#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorMirror.h"
#include "Tr2Renderer.h"
#include "umbraTypes.h"

Tr2InteriorMirror::Tr2InteriorMirror() :
	m_umbraModel( NULL ),
	m_mirrorPortalFront( NULL ),
	m_mirrorPortalBack( NULL ),
	m_cell( NULL ),
	m_warpMatrixFront( Tr2Renderer::GetIdentityTransform() ),
	m_warpMatrixBack( Tr2Renderer::GetIdentityTransform() ),
	m_transformMatrix( Tr2Renderer::GetIdentityTransform() ),
	m_meshIndex( 0 ),
	m_areaIndex( 0 ),
	m_mirrorIndex( 0 ),
	m_minBounds( 0.0f, 0.0f, 0.0f ),
	m_maxBounds( 0.0f, 0.0f, 0.0f ),
	m_placeable( NULL )
{
}

Tr2InteriorMirror::~Tr2InteriorMirror()
{
	// Destroy the Umbra mirror
	DestroyUmbraMirror();
}

int Tr2InteriorMirror::GetMeshIndex( void ) const
{
	return m_meshIndex;
}

int Tr2InteriorMirror::GetAreaIndex( void ) const
{
	return m_areaIndex;
}

const Matrix& Tr2InteriorMirror::GetWarpMatrixFront( void ) const
{
	return m_warpMatrixFront;
}

const Matrix& Tr2InteriorMirror::GetWarpMatrixBack( void ) const
{
	return m_warpMatrixBack;
}

const Matrix& Tr2InteriorMirror::GetTransformMatrix( void ) const
{
	return m_transformMatrix;
}

void Tr2InteriorMirror::SetPlaceable( Tr2InteriorPlaceable* placeable )
{
	m_placeable = placeable;
}

Tr2InteriorPlaceable* Tr2InteriorMirror::GetPlaceable( void ) const
{
	return m_placeable;
}

int Tr2InteriorMirror::GetMirrorIndex( void ) const
{
	return m_mirrorIndex;
}

void Tr2InteriorMirror::SetMirrorIndex( int index )
{
	m_mirrorIndex = index;
}

Umbra::Cell* Tr2InteriorMirror::GetCell() const
{
	return m_cell;
}

void Tr2InteriorMirror::SetCell( Umbra::Cell* cell )
{
	m_cell = cell;
}

void Tr2InteriorMirror::SetMeshIndex( int index )
{
	m_meshIndex = index;
}

void Tr2InteriorMirror::SetAreaIndex( int index )
{
	m_areaIndex = index;
}

void Tr2InteriorMirror::SetWarpMatrixFront( const Matrix& warpMatrix )
{
	m_warpMatrixFront = warpMatrix;
}

void Tr2InteriorMirror::SetWarpMatrixBack( const Matrix& warpMatrix )
{
	m_warpMatrixBack = warpMatrix;
}

void Tr2InteriorMirror::SetTransformMatrix( const Matrix& transformMatrix )
{
	m_transformMatrix = transformMatrix;

	Matrix m = m_transformMatrix;

	if( m_cell )
	{
		Matrix cellTransform;
		m_cell->getCellToWorldMatrix( AS_UMBRA_MATRIX(cellTransform) );
		D3DXMatrixInverse( &cellTransform, NULL, &cellTransform );
		m = m * cellTransform;
	}

	if( m_mirrorPortalFront )
	{
		m_mirrorPortalFront->setObjectToCellMatrix( AS_UMBRA_MATRIX(m) );
	}

	if( m_mirrorPortalBack )
	{
		m_mirrorPortalBack->setObjectToCellMatrix( AS_UMBRA_MATRIX(m) );
	}
}

void Tr2InteriorMirror::SetBoundingBox( const Vector3& minBounds, const Vector3& maxBounds )
{
	m_minBounds = minBounds;
	m_maxBounds = maxBounds;
}

void Tr2InteriorMirror::BuildUmbraMirror( void )
{
	// Destroy old Umbra mirror (if one exists)
	DestroyUmbraMirror();

	// Create Umbra model
	m_umbraModel = (Umbra::Model*)Umbra::OBBModel::create( AS_UMBRA_VECTOR3(m_minBounds), AS_UMBRA_VECTOR3(m_maxBounds) );
	m_umbraModel->set(Umbra::Model::BACKFACE_CULLABLE, true);

	// Create portals
	m_mirrorPortalFront = (Umbra::VirtualPortal*)Umbra::VirtualPortal::create( m_umbraModel, NULL );
	m_mirrorPortalBack = (Umbra::VirtualPortal*)Umbra::VirtualPortal::create( m_umbraModel, NULL );

	// Configure portals
	m_mirrorPortalFront->setTargetPortal( m_mirrorPortalBack );
	m_mirrorPortalFront->set( Umbra::Object::ENABLED, true );
	m_mirrorPortalFront->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
	m_mirrorPortalFront->set( Umbra::Object::INFORM_PORTAL_EXIT, true );
	m_mirrorPortalFront->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );
	m_mirrorPortalFront->set( Umbra::PhysicalPortal::FLOATING_PORTAL, true );
	m_mirrorPortalFront->setUserPointer( (void*)this );

	m_mirrorPortalBack->setTargetPortal( NULL );
	m_mirrorPortalBack->set( Umbra::Object::ENABLED, false );
	m_mirrorPortalBack->setUserPointer( (void*)this );

	// Note: this model is not used for stencil buffer rendering - it just enables
	// Umbra to generate a stencil mask changed event for this portal
	m_mirrorPortalFront->setStencilModel( m_umbraModel );

	m_mirrorPortalFront->setCell( m_cell );
	m_mirrorPortalBack->setCell( m_cell );

	// Set matrices
	m_mirrorPortalFront->setWarpMatrix( AS_UMBRA_MATRIX(m_warpMatrixFront) );
	m_mirrorPortalBack->setWarpMatrix( AS_UMBRA_MATRIX(m_warpMatrixBack) );

	// Set translation
	Matrix m = m_transformMatrix;

	if( m_cell )
	{
		Matrix cellTransform;
		m_cell->getCellToWorldMatrix( AS_UMBRA_MATRIX(cellTransform) );
		D3DXMatrixInverse( &cellTransform, NULL, &cellTransform );
		m = m * cellTransform;
	}

	m_mirrorPortalFront->setObjectToCellMatrix( AS_UMBRA_MATRIX(m) );
	m_mirrorPortalBack->setObjectToCellMatrix( AS_UMBRA_MATRIX(m) );
}

void Tr2InteriorMirror::DestroyUmbraMirror( void )
{
	// Destroy the front portal
	if( m_mirrorPortalFront )
	{
		m_mirrorPortalFront->setCell( NULL );
		m_mirrorPortalFront->release();
		m_mirrorPortalFront = NULL;
	}

	// Destroy the back portal
	if( m_mirrorPortalBack )
	{
		m_mirrorPortalBack->setCell( NULL );
		m_mirrorPortalBack->release();
		m_mirrorPortalBack = NULL;
	}

	// Destroy the model
	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}
}

// --------------------------------------------------------------------------------------
// Description
//   Enable or disable Umbra portals for this mirror.
// Arguments:
//   enable - If true - enable portals
//			  If false - disable portals
// --------------------------------------------------------------------------------------
void Tr2InteriorMirror::EnablePortals( bool enable )
{
	if( m_mirrorPortalFront )
	{
		m_mirrorPortalFront->set( Umbra::Object::ENABLED, enable );
	}
	if( m_mirrorPortalBack )
	{
		m_mirrorPortalBack->set( Umbra::Object::ENABLED, enable );
	}
}

#endif
