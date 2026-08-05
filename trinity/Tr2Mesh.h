// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2Mesh_H
#define Tr2Mesh_H

#include "Tr2MeshBase.h"
#include "Resources/Tr2LoadPrepareFence.h"


BLUE_DECLARE( Tr2SerializedMorphAnimation );

BLUE_CLASS( Tr2SerializedMorphAnimation ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	Tr2SerializedMorphAnimation( IRoot* lockobj = NULL ) :
		m_weight( 0.f ){};
	~Tr2SerializedMorphAnimation(){};

	std::string m_name;
	float m_weight;
};

TYPEDEF_BLUECLASS( Tr2SerializedMorphAnimation );
BLUE_DECLARE_VECTOR( Tr2SerializedMorphAnimation );


BLUE_CLASS( Tr2Mesh ) :
	public Tr2MeshBase,
	public IInitialize,
	public INotify,
	public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2Mesh( IRoot* lockobj = NULL );
	~Tr2Mesh();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	const char* GetMeshResPath() const
	{
		return m_meshResPath.c_str();
	}
	void SetMeshResPath( const char* path );

	const char* GetLowResMeshResPath() const;
	void SetLowResMeshResPath( const char* path );

	TriGeometryRes* GetGeometryResource() const override;
	void SetGeometryRes( TriGeometryRes * res );

	bool IsLoading() const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var * val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	virtual void ReleaseCachedData( BlueAsyncRes * p );
	virtual void RebuildCachedData( BlueAsyncRes * p );

	std::vector<std::string>* GetMorphTargetNames() const override;
	bool IsBakedMorph( int index ) const override;
	void SetMorphTargetWeight( const char* name, float weight ) override;
	float GetMorphTargetWeight( const char* name ) override;
	std::vector<bool>* GetAllBakedMorphTargetStates() const override;
	void SetBakedMorphTarget( const char* name, bool isBaked ) override;
	bool GetBakedMorphTarget( const char* name ) override;
	const std::unordered_map<std::string, Tr2MorphTargetAnimationData>& GetMorphAnimations() const override;

private:
	void InitializeGeometryResource();

	void InitializeMorphTargets();

	void PySetGeometryRes( TriGeometryRes * geometryRes );
	int GetAreasCount() const;
	void SetLowResGeometryRes( TriGeometryRes * res );

private:
	TriGeometryResPtr m_geometryResource;
	TriGeometryResPtr m_lowResGeometryResource;
	std::string m_meshResPath;
	std::string m_geomResourceEx;

	Tr2LoadPrepareFence m_loadFence;

	std::unordered_map<std::string, Tr2MorphTargetAnimationData> m_morphAnimations;
	PTr2SerializedMorphAnimationVector m_serializedMorphAnimations;

protected:
	bool m_deferGeometryLoad;
};

TYPEDEF_BLUECLASS( Tr2Mesh );
BLUE_DECLARE_VECTOR( Tr2Mesh );

//
// Helpers for mesh sorting
//
struct Tr2MeshItem
{
	Tr2Mesh* m_mesh;
	float m_distance;
	bool operator<( const Tr2MeshItem& other ) const
	{
		return m_distance > other.m_distance;
	}
};
typedef TrackableStdVector<Tr2MeshItem> Tr2MeshItemList;

#endif
