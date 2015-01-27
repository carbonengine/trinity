#include "StdAfx.h"

#include "Tr2InteriorMirror.h"
#include "Tr2Renderer.h"

Tr2InteriorMirror::Tr2InteriorMirror() :
	m_warpMatrixFront( Tr2Renderer::GetIdentityTransform() ),
	m_warpMatrixBack( Tr2Renderer::GetIdentityTransform() ),
	m_transformMatrix( Tr2Renderer::GetIdentityTransform() ),
	m_meshIndex( 0 ),
	m_areaIndex( 0 ),
	m_mirrorIndex( 0 ),
	m_minBounds( 0.0f, 0.0f, 0.0f ),
	m_maxBounds( 0.0f, 0.0f, 0.0f ),
	m_placeable( NULL )
{
}

Tr2InteriorMirror::~Tr2InteriorMirror()
{
}

int Tr2InteriorMirror::GetMeshIndex( void ) const
{
	return m_meshIndex;
}

int Tr2InteriorMirror::GetAreaIndex( void ) const
{
	return m_areaIndex;
}

const Matrix& Tr2InteriorMirror::GetWarpMatrixFront( void ) const
{
	return m_warpMatrixFront;
}

const Matrix& Tr2InteriorMirror::GetWarpMatrixBack( void ) const
{
	return m_warpMatrixBack;
}

const Matrix& Tr2InteriorMirror::GetTransformMatrix( void ) const
{
	return m_transformMatrix;
}

void Tr2InteriorMirror::SetPlaceable( Tr2InteriorPlaceable* placeable )
{
	m_placeable = placeable;
}

Tr2InteriorPlaceable* Tr2InteriorMirror::GetPlaceable( void ) const
{
	return m_placeable;
}

int Tr2InteriorMirror::GetMirrorIndex( void ) const
{
	return m_mirrorIndex;
}

void Tr2InteriorMirror::SetMirrorIndex( int index )
{
	m_mirrorIndex = index;
}

void Tr2InteriorMirror::SetMeshIndex( int index )
{
	m_meshIndex = index;
}

void Tr2InteriorMirror::SetAreaIndex( int index )
{
	m_areaIndex = index;
}

void Tr2InteriorMirror::SetWarpMatrixFront( const Matrix& warpMatrix )
{
	m_warpMatrixFront = warpMatrix;
}

void Tr2InteriorMirror::SetWarpMatrixBack( const Matrix& warpMatrix )
{
	m_warpMatrixBack = warpMatrix;
}

void Tr2InteriorMirror::SetTransformMatrix( const Matrix& transformMatrix )
{
	m_transformMatrix = transformMatrix;
}

void Tr2InteriorMirror::SetBoundingBox( const Vector3& minBounds, const Vector3& maxBounds )
{
	m_minBounds = minBounds;
	m_maxBounds = maxBounds;
}

