// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "TriView.h"
#include "Tr2Renderer.h"

// --------------------------------------------------------------------------------------
// Description:
//   Constructor.  Initializes the view matrix to the identity.
// --------------------------------------------------------------------------------------
TriView::TriView( void ) :
	m_transform( XMMatrixIdentity() )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the view matrix.
// Arguments:
//   m - The new view matrix
// --------------------------------------------------------------------------------------
void TriView::SetTransform( const Matrix& m )
{
	m_transform = m;
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the current view matrix.
// Return Value:
//   The view matrix
// --------------------------------------------------------------------------------------
const Matrix& TriView::GetTransform() const
{
	return m_transform;
}

// --------------------------------------------------------------------------------------
// Description:
//   Constructs a look-at view matrix from the given eye position, target position, and
//   up vector.  This function is handedness-aware.
// Arguments:
//   eye - The view position in world-space
//   at - The view-target position in world-space
//   up - The up direction in world-space
// --------------------------------------------------------------------------------------
void TriView::SetLookAtPosition( const Vector3& eye, const Vector3& at, const Vector3& up )
{
	m_transform = XMMatrixLookAtRH( eye, at, up );
}
