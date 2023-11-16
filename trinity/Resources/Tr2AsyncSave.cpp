#include "StdAfx.h"
#include "Tr2AsyncSave.h"

using namespace Tr2RenderContextEnum;


Tr2AsyncSave::Tr2AsyncSave()
	: m_isSaving( false )
	, m_isSavePrepared( false )
	, m_saveSucceeded( false )
	, m_saveCbId( 0 )
	, m_prepareSaveCbId( 0 )
{	
}

Tr2AsyncSave::~Tr2AsyncSave()
{
	CleanupAsyncSave( false, true );	
}

void Tr2AsyncSave::WaitForSave( void ) const
{
	while( !IsSaveCompleted() )
	{
#if BLUE_WITH_PYTHON
		PyOS->Yield();
#endif
	}
}

bool Tr2AsyncSave::StartAsyncSave( const wchar_t* filename )
{
	if( m_isSavePrepared || m_saveCbId || m_prepareSaveCbId )
	{
		CCP_LOGWARN( "Can't save to file %S - currently being saved to file %S", filename, m_saveFilename.c_str() );
		return false;
	}

	m_saveFilename = filename;
	
	m_isSaving = TRUE;
	m_saveSucceeded = FALSE;
	m_isSavePrepared = FALSE;

	// Queue a request for the D3D lock.unlock calls on the main thread
	BeResMan->AddToQueue( BRMQ_MAIN, StaticPrepare, this, 0, &m_prepareSaveCbId );
	return true;
}

void Tr2AsyncSave::StaticPrepare( void* pContext )
{
	Tr2AsyncSave* pThis = static_cast<Tr2AsyncSave*>(pContext);
	pThis->PrepareSave();
}

void Tr2AsyncSave::PrepareSave()
{
	// Clear out the prepare task ID, even if this fails, so we can try again in the future.
	ON_BLOCK_EXIT( [&]{ m_prepareSaveCbId = 0; } );

	if( !DoPrepareAsyncSave() )
	{
		CleanupAsyncSave( true );
		return;
	}

	// Save is prepared
	m_isSavePrepared = TRUE;

	// Queue a request for the file write on the background thread
	BeResMan->AddToQueue( BRMQ_BACKGROUND, StaticSave, this, 0, &m_saveCbId );
}

void Tr2AsyncSave::StaticSave( void* pContext )
{
	Tr2AsyncSave* pThis = static_cast<Tr2AsyncSave*>(pContext);
	pThis->DoSave();
}

void Tr2AsyncSave::DoSave()
{
	const bool OK = DoExecuteAsyncSave();	
	m_saveCbId = 0;
	CleanupAsyncSave( !OK );
}

void Tr2AsyncSave::CleanupAsyncSave( bool failed, bool fromDestructor )
{
	bool saveInProgress = false;
	uint32_t saveCbId = m_saveCbId;
	if( saveCbId )
	{
		saveInProgress = true;
		BeResMan->CancelFromQueue( BRMQ_BACKGROUND, saveCbId );
		m_saveCbId = 0;
	}

	uint32_t prepareSaveCbId = m_prepareSaveCbId;
	if( prepareSaveCbId )
	{
		saveInProgress = true;
		BeResMan->CancelFromQueue( BRMQ_MAIN, prepareSaveCbId );
		m_prepareSaveCbId = 0;
	}
		
	if( saveInProgress )
	{
		CCP_LOGERR( "Save canceled for '%S'", m_saveFilename.c_str() );
	}

	if( !fromDestructor )
	{
		DoCleanupAsyncSave();
	}

	m_isSaving = FALSE;
	m_saveSucceeded = !failed;
	m_isSavePrepared = FALSE;
}
