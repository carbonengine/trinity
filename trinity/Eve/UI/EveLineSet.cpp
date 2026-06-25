// Copyright © 2023 CCP ehf.

#include "StdAfx.h"
#include "Utilities/BoundingSphere.h"
#include "EveLineSet.h"
#include "Eve/EveConstantBufferFormats.h"
#include "TriRenderBatch.h"
#include "Tr2PerObjectData.h"
#include "Eve/EveUpdateContext.h"
#include "TriRenderBatch.h"
#include "Shader/Tr2Effect.h"
#include "Tr2Renderer.h"


using namespace Tr2RenderContextEnum;

EveLineSet::EveLineSet( IRoot* lockobj /*= NULL*/ ) :
	m_display( true ),
	m_boundingSphere( 0.f, 0.f, 0.f, 0.f ),
	m_vertexDeclHandle( Tr2EffectStateManager::UNINITIALIZED_DECLARATION ),
	m_currentSubmittedLineCount( 0 ),
	m_maxCurrentLineCount( 0 ),
	m_isRenderedAsTransparent( false ),
	m_scaling( 1.0f, 1.0f, 1.0f )
{
}

EveLineSet::~EveLineSet()
{
	ReleaseResources( TRISTORAGE_ALL );
}

//////////////////////////////////////////////////////////////////////////////////////
// IInitialize
bool EveLineSet::Initialize()
{
	m_maxCurrentLineCount = 100;
	m_lines.reserve( m_maxCurrentLineCount );
	PrepareResources();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
// ITriDeviceResource
void EveLineSet::ReleaseResources( TriStorage s )
{
	m_vertexDeclHandle = Tr2EffectStateManager::UNINITIALIZED_DECLARATION;
	// CComPtr, this is safe!
	m_vertexBuffer = Tr2BufferAL();
}

bool EveLineSet::OnPrepareResources()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		Tr2VertexDefinition vd;
		vd.Add( vd.FLOAT32_3, vd.POSITION );
		vd.Add( vd.FLOAT32_4, vd.TEXCOORD );

		m_vertexDeclHandle = Tr2EffectStateManager::GetVertexDeclarationHandle( vd );
		if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
		{
			return false;
		}
	}

	if( !m_vertexBuffer.IsValid() )
	{
		CR_RETURN_VAL(
			m_vertexBuffer.Create(
				sizeof( EveLineData ),
				m_maxCurrentLineCount,
				Tr2GpuUsage::VERTEX_BUFFER,
				Tr2CpuUsage::WRITE_OFTEN,
				nullptr,
				renderContext ),
			false );
	}

	void* vertexBuffer;
	CR_RETURN_VAL( m_vertexBuffer.MapForWriting( vertexBuffer, renderContext ), false );

	if( !m_lines.empty() )
	{
		memcpy( vertexBuffer, &m_lines[0], sizeof( EveLineData ) * m_lines.size() );
	}

	m_vertexBuffer.UnmapForWriting( renderContext );
	m_currentSubmittedLineCount = (unsigned int)m_lines.size();

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
// IEveSpaceObject2
void EveLineSet::UpdateSyncronous( const EveUpdateContext& updateContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Quaternion rotation( 0.0f, 0.0f, 0.0f, 1.0f );
	Vector3 translation( 0.0f, 0.0f, 0.0f );

	if( m_ballPosition )
	{
		m_ballPosition->Update( &translation, updateContext.GetTime() );
	}
	if( m_ballRotation )
	{
		m_ballRotation->Update( &rotation, updateContext.GetTime() );
	}

	m_worldTransform = TransformationMatrix( m_scaling, rotation, translation );
}

void EveLineSet::UpdateAsyncronous( const EveUpdateContext& updateContext )
{
}

void EveLineSet::Update( const EveUpdateContext& updateContext )
{
	UpdateSyncronous( updateContext );
}

// --------------------------------------------------------------------------------
// Description:
//   Updates all the view dependent data, needed for multi-screen rendering
// --------------------------------------------------------------------------------
void EveLineSet::UpdateViewDependentData( const TriFrustum& frustum, const Matrix& parentTransform )
{
}

void EveLineSet::UpdateVisibility( const EveUpdateContext& updateContext, const Matrix& parentTransform )
{
}

void EveLineSet::GetRenderables( std::vector<ITr2Renderable*>& renderables, Tr2ImpostorManager* impostors )
{
	if( !m_display )
	{
		return;
	}

	renderables.push_back( this );
}

void EveLineSet::GetRenderables( std::vector<ITr2Renderable*>& renderables )
{
	return GetRenderables( renderables, nullptr );
}

bool EveLineSet::GetBoundingSphere( Vector4& sphere, BoundingSphereQuery query ) const
{
	sphere = m_boundingSphere;
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////
// ITr2Renderable
bool EveLineSet::HasTransparentBatches()
{
	return true;
}

void EveLineSet::GetBatches( ITriRenderBatchAccumulator* accumulator,
							 TriBatchType batchType,
							 const Tr2PerObjectData* perObjectData,
							 Tr2RenderReason reason )
{
	// Is only rendered as transparent or additive.
	if( !( batchType == TRIBATCHTYPE_TRANSPARENT && m_isRenderedAsTransparent ) &&
		!( batchType == TRIBATCHTYPE_ADDITIVE && !m_isRenderedAsTransparent ) )
	{
		return;
	}

	if( !m_display )
	{
		return;
	}

	if( !m_vertexBuffer.IsValid() )
	{
		return;
	}

	if( m_vertexDeclHandle == Tr2EffectStateManager::UNINITIALIZED_DECLARATION )
	{
		return;
	}

	Tr2RenderBatch batch;
	batch.SetMaterial( m_effect );
	batch.SetPerObjectData( perObjectData );
	batch.SetVertexDeclaration( m_vertexDeclHandle );
	batch.SetStreamSource( 0, m_vertexBuffer, sizeof( EveLineData ) / 2 );
	batch.SetTopology( TOP_LINES );
	batch.SetDrawInstanced( m_currentSubmittedLineCount * 2, 1, 0, 0 );
	accumulator->Commit( batch );
}

float EveLineSet::GetSortValue()
{
	Vector3 d = Tr2Renderer::GetViewPosition() - m_worldTransform.GetTranslation();
	float distance = Length( d );
	return distance;
}

Tr2PerObjectData* EveLineSet::GetPerObjectData( ITriRenderBatchAccumulator* accumulator )
{
	Tr2PerObjectDataStandard* data = accumulator->Allocate<Tr2PerObjectDataStandard>();

	if( !data )
	{
		return nullptr;
	}

	EvePerObjectPSData perObjectPSBuffer;
	EvePerObjectVSData perObjectVSBuffer;

	// column_major for shaders
	perObjectVSBuffer.WorldMat = Transpose( m_worldTransform );
	perObjectPSBuffer.WorldMat = Transpose( m_worldTransform );

	data->CopyToVSFloatBuffer( perObjectVSBuffer );
	data->CopyToPSFloatBuffer( perObjectPSBuffer );

	return data;
}

// Python Exposed Methods
unsigned int EveLineSet::AddLine( const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2 )
{
	EveLineData newLine;

	newLine.m_position1 = position1;
	newLine.m_color1 = color1;
	newLine.m_position2 = position2;
	newLine.m_color2 = color2;

	m_lines.push_back( newLine );
	return (unsigned int)m_lines.size() - 1;
}

bool EveLineSet::SubmitChanges()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_lines.size() > m_maxCurrentLineCount )
	{
		// increase the size of the buffer
		m_maxCurrentLineCount = (unsigned int)m_lines.capacity();
		ReleaseResources( TRISTORAGE_ALL );
	}

	// We have to make sure that we're prepared
	PrepareResources();

	if( !m_vertexBuffer.IsValid() )
	{
		return false;
	}

	void* vertexBuffer;
	CR_RETURN_VAL( m_vertexBuffer.MapForWriting( vertexBuffer, renderContext ), false );

	memcpy( vertexBuffer, &m_lines[0], sizeof( EveLineData ) * m_lines.size() );

	m_vertexBuffer.UnmapForWriting( renderContext );

	m_currentSubmittedLineCount = (unsigned int)m_lines.size();

	// also update the bounding sphere here
	BoundingSphereInitialize( m_boundingSphere );
	for( auto it = m_lines.begin(); it != m_lines.end(); ++it )
	{
		BoundingSphereUpdate( it->m_position1, m_boundingSphere );
		BoundingSphereUpdate( it->m_position2, m_boundingSphere );
	}

	return true;
}

void EveLineSet::RemoveLine( unsigned int id )
{
	if( id < m_lines.size() )
	{
		m_lines.erase( m_lines.begin() + id );
	}
}

void EveLineSet::ChangeLine( unsigned int id, const Vector3& position1, const Vector4& color1, const Vector3& position2, const Vector4& color2 )
{
	if( id < m_lines.size() )
	{
		EveLineData& line = m_lines[id];
		line.m_position1 = position1;
		line.m_position2 = position2;
		line.m_color1 = color1;
		line.m_color2 = color2;
	}
}

void EveLineSet::ChangeLineColor( unsigned int id, const Vector4& color1, const Vector4& color2 )
{
	if( id < m_lines.size() )
	{
		EveLineData& line = m_lines[id];
		line.m_color1 = color1;
		line.m_color2 = color2;
	}
}

void EveLineSet::ChangeLinePosition( unsigned int id, const Vector3& position1, const Vector3& position2 )
{
	if( id < m_lines.size() )
	{
		EveLineData& line = m_lines[id];
		line.m_position1 = position1;
		line.m_position2 = position2;
	}
}

void EveLineSet::ClearLines()
{
	m_lines.clear();
}