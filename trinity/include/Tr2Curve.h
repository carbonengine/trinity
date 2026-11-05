// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2Curve_h
#define Tr2Curve_h
#include "ITriFunction.h"
#include "ITriCurveLength.h"

#include <math.h>
#ifdef _WIN32
#define isnan( x ) _isnan( x )
#define isinf( x ) !_finite( x )
#endif

enum Interpolation
{
	CONSTANT,
	LINEAR,
	HERMITE,
	CATMULLROM,
	SPHERICAL_LINEAR, // spherical linear interpolation
	SPHERICAL_QUADRANGLE, // spherical quadrangle interpolation
};

template <class T>
class Tr2Key
{
public:
	float m_time;
	T m_value;
	Interpolation m_interpolation;
};

template <class Key, class KeyList, class KeyValue>
class Tr2CurveBase : public IInitialize,
					 public ITriCurveLength
{
public:
	Tr2CurveBase( IRoot* lockobj = NULL );

	//////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	//////////////////////////////////////////////////////////////////////////
	// ITriCurveLength
	float Length()
	{
		return m_length;
	}

	bool m_reversed;
	bool m_cycle;
	float m_length;
	float m_localTime;
	std::string m_name;

	// internal time offset
	float m_timeOffset;
	// internal time scale
	float m_timeScale;

	KeyValue m_startValue;
	KeyValue m_currentValue;
	KeyValue m_endValue;
	unsigned int m_interpolation;

	KeyList m_keys;

	// Caching objects
	int m_currentKeyIdx; // index to the current key in the list
	Key* m_lastKey;
	Key* m_nextKey;
	float m_startOfSegment;
	float m_endOfSegment;

	// Value access
	KeyValue GetValue( double time )
	{
		return GetValueAt( time );
	}
	KeyValue GetValueAt( double time );
	const KeyValue& GetKeyValue( unsigned int idx );
	void SetKeyValue( unsigned int idx, const KeyValue& value );

	float GetKeyTime( unsigned int idx );
	void SetKeyTime( unsigned int idx, float time );

	unsigned int GetKeyInterpolation( unsigned int idx );
	void SetKeyInterpolation( unsigned int idx, unsigned int interp );

	unsigned int GetKeyCount() const
	{
		return (unsigned int)m_keys.size();
	}

	// String access
	std::string GetName() const;
	void SetName( std::string name );

	// Key manipulation
	int AddKey( float time, const KeyValue& value );
	void RemoveKey( unsigned int idx );
	virtual void Sort() = 0;

	virtual KeyValue* Interpolate( KeyValue* out, Key* startKey, Key* endKey ) = 0;

private:
	virtual void AddKey_( float time, const KeyValue& value ) = 0;
};

template <class Key, class KeyList, class KeyValue>
class Tr2Curve : public Tr2CurveBase<Key, KeyList, KeyValue>,
				 public ITriFunction
{
public:
	Tr2Curve( IRoot* lockobj = NULL ) :
		Tr2CurveBase<Key, KeyList, KeyValue>( lockobj )
	{
	}

	//////////////////////////////////////////////////////////////////////////
	// ITriFunction
	void UpdateValue( double time )
	{
		this->m_currentValue = this->GetValueAt( time );
	}
};

/////////////////////////////////////////////////////////////////////////////
// IInitialize
template <class Key, class KeyList, class KeyValue>
bool Tr2CurveBase<Key, KeyList, KeyValue>::Initialize()
{
	Sort();
	return true;
}

template <class Key, class KeyList, class KeyValue>
Tr2CurveBase<Key, KeyList, KeyValue>::Tr2CurveBase( ::IRoot* lockobj ) :
	PARENTLOCK2( m_keys, IInitialize )
{
	m_lastKey = NULL;
	m_nextKey = NULL;
	m_startOfSegment = 0.0f;
	m_endOfSegment = 0.0f;

	m_length = 0.0f;
	m_cycle = false;
	m_reversed = false;
	m_currentKeyIdx = 0;
	m_interpolation = LINEAR;
	m_timeOffset = 0.f;
	m_timeScale = 1.f;
	m_localTime = 0;
}

template <class Key, class KeyList, class KeyValue>
std::string Tr2CurveBase<Key, KeyList, KeyValue>::GetName() const
{
	return m_name;
}

template <class Key, class KeyList, class KeyValue>
void Tr2CurveBase<Key, KeyList, KeyValue>::SetName( std::string name )
{
	m_name = name;
}

template <class Key, class KeyList, class KeyValue>
void Tr2CurveBase<Key, KeyList, KeyValue>::RemoveKey( unsigned int idx )
{
	size_t numKeys = m_keys.size();
	if( numKeys && idx < numKeys )
	{
		m_keys.Remove( idx );
		Sort();

		m_lastKey = NULL;
		m_nextKey = NULL;
	}
}

template <class Key, class KeyList, class KeyValue>
int Tr2CurveBase<Key, KeyList, KeyValue>::AddKey( float time, const KeyValue& value )
{
	// This function should return the index of the key where it was added to m_keys
	// If the time is already in a key and thus the key's value is updated for that time
	// return the index of the updated key.
	// Otherwise, if no key was added, it returns -1.
	int keyIndex = -1;

	// Key at zero is the start point
	if( time == 0.0f )
	{
		m_startValue = value;
		return keyIndex;
	}

	// time at the end point is the end key
	if( time == m_length )
	{
		m_endValue = value;
		return keyIndex;
	}

	// If there are no keys, the sorting will not fix the curve
	// Therefor, we have to create a new key at current endpoint and then move the endpoint
	// to the new value.
	if( m_keys.empty() )
	{
		if( time > m_length )
		{
			if( m_length > 0.0f )
			{
				// Need to create a new key here using the current endpoint value
				AddKey_( m_length, m_endValue );

				// We know that we have 1 and only 1 key in m_keys.
				keyIndex = 0;
			}

			// Update end value
			m_length = time;
			m_endValue = value;

			// If keyIndex == -1 =>
			// We got a time value > 0 for a curve of zero length
			// Thus, we have shifted the length to that time value
			// and updated the end value accordingly.
			if( keyIndex == -1 )
			{
				return keyIndex;
			}
		}
		else
		{
			// If we are between the begin and end.
			AddKey_( time, value );
			return 0;
		}
	}
	else
	{
		// If a key exists at the same time as the new key... don't add, just update
		for( unsigned int i = 0; i < m_keys.size(); ++i )
		{
			if( m_keys[i]->m_time == time )
			{
				m_keys[i]->m_value = value;
				return i;
			}
		}

		// Call inherited implementation
		AddKey_( time, value );

		Key* key = m_keys.back();

		// Maintain base invariants
		Sort();

		// Get the index of the inserted key
		keyIndex = (int)m_keys.FindKey( key->GetRawRoot() );
	}

	m_lastKey = NULL;
	m_nextKey = NULL;

	return keyIndex;
}

template <class Key, class KeyList, class KeyValue>
const KeyValue& Tr2CurveBase<Key, KeyList, KeyValue>::GetKeyValue( unsigned int idx )
{
	if( idx < m_keys.size() )
	{
		return m_keys[idx]->m_value;
	}
	return m_endValue;
}

template <class Key, class KeyList, class KeyValue>
void Tr2CurveBase<Key, KeyList, KeyValue>::SetKeyValue( unsigned int idx, const KeyValue& value )
{
	if( idx < m_keys.size() )
	{
		m_keys[idx]->m_value = value;
	}
}

template <class Key, class KeyList, class KeyValue>
float Tr2CurveBase<Key, KeyList, KeyValue>::GetKeyTime( unsigned int idx )
{
	if( idx < m_keys.size() )
	{
		return m_keys[idx]->m_time;
	}
	return m_length;
}

template <class Key, class KeyList, class KeyValue>
void Tr2CurveBase<Key, KeyList, KeyValue>::SetKeyTime( unsigned int idx, float time )
{
	if( idx < m_keys.size() )
	{
		m_keys[idx]->m_time = time;
	}
}


template <class Key, class KeyList, class KeyValue>
unsigned int Tr2CurveBase<Key, KeyList, KeyValue>::GetKeyInterpolation( unsigned int idx )
{
	if( idx < m_keys.size() )
	{
		return (unsigned int)m_keys[idx]->m_interpolation;
	}
	return m_interpolation;
}

template <class Key, class KeyList, class KeyValue>
void Tr2CurveBase<Key, KeyList, KeyValue>::SetKeyInterpolation( unsigned int idx, unsigned int interp )
{
	if( idx < m_keys.size() )
	{
		m_keys[idx]->m_interpolation = (Interpolation)interp;
	}
}

template <class Key, class KeyList, class KeyValue>
KeyValue Tr2CurveBase<Key, KeyList, KeyValue>::GetValueAt( double time )
{
	KeyValue result;

	if( isnan( time ) )
	{
		time = 0.0;
	}

	time = time / (double)m_timeScale - (double)m_timeOffset;

	if( m_length <= 0.0f || time <= 0.0 )
	{
		return m_startValue;
	}

	if( time > m_length )
	{
		if( m_cycle )
		{
			time = fmod( time, (double)m_length );
		}
		else
		{
			if( m_reversed )
			{
				return m_startValue;
			}
			else
			{
				return m_endValue;
			}
		}
	}

	if( m_reversed )
	{
		time = (double)m_length - time;
	}

	m_localTime = (float)time;

	if( !m_keys.size() )
	{
		Interpolate( &result, NULL, NULL );
	}
	else
	{
		// If the current time is within our cached segment
		if( ( m_lastKey || m_nextKey ) && time > m_startOfSegment && time <= m_endOfSegment )
		{
			Interpolate( &result, m_lastKey, m_nextKey );
		}
		else
		{
			Key* startKey = m_keys[0];
			Key* endKey = m_keys.back();
			if( time <= startKey->m_time ) // We are between the start of the curve and the first key
			{
				m_startOfSegment = 0.0f;
				m_endOfSegment = startKey->m_time;
				m_currentKeyIdx = 0;
				m_lastKey = NULL;
				m_nextKey = startKey;
				Interpolate( &result, NULL, startKey );
			}
			else if( time >= endKey->m_time ) // We are between the last key and the end of the curve
			{
				m_startOfSegment = endKey->m_time;
				m_endOfSegment = m_length;
				m_currentKeyIdx = 0;
				m_lastKey = endKey;
				m_nextKey = NULL;
				Interpolate( &result, endKey, NULL );
			}
			else
			{
				startKey = m_keys[m_currentKeyIdx];
				// If the caching is wrong, start from the beginning
				if( startKey->m_time > time )
				{
					startKey = m_keys[0];
					m_currentKeyIdx = 0;
				}

				endKey = m_keys[m_currentKeyIdx + 1];
				for( size_t i = m_currentKeyIdx; i < ( m_keys.size() - 1 ); ++i )
				{
					if( startKey->m_time <= time && endKey->m_time > time )
					{
						break;
					}
					++m_currentKeyIdx;
					startKey = endKey;
					endKey = m_keys[m_currentKeyIdx + 1];
				}
				m_startOfSegment = startKey->m_time;
				m_endOfSegment = endKey->m_time;
				m_lastKey = startKey;
				m_nextKey = endKey;
				Interpolate( &result, startKey, endKey );
			}
		}
	}
	return result;
}

#endif //Tr2Curve_h
