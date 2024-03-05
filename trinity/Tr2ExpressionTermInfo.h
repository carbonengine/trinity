////////////////////////////////////////////////////////////
//
//    Created:   May 2018
//    Copyright: CCP 2018
//

#pragma once


BLUE_CLASS( Tr2ExpressionTermInfo ) : public IRoot
{
public:
	EXPOSE_TO_BLUE();

	enum TermType
	{
		VARIABLE,
		FUNCTION,
		STRING_FUNCTION,
	};

	TermType m_type;
	std::string m_category;
	std::string m_name;
	std::vector<std::string> m_arguments;
	std::string m_description;

	std::vector<std::string> GetArguments() const;

	static Tr2ExpressionTermInfoPtr Variable( const char* category, const char* name, const char* description );
	static Tr2ExpressionTermInfoPtr Function( const char* category, const char* name, const char* description );
	static Tr2ExpressionTermInfoPtr Function( const char* category, const char* name, const char* arg0, const char* description );
	static Tr2ExpressionTermInfoPtr Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* description );
	static Tr2ExpressionTermInfoPtr Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* arg2, const char* description );
	static Tr2ExpressionTermInfoPtr Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* arg2, const char* arg3, const char* description );
	static Tr2ExpressionTermInfoPtr Function( const char* category, const char* name, const char* arg0, const char* arg1, const char* arg2, const char* arg3, const char* arg4, const char* arg5, const char* description );
	static Tr2ExpressionTermInfoPtr StringFunction( const char* category, const char* name, const char* arg0, const char* description );
};

TYPEDEF_BLUECLASS( Tr2ExpressionTermInfo );
