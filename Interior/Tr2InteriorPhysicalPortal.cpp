#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorPhysicalPortal.h"

#include "umbraTypes.h"

#include "TriLineSet.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorPortalSocket.h"

// ------------------------------------------------------------------------------------------------------
Tr2InteriorPhysicalPortal::Tr2InteriorPhysicalPortal( IRoot* lockobj ) :
	m_umbraPortalAtoB( NULL ),
	m_umbraPortalBtoA( NULL ),
	m_umbraModel( NULL ),
	m_minBounds( -1.0f, -1.0f, -1.0f ),
	m_maxBounds( 1.0f, 1.0f, 1.0f ),
	m_name("PhysicalPortal"),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_worldTransform(),
	m_enabled( true ),
	m_doorInCellA( NULL ),
	m_doorInCellB( NULL )
{
	// Initialize the transform matrix to the identity
	m_worldTransform = Tr2Renderer::GetIdentityTransform();
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorPhysicalPortal::~Tr2InteriorPhysicalPortal()
{
	// Get out of cell and free umbra stuff
	RemoveFromCells();
	ReleaseDoorUmbraObjects();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorPhysicalPortal::Initialize( void )
{
	// position & orientation modify: calc new transform matrix...
	D3DXMatrixTransformation( &m_worldTransform, NULL, NULL, NULL, NULL, &m_rotation, &m_position );

	// Reset the portals
	Tr2InteriorCellPtr cellA = m_cellA;
	Tr2InteriorCellPtr cellB = m_cellB;
	
	AddToCells( cellA, cellB );
	AddDoorToUmbra();

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorPhysicalPortal::OnModified( Be::Var* value )
{
	// Update the world transform
	if( IsMatch( value, m_position ) || IsMatch( value, m_rotation ) )
	{
		// Calculate new world-space transformation matrix
		D3DXMatrixTransformation( &m_worldTransform, NULL, NULL, NULL, NULL, &m_rotation, &m_position );

		// Notify the Umbra portals about the new matrix
		if( m_umbraPortalAtoB )
		{
			if( m_cellA )
			{
				Matrix cellTransform;
				D3DXMatrixInverse( &cellTransform, NULL, &m_cellA->GetWorldTransform() );
				cellTransform = m_worldTransform * cellTransform;

				m_umbraPortalAtoB->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
			}
			else
			{
				m_umbraPortalAtoB->setObjectToCellMatrix( AS_UMBRA_MATRIX(m_worldTransform) );
			}
		}
		if( m_umbraPortalBtoA )
		{
			if( m_cellB )
			{
				Matrix cellTransform;
				D3DXMatrixInverse( &cellTransform, NULL, &m_cellB->GetWorldTransform() );
				cellTransform = m_worldTransform * cellTransform;

				m_umbraPortalBtoA->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
			}
			else
			{
				m_umbraPortalBtoA->setObjectToCellMatrix( AS_UMBRA_MATRIX(m_worldTransform) );
			}
		}
	}

	// Update the Umbra model
	if( IsMatch( value, m_minBounds ) || IsMatch( value, m_maxBounds ) )
	{
		RemoveFromCells();
		AddToCells( m_cellA, m_cellB );
	}

	if( IsMatch( value, m_enabled ) )
	{
		if( m_enabled )
		{
			AddToCells( m_cellA, m_cellB );
		}
		else
		{
			// Trick RemoveFromCells to actually do the job
			m_enabled = true;
			RemoveFromCells();
			m_enabled = false;
		}
	}

	if( IsMatch( value, m_doorObject ) )
	{
		ReleaseDoorUmbraObjects();
		AddDoorToUmbra();
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorCellPtr Tr2InteriorPhysicalPortal::GetCellA( void ) const
{
	return m_cellA;
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorCellPtr Tr2InteriorPhysicalPortal::GetCellB( void ) const
{
	return m_cellB;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::ConnectCells( Tr2InteriorCell* cellA, Tr2InteriorCell* cellB )
{
	// First, remove the old cell connections	
	RemoveFromCells();
	ReleaseDoorUmbraObjects();

	m_cellA = NULL;
	m_cellB = NULL;
	m_socketA = NULL;
	m_socketB = NULL;

	// Add the new cell connections
	AddToCells( cellA, cellB );
	AddDoorToUmbra();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::ConnectSockets( Tr2InteriorPortalSocket* socketA, Tr2InteriorPortalSocket* socketB )
{
	// First, remove the old cell connections	
	RemoveFromCells();
	ReleaseDoorUmbraObjects();

	m_socketA = socketA;
	m_socketB = socketB;

	// Add the new cell connections
	Tr2InteriorCell* cellA = 0;
	if( socketA && socketA->GetCell() )
	{
		cellA = socketA->GetCell();
	}
	Tr2InteriorCell* cellB = 0;
	if( socketB && socketB->GetCell() )
	{
		cellB = socketB->GetCell();
	}
	AddToCells( cellA, cellB );
	AddDoorToUmbra();
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorPortalSocket* Tr2InteriorPhysicalPortal::GetSocketA() const
{
	return m_socketA;
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorPortalSocket* Tr2InteriorPhysicalPortal::GetSocketB() const
{
	return m_socketB;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::SetOutputSocketColor( Tr2InteriorPortalSocket* source, const Matrix& redMat, const Matrix& greenMat, const Matrix& blueMat )
{
	if( !m_enabled )
	{
		return;
	}

	if( source == m_socketA )
	{
		if( m_socketB )
		{
			m_socketB->SetOutputColor( redMat, greenMat, blueMat );
		}
	}
	else if( source == m_socketB )
	{
		if( m_socketA )
		{
			m_socketA->SetOutputColor( redMat, greenMat, blueMat );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::CreateSockets()
{
	if( m_cellA != NULL && m_socketA == NULL )
	{
		m_socketA.CreateInstance();
		m_socketA->SetLocalBoundingBox( m_minBounds, m_maxBounds );
		Matrix cellTransformInv;
		D3DXMatrixInverse( &cellTransformInv, NULL, &m_cellA->GetWorldTransform() );
		m_socketA->SetTransform( m_worldTransform * cellTransformInv );
		m_cellA->AddPortalSocket( m_socketA );
		m_socketA->AddToPortal( this );
		if( m_enabled )
		{
			m_cellA->RemoveCellNeighbor( m_cellB );
		}
	}
	if( m_cellB != NULL && m_socketB == NULL )
	{
		m_socketB.CreateInstance();
		m_socketB->SetLocalBoundingBox( m_minBounds, m_maxBounds );
		Matrix cellTransformInv;
		D3DXMatrixInverse( &cellTransformInv, NULL, &m_cellB->GetWorldTransform() );
		m_socketB->SetTransform( m_worldTransform * cellTransformInv );
		m_cellB->AddPortalSocket( m_socketB );
		m_socketB->AddToPortal( this );
		if( m_enabled )
		{
			m_cellB->RemoveCellNeighbor( m_cellA );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::PositionFromSockets()
{
	bool assigned = false;
	float volumeA;
	if( m_socketA )
	{
		PositionFromSocket( m_socketA, m_socketB, m_minBounds, m_maxBounds );
		Vector3 size = m_maxBounds - m_minBounds;
		volumeA = abs( size.x * size.y * size.z );
		m_worldTransform = m_socketA->GetWorldTransform();
	}
	if( m_socketB )
	{
		Vector3 minBoundsB, maxBoundsB;
		PositionFromSocket( m_socketB, m_socketA, minBoundsB, maxBoundsB );
		Vector3 size = maxBoundsB - minBoundsB;
		float volumeB = abs( size.x * size.y * size.z );
		if( !assigned || volumeB < volumeA )
		{
			m_minBounds = minBoundsB;
			m_maxBounds = maxBoundsB;
			m_worldTransform = m_socketB->GetWorldTransform();
			assigned = true;
		}
	}
	if( assigned )
	{
		Vector3 scale;
		D3DXMatrixDecompose( &scale, &m_rotation, &m_position, &m_worldTransform );

		// Remove and re-add, which will recreate the Umbra model
		RemoveFromCells();
		ReleaseDoorUmbraObjects();
		AddToCells( m_cellA, m_cellB );
		AddDoorToUmbra();
	}
}

// ------------------------------------------------------------------------------------------------------
static void IncludeInBounds( const Vector3 &position, Vector3& minBounds, Vector3& maxBounds )
{
	if( minBounds.x > position.x )
	{
		minBounds.x = position.x;
	}
	if( minBounds.y > position.y )
	{
		minBounds.y = position.y;
	}
	if( minBounds.z > position.z )
	{
		minBounds.z = position.z;
	}
	if( maxBounds.x < position.x )
	{
		maxBounds.x = position.x;
	}
	if( maxBounds.y < position.y )
	{
		maxBounds.y = position.y;
	}
	if( maxBounds.z < position.z )
	{
		maxBounds.z = position.z;
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::PositionFromSocket( Tr2InteriorPortalSocket* transformSocket, Tr2InteriorPortalSocket* boundsSocket, Vector3& minBounds, Vector3& maxBounds )
{
	transformSocket->GetLocalBoundingBox( minBounds, maxBounds );
	if( minBounds.x > maxBounds.x )
	{
		std::swap( minBounds.x, maxBounds.x );
	}
	if( minBounds.y > maxBounds.y )
	{
		std::swap( minBounds.y, maxBounds.y );
	}
	if( minBounds.z > maxBounds.z )
	{
		std::swap( minBounds.z, maxBounds.z );
	}
	if( boundsSocket )
	{
		// Enlarge bounding box to contain all corners of boundsSocket bounding box
		Matrix transform = transformSocket->GetWorldTransform();
		D3DXMatrixInverse( &transform, NULL, &transform );
		transform = boundsSocket->GetWorldTransform() * transform;

		Vector3 socketMin, socketMax;
		boundsSocket->GetLocalBoundingBox( socketMin, socketMax );

		Vector3 tmp = Vector3( socketMin.x, socketMin.y, socketMin.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMin.x, socketMin.y, socketMax.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMin.x, socketMax.y, socketMin.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMin.x, socketMax.y, socketMax.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMax.x, socketMin.y, socketMin.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMax.x, socketMin.y, socketMax.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMax.x, socketMax.y, socketMin.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );

		tmp = Vector3( socketMax.x, socketMax.y, socketMax.z );
		D3DXVec3TransformCoord( &tmp, &tmp, &transform );
		IncludeInBounds( tmp, minBounds, maxBounds );
	}
}

void Tr2InteriorPhysicalPortal::OnCellTransformChanged( Tr2InteriorCell* cell )
{
	if( cell == m_cellA && m_umbraPortalAtoB != NULL )
	{
		Matrix cellTransform;
		D3DXMatrixInverse( &cellTransform, NULL, &cell->GetWorldTransform() );

		if( m_doorInCellA )
		{
			m_doorInCellA->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
		}

		cellTransform = m_worldTransform * cellTransform;
		m_umbraPortalAtoB->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
	}
	if( cell == m_cellB && m_umbraPortalBtoA != NULL )
	{
		Matrix cellTransform;
		D3DXMatrixInverse( &cellTransform, NULL, &cell->GetWorldTransform() );

		if( m_doorInCellB )
		{
			m_doorInCellB->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
		}

		cellTransform = m_worldTransform * cellTransform;
		m_umbraPortalBtoA->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::RenderDebugInfo( TriLineSetPtr lines ) const
{
	lines->AddOrientedBox( m_worldTransform, m_minBounds, m_maxBounds, 0x8000ff00 );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::AddToCells( Tr2InteriorCell* cellA, Tr2InteriorCell* cellB )
{
	// Cache pointers to the Tr2InteriorCells we're connecting
	m_cellA = cellA;
	m_cellB = cellB;

	if( m_enabled )
	{
		// Create bounding box and add to cell
		m_umbraModel = (Umbra::Model*)Umbra::OBBModel::create( AS_UMBRA_VECTOR3(m_minBounds), AS_UMBRA_VECTOR3(m_maxBounds) );
		m_umbraModel->set(Umbra::Model::BACKFACE_CULLABLE, true);

		// Get the Umbra cells
		Umbra::Cell* umbraCellA = NULL;
		Umbra::Cell* umbraCellB = NULL;
		if( m_cellA )
		{
			umbraCellA = m_cellA->GetUmbraCell();
			if( m_socketA )
			{
				m_socketA->AddToPortal( this );
			}
			else
			{
				m_cellA->AddCellNeighbor( m_cellB );
			}
		}
		if( m_cellB )
		{
			umbraCellB = m_cellB->GetUmbraCell();
			if( m_socketB )
			{
				m_socketB->AddToPortal( this );
			}
			else
			{
				m_cellB->AddCellNeighbor( m_cellA );
			}
		}

		// Create the portals
		m_umbraPortalAtoB = Umbra::PhysicalPortal::create( m_umbraModel, umbraCellB /*the target cell*/);
		m_umbraPortalBtoA = Umbra::PhysicalPortal::create( m_umbraModel, umbraCellA /*the target cell*/);

		if( m_umbraPortalAtoB )
		{
			m_umbraPortalAtoB->setCell( umbraCellA );

			if( m_cellA )
			{
				Matrix cellTransform;
				D3DXMatrixInverse( &cellTransform, NULL, &m_cellA->GetWorldTransform() );
				cellTransform = m_worldTransform * cellTransform;
				m_umbraPortalAtoB->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
			}
			else
			{
				m_umbraPortalAtoB->setObjectToCellMatrix( AS_UMBRA_MATRIX(m_worldTransform) );
			}

			m_umbraPortalAtoB->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
			m_umbraPortalAtoB->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );
			m_umbraPortalAtoB->set( Umbra::Object::INFORM_PORTAL_EXIT, true );

			m_umbraPortalAtoB->setUserPointer( NULL );
		}
		if( m_umbraPortalBtoA )
		{
			m_umbraPortalBtoA->setCell( umbraCellB );

			if( m_cellB )
			{
				Matrix cellTransform;
				D3DXMatrixInverse( &cellTransform, NULL, &m_cellB->GetWorldTransform() );
				cellTransform = m_worldTransform * cellTransform;
				m_umbraPortalBtoA->setObjectToCellMatrix( AS_UMBRA_MATRIX(cellTransform) );
			}
			else
			{
				m_umbraPortalBtoA->setObjectToCellMatrix( AS_UMBRA_MATRIX(m_worldTransform) );
			}

			m_umbraPortalBtoA->set( Umbra::Object::INFORM_PORTAL_ENTER, true );
			m_umbraPortalBtoA->set( Umbra::Object::INFORM_PORTAL_PRE_EXIT, true );
			m_umbraPortalBtoA->set( Umbra::Object::INFORM_PORTAL_EXIT, true );

			m_umbraPortalBtoA->setUserPointer( NULL );
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Adds a door object to Umbra cells.
// -------------------------------------------------------------
void Tr2InteriorPhysicalPortal::AddDoorToUmbra()
{
	if( m_doorObject )
	{
		if( m_cellA )
		{
			m_doorObject->UpdateUmbraObject( m_cellA->GetUmbraCell(), m_doorInCellA );
		}
		if( m_cellB )
		{
			m_doorObject->UpdateUmbraObject( m_cellB->GetUmbraCell(), m_doorInCellB );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorPhysicalPortal::RemoveFromCells( void )
{
	if( m_enabled )
	{
		// Remove old cell connections
		if( m_cellA )
		{
			if( m_socketA )
			{
				m_socketA->AddToPortal( NULL );
			}
			else
			{
				m_cellA->RemoveCellNeighbor( m_cellB );
			}
		}
		if( m_cellB )
		{
			if( m_socketB )
			{
				m_socketB->AddToPortal( NULL );
			}
			else
			{
				m_cellB->RemoveCellNeighbor( m_cellA );
			}
		}
	}

	// Remove portals from Umbra cells and clean-up portals
	if( m_umbraPortalAtoB )
	{
		m_umbraPortalAtoB->setCell( NULL );
		m_umbraPortalAtoB->release();
		m_umbraPortalAtoB = NULL;
	}
	if( m_umbraPortalBtoA )
	{
		m_umbraPortalBtoA->setCell( NULL );
		m_umbraPortalBtoA->release();
		m_umbraPortalBtoA = NULL;
	}

	// Clean-up Umbra model
	if( m_umbraModel )
	{
		m_umbraModel->release();
		m_umbraModel = NULL;
	}
}

// -------------------------------------------------------------
// Description:
//   Deletes Umbra door objects.
// -------------------------------------------------------------
void Tr2InteriorPhysicalPortal::ReleaseDoorUmbraObjects()
{
	if( m_doorInCellA )
	{
		m_doorInCellA->release();
		m_doorInCellA = NULL;
	}

	if( m_doorInCellB )
	{
		m_doorInCellB->release();
		m_doorInCellB = NULL;
	}
}

#endif
