////////////////////////////////////////////////////////////////////////////////
//
// Created:		August 2014
// Copyright:	CCP 2014
//

#include "StdAfx.h"
#include "Tr2MeshLod.h"
#include "Resources/TriGeometryRes.h"


Tr2MeshLod::Tr2MeshLod( IRoot* lockobj /*= NULL */ ) :
	PARENTLOCK( m_associatedResources )
{

}

Tr2MeshLod::~Tr2MeshLod()
{

}

void Tr2MeshLod::SelectLod( Tr2Lod lod )
{
	if( m_selectedLod == lod )
	{
		return;
	}

	m_geometryRes->SelectLod( lod );
	for( auto it = m_associatedResources.begin(); it != m_associatedResources.end(); ++it )
	{
		(*it)->SelectLod( lod );
	}
	m_selectedLod = lod;
}

void Tr2MeshLod::AddAssociatedResource( Tr2LodResource* lr )
{
	m_associatedResources.Append( lr );
}

void Tr2MeshLod::RemoveAssociatedResource( Tr2LodResource* lr )
{
	auto key = m_associatedResources.FindKey( lr );
	if( key != -1 )
	{
		m_associatedResources.Remove( key );
	}
}

void Tr2MeshLod::ClearAssociatedResources()
{
	m_associatedResources.Remove( -1 );
}

TriGeometryRes* Tr2MeshLod::GetGeometryResource() const
{
	if( m_geometryRes )
	{
		return dynamic_cast<TriGeometryRes*>( m_geometryRes->GetResource() );
	}

	return nullptr;
}

void Tr2MeshLod::SetGeometryResource( Tr2LodResource* lodResource )
{
	m_geometryRes = lodResource;
}
