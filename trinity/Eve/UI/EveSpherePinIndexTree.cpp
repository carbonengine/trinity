#include "StdAfx.h"
#include "EveSpherePinIndexTree.h"
#include <vector>
#include "include/TriMath.h"
#include "Resources/TriGeometryRes.h"
#include "Shader/Tr2EffectStateManager.h"

const float PHI_MIN = -XM_PI;
const float PHI_MAX = XM_PI;

const float THETA_MAX = XM_PI * 0.5f;
const float THETA_MIN = -XM_PI * 0.5f;

struct EveSpherePinIndexTree::TreeNode
{
	TreeNode() :
		left( 0 ),
		right( 0 ),
		thetaMin( 0 ),
		thetaMax( 0 ),
		phiMin( 0 ),
		phiMax( 0 )
	{}

	float thetaMin, thetaMax, phiMin, phiMax;

	TreeNode* left;
	TreeNode* right;

	std::vector<Face*> faces;
};

struct EveSpherePinIndexTree::Face
{
	Face( ) : index1( 0 ), index2( 0 ),  index3( 0 ), flag( false ), radius( 0.f )
	{}

	Vector3 center;
	float radius;
	unsigned int index1, index2, index3;
		
	bool flag;
};

// ------------------------------------------------------------------------------------------------------
void CreateChildNodes( EveSpherePinIndexTree::TreeNode* node )
{
	node->left = new EveSpherePinIndexTree::TreeNode();
	node->right = new EveSpherePinIndexTree::TreeNode();

	if( ( node->phiMax - node->phiMin ) > ( node->thetaMax - node->thetaMin ) )
	{
		float medium = 0.5f * ( node->phiMax + node->phiMin );

		node->left->thetaMax = node->thetaMax;
		node->left->thetaMin = node->thetaMin;
		
		node->left->phiMin = node->phiMin;
		node->left->phiMax = medium;

		node->right->thetaMax = node->thetaMax;
		node->right->thetaMin = node->thetaMin;
		
		node->right->phiMin = medium;
		node->right->phiMax = node->phiMax;
	}
	else
	{
		float medium = 0.5f * ( node->thetaMax + node->thetaMin );

		node->left->thetaMax = medium;
		node->left->thetaMin = node->thetaMin;
		
		node->left->phiMin = node->phiMin;
		node->left->phiMax = node->phiMax;

		node->right->thetaMax = node->thetaMax;
		node->right->thetaMin = medium;
		
		node->right->phiMin = node->phiMin;
		node->right->phiMax = node->phiMax;
	}
}

// ------------------------------------------------------------------------------------------------------
EveSpherePinIndexTree::TreeNode* CreateTree( EveSpherePinIndexTree::TreeNode* node, int levels )
{
	if( !node )
	{
		EveSpherePinIndexTree::TreeNode* tree = new EveSpherePinIndexTree::TreeNode();
		tree->phiMin = PHI_MIN;
		tree->phiMax = PHI_MAX;

		tree->thetaMax = THETA_MAX;
		tree->thetaMin = THETA_MIN;

		node = tree;
	}

	--levels;
	if( levels )
	{
		CreateChildNodes( node );

		CreateTree( node->left, levels );
		CreateTree( node->right, levels );
	}

	return node;
}

// ------------------------------------------------------------------------------------------------------
inline void CarthesianToSpherical( Vector3& carth, Vector2& spherical )
{	// spherical = Vector2( theta, phi )
	float radius = sqrt( Dot( carth, carth ) );
	spherical.x = XM_PI / 2.0f - acos( carth.y / radius );
	spherical.y = atan2( -carth.z, carth.x );
}

// ------------------------------------------------------------------------------------------------------
bool ExtractVertices( TriGeometryResMeshData* mesh, std::vector<Vector2>& sphericalVerts, std::vector<Vector3>& verts, Tr2RenderContext& renderContext )
{
	if( !mesh->m_vertexBuffer.IsValid() )
	{
		return 0;
	}

	const uint8_t* pVertices;

	int numVerts = mesh->m_vertexCount;
	int vertSize = mesh->m_bytesPerVertex;

	sphericalVerts.resize(numVerts);
	verts.resize(numVerts);
	if (FAILED(mesh->m_vertexBuffer.MapForReading( pVertices, renderContext )))
	{
		return 0;
	}
	ON_BLOCK_EXIT( [&]{ mesh->m_vertexBuffer.UnmapForReading( renderContext ); } );

	Tr2VertexDefinition vd;
	if ( !Tr2EffectStateManager::GetVertexDeclarationElements( mesh->m_vertexDeclaration, vd ) )
	{	
		return false;
	}

	auto position = vd.Find( vd.POSITION );
	if( !position )
	{
		return false;
	}

	for ( int i = 0; i < numVerts; i++ )
	{
		Vector3 p1;
		if( position->m_dataType == vd.FLOAT16_4 )
		{
			p1 = *reinterpret_cast<const Vector3_16*>( pVertices + position->m_offset + i * vertSize );
		}
		else
		{
			p1 = *reinterpret_cast<const Vector3*>( pVertices + position->m_offset + i * vertSize );
		}
		verts[i] = p1;
		CarthesianToSpherical( p1, sphericalVerts[i] );
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
EveSpherePinIndexTree::Face* ExtractFaceData( TriGeometryResMeshData* mesh, std::vector<Vector3>& verts, Tr2RenderContext& renderContext )
{
	if( !mesh->m_indexBuffer.IsValid() )
	{
		return 0;
	}

	const unsigned short* pShortIndices = nullptr;
	const unsigned int  * pLongIndices = nullptr;
	
	if( mesh->m_indexBuffer.GetDesc().stride == 2 )
	{
		if (FAILED( mesh->m_indexBuffer.MapForReading( pShortIndices, renderContext )))
		{		
			mesh->m_vertexBuffer.UnmapForReading( renderContext );
			return 0;
		}
	}
	else
	{
		if (FAILED( mesh->m_indexBuffer.MapForReading( pLongIndices, renderContext )))
		{		
			mesh->m_vertexBuffer.UnmapForReading( renderContext );
			return 0;
		}
	}
	ON_BLOCK_EXIT( [&]{ mesh->m_indexBuffer.UnmapForReading( renderContext ); } );

	int numPrim = mesh->m_primitiveCount;
	EveSpherePinIndexTree::Face* faces = new EveSpherePinIndexTree::Face[numPrim];

	for ( int i = 0; i < numPrim; i++ )
	{	
		unsigned int index1 = 0;
		unsigned int index2 = 0;
		unsigned int index3 = 0;

		if( pShortIndices )
		{
			index1 = pShortIndices[i*3];
			index2 = pShortIndices[(i*3)+1];
			index3 = pShortIndices[(i*3)+2];
		}
		else
		{
			index1 = pLongIndices[i*3];
			index2 = pLongIndices[(i*3)+1];
			index3 = pLongIndices[(i*3)+2];
		}

		faces[i].index1 = index1;
		faces[i].index2 = index2;
		faces[i].index3 = index3;

		faces[i].center.x = 
			( min( min( verts[index1].x, verts[index2].x ), verts[index3].x ) +
			max( max( verts[index1].x, verts[index2].x ), verts[index3].x ) ) / 2.0f;
		faces[i].center.y = 
			( min( min( verts[index1].y, verts[index2].y ), verts[index3].y ) +
			max( max( verts[index1].y, verts[index2].y ), verts[index3].y ) ) / 2.0f;
		faces[i].center.z = 
			( min( min( verts[index1].z, verts[index2].z ), verts[index3].z ) +
			max( max( verts[index1].z, verts[index2].z ), verts[index3].z ) ) / 2.0f;
		
		Vector3 d1(verts[index1] - faces[i].center);
		Vector3 d2(verts[index2] - faces[i].center);
		Vector3 d3(verts[index3] - faces[i].center);
		faces[i].radius = Length( d1 );
		faces[i].radius = max( faces[i].radius, Length( d2 ) );
		faces[i].radius = max( faces[i].radius, Length( d3 ) );
	}

	return faces;
}

// ------------------------------------------------------------------------------------------------------
int OverlapTest( EveSpherePinIndexTree::TreeNode* n, EveSpherePinIndexTree::Face* f, std::vector<Vector2>& vertices )
{
	float maxTheta = max( vertices[f->index1].x, max( vertices[f->index2].x, vertices[f->index3].x) );
	float minTheta = min( vertices[f->index1].x, min( vertices[f->index2].x, vertices[f->index3].x) );
	float maxPhi = max( vertices[f->index1].y, max( vertices[f->index2].y, vertices[f->index3].y) );
	float minPhi = min( vertices[f->index1].y, min( vertices[f->index2].y, vertices[f->index3].y) );
	
	if( (maxPhi - minPhi) > XM_PI )
	{
		float temp = minPhi;
		minPhi = maxPhi;
		maxPhi = temp + 2*XM_PI;
	}

	int phiOverlap = 0;
	if( minPhi < n->phiMin && maxPhi > n->phiMin )
	{
		phiOverlap = 1;
	}
	else if( minPhi < n->phiMax && minPhi >= n->phiMin  )
	{
		phiOverlap = 1;
	}

	int thetaOverlap = 0;
	if( minTheta < n->thetaMin && maxTheta > n->thetaMin )
	{
		thetaOverlap = 1;
	}
	else if( minTheta < n->thetaMax && minTheta >= n->thetaMin )
	{
		thetaOverlap = 1;
	}

	return thetaOverlap && phiOverlap;
}

// ------------------------------------------------------------------------------------------------------
int OverlapTest( EveSpherePinIndexTree::TreeNode* n, float minTheta, float maxTheta, float minPhi, float maxPhi )
{ 
	int phiOverlap = 0;
	if( minPhi < n->phiMin && maxPhi > n->phiMin )
	{
		phiOverlap = 1;
	}
	else if( minPhi < n->phiMax && minPhi >= n->phiMin  )
	{
		phiOverlap = 1;
	}

	int thetaOverlap = 0;
	if( minTheta < n->thetaMin && maxTheta > n->thetaMin )
	{
		thetaOverlap = 1;
	}
	else if( minTheta < n->thetaMax && minTheta >= n->thetaMin )
	{
		thetaOverlap = 1;
	}

	return thetaOverlap && phiOverlap;
}

// ------------------------------------------------------------------------------------------------------
void AddFaceToTree( EveSpherePinIndexTree::TreeNode* node, EveSpherePinIndexTree::Face* face, std::vector<Vector2>& vertices )
{
	if( node->left && node->right )
	{
		if( OverlapTest( node->left, face, vertices ) )
		{
			AddFaceToTree( node->left, face, vertices );
		}
		if( OverlapTest( node->right, face, vertices ) )
		{
			AddFaceToTree( node->right, face, vertices );
		}

		return;
	}

	node->faces.push_back( face );
}

void ClearTree( EveSpherePinIndexTree::TreeNode* node )
{
	if( node->left )
	{
		ClearTree( node->left );
	}
	if( node->right )
	{
		ClearTree( node->right );
	}

	delete node;
}

// ------------------------------------------------------------------------------------------------------
EveSpherePinIndexTree::EveSpherePinIndexTree(void) :
	m_initialized( 0 ),
	m_tree( 0 ),
	m_faces( 0 )
{
}

// ------------------------------------------------------------------------------------------------------
EveSpherePinIndexTree::~EveSpherePinIndexTree(void)
{
	if( m_tree )
	{
		ClearTree( m_tree );
		m_tree = 0;
	}

	if( m_faces )
	{
		delete m_faces;
		m_faces = 0;
	}
}

// ------------------------------------------------------------------------------------------------------
int EveSpherePinIndexTree::Initialize( TriGeometryResMeshData *mesh )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	m_initialized = 0;

	if( !mesh )
	{
		return 0;
	}

	int triangleCount = mesh->m_primitiveCount;

	if( m_tree )
	{
		ClearTree( m_tree );
		m_tree = 0;
	}
	
	if( m_faces )
	{
		delete m_faces;
		m_faces = 0;
	}

	m_tree = CreateTree( 0, 9 );

	std::vector<Vector2> sphericalVerts;
	std::vector<Vector3> verts;
	ExtractVertices( mesh, sphericalVerts, verts, renderContext );

	m_faces = ExtractFaceData( mesh, verts, renderContext );

	for( int i = 0; i < triangleCount; i++ )
	{
		AddFaceToTree( m_tree, &m_faces[i], sphericalVerts );
	}

	m_initialized = 1;
	return 1;
}

// ------------------------------------------------------------------------------------------------------
int EveSpherePinIndexTree::MarkFaces( EveSpherePinIndexTree::TreeNode* node, float minTheta, float maxTheta, float minPhi, float maxPhi )
{
	int faceCount = 0;
	if( node->left && node->right )
	{
		if( OverlapTest( node->left, minTheta, maxTheta, minPhi, maxPhi ) )
		{
			faceCount += MarkFaces( node->left, minTheta, maxTheta, minPhi, maxPhi );
		}
		if( OverlapTest( node->right, minTheta, maxTheta, minPhi, maxPhi ) )
		{
			faceCount += MarkFaces( node->right, minTheta, maxTheta, minPhi, maxPhi );
		}

		return faceCount;
	}
	
	for( std::vector<Face*>::const_iterator it = node->faces.begin(); it != node->faces.end(); ++it )
	{
		if( !((*it)->flag) )
		{
			(*it)->flag = 1;
			faceCount++;
			m_markedFaces.push_back( *it );
		}
	}

	return faceCount;
}

// ------------------------------------------------------------------------------------------------------
int EveSpherePinIndexTree::GetIndices( Vector3& point, float radius, int& primitives, std::vector<unsigned short>& indices )
{
	if( !m_initialized )
	{
		return 0;
	}

	Vector3 pole(0,1,0);

	Vector2 p, phiMinSC, phiMaxSC;
	CarthesianToSpherical( point, p );
	
	float minTheta = p.x - radius;
	float maxTheta = p.x + radius;
	float minPhi;
	float maxPhi;

	Matrix zRotation;

	if( p.x >= 0 ) // Upper hemisphere
	{
		if( acos( Dot( pole, point ) ) < radius )
		{
			minPhi = PHI_MIN;
			maxPhi = PHI_MAX;
			maxTheta = THETA_MAX;
		}
		else
		{
			minPhi = p.y - radius / cos( maxTheta );
			maxPhi = p.y + radius / cos( maxTheta );
		}
	}
	else
	{
		if( acos( -Dot( pole, point ) ) < radius )
		{
			minPhi = PHI_MIN;
			maxPhi = PHI_MAX;
			minTheta = THETA_MIN;
		}
		else
		{
			minPhi = p.y - radius / cos( minTheta );
			maxPhi = p.y + radius / cos( minTheta );
		}
	}
	
	if( maxPhi > PHI_MAX )
	{
		primitives = MarkFaces( m_tree, minTheta, maxTheta, minPhi, PHI_MAX ) + MarkFaces( m_tree, minTheta, maxTheta, PHI_MIN, maxPhi - 2 * XM_PI );
	}
	else if( minPhi < PHI_MIN )
	{
		primitives = MarkFaces( m_tree, minTheta, maxTheta, 2 * XM_PI + minPhi, PHI_MAX ) + MarkFaces( m_tree, minTheta, maxTheta, PHI_MIN, maxPhi );
	}
	else
	{
		primitives = MarkFaces( m_tree, minTheta, maxTheta, minPhi, maxPhi );
	}

	indices.resize( primitives * 3 );
	int rejected = 0;
	
	for( int i = 0; i < primitives; i++ )
	{
		Face* face = m_markedFaces[i]; 
		face->flag = 0;
		Vector3 d(face->center - point );
		float len = Length( d );

		if( len <= radius + face->radius )
		{
			indices[(i - rejected) * 3] = face->index1;
			indices[(i - rejected) * 3 + 1] = face->index2;
			indices[(i - rejected) * 3 + 2] = face->index3;
		}
		else
		{
			rejected++;
		}
	}

	primitives -= rejected;
	m_markedFaces.clear();

	return 1;
}