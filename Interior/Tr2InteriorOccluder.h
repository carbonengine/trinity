#pragma once
#ifndef Tr2InteriorOccluder_H
#define Tr2InteriorOccluder_H

// --------------------------------------------------------------------------------------
// Other forwards
namespace Umbra
{
	class Model;
	class Object;
	class Cell;
}

BLUE_DECLARE( Tr2InteriorOccluder );
BLUE_DECLARE( TriLineSet );
BLUE_DECLARE( Tr2InteriorCell );

class Tr2InteriorOccluder:
	public INotify
{
public:

	Tr2InteriorOccluder( IRoot* lockobj = 0 );
	~Tr2InteriorOccluder();

	EXPOSE_TO_BLUE();

	// Sets parent cell pointer
	void SetParentCell( Tr2InteriorCell* cell );

	// Returns bounding box in local coordinate system
	bool GetLocalBoundingBox( Vector3& min, Vector3& max ) const;
	// Returns bounding box in world coordinate system
	bool GetWorldBoundingBox( Vector3& min, Vector3& max ) const;


	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:
	//	Builds some umbra data
	void BuildUmbraData( Umbra::Cell* cell );

	//	Clears umbra data
	void ClearUmbraData();

	//	Modifies the transform
	void TransformModified();

	const Matrix& GetParentTransform() const;

	//	Creates a mesh model
	static Umbra::Model* GetMeshModel();

private:
	// The cell we are a member of
	Tr2InteriorCell* m_parentCell;

	// The umbra model used by our umbra object
	Umbra::Object*	m_umbraObject;

	// The transform of this object in the world
	Matrix			m_transform;

private:
	// The umbra model shared between all occluders
	static Umbra::Model*	s_umbraModel;

};

TYPEDEF_BLUECLASS( Tr2InteriorOccluder );
BLUE_DECLARE_VECTOR( Tr2InteriorOccluder );
#endif
