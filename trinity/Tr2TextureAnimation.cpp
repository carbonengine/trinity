////////////////////////////////////////////////////////////
//
//    Created:   March 2023
//    Copyright: CCP 2023
//

#include "StdAfx.h"
#include "Tr2TextureAnimation.h"
#include "Resources/Tr2TextureLodManager.h"

namespace
{

std::vector<std::pair<IBlueCallbackMan::CallbackFunc, void*>> s_tasks;
std::mutex s_tasksMutex;
std::condition_variable s_tasksCV;
bool s_initialized = false;

void TasksWorker()
{
	for( ;; )
	{
		IBlueCallbackMan::CallbackFunc func;
		void* context;
		{
			std::unique_lock lk( s_tasksMutex );
			s_tasksCV.wait( lk, [] { return !s_tasks.empty(); } );
			func = s_tasks[0].first;
			context = s_tasks[0].second;
			s_tasks.erase( s_tasks.begin() );
		}

		( *func )( context );
	}
}

void AddToComputeQueue( IBlueCallbackMan::CallbackFunc pCb, void* pContext )
{
	if( !s_initialized )
	{
		std::thread( &TasksWorker ).detach();
		s_initialized = true;
	}
	{
		std::lock_guard lock( s_tasksMutex );
		s_tasks.push_back( { pCb, pContext } );
	}
	s_tasksCV.notify_one();
}

}

ptrdiff_t Tr2TextureAnimation::MemoryReadStream::Read( void* buffer, ptrdiff_t count )
{
	if( count <= 0 )
	{
		return 0;
	}
	auto size = ptrdiff_t( m_data.size() );
	if( m_offset + count > size )
	{
		count = size - m_offset;
	}
	memcpy( buffer, m_data.data() + m_offset, count );
	m_offset += count;
	return count;
}

ptrdiff_t Tr2TextureAnimation::MemoryReadStream::Seek( ptrdiff_t distance, SeekOrigin method )
{
	auto size = ptrdiff_t( m_data.size() );
	ptrdiff_t position;
	switch( method )
	{
	case SO_CURRENT:
		position = m_offset + distance;
		break;
	case SO_END:
		position = size - distance;
		break;
	default:
		position = distance;
	}
	if( position >= 0 && position <= size )
	{
		m_offset = position;
	}
	return m_offset;
}

ptrdiff_t Tr2TextureAnimation::MemoryReadStream::Write( const void* source, size_t count )
{
	return 0;
}

ptrdiff_t Tr2TextureAnimation::MemoryReadStream::GetPosition()
{
	return m_offset;
}

ptrdiff_t Tr2TextureAnimation::MemoryReadStream::GetSize()
{
	return ptrdiff_t( m_data.size() );
}


Tr2TextureAnimation::~Tr2TextureAnimation()
{
	if( m_asyncState )
	{
		m_asyncState->cancel = true;
	}
}

bool Tr2TextureAnimation::Initialize()
{
	ReadData();
	return true;
}

bool Tr2TextureAnimation::OnModified( Be::Var* value )
{
	ReadData();
	return true;
}

Tr2TextureAL Tr2TextureAnimation::GetTexture( const BlueSharedString& channel ) const
{
	for( auto& grid : m_grids )
	{
		if( grid.name == channel )
		{
			return grid.frame;
		}
	}
	return Tr2TextureAL();
}

void Tr2TextureAnimation::AdvanceTime( float dt )
{
	if( !m_asyncState )
	{
		return;
	}

	if( m_asyncState->bitmapsReady && m_grids.empty() )
	{
		USE_MAIN_THREAD_RENDER_CONTEXT();

		m_grids.resize( m_asyncState->decoders.size() );
		for( uint32_t i = 0; i < m_asyncState->reader.GetGridCount(); ++i )
		{
			m_grids[i].name = BlueSharedString( m_asyncState->reader.GetGridInfo( i ).name );

			auto& bitmap = m_asyncState->decoders[i].GetFrameBitmap();

			Tr2SubresourceData mip0;
			mip0.m_sysMem = bitmap.GetRawData();
			mip0.m_sysMemSlicePitch = bitmap.GetMipSize( 0 ) / std::max( bitmap.GetMipDepth( 0 ), 1u );
			mip0.m_sysMemPitch = bitmap.GetMipPitch( 0 );

			if( FAILED( m_grids[i].frame.Create( bitmap, Tr2MsaaDesc(), Tr2GpuUsage::SHADER_RESOURCE, Tr2CpuUsage::WRITE, &mip0, renderContext ) ) )
			{
				CCP_LOGERR( "Failed to create texture (%S)", m_filename.c_str() );
			}
		}
		m_frame = 0;
		m_time = 0;
		m_restartState = NotRestarting;

		AddToComputeQueue( &DecodeNextFrame, MakeRequest() );
	}

	if( m_grids.empty() )
	{
		return;
	}

	switch( m_restartState )
	{
	case WaitingToRestart:
		if( m_asyncState->bitmapsReady )
		{
			AddToComputeQueue( &RestartAndDecodeFrame, MakeRequest() );
			m_restartState = WaitingForFrame;
		}
		return;
	case WaitingForFrame:
		if( m_asyncState->bitmapsReady )
		{
			UpdateGrids();
			AddToComputeQueue( &DecodeNextFrame, MakeRequest() );
			m_frame = 0;
			m_time = 0;
			m_restartState = NotRestarting;
		}
		return;
	}

	if( !m_paused )
	{
		m_time += dt * m_fps;
		if( m_time > 1 && m_asyncState->bitmapsReady )
		{
			CCP_STATS_ZONE( __FUNCTION__ );

			++m_frame;
			if( m_frame >= m_asyncState->reader.GetFrameCount() )
			{
				if( !m_looped )
				{
					--m_frame;
					m_time = 1;
					return;
				}
				m_frame = 0;
			}
			m_time = fmodf( m_time, 1.f );
			UpdateGrids();
			AddToComputeQueue( &DecodeNextFrame, MakeRequest() );
		}
	}
}

void Tr2TextureAnimation::UpdateGrids()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	for( size_t i = 0; i < m_grids.size(); ++i )
	{
		CCP_STATS_ZONE( "UpdateSubresource" );
		auto& bitmap = m_asyncState->decoders[i].GetFrameBitmap();
		m_grids[i].frame.UpdateSubresource( Tr2TextureSubresource( 0 ), bitmap.GetRawData(), bitmap.GetPitch(), bitmap.GetPitch() * bitmap.GetHeight(), renderContext );
	}
}

void Tr2TextureAnimation::RestartAnimation()
{
	if( m_restartState == NotRestarting )
	{
		if( m_asyncState->bitmapsReady )
		{
			AddToComputeQueue( &RestartAndDecodeFrame, MakeRequest() );
			m_restartState = WaitingForFrame;
		}
		else
		{
			m_restartState = WaitingToRestart;
		}
	}
}

void Tr2TextureAnimation::ReadData()
{
	if( m_asyncState )
	{
		m_asyncState->cancel = true;
	}

	m_grids.clear();
	if( m_filename.empty() )
	{
		m_asyncState = nullptr;
		return;
	}

	m_asyncState = std::make_shared<AsyncState>();
	BeResMan->AddToQueue( BRMQ_BACKGROUND, &ReadFile, MakeRequest(), IBlueCallbackMan::BCBF_NONE, nullptr );
}

std::vector<BlueSharedString> Tr2TextureAnimation::GetChannelNames() const
{
	std::vector<BlueSharedString> names;
	names.reserve( m_grids.size() );
	for( auto& grid : m_grids )
	{
		names.push_back( grid.name );
	}
	return names;
}

Tr2TextureAnimation::AsyncRequest* Tr2TextureAnimation::MakeRequest()
{
	m_asyncState->bitmapsReady = false;

	auto request = new AsyncRequest();
	request->filename = m_filename;
	if( Tr2TextureLodManager::Instance().GetUseLowResVtaFilesSetting() )
	{
		if( m_filename.length() > 4 && wcscmp( m_filename.c_str() + m_filename.length() - 4, L".vta" ) == 0 )
		{
			request->filename = m_filename.substr( 0, m_filename.length() - 4 ) + L"_lowdetail.vta";
		}
	}
	request->state = m_asyncState;
	return request;
}

void Tr2TextureAnimation::ReadFile( void* ctx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto request = static_cast<AsyncRequest*>( ctx );
	IBlueStreamPtr stream;
	if( !BePaths->GetStreamFromPathW( request->filename.c_str(), &stream ) )
	{
		CCP_LOGERR( "Error opening file (%S)", request->filename.c_str() );
		delete request;
		return;
	}
	if( request->state->cancel )
	{
		delete request;
		return;
	}
	size_t size = size_t( stream->GetSize() );
	request->state->stream.m_data.resize( size );
	if( stream->Read( request->state->stream.m_data.data(), size ) != size )
	{
		CCP_LOGERR( "Error reading file (%S)", request->filename.c_str() );
		request->state->stream = {};
		delete request;
		return;
	}
	auto result = request->state->reader.SetStream( &request->state->stream );
	if( !result )
	{
		CCP_LOGERR( "Error reading VTA file (%S): %s", request->filename.c_str(), result.GetErrorMessage().c_str() );
		request->state->stream = {};
		delete request;
		return;
	}

	if( request->state->cancel )
	{
		delete request;
		return;
	}

	AddToComputeQueue( &DecodeFirstFrame, request );
}

void Tr2TextureAnimation::DecodeFirstFrame( void* ctx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto request = static_cast<AsyncRequest*>( ctx );
	for( uint32_t i = 0; i < request->state->reader.GetGridCount(); ++i )
	{
		if( request->state->cancel )
		{
			delete request;
			return;
		}
		request->state->decoders.push_back( ImageIO::Vta::FrameDecoder( request->state->reader, i ) );
	}
	request->state->bitmapsReady = true;
	delete request;
}

void Tr2TextureAnimation::DecodeNextFrame( void* ctx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto request = static_cast<AsyncRequest*>( ctx );
	for( auto& decoder : request->state->decoders )
	{
		decoder.AdvanceFrame();
		if( request->state->cancel )
		{
			delete request;
			return;
		}
	}
	request->state->bitmapsReady = true;
	delete request;
}

void Tr2TextureAnimation::RestartAndDecodeFrame( void* ctx )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto request = static_cast<AsyncRequest*>( ctx );
	for( auto& decoder : request->state->decoders )
	{
		decoder.Restart();
		if( request->state->cancel )
		{
			delete request;
			return;
		}
	}
	request->state->bitmapsReady = true;
	delete request;
}

bool Tr2TextureAnimation::UpdateOnlyWhenRendered() const
{
	return m_updateOnlyWhenRendered;
}