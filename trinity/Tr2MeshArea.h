#pragma once
#ifndef Tr2MeshArea_H
#define Tr2MeshArea_H

#include "Resources/Tr2LodResource.h"


BLUE_DECLARE( Tr2Material );
BLUE_DECLARE( Tr2MeshBase );
class Tr2RaytracingMeshArea;

BLUE_CLASS ( Tr2MeshArea ) :
	public IRoot
{
public:
	Tr2MeshArea( IRoot* lockobj = 0 );
	~Tr2MeshArea();

	Tr2MeshArea& operator=( const Tr2MeshArea& other );

	const std::string& GetName() const;
	void SetName( const std::string& name );

	int GetIndex() const;
	void SetIndex( int ix );

    int GetCount() const;
    void SetCount( int n );

    bool GetReversed() const;
    void SetReversed( bool reversed );

	bool GetDisplay() const;
	void SetDisplay( bool display );

	bool IsCastingShadows() const;
	void SetCastsShadows( bool castShadows );

	bool GetGenerateDepthArea() const;
	void SetGenerateDepthArea( bool generate );

	bool GetUseSHLighting() const;

	void SetMaterial( Tr2Material* mat );

	Tr2Material* GetMaterialInterface() const;

	unsigned int GetJointCount() const;
	void SetJointCount( unsigned int val );

	void AddOwnerMesh( Tr2MeshBase * mesh );
	void RemoveOwnerMesh( Tr2MeshBase * mesh );


	unsigned int* GetJointMappingAnimRig() const;
	
	// the provided array is NOT owned by this instance, it is owned by the parent mesh!
	// each mesharea gets a pointer on the same array
	void SetJointMappingAnimRig( unsigned int* val );

	bool IsReversed() const;

	Tr2Lod GetMinLod() const;
	void SetMinLod( Tr2Lod lod );

	Tr2RaytracingMeshArea* GetOrCreateRtMeshArea();
	Tr2RaytracingMeshArea* GetRtMeshArea() const;
private:
	Tr2MaterialPtr m_material;
	std::string m_name;
	int32_t m_index;
	int32_t m_count;

	Tr2Lod m_minLod; // minimal visible lod

	std::vector<Tr2MeshBase*> m_ownerMeshes;

	// mesh used for raytracing
	std::unique_ptr<Tr2RaytracingMeshArea> m_rtMeshArea;

	bool m_display;
	// Request reversed order of rendering triangles and reversed cull order 
	bool m_reversed;
	// Does this are require SH lighting instead of "normal" direct lighting
	bool m_useSHLighting;
	// in the near future of trinity the shader will know if we need to generate a depth area for it. for now, we have to keep this info
	bool m_generateDepthArea;
	// do we want this area to cast shadows?
	bool m_castShadows;

	unsigned int m_jointCount;
	unsigned int* m_jointMappingAnimRig;

public:
	EXPOSE_TO_BLUE();
};

TYPEDEF_BLUECLASS( Tr2MeshArea );
BLUE_DECLARE_VECTOR( Tr2MeshArea );

//
// Helpers for mesh area sorting
//
struct Tr2MeshAreaItem
{
	Tr2MeshArea* m_meshArea;
	float m_distance;
	bool operator<( const Tr2MeshAreaItem& other ) const
	{
		return m_distance > other.m_distance;
	}
};
typedef TrackableStdVector<Tr2MeshAreaItem> Tr2MeshAreaItemList;



#endif