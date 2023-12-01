#include "StdAfx.h"
#include "Tr2PostProcess.h"
#include "Tr2VariableStore.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Tr2PostProcess
Tr2PostProcess::Tr2PostProcess( IRoot* lockobj ) :
	PARENTLOCK( m_stages )
{
}

Tr2PostProcess::~Tr2PostProcess()
{
}

bool Tr2PostProcess::Initialize()
{
    return true;
}
