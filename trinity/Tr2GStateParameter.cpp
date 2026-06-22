// Copyright © 2020 CCP ehf.

#include "StdAfx.h"

#if GSTATE_ENABLED

#include "Tr2GStateParameter.h"

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GStateParameter default constructor
// --------------------------------------------------------------------------------------
Tr2GStateParameter::Tr2GStateParameter( IRoot* lockobj ) :
	m_name( "" ),
	m_nodeName( "" ),
	m_value( 0 )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2GStateParameter destructor
// --------------------------------------------------------------------------------------
Tr2GStateParameter::~Tr2GStateParameter()
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Get numeric value of the parameter
// Return Value:
//   float value
// --------------------------------------------------------------------------------------
const float Tr2GStateParameter::GetValue() const
{
	return m_value;
};

// --------------------------------------------------------------------------------------
// Description:
//   Get the name of the parameter
// Return Value:
//   std::string with the name
// --------------------------------------------------------------------------------------
const std::string& Tr2GStateParameter::GetName() const
{
	return m_name;
}

// --------------------------------------------------------------------------------------
// Description:
//   Get the name of the parameter node (of the parent of the parameter)
// Return Value:
//   std::string with the name
// --------------------------------------------------------------------------------------
const std::string& Tr2GStateParameter::GetNodeName() const
{
	return m_nodeName;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the numeric value of the parameter.
// Arguments:
//   val - numeric value typically between -1 and 1 but it could be out of those limits
// Return Value:
//   void
// --------------------------------------------------------------------------------------
void Tr2GStateParameter::SetValue( float val )
{
	m_value = val;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the name of the parameter.
// Arguments:
//   name - std:string with the name of the parameter
// Return Value:
//   void
// --------------------------------------------------------------------------------------
void Tr2GStateParameter::SetName( std::string name )
{
	m_name = name;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the name of the parameter node (the parent of the parameter).
// Arguments:
//   name - std:string with the name of the parameter node
// Return Value:
//   void
// --------------------------------------------------------------------------------------
void Tr2GStateParameter::SetNodeName( std::string name )
{
	m_nodeName = name;
}


// --------------------------------------------------------------------------------------
// Description:
//   Chekcs if the parameter has been initialized.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GStateParameter::IsInitialized() const
{
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Initializa the parameter
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2GStateParameter::Initialize()
{
	return true;
}

#endif