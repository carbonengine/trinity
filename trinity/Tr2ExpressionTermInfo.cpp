// Copyright © 2018 CCP ehf.

#include "StdAfx.h"
#include "Tr2ExpressionTermInfo.h"


std::vector<std::string> Tr2ExpressionTermInfo::GetArguments() const
{
	return m_arguments;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Variable( const char* category, const char* name, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = VARIABLE;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Function( const char* category, const char* name, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Function( const char* category, const char* name, const char* arg0, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	term->m_arguments.push_back( arg0 );
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	term->m_arguments.push_back( arg0 );
	term->m_arguments.push_back( arg1 );
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* arg2, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	term->m_arguments.push_back( arg0 );
	term->m_arguments.push_back( arg1 );
	term->m_arguments.push_back( arg2 );
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* arg2, const char* arg3, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	term->m_arguments.push_back( arg0 );
	term->m_arguments.push_back( arg1 );
	term->m_arguments.push_back( arg2 );
	term->m_arguments.push_back( arg3 );
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	term->m_arguments.push_back( arg0 );
	term->m_arguments.push_back( arg1 );
	term->m_arguments.push_back( arg2 );
	term->m_arguments.push_back( arg3 );
	term->m_arguments.push_back( arg4 );
	term->m_arguments.push_back( arg5 );
	return term;
}

Tr2ExpressionTermInfoPtr Tr2ExpressionTermInfo::StringFunction( const char* category, const char* name, const char* arg0, const char* description )
{
	Tr2ExpressionTermInfoPtr term;
	term.CreateInstance();
	term->m_type = STRING_FUNCTION;
	term->m_category = category;
	term->m_name = name;
	term->m_description = description;
	term->m_arguments.push_back( arg0 );
	return term;
}
