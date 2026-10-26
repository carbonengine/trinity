// Copyright © 2026 CCP ehf.

#pragma once
#ifndef Tr2MeshProcedural_h
#define Tr2MeshProcedural_h

#include "Tr2MeshBase.h"
#include "Resources/Tr2ProceduralGeometryProvider.h"

BLUE_CLASS_IMPL( Tr2MeshProcedural )
class Tr2MeshProcedural : public Tr2MeshBase,
						  public IBlueAsyncResNotifyTarget
{
public:
	EXPOSE_TO_BLUE();

	Tr2MeshProcedural( IRoot* lockobj = NULL );
	~Tr2MeshProcedural();

	Tr2GeometryProvider* GetGeometryResource() const override;
	Tr2ProceduralGeometryProvider* GetGeometry();
	void SetGeometryResource( Tr2ProceduralGeometryProvider* res );

	bool IsLoading() const override;

	/////////////////////////////////////////////////////////////////////////////////////
	// IBlueAsyncResNotifyTarget
	void ReleaseCachedData( BlueAsyncRes* p ) override;
	void RebuildCachedData( BlueAsyncRes* p ) override;

private:
	Tr2ProceduralGeometryProviderPtr m_geometryResource;
};

TYPEDEF_BLUECLASS( Tr2MeshProcedural );
BLUE_DECLARE_VECTOR( Tr2MeshProcedural );

#endif
