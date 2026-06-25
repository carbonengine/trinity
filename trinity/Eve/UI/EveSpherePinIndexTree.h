// Copyright © 2023 CCP ehf.

#pragma once
#ifndef EveSpherePinIndexTree_H
#define EveSpherePinIndexTree_H

BLUE_DECLARE( TriGrannyRes );


class EveSpherePinIndexTree
{
public:
	struct Face;
	struct TreeNode;

	explicit EveSpherePinIndexTree( TriGrannyRes* granny );
	~EveSpherePinIndexTree( void );

	int Initialize();
	int GetIndices( Vector3& point, float radius, int& primitives, std::vector<unsigned short>& indices );

	int IsInitialized()
	{
		return m_initialized;
	}

private:
	TriGrannyResPtr m_granny;

	// A spherically partitioned binary tree of all the faces in the source geometry.
	TreeNode* m_tree;

	// A list of all the faces in the source geometry.
	Face* m_faces;

	int m_initialized;

	std::vector<Face*> m_markedFaces;

	int MarkFaces( TreeNode* node, float minTheta, float maxTheta, float minPhi, float maxPhi );
};

#endif