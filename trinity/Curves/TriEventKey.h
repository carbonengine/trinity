#pragma once
#ifndef TriEventKey_h
#define TriEventKey_h


#include <string>

BLUE_CLASS( TriEventKey ):
     public IRoot
{
public:
    EXPOSE_TO_BLUE();

    TriEventKey( IRoot* lockobj = NULL );
	~TriEventKey();

	float m_time;
	std::wstring m_value;
#if BLUE_WITH_PYTHON
	PyObject* m_callable;
	PyObject* m_callableArgs;
#endif
};
BLUE_DECLARE_VECTOR( TriEventKey );

TYPEDEF_BLUECLASS( TriEventKey );
#endif //TriEventKey_h
