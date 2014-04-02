////////////////////////////////////////////////////////////
//
//    Created:   May 2010
//    Copyright: CCP 2010
//

#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorDusterCache.h"

const unsigned Tr2InteriorDusterCache::MAX_DUSTERS_PER_NODE = 512;
const unsigned Tr2InteriorDusterCache::MAX_TREE_DEPTH = 6;

Tr2InteriorDusterCache::Tr2InteriorDusterCache()
:	m_root( NULL ),
	m_positions( NULL ),
	m_normals( NULL ),
	m_albedoes( NULL ),
	m_scratchMemory( NULL ),
	m_indexes( NULL ),
	m_count( NULL ),
	m_albedoDirty( true ),
	m_lightCount( 0 )
{
}

Tr2InteriorDusterCache::~Tr2InteriorDusterCache()
{
	for( std::map<float, Enlighten::InputLightFalloffTable*>::iterator it = m_falloffTables.begin(); 
		it != m_falloffTables.end(); 
		++it )
	{
		using namespace Enlighten;
		GEO_DELETE( InputLightFalloffTable, it->second );
	}
}

// -------------------------------------------------------------
// Description:
//   Initializes duster cache arrays and builds an octree.
// Arguments:
//   inputWorkspace - Input workspace for Enlighten system
//   lightingBuffer - Lighting buffer: not really used, but 
//					  required for Enlighten calls
// -------------------------------------------------------------
void Tr2InteriorDusterCache::Initialize( const Enlighten::InputWorkspace* inputWorkspace, 
										 const Enlighten::InputLightingBuffer* lightingBuffer )
{
	Clear(); 

	m_count = Enlighten::GetNumberOfPointsInInputWorkspace( inputWorkspace );
	m_positions = GEO_NEW_ARRAY_ALIGNED( XMVECTOR, m_count, 16 );
	m_normals = GEO_NEW_ARRAY_ALIGNED( XMVECTOR, m_count, 16 );
	m_albedoes = GEO_NEW_ARRAY_ALIGNED( XMVECTOR, m_count, 16 );
	m_scratchMemory = GEO_NEW_ARRAY_ALIGNED( XMVECTOR, m_count, 16 );
	m_indexes = new unsigned int[m_count];

	XMVECTOR invalidAlbedo = Vector4( -1.0f, -1.0f, -1.0f, -1.0f );
	Vector3 minBounds, maxBounds;
	for( unsigned int i = 0; i < m_count; ++i )
	{
		Enlighten::InputWorkspaceDebugPoint debugPoint;
		Enlighten::GetInputWorkspaceDebugPoint( inputWorkspace, &debugPoint, i );
		m_positions[i] = Vector4( debugPoint.m_Position[0], debugPoint.m_Position[1], debugPoint.m_Position[2], 1.0f );
		m_normals[i] = Vector4( debugPoint.m_Normal[0], debugPoint.m_Normal[1], debugPoint.m_Normal[2], 0.0f );
		m_albedoes[i] = invalidAlbedo;

		if( i == 0 || debugPoint.m_Position[0] < minBounds.x )
		{
			minBounds.x = debugPoint.m_Position[0];
		}
		if( i == 0 || debugPoint.m_Position[1] < minBounds.y )
		{
			minBounds.y = debugPoint.m_Position[1];
		}
		if( i == 0 || debugPoint.m_Position[2] < minBounds.z )
		{
			minBounds.z = debugPoint.m_Position[2];
		}
		if( i == 0 || debugPoint.m_Position[0] > maxBounds.x )
		{
			maxBounds.x = debugPoint.m_Position[0];
		}
		if( i == 0 || debugPoint.m_Position[1] > maxBounds.y )
		{
			maxBounds.y = debugPoint.m_Position[1];
		}
		if( i == 0 || debugPoint.m_Position[2] > maxBounds.z )
		{
			maxBounds.z = debugPoint.m_Position[2];
		}
		m_indexes[i] = i;
	}

	m_minBounds = Vector3( XMVectorGetX( minBounds ), XMVectorGetY( minBounds ), XMVectorGetZ( minBounds ) );
	m_maxBounds = Vector3( XMVectorGetX( maxBounds ), XMVectorGetY( maxBounds ), XMVectorGetZ( maxBounds ) );

	m_root = new Node;
	m_root->start = 0;
	m_root->count = m_count;
	for( unsigned int i = 0; i < 8; ++i )
	{
		m_root->children[i] = 0;
	}
	BuildTree( m_root, 0, m_minBounds, m_maxBounds );

	m_albedoDirty = true;
}

// -------------------------------------------------------------
// Description:
//   Splits an octree node if it contains more dusters that needed.
// Arguments:
//   node - Node to split
//   depth - Node's depth (to limit maximum tree depth)
//   minBounds - Node's min bounds
//   maxBounds - Node's max bounds
// -------------------------------------------------------------
void Tr2InteriorDusterCache::BuildTree( Node* node, unsigned int depth, const Vector3& minBounds, const Vector3& maxBounds )
{
	if( node->count > MAX_DUSTERS_PER_NODE && depth < MAX_TREE_DEPTH )
	{
		unsigned int* childIndexes[8];
		node->center = ( minBounds + maxBounds ) / 2.f;
		XMVECTOR center = node->center;
		for( int i = 0; i < 8; ++i )
		{
			node->children[i] = new Node;
			node->children[i]->count = 0;
			childIndexes[i] = new unsigned int[node->count];
			for( int j = 0; j < 8; ++j )
			{
				node->children[i]->children[j] = 0;
			}
		}
		for( unsigned int i = 0; i < node->count; ++i )
		{
			Vector3 point = Vector3( XMVectorGetX( m_positions[m_indexes[node->start + i]] ),
									 XMVectorGetY( m_positions[m_indexes[node->start + i]] ),
									 XMVectorGetZ( m_positions[m_indexes[node->start + i]] ) );
			int child;
			if( point.x < node->center.x )
			{
				if( point.y < node->center.y )
				{
					if( point.z < node->center.z )
					{
						child = 0;
					}
					else
					{
						child = 4;
					}
				}
				else
				{
					if( point.z < node->center.z )
					{
						child = 2;
					}
					else
					{
						child = 6;
					}
				}
			}
			else
			{
				if( point.y < node->center.y )
				{
					if( point.z < node->center.z )
					{
						child = 1;
					}
					else
					{
						child = 5;
					}
				}
				else
				{
					if( point.z < node->center.z )
					{
						child = 3;
					}
					else
					{
						child = 7;
					}
				}
			}
			childIndexes[child][node->children[child]->count++] = m_indexes[node->start + i];
		}
		unsigned int start = node->start;
		for( int i = 0; i < 8; ++i )
		{
			node->children[i]->start = start;
			memcpy( m_indexes + start, childIndexes[i], node->children[i]->count * sizeof( unsigned int ) );
			start += node->children[i]->count;
			delete[] childIndexes[i];
		}
		BuildTree( node->children[0], 
				   depth + 1,
				   minBounds, 
				   node->center );
		BuildTree( node->children[1], 
				   depth + 1,
				   Vector3( node->center.x, minBounds.y, minBounds.z ), 
				   Vector3( maxBounds.x, node->center.y, node->center.z ) );
		BuildTree( node->children[2], 
				   depth + 1,
				   Vector3( minBounds.x, node->center.y, minBounds.z ), 
				   Vector3( node->center.x, maxBounds.y, node->center.z ) );
		BuildTree( node->children[3], 
				   depth + 1,
				   Vector3( node->center.x, node->center.y, minBounds.z ), 
				   Vector3( maxBounds.x, maxBounds.y, node->center.z ) );
		BuildTree( node->children[4],
				   depth + 1,
				   Vector3( minBounds.x, minBounds.y, node->center.z ), 
				   Vector3( node->center.x, node->center.y, maxBounds.z ) );
		BuildTree( node->children[5], 
				   depth + 1,
				   Vector3( node->center.x, minBounds.y, node->center.z ), 
				   Vector3( maxBounds.x, node->center.y, maxBounds.z ) );
		BuildTree( node->children[6],
				   depth + 1,
				   Vector3( minBounds.x, node->center.y, node->center.z ), 
				   Vector3( node->center.x, maxBounds.y, maxBounds.z ) );
		BuildTree( node->children[7], 
				   depth + 1,
				   node->center, 
				   maxBounds );
	}
}

// -------------------------------------------------------------
// Description:
//   Deletes a node in octree octree.
// Arguments:
//   node - Node to delete
// -------------------------------------------------------------
void Tr2InteriorDusterCache::DeleteTree( Node* node )
{
	if( node->children[0] )
	{
		for( int i = 0; i < 8; ++i )
		{
			DeleteTree( node->children[i] );
		}
	}
	delete node;
}

// -------------------------------------------------------------
// Description:
//   Deletes all data for duster cache.
// -------------------------------------------------------------
void Tr2InteriorDusterCache::Clear()
{
	if( m_root )
	{
		DeleteTree( m_root );
		m_root = NULL;
	}
	if( m_positions )
	{
		GEO_DELETE_ARRAY( XMVECTOR, m_positions );
		m_positions = NULL;
	}
	if( m_normals )
	{
		GEO_DELETE_ARRAY( XMVECTOR, m_normals );
		m_normals = NULL;
	}
	if( m_albedoes )
	{
		GEO_DELETE_ARRAY( XMVECTOR, m_albedoes );
		m_albedoes = NULL;
	}
	if( m_scratchMemory )
	{
		GEO_DELETE_ARRAY( XMVECTOR, m_scratchMemory );
		m_scratchMemory = NULL;
	}
	if( m_indexes )
	{
		delete[] m_indexes;
		m_indexes = NULL;
	}
}

// -------------------------------------------------------------
// Description:
//   Invalidate albedo array. Next time GetAlbedoes is called
//   it will set all albedo colors to an uninitialized value.
// -------------------------------------------------------------
void Tr2InteriorDusterCache::InvalidateAlbedo()
{
	m_albedoDirty = true;
}

// -------------------------------------------------------------
// Description:
//   Sets all albedos to an invalid value if the albedo dirty 
//   flag is set.
// -------------------------------------------------------------
void Tr2InteriorDusterCache::FillAlbedo()
{
	if( m_albedoDirty && m_albedoes )
	{
		XMVECTOR invalidAlbedo = GetUninitializedAlbedoValue();
		for( unsigned int i = 0; i < m_count; ++i )
		{
			m_albedoes[i] = invalidAlbedo;
		}
		m_albedoDirty = false;
	}
}

// -------------------------------------------------------------
// Description:
//   Returns an uninitialized albedo value. Users of GetAlbedo
//   function should check albedoes for this value to see if
//   it is uninitialized.
// Return Value:
//   Uninitialized albedo value
// -------------------------------------------------------------
XMVECTOR Tr2InteriorDusterCache::GetUninitializedAlbedoValue()
{
	return Vector4( -1.0f, -1.0f, -1.0f, -1.0f );
}

// -------------------------------------------------------------
// Description:
//   Adds a new Enlighten light source to the array (to be 
//   processed by Enlighten later during asyncronous update).
// Return Value:
//   Reference to a new Enlighten light source
// -------------------------------------------------------------
Enlighten::InputLight& Tr2InteriorDusterCache::AddEnlightenLightSource()
{
	if( int( m_lightCount ) >= m_lights.GetSize() )
	{
		m_lights.Resize( m_lights.GetSize() + 32 );
	}
	return m_lights[m_lightCount++];
}

// -------------------------------------------------------------
// Description:
//   Returns light falloff table for a given falloff power. Creates
//   a new table or reuses an existing one if it was requested
//   before.
// Arguments:
//   falloff - Light falloff power
// Return Value:
//   Pointer to Enlighten falloff table for given falloff power
// -------------------------------------------------------------
Enlighten::InputLightFalloffTable* Tr2InteriorDusterCache::GetFalloffTable( float falloff )
{
	std::map<float, Enlighten::InputLightFalloffTable*>::iterator it = m_falloffTables.find( falloff );
	if( it == m_falloffTables.end() )
	{
		Enlighten::InputLightFalloffTable* table = GEO_NEW( Enlighten::InputLightFalloffTable );

		for( Geo::s32 n = 0; n < Enlighten::InputLightFalloffTable::g_SampleCount; ++n ) 
		{ 
			float distance = Enlighten::InputLightFalloffTable::g_TableSampleIndices[n];
			table->m_Table[n] = 1.0f - pow( distance, falloff ); 
		}
		table->m_Table[0] = 0.0f;
		table->m_Table[Enlighten::InputLightFalloffTable::g_SampleCount - 1] = 1.0f;

		m_falloffTables[falloff] = table;
		return table;
	}
	return it->second;
}

// -------------------------------------------------------------
// Description:
//   Returns an array of Enlighten light sources to be 
//   processed by Enlighten later during asyncronous update.
// Return Value:
//   Array of Enlighten light sources
// -------------------------------------------------------------
const Enlighten::InputLight* Tr2InteriorDusterCache::GetLights() const
{
	return m_lights.Begin();
}

// -------------------------------------------------------------
// Description:
//   Returns the number of elements in array of Enlighten light  
//   source to be processed by Enlighten later during asyncronous 
//   update.
// Return Value:
//   Number of elements in array of Enlighten light sources
// -------------------------------------------------------------
unsigned int Tr2InteriorDusterCache::GetLightCount() const
{
	return m_lightCount;
}

// -------------------------------------------------------------
// Description:
//   Resets light source array and duster cache. Should be called
//   before adding duster data or light sources when updating
//   Enlighten lighting.
// -------------------------------------------------------------
void Tr2InteriorDusterCache::ClearLightData()
{
	m_lightCount = 0;
	memset( GetDusterMemory(), 0, sizeof( XMVECTOR ) * GetCount() );
}

#endif
