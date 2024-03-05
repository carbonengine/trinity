#include "StdAfx.h"
#include "Tr2ScalingTool.h"
#include "Tr2SolidSet.h"
#include "Tr2LineSet.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Renderer.h"

extern float g_primitiveDistanceScaleMultiplier;

namespace
{

Vector3 TransformPoint( const Vector3& localCenter, Tr2SolidSet* solid, Matrix& viewMatrix )
{
	XMVECTOR det;
	Matrix viewInv( XMMatrixInverse( &det, viewMatrix ) );
	
	Vector3 viewPos = viewInv.GetTranslation();
	Vector3 lineSetPos = Vector3( solid->m_localTransform._41, solid->m_localTransform._42, solid->m_localTransform._43 );
	Vector3 normal = Vector3( viewMatrix._13, viewMatrix._23, viewMatrix._33 );
	Vector3 dir( viewPos - lineSetPos );
	float scale = fabs( Dot( dir, normal ) * g_primitiveDistanceScaleMultiplier * Tr2Renderer::GetFieldOfView() );

	Matrix scaleMat, finalTransform;
	scaleMat = ScalingMatrix( scale, scale, scale );
	finalTransform = scaleMat * solid->m_localTransform;

	return Vector3( XMVector3TransformCoord( localCenter, finalTransform ) );
}

}

Tr2ScalingTool::Tr2ScalingTool( IRoot* lockobj )
	:m_initialLength( 1.0f ),
	m_scale( 1.0f, 1.0f, 1.0f ),
	m_initialScale( 1.0f, 1.0f, 1.0f )
{
	GenLineSets();
	SelectAxis("w");

	Init(m_localTransform);

}

void Tr2ScalingTool::Move( int mouseX, int mouseY, int mouseXDelta, int mouseYDelta, Tr2Viewport& viewport, Matrix& viewMatrix, Matrix& projectionMatrix )
{
	Vector3 pos;
	Vector2 screenPos;
	Vector3 normal;
	Vector3 ray;
	Vector3 startPos;
	pos.x = m_localTransform._41;
	pos.y = m_localTransform._42;
	pos.z = m_localTransform._43;

	
	if( m_initialLength < 0.0 )
	{
        Vector3 vecResult;
        Vector3 vecCenterOfMass = TransformPoint( Vector3( 1, 0, 0 ), m_xBox, viewMatrix );
        float tmpInitialLength = Length( pos - vecCenterOfMass );
		// the initiallength could not have been set up...
		m_initialLength = tmpInitialLength;
	}


	// We might be in world or local
	Vector3 xAxis;
	Vector3 yAxis;
	Vector3 zAxis;
	GetBaseVectors( xAxis, yAxis, zAxis );

	// Get the location of the tool on screen
	PointToScreenCoordinates(pos, screenPos, viewport, viewMatrix, projectionMatrix );
	int curMouseX = int( screenPos.x );
	int curMouseY = int( screenPos.y );
	// Get the pick ray from the tools screen location
	ScreenCoordinatesToRay(curMouseX, curMouseY, ray, startPos, viewport, viewMatrix, projectionMatrix );

	// Get the normal for the best plane to use
	normal = GetDesiredPlaneNormal( ray, viewMatrix );
	Vector3 delta = MovePointOnPlane(curMouseX, curMouseY, curMouseX+mouseXDelta, curMouseY+mouseYDelta, pos, normal, viewport, viewMatrix, projectionMatrix );

	Vector3 translate;
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

	{
		Matrix temp;
		// Get x y z vectors, ... scale them by the length and apply to each component
		// We might be in world or local
		Vector3 xAxis;
		Vector3 yAxis;
		Vector3 zAxis;
		GetBaseVectors( xAxis, yAxis, zAxis );

		if( m_selectedAxis == "w" )
		{
			//
			Vector3 vecA, vecB, vecC;
			vecA.x = m_worldTransform._41;
			vecA.y = m_worldTransform._42;
			vecA.z = m_worldTransform._43;

			vecB = TransformPoint( Vector3( 1, 0, 0 ), m_xBox, viewMatrix );

			vecC = vecA - vecB;
			float delta = Length( m_movement );
			float length = Length( vecC );
			float scale = length / m_initialLength;

			if( mouseXDelta < 0 )
			{
				delta = -delta;
			}

			if( Dot( vecC, xAxis ) >= 0.0f )
			{
				scale = -scale;
			}

			// x
			Vector3 translate;
			Matrix translation;
			translate = xAxis * delta;
			translation = TranslationMatrix( translate );
			m_xBox->m_localTransform = m_xBox->m_localTransform * translation;

			// y
			translate = yAxis * delta;
			translation = TranslationMatrix( translate );
			m_yBox->m_localTransform = m_yBox->m_localTransform * translation;

			// z
			translate = zAxis * delta;
			translation = TranslationMatrix( translate );
			m_zBox->m_localTransform = m_zBox->m_localTransform * translation;

			Matrix scaleA, scaleB;
			scaleA = ScalingMatrix( m_scale.x, m_scale.y, m_scale.z );

			Vector3 nextScale = m_scale;

			nextScale.x = ( m_initialScale.x*scale );
			nextScale.y = ( m_initialScale.y*scale );
			nextScale.z = ( m_initialScale.z*scale );

			scaleB = ScalingMatrix( nextScale.x, nextScale.y, nextScale.z );
			// Check the move callback for if we should be moving or not
			if( OnMoveCallback( scaleA, scaleB ) )
			{
				m_scale = nextScale;
				m_movement.x = 0.0f;
				m_movement.y = 0.0f;
				m_movement.z = 0.0f;
			}
		}
		else
		{
			Vector3 axis, center;
			Tr2SolidSet* selectedPrimitive = NULL;
			if( m_selectedAxis == "x" )
			{
				axis = xAxis;
				selectedPrimitive = m_xBox;
				center = Vector3( 1, 0, 0 );
			}
			else if( m_selectedAxis == "y" )
			{
				axis = yAxis;
				selectedPrimitive = m_yBox;
				center = Vector3( 0, 1, 0 );
			}
			else
			{
				axis = zAxis;
				selectedPrimitive = m_zBox;
				center = Vector3( 0, 0, 1 );
			}

			Vector3 vecA, vecB, vecC;
			vecA.x = m_worldTransform._41;
			vecA.y = m_worldTransform._42;
			vecA.z = m_worldTransform._43;

			vecB = TransformPoint( center, selectedPrimitive, viewMatrix );

			vecC = vecA - vecB;

			float l1 = Length( vecC );
			float scale = l1 / m_initialLength;

			Matrix scaleA, scaleB;
			scaleA = ScalingMatrix( m_scale.x, m_scale.y, m_scale.z );

			if( Dot( vecC, axis ) >= 0.0f )
			{
				scale = -scale;
			}

			Matrix translation;
			Vector3 nextScale = m_scale;
			if( m_selectedAxis == "x" )
			{
				nextScale.x = ( m_initialScale.x*scale );
			}
			else if( m_selectedAxis == "y" )
			{
				nextScale.y = ( m_initialScale.y*scale );
			}
			else if( m_selectedAxis == "z" )
			{
				nextScale.z = ( m_initialScale.z*scale );
			}
			scaleB = ScalingMatrix( nextScale.x, nextScale.y, nextScale.z );

			translation = TranslationMatrix( m_movement );
			selectedPrimitive->m_localTransform = selectedPrimitive->m_localTransform * translation;
			// Check the move callback for if we should be moving or not
			if( OnMoveCallback( scaleA, scaleB ) )
			{
				m_scale = nextScale;
				m_movement.x = 0.0f;
				m_movement.y = 0.0f;
				m_movement.z = 0.0f;
			}
		}
	}
}

std::vector<ITr2Renderable*>& Tr2ScalingTool::GetPrimitivesToRender( )
{
	// Depending on the mode of the tool. We want to control which parts of it will
	// be rendered
	m_visibleObjects.clear();
	if( !m_captured || m_selectedAxis == "w" )
	{
		for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
		{
			m_visibleObjects.push_back((*it));	
		}
	}
	else
	{
		// When we have it captured. We only display the currently selected axis and the center
		// ring
		for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
		{
			if((*it)->m_name == "w" || (*it)->m_name == m_selectedAxis
				|| (*it)->m_name == "_"+m_selectedAxis )
			{
				m_visibleObjects.push_back((*it));	
			}			
		}		
	}
	return m_visibleObjects;
}

void Tr2ScalingTool::ResetPrimitiveColors()
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
	}
}

void Tr2ScalingTool::GenLineSets()
{
	Matrix translationMat;
	Vector3* c_tris;
	int numVectors = 0;

	Tr2EffectPtr mSolidEffect;
	Tr2EffectPtr mLineEffect;
	Tr2EffectPtr pEffect;

	if( !m_xLine	.CreateInstance()	 ||
		!m_yLine	.CreateInstance()	 ||
		!m_zLine	.CreateInstance()	 ||
		!m_xBox		.CreateInstance()	 ||
		!m_yBox		.CreateInstance()	 ||
		!m_zBox		.CreateInstance()	 ||
		!m_wBox		.CreateInstance()	 ||
		!mLineEffect.CreateInstance()	 ||
		!mSolidEffect.CreateInstance()	 ||
		!pEffect	.CreateInstance() )
	{
		return;
	}
		
	mLineEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Utility/LinesNoZ.fx");
	mSolidEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Utility/SolidsNoZ.fx");
	pEffect->SetEffectPathName("res:/Graphics/Effect/Managed/Utility/PrimitivePicking.fx" );

	m_xLine->AddLine( v3Zero, v4LightGray,  v3X, v4LightGray );
	m_xLine->SubmitChanges();
	m_xLine->m_name = "_x";

	m_yLine->AddLine( v3Zero, v4LightGray,  v3Y, v4LightGray );	
	m_yLine->SubmitChanges();
	m_yLine->m_name = "_y";

	m_zLine->AddLine( v3Zero, v4LightGray,  v3Z, v4LightGray );	
	m_zLine->SubmitChanges();
	m_zLine->m_name = "_z";

	m_xLine->m_scaleByDistanceToView = m_yLine->m_scaleByDistanceToView = m_zLine->m_scaleByDistanceToView = false;
	m_xLine->m_effect = m_yLine->m_effect = m_zLine->m_effect = mLineEffect;
	m_xLine->m_pickEffect = m_yLine->m_pickEffect = m_zLine->m_pickEffect = pEffect;

	//xBox
	c_tris = Tr2ManipulationTool::GetBoxTriangles( Vector3( -0.075f, -0.075f, -0.075f ),Vector3( 0.075f, 0.075f, 0.075f ), &numVectors );
	translationMat = TranslationMatrix( 1.0f, 0.0f, 0.0f );
	TransformCoords( c_tris, numVectors, translationMat );

	for( int i = 0; i < numVectors/3; i++ )
	{
		m_xBox->AddTriangle(c_tris[i*3], v4Red, c_tris[i*3+1], v4Red, c_tris[i*3+2], v4Red );
	}        
	delete [] c_tris;

	m_xBox->SubmitChanges();
	m_xBox->m_name = "x";
	m_xBox->m_scaleByDistanceToView = true;
	m_xBox->m_effect = mSolidEffect;
	m_xBox->m_pickEffect = pEffect;

	//yBox
	c_tris = Tr2ManipulationTool::GetBoxTriangles( Vector3( -0.075f, -0.075f, -0.075f ),Vector3( 0.075f, 0.075f, 0.075f ), &numVectors );
	translationMat = TranslationMatrix( 0.0f, 1.0f, 0.0f );
	TransformCoords( c_tris, numVectors, translationMat );

	for( int i = 0; i < numVectors/3; i++ )
	{
		m_yBox->AddTriangle(c_tris[i*3], v4Green, c_tris[i*3+1], v4Green, c_tris[i*3+2], v4Green );
	}        
	delete [] c_tris;

	m_yBox->SubmitChanges();
	m_yBox->m_name = "y";
	m_yBox->m_scaleByDistanceToView = true;
	m_yBox->m_effect = mSolidEffect;
	m_yBox->m_pickEffect = pEffect;

	//yBox
	c_tris = Tr2ManipulationTool::GetBoxTriangles( Vector3( -0.075f, -0.075f, -0.075f ),Vector3( 0.075f, 0.075f, 0.075f ), &numVectors );
	translationMat = TranslationMatrix( 0.0f, 0.0f, 1.0f );
	TransformCoords( c_tris, numVectors, translationMat );

	for( int i = 0; i < numVectors/3; i++ )
	{
		m_zBox->AddTriangle(c_tris[i*3], v4Blue, c_tris[i*3+1], v4Blue, c_tris[i*3+2], v4Blue );
	}        
	delete [] c_tris;

	m_zBox->SubmitChanges();
	m_zBox->m_name = "z";
	m_zBox->m_scaleByDistanceToView = true;
	m_zBox->m_effect = mSolidEffect;
	m_zBox->m_pickEffect = pEffect;

	//wBox
	c_tris = Tr2ManipulationTool::GetBoxTriangles( Vector3( -0.075f, -0.075f, -0.075f ),Vector3( 0.075f, 0.075f, 0.075f ), &numVectors );
	for( int i = 0; i < numVectors/3; i++ )
	{
		m_wBox->AddTriangle(c_tris[i*3], v4Cyan, c_tris[i*3+1], v4Cyan, c_tris[i*3+2], v4Cyan );
	}        
	delete [] c_tris;

	m_wBox->SubmitChanges();
	m_wBox->m_name = "w";
	m_wBox->m_scaleByDistanceToView = true;
	m_wBox->m_effect = mSolidEffect;
	m_wBox->m_pickEffect = pEffect;

	m_primitives.Insert( -1, m_xLine->GetRawRoot());
	m_primitives.Insert( -1, m_yLine->GetRawRoot());
	m_primitives.Insert( -1, m_zLine->GetRawRoot());

	m_primitives.Insert( -1, m_xBox->GetRawRoot());
	m_primitives.Insert( -1, m_yBox->GetRawRoot());
	m_primitives.Insert( -1, m_zBox->GetRawRoot());
	m_primitives.Insert( -1, m_wBox->GetRawRoot());
}

void Tr2ScalingTool::Update()
{
	Matrix translation = TranslationMatrix( m_pivot );
	m_worldTransform = m_localTransform * translation;
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		(*it)->UpdateTransform();
	}

	UpdateLines();
}

void Tr2ScalingTool::UpdateLines()
{
	Vector3 wCenter = m_wBox->GetCenterOfMass();
	// x
	m_xLine->ClearLines();
	m_xLine->AddLine(	wCenter, 
		v4LightGray,  
		m_xBox->GetCenterOfMass(), 
		v4LightGray );
	m_xLine->SubmitChanges();

	// y
	m_yLine->ClearLines();
	m_yLine->AddLine(	wCenter, 
		v4LightGray,  
		m_yBox->GetCenterOfMass(), 
		v4LightGray );
	m_yLine->SubmitChanges();

	// z
	m_zLine->ClearLines();
	m_zLine->AddLine(	wCenter, 
		v4LightGray,  
		m_zBox->GetCenterOfMass(), 
		v4LightGray );
	m_zLine->SubmitChanges();
}

void Tr2ScalingTool::ResetPrimitives(){
	for( PrimitiveIterator it = m_primitives.begin(); it != m_primitives.end(); ++it )
	{
		(*it)->m_localTransform = m_localTransform;
		(*it)->UpdateTransform();
	}
	m_xLine->m_localTransform = IdentityMatrix();
	m_yLine->m_localTransform = IdentityMatrix();
	m_zLine->m_localTransform = IdentityMatrix();

	UpdateLines();
}


void Tr2ScalingTool::Init( Matrix& initialTransform )
{
	// Extract the scaling and translation
	Vector3 scaling;
	Vector3 pos;	
	Quaternion rotation;
	Matrix rotationMatrix;
	Decompose( scaling, rotation, pos, initialTransform );
	rotationMatrix = RotationMatrix( rotation );

	m_localTransform = TranslationMatrix( pos );
	m_localTransform = rotationMatrix * m_localTransform;

	Matrix translation = TranslationMatrix( m_pivot );
	m_worldTransform = m_localTransform * translation;
	ResetPrimitives();
	// Compute initial length when captured
	Vector3 vecA, vecB, vecC;
	vecA.x = m_worldTransform._41; 
	vecA.y = m_worldTransform._42;
	vecA.z = m_worldTransform._43;
	vecB = m_xBox->GetCenterOfMass();

	m_initialLength = -1;
	m_scale = scaling;
	m_initialScale = scaling;
	 
}
