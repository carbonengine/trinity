#pragma once
#ifndef Tr2AsyncSave_h_
#define Tr2AsyncSave_h_

class Tr2AsyncSave
{
public:
	Tr2AsyncSave();	

	bool IsSaving()			const { return m_isSavePrepared || m_saveCbId || m_prepareSaveCbId; }
	bool IsSaveCompleted()	const { return !m_isSaving; }
	bool IsSaveSucceeded()	const { return m_saveSucceeded != 0; }
	void WaitForSave()		const;

protected:
	// Main entry point for classes that inherit from Tr2AsyncSave: call this to start the whole
	// process.
	bool StartAsyncSave( const wchar_t* filename );
	// Call this to cancel or clean up any in-progress saving, eg. from inside your DoLoad method.
	void CleanupAsyncSave( bool failed, bool fromDestructor = false );

	std::wstring m_saveFilename;
	
private:
	// Do preparation work on the main thread
	virtual bool DoPrepareAsyncSave() = 0;
	// Do the actual saving on a worker thread
	virtual bool DoExecuteAsyncSave() = 0;
	// Free resources used for saving. Called when the save task is finished, either because
	// it worked or because something went wrong
	virtual void DoCleanupAsyncSave() = 0;

	// Called on main thread, will do bookkeeping and then call DoPrepareAsyncSave
	static void StaticPrepare( void* pContext );
	void PrepareSave();

	// Called on worker thread, will do bookkeeping and then call DoExecuteAsyncSave
	static void StaticSave( void* pContext );
	void DoSave();

	CcpAtomic<uint32_t> m_isSaving;
	CcpAtomic<uint32_t> m_isSavePrepared;
	CcpAtomic<uint32_t> m_saveSucceeded;
	
	CcpAtomic<uint32_t> m_saveCbId;			/// When set, save has been scheduled on the background thread
	CcpAtomic<uint32_t> m_prepareSaveCbId;		/// When set, D3D texture lock/unlock has been scheduled on the main thread

protected:
	~Tr2AsyncSave();

private:
	Tr2AsyncSave( Tr2AsyncSave& );
	Tr2AsyncSave& operator=( Tr2AsyncSave& );
	Tr2AsyncSave& operator=( Tr2AsyncSave&& );
};

#endif // !Tr2AsyncSave_h_
