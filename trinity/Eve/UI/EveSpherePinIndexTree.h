#pragma once
#ifndef EveSpherePinIndexTree_H
#define EveSpherePinIndexTree_H

struct TriGeometryResMeshData;

class EveSpherePinIndexTree
{
public:
	struct Face;
	struct TreeNode;

	EveSpherePinIndexTree(void);
	~EveSpherePinIndexTree(void);

	int Initialize( TriGeometryResMeshData* mesh );
	int GetIndices( Vector3& point, float radius, int& primitives, std::vector<unsigned short>& indices );

	int IsInitialized() { return m_initialized; }
private:
	// A spherically partitioned binary tree of all the faces in the source geometry.
	TreeNode* m_tree;

	// A list of all the faces in the source geometry.
	Face* m_faces;

	int m_initialized;
	
	std::vector<Face*> m_markedFaces;

	int MarkFaces( TreeNode* node, float minTheta, float maxTheta, float minPhi, float maxPhi );
};

#endif