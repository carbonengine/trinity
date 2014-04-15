#pragma once
#ifndef Tr2MeshArea_H
#define Tr2MeshArea_H


#include "Tr2HideableMixin.h"

BLUE_DECLARE_INTERFACE( ITr2ShaderMaterial );

BLUE_CLASS ( Tr2MeshArea ) :
	public IRoot, 
	public Tr2HideableMixin
{
public:
	Tr2MeshArea( IRoot* lockobj = 0 );
	~Tr2MeshArea();

	Tr2MeshArea& operator=( const Tr2MeshArea& other );

	const std::string& GetName() const;
	int GetIndex() const;

	void SetIndex( int ix );

    int GetCount() const;

    void SetCount( int n );

    bool GetReversed() const;

    void SetReversed( bool reversed );

	bool GetUseSHLighting() const;

	void SetMaterial( ITr2ShaderMaterial* mat );

	void SetName( const std::string& name );

	ITr2ShaderMaterial* GetMaterialInterface() const;

	unsigned int GetJointCount() const;
	
	void SetJointCount( unsigned int val );


	unsigned int* GetJointMappingAnimRig() const;
	
	// the provided array is NOT owned by this instance, it is owned by the parent mesh!
	// each mesharea gets a pointer on the same array
	void SetJointMappingAnimRig( unsigned int* val );

	bool IsReversed() const;


private:
	ITr2ShaderMaterialPtr m_material;
	std::string m_name;
	int m_index;
    int m_count;
	// Request reversed order of rendering triangles and reversed cull order 
	bool m_reversed;
	// Does this are require SH lighting instead of "normal" direct lighting
	bool m_useSHLighting;

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
