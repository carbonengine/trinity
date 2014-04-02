////////////////////////////////////////////////////////////
//
//    Created:   July 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2InteriorDusterCache_H
#define Tr2InteriorDusterCache_H


namespace Enlighten
{
class InputWorkspace;
class InputLightingBuffer;
class InputLight;
class InputLightFalloffTable;
}

// -------------------------------------------------------------
// Description:
//   Tr2InteriorDusterCache class contains a cache of Enlighten
//   system duster points. It is used for non-standard light 
//   sources to directly color dusters in the system. The class
//   maintains an octree to speedup searching for dusters in
//   a given bounding box. An instance of this class is 
//   maintained by Tr2InteriorEnlightenSystem.
// SeeAlso:
//   Tr2InteriorEnlightenSystem, Tr2InteriorBoxLight
// -------------------------------------------------------------
class Tr2InteriorDusterCache
{
public:
	Tr2InteriorDusterCache();
	~Tr2InteriorDusterCache();

	void Initialize( const Enlighten::InputWorkspace* inputWorkspace, 
					 const Enlighten::InputLightingBuffer* lightingBuffer );
	void Clear();
	void InvalidateAlbedo();

	// Get number of duster points
	unsigned int GetCount() const { return m_count; }
	// Get an array of duster points positions (GetCount() elements) 
	const XMVECTOR* GetPositions() const { return m_positions; }
	// Get an array of duster points normals (GetCount() elements) 
	const XMVECTOR* GetNormals() const { return m_normals; }
	// Get an array of duster points albedo color (GetCount() elements) 
	// Albedo colors are lazily evaluated by users of this class.
	// Uninitialized values are set to (-1, -1, -1, -1).
	XMVECTOR* GetAlbedoes() { FillAlbedo(); return m_albedoes; }
	// Returns an array of scratch memory that could be used by
	// user classes (GetCount() elements) 
	XMVECTOR* GetDusterMemory() { return m_scratchMemory; }

	// -------------------------------------------------------------
	// Description:
	//   Walk through an octree calling the given function for each
	//   leaf that intersects a given bounding box.
	// Arguments:
	//   minBounds - Minimum point of bounding box to intersect
	//               nodes with.
	//   maxBounds - Maximum point of bounding box to intersect
	//               nodes with.
	//   function - Functor that is called for each octree leaf that
	//				intersects the given bounding box. Functor is 
	//				assumed to have the declaration:
	//				void function( const unsigned int *indexes, unsigned int count )
	//				where indexes is an array of duster indexes that 
	//				potentially intersect the bounding box and count
	//				is the number of elements in this array.
	// -------------------------------------------------------------
	template <typename Function>
	void ProcessDustersInBounds( const Vector3& minBounds, const Vector3& maxBounds, Function &function )
	{
		if( m_root )
		{
			ProcessDustersInBounds( m_root, m_minBounds, m_maxBounds, minBounds, maxBounds, function );
		}
	}

	Enlighten::InputLight& AddEnlightenLightSource();
	Enlighten::InputLightFalloffTable* GetFalloffTable( float falloff );
	const Enlighten::InputLight* GetLights() const;
	unsigned int GetLightCount() const;
	void ClearLightData();

	static XMVECTOR GetUninitializedAlbedoValue();
private:
	// Octree node structure
	struct Node
	{
		// Node's center point
		Vector3 center;
		// Offset of node's duster indexes in m_indexes array
		unsigned int start;
		// Number of duster points
		unsigned int count;
		// Children nodes
		Node* children[8];
	};

	// -------------------------------------------------------------
	// Description:
	//   Walk through an octree calling the given function for each
	//   leaf that intersects a given bounding box.
	// Arguments:
	//   node - Current octree node to examine
	//   nodeMinBounds - Node's minimum point of bounding box 
	//   nodeMaxBounds - Node's maximum point of bounding box 
	//   minBounds - Minimum point of bounding box to intersect
	//               nodes with.
	//   maxBounds - Maximum point of bounding box to intersect
	//               nodes with.
	//   function - Functor that is called for each octree leaf that
	//				intersects the given bounding box. Functor is 
	//				assumed to have the declaration:
	//				void function( const unsigned int *indexes, unsigned int count )
	//				where indexes is an array of duster indexes that 
	//				potentially intersect the bounding box and count
	//				is the number of elements in this array.
	// -------------------------------------------------------------
	template <typename Function>
	void ProcessDustersInBounds( Node* node, 
								 const Vector3& nodeMinBounds, 
								 const Vector3& nodeMaxBounds, 
								 const Vector3& minBounds, 
								 const Vector3& maxBounds, 
								 Function &function ) const
	{
		if( nodeMinBounds.x > maxBounds.x || nodeMaxBounds.x < minBounds.x || 
			nodeMinBounds.y > maxBounds.y || nodeMaxBounds.y < minBounds.y || 
			nodeMinBounds.z > maxBounds.z || nodeMaxBounds.z < minBounds.z )
		{
			return;
		}
		if( node->children[0] )
		{
			ProcessDustersInBounds( node->children[0], 
									nodeMinBounds, 
									node->center, 
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[1], 
									Vector3( node->center.x, nodeMinBounds.y, nodeMinBounds.z ), 
									Vector3( nodeMaxBounds.x, node->center.y, node->center.z ),
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[2], 
									Vector3( nodeMinBounds.x, node->center.y, nodeMinBounds.z ), 
									Vector3( node->center.x, nodeMaxBounds.y, node->center.z ),
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[3], 
									Vector3( node->center.x, node->center.y, nodeMinBounds.z ), 
									Vector3( nodeMaxBounds.x, nodeMaxBounds.y, node->center.z ),
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[4],
									Vector3( nodeMinBounds.x, nodeMinBounds.y, node->center.z ), 
									Vector3( node->center.x, node->center.y, nodeMaxBounds.z ),
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[5], 
									Vector3( node->center.x, nodeMinBounds.y, node->center.z ), 
									Vector3( nodeMaxBounds.x, node->center.y, nodeMaxBounds.z ),
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[6],
									Vector3( nodeMinBounds.x, node->center.y, node->center.z ), 
									Vector3( node->center.x, nodeMaxBounds.y, nodeMaxBounds.z ),
									minBounds, 
									maxBounds, 
									function );
			ProcessDustersInBounds( node->children[7], 
									node->center, 
									nodeMaxBounds,
									minBounds, 
									maxBounds, 
									function );
		}
		else
		{
			function( m_indexes + node->start, node->count );
		}
	}

	void BuildTree( Node* node, unsigned int depth, const Vector3& minBounds, const Vector3& maxBounds );
	void DeleteTree( Node* node );
	void FillAlbedo();

	static const unsigned MAX_DUSTERS_PER_NODE;
	static const unsigned MAX_TREE_DEPTH;

	// Octree root
	Node* m_root;
	// Array of duster positions
	XMVECTOR *m_positions;
	// Array of duster normals
	XMVECTOR *m_normals;
	// Array of duster albedoes
	XMVECTOR *m_albedoes;
	// Scratch memory
	XMVECTOR *m_scratchMemory;
	// System's min bounds
	Vector3 m_minBounds;
	// System's max bounds
	Vector3 m_maxBounds;
	// Indexes into dusters array (used by octree)
	unsigned int *m_indexes;
	// Number of dusters
	unsigned int m_count;
	// Dirty flag for albedo array
	bool m_albedoDirty;
	// Array of Enlighten light sources
	Geo::GeoArray<Enlighten::InputLight> m_lights;
	// Number of actual light sources in the array
	unsigned int m_lightCount;
	// Light falloff tables indexed by falloff power
	std::map<float, Enlighten::InputLightFalloffTable*> m_falloffTables;
};


#endif // Tr2InteriorDusterCache_H