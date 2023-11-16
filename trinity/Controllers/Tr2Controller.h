////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once

#include "ITr2Controller.h"
#include "ccpparser.h"

BLUE_DECLARE( Tr2ControllerFloatVariable );
BLUE_DECLARE_VECTOR( Tr2ControllerFloatVariable );
BLUE_DECLARE( Tr2StateMachine );
BLUE_DECLARE_VECTOR( Tr2StateMachine );
BLUE_DECLARE( Tr2ControllerEventHandler );
BLUE_DECLARE_VECTOR( Tr2ControllerEventHandler );
BLUE_DECLARE_INTERFACE( ITr2Updateable );

BLUE_CLASS( Tr2Controller ):
	public ITr2Controller,
	public IListNotify
{
public:
	Tr2Controller( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list );

	virtual void Link( IRoot& owner );
	virtual void Unlink();
	bool IsLinked() const override;

	virtual void Start();
	virtual void Stop();
	virtual void Update();

	virtual void SetVariable( const char* name, float value );

	void HandleEvent( const char* eventName ) override;

	IRoot* GetOwner() const;
	Tr2ControllerFloatVariable* GetVariableByName( const char* name ) const;

	const PTr2ControllerFloatVariableVector& GetVariables() const;
	CcpParser::VariableView GetVariableView() const;
	void* GetVariableBuffer() const;
	void EnsureTempArenaSize( size_t size ) const;
	void* GetTempArena() const;
	const std::vector<std::pair<std::string, IRoot*>>& GetBindingPathRoots() const;

	void RegisterUpdateable( ITr2Updateable& updateable );
	void UnRegisterUpdateable( ITr2Updateable& updateable );

	void Callback( BlueSharedString callbackName);
	void RegisterCallback( BlueSharedString callbackName, BlueScriptCallback callback );
	void ClearCallbacks();

	void ReLink();

private:
	size_t GetCallbackCount() { return m_callbacks.size(); };

	std::string m_name;
	PTr2StateMachineVector m_stateMachines;
	PTr2ControllerFloatVariableVector m_variables;
	PTr2ControllerEventHandlerVector m_eventHandlers;
	
	TrackableStdSet<ITr2UpdateablePtr> m_updateables;
	std::vector<std::pair<BlueSharedString, BlueScriptCallback>>  m_callbacks;
	std::vector<CcpParser::Variable> m_variableView;
	CcpMallocBuffer m_variableData;
	mutable CcpMallocBuffer m_tempArena;
	uint64_t m_dirtyVariables;

	mutable std::vector<std::pair<std::string, IRoot*>> m_bindingPathRoots;

	IRoot* m_owner;
	bool m_isActive;
	bool m_isShared;
};

TYPEDEF_BLUECLASS( Tr2Controller );
