// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2MeshProcedural_H
#define Tr2MeshProcedural_H

#include "Tr2MeshBase.h"

BLUE_CLASS( Tr2MeshProcedural ) :
	public Tr2MeshBase
{
public:
	EXPOSE_TO_BLUE();

	Tr2MeshProcedural( IRoot* lockobj = NULL );
	~Tr2MeshProcedural();

	Tr2GeometryProvider* GetGeometryResource() const override;
	void SetGeometryRes( Tr2GeometryProvider * res );

	bool IsLoading() const override;

private:
	int GetAreasCount() const;
	void PySetGeometryRes( Tr2GeometryProvider * geometryRes );

private:
	Tr2GeometryProviderPtr m_geometryResource;
};

TYPEDEF_BLUECLASS( Tr2MeshProcedural );
BLUE_DECLARE_VECTOR( Tr2MeshProcedural );

//
// Helpers for mesh sorting
//
struct Tr2MeshProceduralItem
{
	Tr2MeshProcedural* m_mesh;
	float m_distance;
	bool operator<( const Tr2MeshProceduralItem& other ) const
	{
		return m_distance > other.m_distance;
	}
};
typedef TrackableStdVector<Tr2MeshProceduralItem> Tr2MeshProceduralItemList;

#endif
