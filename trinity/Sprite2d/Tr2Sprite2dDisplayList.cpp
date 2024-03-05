////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2012
// Copyright:	CCP 2012
//

#include "StdAfx.h"
#include "Tr2Sprite2dDisplayList.h"
#include "TriDevice.h"
#include "Tr2Renderer.h"

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( displayListCount, "Trinity/SpriteScene/displayListCount", false, CST_COUNTER_LOW, "Number of cached display lists.");
CCP_STATS_DECLARE( displayListVertexBufferSize, "Trinity/SpriteScene/displayListVertexBufferSize", false, CST_COUNTER_HIGH, "Total size of vertex buffers used by cached display lists.");
CCP_STATS_DECLARE( displayListIndexBufferSize, "Trinity/SpriteScene/displayListIndexBufferSize", false, CST_COUNTER_HIGH, "Total size of index buffers used by cached display lists.");

Tr2Sprite2dDisplayList::Tr2Sprite2dDisplayList( ITr2SpriteObject* owner ) :
	m_owner( owner )
{
	CCP_STATS_INC( displayListCount );
	TriDevice::RegisterResource( this );
}

Tr2Sprite2dDisplayList::~Tr2Sprite2dDisplayList()
{
	TriDevice::UnregisterResource( this );
	CCP_STATS_DEC( displayListCount );

	if( vertexBuffer.IsValid() )
	{
		CCP_STATS_ADD( displayListVertexBufferSize, -double( vertexBuffer.GetSize() ) );
		vertexBuffer = Tr2BufferAL();
	}
	if( indexBuffer.IsValid() )
	{
		CCP_STATS_ADD( displayListIndexBufferSize, -double( indexBuffer.GetSize() ) );
		indexBuffer = Tr2BufferAL();
	}
}

void Tr2Sprite2dDisplayList::ReleaseResources( TriStorage s )
{
	if( vertexBuffer.IsValid() )
	{
		CCP_STATS_ADD( displayListVertexBufferSize, -double( vertexBuffer.GetSize() ) );
		vertexBuffer = Tr2BufferAL();
	}
	if( indexBuffer.IsValid() )
	{
		CCP_STATS_ADD( displayListIndexBufferSize, -double( indexBuffer.GetSize() ) );
		indexBuffer = Tr2BufferAL();
	}
	entries.clear();

	m_owner->SetDirty();
}

bool Tr2Sprite2dDisplayList::OnPrepareResources()
{
	return true;
}

void Tr2Sprite2dDisplayList::Entry::SubmitGeometry( Tr2RenderContext& renderContext )
{
	Matrix transposedMatrixes[TR2_SS_MAX_TRANSFORM_COUNT];
	for( unsigned i = 0; i < TR2_SS_MAX_TRANSFORM_COUNT; ++i )
	{
		transposedMatrixes[i] = Transpose( transformArray[i] );
	}

	HRESULT result = FillAndSetConstants(	
											*m_uiTransformsCb, 
											transposedMatrixes,
											sizeof( transposedMatrixes[0] ) * TR2_SS_MAX_TRANSFORM_COUNT, 
											VERTEX_SHADER,
											Tr2Renderer::GetPerObjectVSGUIStartRegister(),
											renderContext );

	if( FAILED(result) )
	{
		CCP_LOGWARN( "%s - couldn't set VS constant", __FUNCTION__ );
	}
	else
	{
		if( FAILED(result) )
		{
			CCP_LOGWARN( "%s - couldn't commit transform changes", __FUNCTION__ );
		}
	}

	renderContext.SetTopology( Tr2RenderContextEnum::TOP_TRIANGLES );
	CR( renderContext.DrawIndexedPrimitive( numVertices, startIndex, primitiveCount ) );
}
