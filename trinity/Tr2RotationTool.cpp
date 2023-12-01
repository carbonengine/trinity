#include "StdAfx.h"
#include "Tr2RotationTool.h"
#include "Tr2PrimitiveSet.h"
#include "Tr2LineSet.h"
#include "Shader/Tr2Effect.h"

Tr2RotationTool::Tr2RotationTool( IRoot* lockobj )
	:m_precision( 1 ),
	m_rotation( IdentityQuaternion() ),
	m_angle( 0.0f )
{
	GenLineSets();
	SelectAxis( "w" );
}


void Tr2RotationTool::Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix )
{
	Vector3 pos;
	Vector3 normal;
	Vector3 ray;
	Vector3 startPos;
	
	pos.x = m_localTransform._41;
	pos.y = m_localTransform._42;
	pos.z = m_localTransform._43;

	// Get the pick ray from the tools screen location
	ScreenCoordinatesToRay( mouseX, mouseY, ray, startPos, viewport, viewMatrix, projectionMatrix );

	// Get the normal for the best plane to use
	normal = GetDesiredPlaneNormal( ray, viewMatrix );

	// Tracking a point in 3d space based on the screen positions
	ScreenCoordinatesToRay( mouseX-mouseXDelta, mouseY-mouseYDelta, ray, startPos, viewport, viewMatrix, projectionMatrix );
	Vector3 startPlanePos = RayToPlaneIntersection( startPos, ray, pos, normal );

	ScreenCoordinatesToRay( mouseX, mouseY, ray, startPos, viewport, viewMatrix, projectionMatrix );
	Vector3 endPlanePos = RayToPlaneIntersection( startPos, ray, pos, normal );

	Vector3 start, end;
	start = Normalize( startPlanePos - pos );
	end = Normalize( endPlanePos - pos );

	float dot = Dot( start, end );
	
	if(( 1.0f - dot ) < FLT_EPSILON)
	{
		return;
	}

	float angleSign = 1.0f;
	Vector3 dnormal = Cross( start, end );
	
	Vector3 xAxis;
	Vector3 yAxis;
	Vector3 zAxis;
	GetBaseVectors( xAxis, yAxis, zAxis );

	if( m_selectedAxis == "w" )
	{
		Matrix worldInv;
		Vector3 viewVec;
		viewVec.x = viewMatrix._13;
		viewVec.y = viewMatrix._23;
		viewVec.z = viewMatrix._33;

		worldInv = Inverse( m_localTransform );
		m_movement = TransformNormal( viewVec, worldInv );
		float rdot = Dot( m_movement, viewVec );
		float ddot = Dot( dnormal, m_movement );
		if( ( ddot < 0.0f && rdot > 0.0f ) || ( ddot > 0.0f && rdot < 0.0f ) )
		{
			//If the angle between the view vector in model space and view vector in world space is more then +-PI/2.0 we reverse the rotation angle,
			//but only if the angle between the cross-product of the start and end points and the target rotation axis is positive.
			angleSign *= -1.0f;
		}
	}
	else if( m_selectedAxis == "ww" )
	{
		Vector3 curP = Hemisphere( mouseX, mouseY, viewport, viewMatrix, projectionMatrix );
		Vector3 preP = Hemisphere( mouseX-mouseXDelta, mouseY-mouseYDelta, viewport, viewMatrix, projectionMatrix );
		// Get the target rotation axis
		Vector3 norm = Cross( preP, curP );
		// Since the hemisphere vectors are in view space we have to make sure 
		// the translation of camera is undone     
		Matrix invView = Inverse( viewMatrix );
		norm = TransformNormal( norm, invView );
		Matrix worldInv = Inverse( m_localTransform );
		m_movement = TransformNormal( norm, worldInv );

		dot = Dot( curP, preP );
	}
	else
	{
		Vector3 untransformedXAxis;
		Vector3 untransformedYAxis;
		Vector3 untransformedZAxis;
		GetUnTransformedBaseVectors( untransformedXAxis, untransformedYAxis, untransformedZAxis );

		Vector3 tmp;
		if( m_selectedAxis == "x" )
		{
			tmp = xAxis;
			m_movement = untransformedXAxis;
		}
		else if( m_selectedAxis == "y" )
		{
			tmp = yAxis;
			m_movement = untransformedYAxis;
		}
		else
		{
			tmp = zAxis;
			m_movement = untransformedZAxis;
		}
		
		// Reverse the axis if the cross product of the start and end vectors is pointing away from the target rotation axis
		// dnormal is in world, so axis must be too
		if( Dot( dnormal, tmp ) < 0.0f )
		{
			angleSign *= -1.0f;
		}
	}
	m_angle = acos( dot ) * angleSign * m_precision;
	m_moved = true;
}

Vector3 Tr2RotationTool::GetDesiredPlaneNormal( Vector3& ray, Matrix& viewMatrix )
{
	// What is the best plane to work on when moving the tool
	// Since the eye could lie on one of the valid planes
	Vector3 xAxis;
	Vector3 yAxis;
	Vector3 zAxis;
	Vector3 view;
	view.x = viewMatrix._13;
	view.y = viewMatrix._23;
	view.z = viewMatrix._33;

	Vector3 norm;
	GetBaseVectors( xAxis, yAxis, zAxis );
	if( m_selectedAxis == "w" || m_selectedAxis == "ww" )
	{
		norm = view;
	}
	else
	{
		Vector3 axis;
		if( m_selectedAxis == "x" )
		{
			axis = xAxis;
		}
		else if( m_selectedAxis == "y" )
		{
			axis = yAxis;
		}
		else
		{
			axis = zAxis;
		}
		norm = axis;
		if( Dot( view, norm )  > 0.0f )
		{
			norm = -norm;
		}
	} 
	norm = Normalize( norm );
	return norm;
}

std::vector<ITr2Renderable*>& Tr2RotationTool::GetPrimitivesToRender( )
{
	m_visibleObjects.clear();
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		m_visibleObjects.push_back((*it));
	}
	return m_visibleObjects;
}

void Tr2RotationTool::ResetPrimitiveColors()
{
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		if((*it)->m_name == "x" )
		{
			(*it)->SetCurrentColor((Color&)v4Red);
		}
		else if((*it)->m_name == "y" )
		{
			(*it)->SetCurrentColor((Color&)v4Green);
		}
		else if((*it)->m_name == "z" )
		{
			(*it)->SetCurrentColor((Color&)v4Blue);
		}
		else if((*it)->m_name == "w" )
		{
			(*it)->SetCurrentColor((Color&)v4Cyan);
		}
		else if((*it)->m_name == "ww" )
		{
			(*it)->SetCurrentColor((Color&)v4LightGray);
		}
	}
}

void Tr2RotationTool::GetUnTransformedBaseVectors( Vector3& x, Vector3& y, Vector3& z )
{
	x = Vector3( 1.0f, 0.0f, 0.0f );
	y = Vector3( 0.0f, 1.0f, 0.0f );
	z = Vector3( 0.0f, 0.0f, 1.0f );
}

void Tr2RotationTool::GenLineSets()
{
	Matrix rotateMat;
	Vector3* c_tris = NULL;
	Vector3* p_tris = NULL;
	int numVectors = 0;

	Tr2EffectPtr mSPHLineEffect;
	Tr2EffectPtr mLineEffect;
	Tr2EffectPtr pEffect;


	if( !m_xLine	.CreateInstance()	 ||
		!m_yLine	.CreateInstance()	 ||
		!m_zLine	.CreateInstance()	 ||
		!m_wLine	.CreateInstance()	 ||
		!m_wwLine	.CreateInstance()	 ||
		!mLineEffect.CreateInstance()	 ||
		!mSPHLineEffect.CreateInstance() ||
		!pEffect	.CreateInstance() )
	{
		return;
	}

	mSPHLineEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Utility/LinesRotationTool.fx");
	mLineEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Utility/LinesNoZ.fx");
	pEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Utility/PrimitivePicking.fx" );

	// wwLine
	c_tris = Tr2ManipulationTool::GetCirclePoints(1.0f, 60, &numVectors);
	for( int i = 0; i < numVectors/2; i++ )
	{
		m_wwLine->AddLine( c_tris[i*2], v4LightGray, c_tris[i*2+1], v4LightGray );
	}
	delete [] c_tris;

	c_tris = Tr2ManipulationTool::GetCircleTriangles(1.0f, 60, &numVectors);
	for( int i = 0; i < numVectors/3; i++ )
	{
		m_wwLine->AddPickingTriangle( c_tris[i*3], c_tris[i*3+1], c_tris[i*3+2] );
	}
	delete [] c_tris;
	m_wwLine->SubmitChanges();
	m_wwLine->m_name = "ww";
	m_wwLine->m_scaleByDistanceToView = true;
	m_wwLine->m_effect = mLineEffect;
	m_wwLine->m_pickEffect = pEffect;
	m_wwLine->m_viewOriented = true;

	// wLine
	c_tris = Tr2ManipulationTool::GetCirclePoints(1.2f, 60, &numVectors);
	for( int i = 0; i < numVectors/2; i++ )
	{
		m_wLine->AddLine( c_tris[i*2], v4Cyan, c_tris[i*2+1], v4Cyan );
		int numVectorsB = 0;
		p_tris = Tr2ManipulationTool::GetTrianglesAroundLine( c_tris[i*2], c_tris[i*2+1], 0.08f, &numVectorsB);
		for( int i = 0; i < numVectorsB/3; i++ )
		{
			m_wLine->AddPickingTriangle( p_tris[i*3], p_tris[i*3+1], p_tris[i*3+2] );
		}		
		delete [] p_tris;
	}	
	delete [] c_tris;


	m_wLine->SubmitChanges();
	m_wLine->m_name = "w";
	m_wLine->m_scaleByDistanceToView = true;
	m_wLine->m_effect = mLineEffect;
	m_wLine->m_pickEffect = pEffect;
	m_wLine->m_viewOriented = true;

	// xLine
	c_tris = Tr2ManipulationTool::GetCirclePoints(1.0f, 60, &numVectors);
	rotateMat = RotationYMatrix( XM_PI / 2.0f );
	TransformCoords( c_tris, numVectors, rotateMat );

	for( int i = 0; i < numVectors/2; i++ )
	{
		m_xLine->AddLine( c_tris[i*2], v4Red, c_tris[i*2+1], v4Red );
		int numVectorsB = 0;
		p_tris = Tr2ManipulationTool::GetTrianglesAroundLine( c_tris[i*2], c_tris[i*2+1], 0.08f, &numVectorsB);
		for( int i = 0; i < numVectorsB/3; i++ )
		{
			m_xLine->AddPickingTriangle( p_tris[i*3], p_tris[i*3+1], p_tris[i*3+2] );
		}		
		delete [] p_tris;
	}	
	delete [] c_tris;


	m_xLine->SubmitChanges();
	m_xLine->m_name = "x";
	m_xLine->m_scaleByDistanceToView = true;
	m_xLine->m_effect = mSPHLineEffect;
	m_xLine->m_pickEffect = pEffect;

	// yLine
	c_tris = Tr2ManipulationTool::GetCirclePoints(1.0f, 60, &numVectors);
	rotateMat = RotationXMatrix( XM_PI / 2.0f );
	TransformCoords( c_tris, numVectors, rotateMat );
	for( int i = 0; i < numVectors/2; i++ )
	{
		m_yLine->AddLine( c_tris[i*2], v4Green, c_tris[i*2+1], v4Green );
		int numVectorsB = 0;
		p_tris = Tr2ManipulationTool::GetTrianglesAroundLine( c_tris[i*2], c_tris[i*2+1], 0.08f, &numVectorsB);
		for( int i = 0; i < numVectorsB/3; i++ )
		{
			m_yLine->AddPickingTriangle( p_tris[i*3], p_tris[i*3+1], p_tris[i*3+2] );
		}	
		delete [] p_tris;
	}	
	delete [] c_tris;


	m_yLine->SubmitChanges();
	m_yLine->m_name = "y";
	m_yLine->m_scaleByDistanceToView = true;
	m_yLine->m_effect = mSPHLineEffect;
	m_yLine->m_pickEffect = pEffect;

	// zLine
	c_tris = Tr2ManipulationTool::GetCirclePoints(1.0f, 60, &numVectors);	
	for( int i = 0; i < numVectors/2; i++ )
	{
		m_zLine->AddLine( c_tris[i*2], v4Blue, c_tris[i*2+1], v4Blue );
		int numVectorsB = 0;
		p_tris = Tr2ManipulationTool::GetTrianglesAroundLine( c_tris[i*2], c_tris[i*2+1], 0.08f, &numVectorsB);
		for( int i = 0; i < numVectorsB/3; i++ )
		{
			m_zLine->AddPickingTriangle( p_tris[i*3], p_tris[i*3+1], p_tris[i*3+2] );
		}		
		delete [] p_tris;
	}	
	delete [] c_tris;


	m_zLine->SubmitChanges();
	m_zLine->m_name = "z";
	m_zLine->m_scaleByDistanceToView = true;
	m_zLine->m_effect = mSPHLineEffect;
	m_zLine->m_pickEffect = pEffect;

	m_primitives.Insert( -1, m_wwLine->GetRawRoot());
	m_primitives.Insert( -1, m_wLine->GetRawRoot());
	m_primitives.Insert( -1, m_xLine->GetRawRoot());
	m_primitives.Insert( -1, m_yLine->GetRawRoot());
	m_primitives.Insert( -1, m_zLine->GetRawRoot());
}

void Tr2RotationTool::Update()
{
	if( m_moved )
	{
		Matrix temp;
		Matrix rotation = RotationMatrix( m_movement, m_angle );
		temp = rotation * m_localTransform;

		// Check the move callback for if we should be moving or not
		if( OnMoveCallback( m_localTransform, temp  ))
		{
			m_localTransform = temp;
			m_rotation = RotationQuaternion( temp );
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
		(*it)->m_localTransform = m_localTransform;
		(*it)->UpdateTransform();
	}
}

Vector3 Tr2RotationTool::Hemisphere( int mouseX, int mouseY, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix ) const
{
	Matrix viewProj = viewMatrix * projectionMatrix;

	auto project = [&]( Vector3 in ) -> Vector2
	{
		Vector4 out = Vector4( in.x, in.y, in.z, 1 ) * viewProj;
			
		return Vector2(		viewport.m_x + viewport.m_width  * ( 0.5f + 0.5f * out.x / out.w ),
							viewport.m_y + viewport.m_height * ( 0.5f - 0.5f * out.y / out.w ) );
	};

	float radius = m_wwLine->m_scale;
	
	Vector3 center, viewSide;
	center.x = m_worldTransform._41;
	center.y = m_worldTransform._42;
	center.z = m_worldTransform._43;

	viewSide.x = viewMatrix._11 * radius;
	viewSide.y = viewMatrix._21 * radius;
	viewSide.z = viewMatrix._31 * radius;

	Vector3 side = center - viewSide;
	
	Vector2 screenCenter = project( center );
	Vector2 screenSide   = project( side );
		
	Vector2 dxy = screenCenter - screenSide;
	float radius_pixels = sqrtf( dxy.x * dxy.x + dxy.y * dxy.y ) + 16;	// add the width of the cursor


	float px = (mouseX - screenCenter.x) / radius_pixels;
	float py = (screenCenter.y - mouseY) / radius_pixels;
	float d = sqrt( px*px + py*py );
	float z = 0.0f;
    if( d <= 1.0f )
	{
	    z = 1.0f - d;
    }

	Vector3 result( px, py, z );

	return Vector3( XMVector3Normalize( result ) );
}
