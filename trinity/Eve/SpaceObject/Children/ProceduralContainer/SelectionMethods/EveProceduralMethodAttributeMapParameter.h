// Copyright © 2021 CCP ehf.

#pragma once

#include "Eve/SpaceObject/Children/EveChildRef.h"
#include "Tr2DebugRenderer.h"

BLUE_CLASS( EveProceduralMethodAttributeMapParameter ) :
	public INotify,
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();
	EveProceduralMethodAttributeMapParameter( IRoot* lockobj = NULL );
	~EveProceduralMethodAttributeMapParameter();

	const char* GetName() const;
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

private:
	bool m_modified;
};

TYPEDEF_BLUECLASS( EveProceduralMethodAttributeMapParameter );