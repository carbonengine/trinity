// Copyright © 2021 CCP ehf.

#pragma once

#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "Tr2DebugRenderer.h"

BLUE_CLASS( EveProceduralMethodRandomParameter ) :
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();
	EveProceduralMethodRandomParameter( IRoot* lockobj = NULL );
	~EveProceduralMethodRandomParameter();

	const char* GetName() const;
	void SetName( const char* name );

	int GetWeighting() const;
	void SetModified( bool isModified );
	bool IsModified() const;
	EveChildRefPtr GetChild();
	void Load();

	//  INotify
	bool OnModified( Be::Var * value ) override;

	//  IInitialize
	bool Initialize() override;

protected:
	BlueSharedString m_name;
	EveChildRefPtr m_child;
	int m_weighting;

private:
	bool m_modified;
};

TYPEDEF_BLUECLASS( EveProceduralMethodRandomParameter );