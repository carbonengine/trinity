#pragma once
#ifndef TriEventCurve_h
#define TriEventCurve_h


#include "TriEventKey.h"
#include "include/ITriFunction.h"
#include "include/ITriCurveLength.h"

BLUE_DECLARE( TriEventCurve );


class TriEventCurve:
     public ITriFunction,
	 public IInitialize,
	 public ITriCurveLength
{
public:
    EXPOSE_TO_BLUE();
    TriEventCurve( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	void UpdateValue( double time );

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITriEventCurve
	float Length() { return m_length; }

	void Sort();

	void AddKey( float time, const std::wstring& evtName );

	void InsertKey( TriEventKey* key );
#if BLUE_WITH_PYTHON
	void AddCallableKey( float time, PyObject* callable, PyObject* args );
#endif
	void RemoveKey( int ix );
	float GetKeyTime( int ix );
	std::wstring GetKeyValue( int ix );
	void SetKeyTime( int ix, float time );
	void SetKeyValue( int ix, std::wstring value );

#if BLUE_WITH_PYTHON
    PyObject* GetCallableKeyValue( int ix );
    PyObject* GetCallableKeyArgs( int ix );
#endif
	int GetKeyCount();

private:
	std::string m_name;
	double m_time;
	float m_localTime;
	std::wstring m_value;
	float m_length;

	TRIEXTRAPOLATION m_extrapolation;
	PTriEventKeyVector m_keys;
	TriEventKeyVector::const_iterator m_currentKeyIt;

	IBlueEventListenerPtr m_eventListener;
};

TYPEDEF_BLUECLASS( TriEventCurve );
#endif //TriEventCurve_h
