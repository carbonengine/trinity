// Copyright © 2025 CCP ehf.

#pragma once


#include "ITr2Controller.h"
#include "Actions/ITr2ControllerAction.h"
#include "Eve/SpaceObject/Utils/EveThrottleable.h"

BLUE_DECLARE( Tr2ControllerFloatVariable );
BLUE_DECLARE_VECTOR( Tr2ControllerFloatVariable );
BLUE_DECLARE( Tr2ControllerEventHandler );
BLUE_DECLARE_VECTOR( Tr2ControllerEventHandler );
BLUE_DECLARE_INTERFACE( ITr2Updateable );


struct Tr2TimelineEntry
{
	float startTime;
	float endTime;
	uint32_t trackID;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2TimelineEntry );


BLUE_CLASS( Tr2TimelineController ) :
	public ITr2ActionController,
	public ISimTimeRebaseNotify,
	public EveThrottleable
{
public:
	Tr2TimelineController( IRoot* lockobj = nullptr );
	~Tr2TimelineController();

	EXPOSE_TO_BLUE();

	void Link( IRoot & owner ) override;
	void Unlink( UnlinkReason reason = UnlinkReason::UNLINKING ) override;
	bool IsLinked() const override;
	void Start() override;
	void Stop() override;
	void Update( float normalizedUpdateFrequency ) override;
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
	BlueStdResult GetActionTrackID( size_t index, uint32_t& trackID ) const;
	BlueStdResult SetActionStartTime( size_t index, float startTime );
	BlueStdResult SetActionEndTime( size_t index, float endTime );
	BlueStdResult SetActionTrackID( size_t index, uint32_t trackID );
	void AddAction( ITr2ControllerAction * action, float startTime, float endTime, uint32_t trackID = 0 );
	BlueStdResult RemoveAction( size_t index );

	bool IsActionEnabled( size_t index ) const;

	bool IsTrackEnabled( uint32_t trackID ) const;
	void EnableTrack( uint32_t trackID, bool enable );

	void RegisterCallback( const BlueSharedString& callbackName, const BlueScriptCallback& callback );
	void ClearCallbacks();

	float GetTime() const;
	void SetTime( float time );

	void Pause();
	void Resume();

	void ReLink();

private:
	std::string m_name;
	PITr2ControllerActionVector m_actions;
	PTr2TimelineEntryStructureList m_entries;
	PTr2ControllerFloatVariableVector m_variables;
	PTr2ControllerEventHandlerVector m_eventHandlers;

	std::vector<std::pair<BlueSharedString, BlueScriptCallback>> m_callbacks;

	std::vector<uint32_t> m_disabledTracks;

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

TYPEDEF_BLUECLASS( Tr2TimelineController );