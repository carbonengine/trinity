#include "StdAfx.h"

#include "WodBakingScene.h"
#include "Tr2SkinnedObject.h"

#include "TriDevice.h"
#include "Tr2ConstantBufferFormats.h"
#include "Tr2Renderer.h"
#include "TriRenderBatch.h"

static void PopulatePerFrameVSData( Tr2PerFrameVSData &data )
{
	// 0
	memset( &data, 0, sizeof( Tr2PerFrameVSData ) );

	// column_major for shaders
	data.ViewMat = Transpose( Tr2Renderer::GetViewTransform() );
	data.ProjectionMat = Transpose( Tr2Renderer::GetProjectionTransform() );
	
	Matrix viewProjectionMat = Tr2Renderer::GetViewTransform() * Tr2Renderer::GetProjectionTransform();
	data.ViewProjectionMat = Transpose( viewProjectionMat );

	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();
	// sun
	data.sunDirWorld.x = 0.0f; 
	data.sunDirWorld.y = 0.0f; 
	data.sunDirWorld.z = 1.0f;
}

static void PopulatePerFramePSData( Tr2PerFramePSData &data )
{
	// 0
	memset( &data, 0, sizeof( Tr2PerFramePSData ) );

	// column_major for shaders
    Matrix viewProj = Tr2Renderer::GetViewTransform() * Tr2Renderer::GetProjectionTransform();
	data.ViewProjectionMat = Transpose( viewProj );
	// attention: need the transposed, but shader also needs column_major, so it is transpose(transpose(m)) == m
	data.ViewInverseTransposeMat = Tr2Renderer::GetInverseViewTransform();

	// Hard code diffuse
	data.sunDiffuseColor.r = 0.0f;
	data.sunDiffuseColor.g = 0.0f;
	data.sunDiffuseColor.b = 0.0f;
	data.sunDiffuseColor.a = 1.0f;
	
	// Hard code specular
	data.sunSpecularColor.r = 0.8f;
	data.sunSpecularColor.g = 0.8f;
	data.sunSpecularColor.b = 0.8f;
	data.sunSpecularColor.a = 1.0f;

	// Hard code ambient		
	data.sceneAmbientColor.r = 0.0f;
	data.sceneAmbientColor.g = 0.0f;
	data.sceneAmbientColor.b = 0.0f;
	data.sceneAmbientColor.a = 0.0f;

	data.sunDirWorld.x = 0.0f; 
	data.sunDirWorld.y = 0.0f; 
	data.sunDirWorld.z = 1.0f;

	data.cullDirection = 1.0f;
	data.shScale = 1.0f;
}

WodBakingScene::WodBakingScene( IRoot* lockobj /*= NULL */ ):
	m_visualizeMethod( VM_NONE )
{
	// Create render batch accumulators
	TriPoolAllocator* allocator = Tr2Renderer::GetPoolAllocator();
	m_opaqueRenderBatches = CCP_NEW( "Tr2InteriorScene/m_primaryRenderBatches" ) TriRenderBatchAccumulator<EffectKeyGenerator>( allocator );
}

WodBakingScene::~WodBakingScene()
{
	CCP_DELETE( m_opaqueRenderBatches );
}

void WodBakingScene::Render( Tr2RenderContext& renderContext ) 
{
	D3DPERF_EVENT(L"WodBakingScene::Render");

	// Nothing to render if we do not have a skinned object to bake out.
	if ( !m_skinnedObject )
	{
		return;
	}

	renderContext.AddGpuMarker( __FUNCTION__ );

	Tr2PerObjectData* perObjectData = m_skinnedObject->GetPerObjectData( m_opaqueRenderBatches );
	m_skinnedObject->GetBatches( m_opaqueRenderBatches, TRIBATCHTYPE_OPAQUE, perObjectData );	
	
	renderContext.m_esm.BeginManagedRendering();
	renderContext.m_esm.SetInvertedCullMode( false );
	using namespace Tr2RenderContextEnum;

	// Actual rendering code goes in here
	Tr2PerFrameVSData vsData;
	Tr2PerFramePSData psData;

	// Set per-frame data	
	PopulatePerFramePSData( psData );
	PopulatePerFrameVSData( vsData );

	FillAndSetConstants( m_VSBuffer, vsData, VERTEX_SHADER, Tr2Renderer::GetPerFrameVSStartRegister(), renderContext );
	FillAndSetConstants( m_PSBuffer, psData, PIXEL_SHADER,  Tr2Renderer::GetPerFramePSStartRegister(), renderContext );
	
	m_opaqueRenderBatches->Finalize();
	renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_OPAQUE );

	// Render!
	renderContext.RenderBatches( m_opaqueRenderBatches );

	// Clear the batches
	m_opaqueRenderBatches->Clear();
	
	//m_transparentBatchStore->Clear();

	renderContext.m_esm.EndManagedRendering();

}

void WodBakingScene::RenderDebugInfo( Tr2RenderContext& renderContext )
{
}

void WodBakingScene::Update( Be::Time realTime, Be::Time simTime )
{
	if( m_skinnedObject )
	{
		m_skinnedObject->PrePhysicsUpdate( simTime );
		m_skinnedObject->PostPhysicsUpdate( simTime, NULL );
	}
}

