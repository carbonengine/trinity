// Copyright © 2013 CCP ehf.

#pragma once
#ifndef ALRESULT_H
#define ALRESULT_H

#ifndef TRACK_ALRESULT
#if TRINITYDEV == 1
#define TRACK_ALRESULT 1
#else
#define TRACK_ALRESULT 0
#endif
#endif

#ifndef _WIN32
typedef int32_t HRESULT;
#define SUCCEEDED( hr ) ( ( (HRESULT)( hr ) ) >= 0 )
#define FAILED( hr ) ( ( (HRESULT)( hr ) ) < 0 )

#define S_OK ( (HRESULT)0L )
#define S_FALSE ( (HRESULT)1L )
#define E_FAIL HRESULT( 0x80004005L )
#define E_INVALIDARG HRESULT( 0x80070057L )
#define E_INVALIDCALL HRESULT( 0x8876086C )
#define E_OUTOFMEMORY HRESULT( 0x8007000E )
#define E_DEVICELOST HRESULT( 0x88760868 )
#elif ( TRINITYPLATFORM != TRINITYSTUB )
#define E_INVALIDCALL D3DERR_INVALIDCALL
#define E_DEVICELOST D3DERR_DEVICELOST
#else
#define E_INVALIDCALL HRESULT( 0x8876086C )
#define E_DEVICELOST HRESULT( 0x88760868 )
#endif


#if defined( _DEBUG ) || defined( TRINITYDEV )
void ReportHresultError( const char* fileName, int lineNumber, const char* statement, HRESULT hr );
void BreakInDebugger();

#define CR( x )                                                \
	{                                                          \
		HRESULT _hr = x;                                       \
		if( FAILED( _hr ) )                                    \
		{                                                      \
			ReportHresultError( __FILE__, __LINE__, #x, _hr ); \
			BreakInDebugger();                                 \
		}                                                      \
	}

#define CR_RETURN( x )                                         \
	{                                                          \
		HRESULT _hr = x;                                       \
		if( FAILED( _hr ) )                                    \
		{                                                      \
			ReportHresultError( __FILE__, __LINE__, #x, _hr ); \
			BreakInDebugger();                                 \
			return;                                            \
		}                                                      \
	}

#define CR_RETURN_HR( x )                                      \
	{                                                          \
		HRESULT _hr = x;                                       \
		if( FAILED( _hr ) )                                    \
		{                                                      \
			ReportHresultError( __FILE__, __LINE__, #x, _hr ); \
			BreakInDebugger();                                 \
			return _hr;                                        \
		}                                                      \
	}

#define FORWARD_HR( x )     \
	{                       \
		HRESULT _hr = x;    \
		if( FAILED( _hr ) ) \
		{                   \
			return _hr;     \
		}                   \
	}

#define CR_RETURN_VAL( x, failValue )                          \
	{                                                          \
		HRESULT _hr = x;                                       \
		if( FAILED( _hr ) )                                    \
		{                                                      \
			ReportHresultError( __FILE__, __LINE__, #x, _hr ); \
			BreakInDebugger();                                 \
			return failValue;                                  \
		}                                                      \
	}
#else
#define CR( x ) x
#define CR_RETURN( x )    \
	{                     \
		if( FAILED( x ) ) \
			return;       \
	}
#define CR_RETURN_HR( x )   \
	{                       \
		HRESULT _hr = x;    \
		if( FAILED( _hr ) ) \
			return _hr;     \
	}
#define CR_RETURN_VAL( x, failValue ) \
	{                                 \
		if( FAILED( x ) )             \
			return failValue;         \
	}
#define FORWARD_HR( x )     \
	{                       \
		HRESULT _hr = x;    \
		if( FAILED( _hr ) ) \
			return _hr;     \
	}
#endif


#if !TRINITY_AL_WITH_BLUE_EXPOSURE

// If TrinityAL is compiled without BlueExposure module, we need to
// declare its Be::Result.

namespace Be
{

template <typename T>
struct Result
{
};

}

template <typename T>
inline bool BeIsSuccess( const Be::Result<T>& )
{
	return false;
}
template <typename T>
const char* BeGetErrorMessage( const Be::Result<T>& result )
{
	return "";
}

#endif

// --------------------------------------------------------------------------------------
// Description:
//   A specialization of Be::Result for HRESULT type. Most AL operations return this
//   object as a method result. You can use usual macros (SUCCEEDED, FAIL, CR, etc.) with
//   this object.
//   When TRACK_ALRESULT macro is defined to 1 this object also tracks if its result
//   was checked and if it wasn't, the object will check the result in its destructor
//	 using CR macro.
// --------------------------------------------------------------------------------------
template <>
struct Be::Result<HRESULT>
{
public:
	// Broad common categories of AL errors
	enum Category
	{
		// OK
		SUCCESS,
		// General (unclassified) error
		OTHER,
		// Invalid call (bad arguments or context)
		INVALID_CALL,
		// Lost GPU device
		DEVICE_LOST,
		// GPU device is not (yet) available
		DEVICE_NOT_AVAILABLE,
		// Out of memory
		OUT_OF_MEMORY,
	};

#if TRACK_ALRESULT == 0
	Result<HRESULT>() :
		m_result( S_OK )
	{
	}

	Result<HRESULT>( HRESULT result ) :
		m_result( result )
	{
	}

	HRESULT GetResult() const
	{
		return m_result;
	}
#else
	Result<HRESULT>() :
		m_result( S_OK ), m_isChecked( false )
	{
	}

	Result<HRESULT>( HRESULT result ) :
		m_result( result ), m_isChecked( false )
	{
	}

	Result<HRESULT>( const Result<HRESULT>& result ) :
		m_result( result.m_result ), m_isChecked( false )
	{
		result.m_isChecked = true;
	}

	~Result<HRESULT>()
	{
		if( !m_isChecked )
		{
			CR( m_result );
		}
	}

	Result<HRESULT>& operator=( const Result<HRESULT>& other )
	{
		if( !m_isChecked )
		{
			CR( m_result );
		}
		other.m_isChecked = true;
		m_result = other.m_result;
		m_isChecked = false;
		return *this;
	}

	HRESULT GetResult() const
	{
		m_isChecked = true;
		return m_result;
	}
#endif

	operator HRESULT() const
	{
		return GetResult();
	}

	Category GetCategory() const;

	bool operator==( HRESULT hr ) const
	{
		return m_result == hr;
	}

private:
	Result<HRESULT>( bool );
	operator bool() const;

	HRESULT m_result;
#if TRACK_ALRESULT
	mutable bool m_isChecked;
#endif
};

template <>
inline bool BeIsSuccess( const Be::Result<HRESULT>& result )
{
	return SUCCEEDED( result );
}

template <>
const char* BeGetErrorMessage( const Be::Result<HRESULT>& result );


#if TRINITY_AL_WITH_BLUE_EXPOSURE && BLUE_WITH_PYTHON

// It TrinityAL is compiled with BlueExposure we declare, but not define
// GetException function for ALResult. We declare it here so that it's picked up
// by BlueExposure whenever ALResult is used. Its body needs to be defined outside
// TrinityAL.
template <>
PyObject* BeGetException( const Be::Result<HRESULT>& result );

#endif

typedef Be::Result<HRESULT> ALResult;

#endif