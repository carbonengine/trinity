////////////////////////////////////////////////////////////
//
//    Created:   August 2025
//    Copyright: CCP 2025
//

#pragma once


#include "ITr2Controller.h"
#include "Actions/ITr2ControllerAction.h"

BLUE_DECLARE( Tr2ControllerFloatVariable );
BLUE_DECLARE_VECTOR( Tr2ControllerFloatVariable );
BLUE_DECLARE( Tr2ControllerEventHandler );
BLUE_DECLARE_VECTOR( Tr2ControllerEventHandler );
BLUE_DECLARE_INTERFACE( ITr2Updateable );


struct Tr2TimelineEntry
{
	float startTime;
	float endTime;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2TimelineEntry );


BLUE_CLASS( Tr2TimelineContoller ) :
	public ITr2ActionController,
	public ISimTimeRebaseNotify
{
public:
	Tr2TimelineContoller( IRoot* lockobj = nullptr );
	~Tr2TimelineContoller();

	EXPOSE_TO_BLUE();

	void Link( IRoot & owner ) override;
	void Unlink() override;
	bool IsLinked() const override;
	void Start() override;
	void Stop() override;
	void Update() override;
	void SetVariable( const char* name, float value ) override;
	void HandleEvent( const char* eventName ) override;

	IRoot* GetOwner() const override;
	void Callback( BlueSharedString callbackName ) override;
	void RegisterUpdateable( ITr2Updateable & updateable ) override;
	void UnRegisterUpdateable( ITr2Updateable & updateable ) override;
	const std::vector<std::pair<std::string, IRoot*>>& GetBindingPathRoots() const override;
	std::optional<float> GetFloatVariableByName( const char* name ) const override;
	void GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr> & out ) const override;
	CcpParser::VariableView GetVariableView() const override;
	void* GetVariableBuffer() const override;
	void EnsureTempArenaSize( size_t size ) const override;
	void* GetTempArena() const override;

	void OnSimClockRebase( Be::Time oldTime, Be::Time newTime ) override;

	size_t GetActionCount() const;
	BlueStdResult GetAction( size_t index, ITr2ControllerActionPtr& action );
	BlueStdResult GetActionStartTime( size_t index, float& value ) const;
	BlueStdResult GetActionEndTime( size_t index, float& value ) const;
	BlueStdResult SetActionStartTime( size_t index, float startTime );
	BlueStdResult SetActionEndTime( size_t index, float endTime );
	void AddAction( ITr2ControllerAction* action, float startTime, float endTime );
	BlueStdResult RemoveAction( size_t index );

	void RegisterCallback( const BlueSharedString& callbackName, const BlueScriptCallback& callback );
	void ClearCallbacks();

	float GetTime() const;
	void SetTime( float time );

	void ReLink();

private:
	std::string m_name;
	PITr2ControllerActionVector m_actions;
	PTr2TimelineEntryStructureList m_entries;
	PTr2ControllerFloatVariableVector m_variables;
	PTr2ControllerEventHandlerVector m_eventHandlers;

	std::vector<std::pair<BlueSharedString, BlueScriptCallback>> m_callbacks;

	Be::Time m_lastUpdateTime = 0;
	float m_time = 0;
	float m_timeScale = 1.0f;
	IRoot* m_owner = nullptr;

	std::vector<ITr2UpdateablePtr> m_updateables;

	std::vector<CcpParser::Variable> m_variableView;
	CcpMallocBuffer m_variableData;
	mutable CcpMallocBuffer m_tempArena;
	mutable std::vector<std::pair<std::string, IRoot*>> m_bindingPathRoots;

	bool m_isActive = false;
	bool m_isPaused = false;
};

TYPEDEF_BLUECLASS( Tr2TimelineContoller );