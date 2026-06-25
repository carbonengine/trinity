// Copyright © 2010 CCP ehf.

#pragma once
#ifndef Tr2MaterialParameterStore_H
#define Tr2MaterialParameterStore_H

#include "include/ITriEffectParameter.h"

BLUE_DECLARE( Tr2MaterialParameterStore );

// --------------------------------------------------------------------------------------
// Description:
//   Tr2MaterialParameterStore is a collection of shader parameters used to override the defaults
//		of a HighLevelShader.  The MaterialStore may be chained upwards, deriving from
//		a parent's selection of material parameters
// --------------------------------------------------------------------------------------

class Tr2MaterialParameterStore : public IInitialize,
								  public INotify
{
public:
	Tr2MaterialParameterStore( IRoot* lockobj = NULL );
	virtual ~Tr2MaterialParameterStore();

	EXPOSE_TO_BLUE();

	ITriEffectParameter* FindParameter( const char* name );


	using IInitialize::Lock;
	using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

private:
	void LoadParentResource();

protected:
	//	The name of this material
	std::string m_name;

	//	Optionally the path to a parent material we derive from
	std::string m_parentPath;

	//	The pointer to the parent material, if loaded
	Tr2MaterialParameterStorePtr m_parentStore;

	//	Our local overrides
	PITriEffectParameterDict m_parameters;
};

TYPEDEF_BLUECLASS( Tr2MaterialParameterStore );
BLUE_DECLARE_VECTOR( Tr2MaterialParameterStore );

#endif
