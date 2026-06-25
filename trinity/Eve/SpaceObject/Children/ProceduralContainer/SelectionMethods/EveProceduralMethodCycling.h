// Copyright © 2021 CCP ehf.

#pragma once

#include "IEveProceduralSelectionMethod.h"
#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "EveProceduralMethodCyclingParameter.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveProceduralMethodCyclingParameter );
BLUE_DECLARE_VECTOR( EveProceduralMethodCyclingParameter );
BLUE_DECLARE_INTERFACE( IEveVolume );


BLUE_CLASS( EveProceduralMethodCycling ) :
	public IEveProceduralSelectionMethod,
	public IListNotify,
	public INotify
{
public:
	EXPOSE_TO_BLUE();

	EveProceduralMethodCycling( IRoot* lockobj = NULL );
	~EveProceduralMethodCycling();

	void SelectParameter();

	//  IEveProceduralSelectionMethod
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	bool IsSelectedChildModified() const override;
	EveChildRefPtr GetSelectedChild() override;
	IEveVolumeVector* GetDebugVolumes() override;

	//  IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;

	//  INotify
	bool OnModified( Be::Var * value ) override;

protected:
	BlueSharedString m_name;
	int m_selectedChildIndex;
	PEveProceduralMethodCyclingParameterVector m_parameters;

private:
	bool m_selectedChildModified;
	bool m_randomizeOrder;
	PIEveVolumeVector m_debugVolumes;
	Be::Time m_startTime;
	float m_startTimeOffset;
};

TYPEDEF_BLUECLASS( EveProceduralMethodCycling );