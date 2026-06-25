// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Tr2TranslationTool.h"
#include "Tr2PrimitiveSet.h"
#include "Tr2SolidSet.h"
#include "Tr2LineSet.h"
#include "Shader/Tr2Effect.h"

Tr2TranslationTool::Tr2TranslationTool( IRoot* lockobj ) :
	Tr2ManipulationTool( lockobj )
{
	GenLineSets();
	SelectAxis( "w" );
	m_translation.x = 0.0f;
	m_translation.y = 0.0f;
	m_translation.z = 0.0f;
}

void Tr2TranslationTool::Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix )
{
	/*
		The movement is always based on the delta mouse changes and the current translation tools screen coordinates.

		We first project the tools location to screen coordinates, then we use that as the relative screen point for the mouse movement.
		If there is a move callback we check the return value to see if the movement was legal or not.
	*/
	//

	Vector3 pos;
	Vector2 screenPos;
	Vector3 normal;
	Vector3 ray;
	Vector3 startPos;

	pos.x = m_localTransform._41;
	pos.y = m_localTransform._42;
	pos.z = m_localTransform._43;

	// We might be in world or local
	Vector3 xAxis;
	Vector3 yAxis;
	Vector3 zAxis;
	GetBaseVectors( xAxis, yAxis, zAxis );

	// Get the location of the tool on screen
	PointToScreenCoordinates( pos, screenPos, viewport, viewMatrix, projectionMatrix );
	int curMouseX = (int)screenPos.x;
	int curMouseY = (int)screenPos.y;
	// Get the pick ray from the tools screen location
	ScreenCoordinatesToRay( curMouseX, curMouseY, ray, startPos, viewport, viewMatrix, projectionMatrix );

	// Get the normal for the best plane to use
	normal = GetDesiredPlaneNormal( ray, viewMatrix );

	Vector3 delta = MovePointOnPlane( curMouseX, curMouseY, curMouseX + mouseXDelta, curMouseY + mouseYDelta, pos, normal, viewport, viewMatrix, projectionMatrix );

	Vector3 translate;
	m_moved = true;
	if( m_selectedAxis == "w" )
	{
		translate = delta;
	}
	else if( m_selectedAxis == "x" )
	{
		float scale = Dot( delta, xAxis );
		translate = xAxis * scale;
	}
	else if( m_selectedAxis == "y" )
	{
		float scale = Dot( delta, yAxis );
		translate = yAxis * scale;
	}
	else if( m_selectedAxis == "z" )
	{
		float scale = Dot( delta, zAxis );
		translate = zAxis * scale;
	}
	else
	{
		return;
	}

	m_movement.x += translate.x;
	m_movement.y += translate.y;
	m_movement.z += translate.z;
}

std::vector<ITr2Renderable*>& Tr2TranslationTool::GetPrimitivesToRender()
{
	// Depending on the mode of the tool. We want to control which parts of it will
	// be rendered
	m_visibleObjects.clear();
	if( m_captured )
	{
		// When we have it captured. We only display the currently selected axis and the center
		// ring
		for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
		{
			if( ( *it )->m_name == "w" || ( *it )->m_name == m_selectedAxis )
			{
				m_visibleObjects.push_back( ( *it ) );
			}
		}
	}
	else
	{
		for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
		{
			m_visibleObjects.push_back( ( *it ) );
		}
	}
	return m_visibleObjects;
}

void Tr2TranslationTool::ResetPrimitiveColors()
{
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		if( ( *it )->m_name == "x" )
		{
			( *it )->SetCurrentColor( (Color&)v4Red );
		}
		else if( ( *it )->m_name == "y" )
		{
			( *it )->SetCurrentColor( (Color&)v4Green );
		}
		else if( ( *it )->m_name == "z" )
		{
			( *it )->SetCurrentColor( (Color&)v4Blue );
		}
		else if( ( *it )->m_name == "w" )
		{
			( *it )->SetCurrentColor( (Color&)v4Cyan );
		}
	}
}

void Tr2TranslationTool::GenLineSets()
{
	/*
		TODO.
		This creates the cones, lines and collision geometry.
		There are some hard coded paths in this that we might want to replace. 
		Vector constants and magic numbers that would be nice to remove.
	*/
	Matrix rotateMat, translation, transform;
	rotateMat = RotationZMatrix( -XM_PI / 2.0f );
	translation = TranslationMatrix( 0.0f, 1.0f, 0.0f );
	transform = translation * rotateMat;
	Vector3* c_tris = NULL;
	int numVectors = 0;
	int subDivisions = 20;

	Tr2SolidSetPtr xCone;
	Tr2SolidSetPtr yCone;
	Tr2SolidSetPtr zCone;
	Tr2LineSetPtr xLine;
	Tr2LineSetPtr yLine;
	Tr2LineSetPtr zLine;
	Tr2LineSetPtr wLine;
	Tr2EffectPtr mSolidEffect;
	Tr2EffectPtr mLineEffect;
	Tr2EffectPtr pEffect;

	if( !xCone.CreateInstance() ||
		!yCone.CreateInstance() ||
		!zCone.CreateInstance() ||
		!xLine.CreateInstance() ||
		!yLine.CreateInstance() ||
		!zLine.CreateInstance() ||
		!wLine.CreateInstance() ||
		!pEffect.CreateInstance() ||
		!mLineEffect.CreateInstance() ||
		!mSolidEffect.CreateInstance() )
	{
		return;
	}

	mSolidEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Utility/SolidsNoZ.fx" );
	mLineEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Utility/LinesNoZ.fx" );
	pEffect->SetEffectPathName( "res:/Graphics/Effect/Managed/Utility/PrimitivePicking.fx" );

	xLine->AddLine( v3Zero, v4Red, v3X, v4Red );
	c_tris = Tr2ManipulationTool::GetTrianglesAroundLine( Vector3( 0.3f, 0.0f, 0.0f ), (Vector3&)v3X, 0.08f, &numVectors );
	for( int i = 0; i < numVectors / 3; i++ )
	{
		xLine->AddPickingTriangle( c_tris[i * 3], c_tris[i * 3 + 1], c_tris[i * 3 + 2] );
	}
	delete[] c_tris;

	xLine->SubmitChanges();
	xLine->m_name = "x";

	yLine->AddLine( v3Zero, v4Green, v3Y, v4Green );
	c_tris = Tr2ManipulationTool::GetTrianglesAroundLine( Vector3( 0.0f, 0.3f, 0.0f ), (Vector3&)v3Y, 0.08f, &numVectors );
	for( int i = 0; i < numVectors / 3; i++ )
	{
		yLine->AddPickingTriangle( c_tris[i * 3], c_tris[i * 3 + 1], c_tris[i * 3 + 2] );
	}
	delete[] c_tris;
	yLine->SubmitChanges();
	yLine->m_name = "y";

	zLine->AddLine( v3Zero, v4Blue, v3Z, v4Blue );
	c_tris = Tr2ManipulationTool::GetTrianglesAroundLine( Vector3( 0.0f, 0.0f, 0.3f ), (Vector3&)v3Z, 0.08f, &numVectors );
	for( int i = 0; i < numVectors / 3; i++ )
	{
		zLine->AddPickingTriangle( c_tris[i * 3], c_tris[i * 3 + 1], c_tris[i * 3 + 2] );
	}
	delete[] c_tris;
	zLine->SubmitChanges();
	zLine->m_name = "z";

	xLine->m_scaleByDistanceToView = yLine->m_scaleByDistanceToView = zLine->m_scaleByDistanceToView = true;
	xLine->m_effect = yLine->m_effect = zLine->m_effect = mLineEffect;
	xLine->m_pickEffect = yLine->m_pickEffect = zLine->m_pickEffect = pEffect;

	//xCone
	c_tris = Tr2ManipulationTool::GetConeTriangles( 0.25f, 0.075f, subDivisions, &numVectors );
	TransformCoords( c_tris, numVectors, transform );

	for( int i = 0; i < numVectors / 3; i++ )
	{
		xCone->AddTriangle( c_tris[i * 3], v4Red, c_tris[i * 3 + 1], v4Red, c_tris[i * 3 + 2], v4Red );
	}
	delete[] c_tris;

	xCone->SubmitChanges();
	xCone->m_name = "x";
	xCone->m_scaleByDistanceToView = true;
	xCone->m_effect = mSolidEffect;
	xCone->m_pickEffect = pEffect;

	m_primitives.Insert( -1, xCone->GetRawRoot() );

	//yCone
	c_tris = Tr2ManipulationTool::GetConeTriangles( 0.25f, 0.075f, subDivisions, &numVectors );
	TransformCoords( c_tris, numVectors, translation );

	for( int i = 0; i < numVectors / 3; i++ )
	{
		yCone->AddTriangle( c_tris[i * 3], v4Green, c_tris[i * 3 + 1], v4Green, c_tris[i * 3 + 2], v4Green );
	}
	delete[] c_tris;

	yCone->SubmitChanges();
	yCone->m_name = "y";
	yCone->m_scaleByDistanceToView = true;
	yCone->m_effect = mSolidEffect;
	yCone->m_pickEffect = pEffect;

	m_primitives.Insert( -1, yCone->GetRawRoot() );

	//zCone
	c_tris = Tr2ManipulationTool::GetConeTriangles( 0.25f, 0.075f, subDivisions, &numVectors );
	rotateMat = RotationXMatrix( XM_PI / 2.0f );
	transform = translation * rotateMat;
	TransformCoords( c_tris, numVectors, transform );

	for( int i = 0; i < numVectors / 3; i++ )
	{
		zCone->AddTriangle( c_tris[i * 3], v4Blue, c_tris[i * 3 + 1], v4Blue, c_tris[i * 3 + 2], v4Blue );
	}
	delete[] c_tris;

	zCone->SubmitChanges();
	zCone->m_name = "z";
	zCone->m_scaleByDistanceToView = true;
	zCone->m_effect = mSolidEffect;
	zCone->m_pickEffect = pEffect;

	m_primitives.Insert( -1, zCone->GetRawRoot() );

	// wLines
	c_tris = Tr2ManipulationTool::GetCirclePoints( 0.25f, subDivisions, &numVectors );
	for( int i = 0; i < numVectors / 2; i++ )
	{
		wLine->AddLine( c_tris[i * 2], v4Cyan, c_tris[i * 2 + 1], v4Cyan );
	}
	delete[] c_tris;

	c_tris = Tr2ManipulationTool::GetCircleTriangles( 0.25f, subDivisions, &numVectors );
	for( int i = 0; i < numVectors / 3; i++ )
	{
		wLine->AddPickingTriangle( c_tris[i * 3], c_tris[i * 3 + 1], c_tris[i * 3 + 2] );
	}
	delete[] c_tris;
	wLine->SubmitChanges();
	wLine->m_name = "w";
	wLine->m_scaleByDistanceToView = true;
	wLine->m_effect = mLineEffect;
	wLine->m_pickEffect = pEffect;
	wLine->m_viewOriented = true;

	m_primitives.Insert( -1, wLine->GetRawRoot() );
	m_primitives.Insert( -1, xLine->GetRawRoot() );
	m_primitives.Insert( -1, yLine->GetRawRoot() );
	m_primitives.Insert( -1, zLine->GetRawRoot() );
}

void Tr2TranslationTool::Update()
{
	if( m_moved )
	{
		Matrix temp;
		Matrix translation = TranslationMatrix( m_movement );
		temp = m_localTransform * translation;
		// Check the move callback for if we should be moving or not
		if( OnMoveCallback( m_localTransform, temp ) )
		{
			m_localTransform = temp;
			m_translation.x = m_localTransform._41;
			m_translation.y = m_localTransform._42;
			m_translation.z = m_localTransform._43;
			m_movement.x = 0.0f;
			m_movement.y = 0.0f;
			m_movement.z = 0.0f;
		}
		m_moved = false;
	}

	Matrix translation = TranslationMatrix( m_pivot );
	m_worldTransform = m_localTransform * translation;
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		( *it )->m_localTransform = m_localTransform;
		( *it )->UpdateTransform();
	}
}
