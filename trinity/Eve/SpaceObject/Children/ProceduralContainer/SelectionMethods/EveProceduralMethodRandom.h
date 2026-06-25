// Copyright © 2021 CCP ehf.

#pragma once

#include "IEveProceduralSelectionMethod.h"
#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "EveProceduralMethodRandomParameter.h"
#include "Tr2DebugRenderer.h"

BLUE_DECLARE( EveProceduralMethodRandomParameter );
BLUE_DECLARE_VECTOR( EveProceduralMethodRandomParameter );
BLUE_DECLARE_INTERFACE( IEveVolume );

BLUE_CLASS( EveProceduralMethodRandom ) :
	public IEveProceduralSelectionMethod,
	public INotify,
	public IListNotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	EveProceduralMethodRandom( IRoot* lockobj = NULL );
	~EveProceduralMethodRandom();

	void SelectARandomParameter();
	void GenerateParameterMapping();

	void SetProceduralMethodVariable( const char* name, float value ) override;
	const char* GetProceduralMethodVariable() override;

	//  IEveProceduralSelectionMethod
	void UpdateAsyncronous( const EveUpdateContext& updateContext, const EveChildUpdateParams& params ) override;
	bool IsSelectedChildModified() const override;
	EveChildRefPtr GetSelectedChild() override;
	IEveVolumeVector* GetDebugVolumes() override;

	//  IInitialize
	bool Initialize() override;

	//  IListNotify
	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const struct IList* theList ) override;

	//  INotify
	bool OnModified( Be::Var * value ) override;

protected:
	BlueSharedString m_name;
	BlueSharedString m_seedName;
	float m_seed;
	int m_selectedChildIndex;
	int m_totalWeight;
	std::vector<int> m_parameterMapping;
	PEveProceduralMethodRandomParameterVector m_parameters;

private:
	bool m_selectedChildModified;
	PIEveVolumeVector m_debugVolumes;
};

TYPEDEF_BLUECLASS( EveProceduralMethodRandom );