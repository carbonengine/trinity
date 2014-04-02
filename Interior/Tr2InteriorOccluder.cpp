#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorOccluder.h"
#include "Tr2InteriorCell.h"
#include "umbraTypes.h"

// Initialize umbra model
Umbra::Model* Tr2InteriorOccluder::s_umbraModel = NULL;

// ------------------------------------------------------------------------------------------------------
Tr2InteriorOccluder::Tr2InteriorOccluder( IRoot* lockobj ):
m_umbraObject( NULL ),
m_parentCell( NULL )
// ------------------------------------------------------------------------------------------------------
{
	D3DXMatrixIdentity( &m_transform );
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorOccluder::~Tr2InteriorOccluder()
// ------------------------------------------------------------------------------------------------------
{
//	Remove ourselves from Umbra if we haven't already
	ClearUmbraData();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorOccluder::SetParentCell( Tr2InteriorCell* cell )
{
	m_parentCell = cell;

	ClearUmbraData();

	if( cell != NULL )
	{
		Umbra::Cell* umbraCell=cell->GetUmbraCell();

		if( umbraCell )
		{
			BuildUmbraData( umbraCell );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorOccluder::OnModified( Be::Var* val )
// ------------------------------------------------------------------------------------------------------
{
	if( IsMatch( val, m_transform ) )
	{
		TransformModified();
	}
	else// if (val == (Be::Var*)&m_clockwise)
	{
		Umbra::Cell* umbraCell = m_parentCell ? m_parentCell->GetUmbraCell() : NULL;

		BuildUmbraData( umbraCell );
	}
	return true;
}


// ------------------------------------------------------------------------------------------------------
const Matrix& Tr2InteriorOccluder::GetParentTransform() const
// ------------------------------------------------------------------------------------------------------
{
	return m_parentCell ? m_parentCell->GetWorldTransform() : Tr2Renderer::GetIdentityTransform();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorOccluder::GetLocalBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
// ------------------------------------------------------------------------------------------------------
{
	minBounds = Vector3( -0.5f, -0.5f, -0.5f );
	maxBounds = Vector3( 0.5f, 0.5f, 0.5f );

	return true;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorOccluder::GetWorldBoundingBox( Vector3& min, Vector3& max ) const
// ------------------------------------------------------------------------------------------------------
{
	min = Vector3( -0.5f, -0.5f, -0.5f );
	max = Vector3( 0.5f, 0.5f, 0.5f );

	BoundingBoxTransform( min, max, m_transform );

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorOccluder::ClearUmbraData()
// ------------------------------------------------------------------------------------------------------
{
	if( m_umbraObject )
	{
		m_umbraObject->setCell( NULL );
		m_umbraObject->release();
		m_umbraObject = NULL;
	}
}

// ------------------------------------------------------------------------------------------------------
//	Creates a mesh model to be used as an occlusion writer.  For now, Occluders only support cubes as occluders
//	Eventually they may support more complex shapes.  In the meantime, we should probably be sharing the mesh!
Umbra::Model* Tr2InteriorOccluder::GetMeshModel()
// ------------------------------------------------------------------------------------------------------
{
	if( Tr2InteriorOccluder::s_umbraModel == NULL )
	{
		static const int vertexCount = 8;
		static const int indexCount = 36;

		Vector3 verts[vertexCount] = 
		{
			Vector3( -0.5f, -0.5f, -0.5f ),
			Vector3( -0.5f, -0.5f, 0.5f ),
			Vector3( 0.5f, -0.5f, 0.5f ),
			Vector3( 0.5f, -0.5f, -0.5f ),

			Vector3( -0.5f, 0.5f, -0.5f ),
			Vector3( 0.5f, 0.5f, -0.5f ),
			Vector3( 0.5f, 0.5f, 0.5f ),
			Vector3( -0.5f, 0.5f, 0.5f )
		};

		int indices[indexCount] = 
		{
			//Bottom
			0, 1, 2,
			2, 3, 0,

			//Top
			4, 5, 6,
			6, 7, 4,

			//Front
			0, 3, 5,
			5, 4, 0,

			//Back
			1, 7, 6,
			6, 2, 1,

			//Right
			0, 4, 7,
			7, 1, 0,

			//Left
			2, 6, 5,
			5, 3, 2
		};

		Tr2InteriorOccluder::s_umbraModel = Umbra::MeshModel::create( 
			reinterpret_cast<Umbra::Vector3*>( verts ), 
			reinterpret_cast<Umbra::Vector3i*>( indices ), 
			vertexCount, indexCount / 3, true );
	}
	return( Umbra::Model* ) Tr2InteriorOccluder::s_umbraModel;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorOccluder::BuildUmbraData( Umbra::Cell* umbraCell )
// ------------------------------------------------------------------------------------------------------
{
	ClearUmbraData();

	// Build the Umbra model

	Umbra::Model* umbraModel = ( Umbra::Model* )Tr2InteriorOccluder::GetMeshModel();

	m_umbraObject = Umbra::Object::create( umbraModel );

	if( m_umbraObject )
	{
        m_umbraObject->set( Umbra::Object::INFORM_VISIBLE, false );

		m_umbraObject->setCell( umbraCell );
        m_umbraObject->setWriteModel( umbraModel );
		m_umbraObject->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );

	}

}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorOccluder::TransformModified()
// ------------------------------------------------------------------------------------------------------
{
	if( m_umbraObject )
		m_umbraObject->setObjectToCellMatrix( AS_UMBRA_MATRIX( m_transform ) );
}

#endif
