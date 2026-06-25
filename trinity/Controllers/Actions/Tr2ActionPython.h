// Copyright © 2025 CCP ehf.

#pragma once

#include "ITr2ControllerAction.h"
#include "Include/ITr2Updateable.h"


BLUE_CLASS( Tr2ActionPython ) :
	public ITr2ControllerAction,
	public ITr2Updateable,
	public INotify,
	public IInitialize,
	public ICustomPersist
{
public:
	Tr2ActionPython( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	bool Initialize() override;
	bool OnModified( Be::Var * value ) override;

	void Link( ITr2ActionController & controller ) override;
	void Unlink() override;
	void Start( ITr2ActionController & controller ) override;
	void Stop( ITr2ActionController & controller ) override;

	void Update( Be::Time realTime, Be::Time simTime ) override;

	void GetWriteBufferAndSize( const char* memberName, uint8_t** buffer, size_t* bufferSize ) override;
	void ReleaseWriteBuffer( uint8_t* buffer ) override;
	unsigned char* AllocateReadBuffer( const char* memberName, size_t bufferSize ) override;
	void SetBufferAndSize( const char* memberName, uint8_t* buffer, size_t bufferSize ) override;


	BluePy GetInstance() const;

private:
	struct VTable
	{
		BlueScriptCallback onLink;
		BlueScriptCallback onUnlink;
		BlueScriptCallback onStart;
		BlueScriptCallback onStop;
		BlueScriptCallback onUpdate;

		BlueScriptCallback onLoad;
		BlueScriptCallback onSave;
	};

	void InstantiateObject();

	std::string m_module;
	std::string m_className;

	BluePy m_instance;
	VTable m_vtable;
	Be::Time m_prevRealTime = 0;
	Be::Time m_prevSimTime = 0;
	ITr2ActionController* m_controller = nullptr;
	bool m_isPlaying = false;
};

TYPEDEF_BLUECLASS( Tr2ActionPython );
