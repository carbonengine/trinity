// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "Tr2ProceduralGeometryProvider.h"
#include "Tr2Renderer.h"
#include "Tr2BackgroundWorker.h"

using namespace Tr2RenderContextEnum;

void Tr2ProceduralGeometryProvider::Rebuild()
{
	CancelBuild();

	m_isLoading = true;

	if( m_synchronous )
	{
		m_build = nullptr;

		Build build;
		build.builder = CreateBuilder();
		build.owner = this;
		if( StaticRunBuild( build ) )
		{
			Prepare( build );
		}
		else
		{
			FailBuild();
		}
		return;
	}

	// we need a fresh piece of memory, as in-flight requests might be modifying the build. (could maybe use a pool or something)
	// and we need to hold onto the pointer to be able to cancel requests.
	m_build = new Build();
	// we need a builder object with a copy of parameters, as this object might be destroyed on main thread while the background worker is trying to access the parameters
	m_build->builder = CreateBuilder();
	// once the builder is done, we continue on the main thread, and we need to call methods on this object
	m_build->owner = this;
	Tr2BackgroundWorker::Instance().Add( &StaticBuild, m_build, IBlueCallbackMan::BCBF_NONE, nullptr );
}

void Tr2ProceduralGeometryProvider::SetSynchronous( bool synchronous )
{
	m_synchronous = synchronous;
}

Tr2ProceduralGeometryProvider::Tr2ProceduralGeometryProvider( IRoot* lockobj ) :
	m_build( nullptr ),
	m_synchronous( false )
{
}

Tr2ProceduralGeometryProvider::~Tr2ProceduralGeometryProvider()
{
	CancelBuild();
	ReleaseResources( TRISTORAGE_ALL );
}

bool Tr2ProceduralGeometryProvider::OnPrepareResources()
{
	if( !IsPrepared() && !IsLoading() )
	{
		Rebuild();
	}
	return true;
}

void Tr2ProceduralGeometryProvider::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_MANAGEDMEMORY )
	{
		CancelBuild();
		ReleaseResourcesHelper();
		SetPrepared( false );
		SetGood( false );
		NotifyReleaseCachedData();
	}
}

#if TRINITYDEV
void Tr2ProceduralGeometryProvider::GetDescription( std::string& desc )
{
	desc = "Tr2ProceduralGeometryProvider: '";
	desc += m_name;
	desc += "'";
}
#endif

// this function runs on worker thread
void Tr2ProceduralGeometryProvider::StaticBuild( void* context )
{
	auto build = static_cast<Build*>( context );
	if( build->cancel )
	{
		// owner has nulled m_build in CancelBuild, so we don't need to do that here (and we possibly can't over here, as owner also might have been destroyed)
		delete build;
		return;
	}

	bool success = StaticRunBuild( *build );

	if( build->cancel )
	{
		// owner has nulled m_build in CancelBuild, so we don't need to do that here (and we possibly can't over here, as owner also might have been destroyed)
		delete build;
		return;
	}

	BeResMan->AddToQueue( BRMQ_MAIN, success ? &StaticPrepare : &StaticFail, build, IBlueCallbackMan::BCBF_NONE, nullptr );
}

void Tr2ProceduralGeometryProvider::StaticPrepare( void* context )
{
	auto build = static_cast<Build*>( context );
	if( !build->cancel )
	{
		build->owner->m_build = nullptr;
		build->owner->Prepare( *build );
	}
	delete build;
}

void Tr2ProceduralGeometryProvider::StaticFail( void* context )
{
	auto build = static_cast<Build*>( context );
	if( !build->cancel )
	{
		build->owner->m_build = nullptr;
		build->owner->FailBuild();
	}
	delete build;
}

// this function may run on main thread or worker thread
bool Tr2ProceduralGeometryProvider::StaticRunBuild( Build& build )
{
	cmf::MemoryAllocator allocator;
	cmf::BufferManager buffers( allocator );
	cmf::Data data;

	if( !build.builder->BuildCmf( allocator, buffers, build.cancel, data ) )
	{
		CCP_LOGERR( "Tr2ProceduralGeometryProvider %s: BuildCmf failed", build.builder->name.c_str() );
		return false;
	}

	build.bytes = cmf::BuildFile( data, buffers );
	return !build.bytes.empty();
}

void Tr2ProceduralGeometryProvider::Prepare( Build& build )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		m_isLoading = false;
		SetPrepared( false );
		return;
	}

	Tr2CmfContents contents( build.bytes.data(), build.bytes.size(), m_name.c_str() );
	if( !contents )
	{
		FailBuild();
		return;
	}

	NotifyReleaseCachedData();
	ReleaseResourcesHelper();

	if( !SetupMeshesFromCMF( *contents.GetData() ) )
	{
		FailBuild();
		return;
	}

	SetupSkeletonsFromCMF( *contents.GetData() );

	SetGood( CreateMeshesFromCMFFile( contents, Tr2CpuUsage::READ, renderContext ) );

	SetPrepared( true );
	m_isLoading = false;
	NotifyRebuildCachedData();
}

void Tr2ProceduralGeometryProvider::FailBuild()
{
	SetPrepared( true );
	m_isLoading = false;
	SetGood( false );
	NotifyRebuildCachedData();
}

void Tr2ProceduralGeometryProvider::CancelBuild()
{
	if( !m_build )
	{
		return;
	}
	m_build->cancel = true;
	m_build->owner = nullptr;
	m_build = nullptr;
	m_isLoading = false;
}
