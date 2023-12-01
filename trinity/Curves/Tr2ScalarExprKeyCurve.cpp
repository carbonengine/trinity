////////////////////////////////////////////////////////////
//
//    Created:   September 2011
//    Copyright: CCP 2011
//

#include "StdAfx.h"
#include "Tr2ScalarExprKeyCurve.h"
#include "include/TriMath.h"

// --------------------------------------------------------------------------------------
// Description:
//   A wrapper for Perlin noise fractal sum function to pass to muParser.
// Arguments:
//   x - agument for function
//   a - octave scale multiplier
//   b - octave frequency multiplier
//   n - number of octaves
// Return Value:
//   Perlin fractal sum
// --------------------------------------------------------------------------------------
static float perlin_wrap( float x, float a, float b, float n )
{
	return (float)((PerlinNoise1D( x, a, b, (int)n)+1.0)*0.5);
}

// --------------------------------------------------------------------------------------
// Description:
//   A wrapper for Perlin noise 3 octave fractal sum function to pass to muParser.
// Arguments:
//   x - agument for function
// Return Value:
//   Perlin fractal sum
// --------------------------------------------------------------------------------------
static float perlin_wrap_simple( float x )
{
	return (float)((PerlinNoise1D( x, 1.1, 2.0, (int)3)+1.0)*0.5);
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a random number between two given numbers.
// Arguments:
//   a - min range for random value
//   b - max range for random value
// Return Value:
//   Random value between a and b
// --------------------------------------------------------------------------------------
static float frandom( float a, float b )
{
	return ((b-a)*((float)rand()/RAND_MAX))+a;
}

namespace
{
CcpParser::Function s_functions[] = {
	CcpParser::Function( "perlin_simple", perlin_wrap_simple, CcpParser::FunctionFlags::PURE_FUNC ),
	CcpParser::Function( "perlin", perlin_wrap, CcpParser::FunctionFlags::PURE_FUNC ),
	CcpParser::Function( "random", frandom ),
};
CcpParser::Constant s_constants[] = {
	{ "pi", 3.1415926f },
	{ "pi2", 2.0f * 3.1415926f },
};
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ScalarExprKey destructor.
// --------------------------------------------------------------------------------------
Tr2ScalarExprKey::Tr2ScalarExprKey( IRoot* lockobj ) 
:	m_inputVar1( 0.0f ),
	m_inputVar2( 0.0f ),
	m_inputVar3( 0.0f ),
	m_inputVar4( 0.0f ),
	m_leftTangent( 0.0f ),
	m_rightTangent( 0.0f ),
	m_prevKeyTime( 0.0f ),
	m_prevKeyValue( 0.0f ),
	m_randomMin( 0.0f ),
	m_randomMax( 0.0f ),
	m_randomConstant( 0.0f )
{
	m_time = 0.0f;
	m_value = 0.0f;
	m_interpolation = LINEAR;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface.  Compiles muParser expressions.
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ScalarExprKey::Initialize()
{
	SetExpression( m_timeParser, m_timeExpression );
	SetExpression( m_valueParser, m_valueExpression );
	SetExpression( m_leftTangentParser, m_leftTangentExpression );
	SetExpression( m_rightTangentParser, m_rightTangentExpression );
	RegenRandomConstant();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements INotify interface.  Allows the key to respond to parameter 
//   changes generated in Python. If one of expression changes it is recompiled.
//   Any change to attributes also triggers all expressions to be re-evaluated.  
// Arguments:
//   value - The Blue-exposed parameter that changed
// Return Value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ScalarExprKey::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_timeExpression ) )
	{
		SetExpression( m_timeParser, m_timeExpression );
	}
	else if( IsMatch( value, m_valueExpression ) )
	{
		SetExpression( m_valueParser, m_valueExpression );
	}
	else if( IsMatch( value, m_leftTangentExpression ) )
	{
		SetExpression( m_leftTangentParser, m_leftTangentExpression );
	}
	else if( IsMatch( value, m_rightTangentExpression ) )
	{
		SetExpression( m_rightTangentParser, m_rightTangentExpression );
	}

	void* buffers[] = { this };
	if( !m_timeExpression.empty() && m_timeParser )
	{
		m_time = m_timeParser.Eval( buffers, m_tempArena.data() );
	}
	if( !m_valueExpression.empty() && m_valueParser )
	{
		m_value = m_valueParser.Eval( buffers, m_tempArena.data() );
	}
	if( !m_leftTangentExpression.empty() && m_leftTangentParser )
	{
		m_leftTangent = m_leftTangentParser.Eval( buffers, m_tempArena.data() );
	}
	if( !m_rightTangentExpression.empty() && m_rightTangentParser )
	{
		m_rightTangent = m_rightTangentParser.Eval( buffers, m_tempArena.data() );
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-compiles muParser with a new expression.  
// Arguments:
//   parser - A parser
//   expression - Expression to compile
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKey::SetExpression( CcpParser::Program& parser, std::string& expression )
{
	if( expression.empty() )
	{
		parser = {};
		return;
	}
	CcpParser::Variable s_variables[] = {
		{ "value", 0, offsetof( Tr2ScalarExprKey, m_time ) },
		{ "time", 0, offsetof( Tr2ScalarExprKey, m_time ) },
		{ "input1", 0, offsetof( Tr2ScalarExprKey, m_inputVar1 ) },
		{ "input2", 0, offsetof( Tr2ScalarExprKey, m_inputVar2 ) },
		{ "input3", 0, offsetof( Tr2ScalarExprKey, m_inputVar3 ) },
		{ "input4", 0, offsetof( Tr2ScalarExprKey, m_inputVar4 ) },
		{ "randomConstant", 0, offsetof( Tr2ScalarExprKey, m_randomConstant ) },
		{ "leftTangent", 0, offsetof( Tr2ScalarExprKey, m_leftTangent ) },
		{ "rightTangent", 0, offsetof( Tr2ScalarExprKey, m_rightTangent ) },
		{ "prevKeyTime", 0, offsetof( Tr2ScalarExprKey, m_prevKeyTime ) },
		{ "prevKeyValue", 0, offsetof( Tr2ScalarExprKey, m_prevKeyValue ) },
	};

	CcpParser::FunctionView functionView[] = { s_functions };
	CcpParser::ConstantView constantView[] = { s_constants };
	CcpParser::VariableView variableView[] = { s_variables };

	CcpParser::Externals externals;
	externals.functions = functionView;
	externals.variables = variableView;
	externals.constants = constantView;
	auto result = CcpParser::Parse( expression.c_str(), externals, parser );
	if( !result )
	{
		CCP_LOGERR( "Tr2ScalarExprKey expression: %s", ToString( result, expression.c_str() ).c_str() );
		return;
	}
	size_t arenaSize = 0;
	arenaSize = std::max( arenaSize, m_timeParser.GetTempArenaSize() );
	arenaSize = std::max( arenaSize, m_valueParser.GetTempArenaSize() );
	arenaSize = std::max( arenaSize, m_leftTangentParser.GetTempArenaSize() );
	arenaSize = std::max( arenaSize, m_rightTangentParser.GetTempArenaSize() );
	if( m_tempArena.size() < arenaSize )
	{
		m_tempArena.resize( arenaSize );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-generates random constant (passed to muParser expressions).  
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKey::RegenRandomConstant()
{
	m_randomConstant = m_randomMin + float( rand() ) / float( RAND_MAX ) * ( m_randomMax - m_randomMin );
}

// --------------------------------------------------------------------------------------
// Description:
//   Updates data from the previous key in the curve and re-evaluates muParser expressions.
// Arguments:
//   previousKey - Previous key in the curve
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKey::UpdateValues( Tr2ScalarExprKey* previousKey )
{
	if( previousKey )
	{
		m_prevKeyTime = previousKey->m_time;
		m_prevKeyValue = previousKey->m_value;
	}
	else
	{
		m_prevKeyTime = 0.0f;
		m_prevKeyValue = 0.0f;
	}
	void* buffers[] = { this };
	if( !m_timeExpression.empty() && m_timeParser )
	{
		m_time = m_timeParser.Eval( buffers, m_tempArena.data() );
	}
	if( !m_valueExpression.empty() && m_valueParser )
	{
		m_value = m_valueParser.Eval( buffers, m_tempArena.data() );
	}
	if( !m_leftTangentExpression.empty() && m_leftTangentParser )
	{
		m_leftTangent = m_leftTangentParser.Eval( buffers, m_tempArena.data() );
	}
	if( !m_rightTangentExpression.empty() && m_rightTangentParser )
	{
		m_rightTangent = m_rightTangentParser.Eval( buffers, m_tempArena.data() );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ScalarExprKeyCurve constructor.
// --------------------------------------------------------------------------------------
Tr2ScalarExprKeyCurve::Tr2ScalarExprKeyCurve( IRoot* lockobj ) 
:	m_currentValue( 0.f ),
	m_reversed( false ),
	m_cycle( false ),
	m_timeOffset( 0.f ),
	m_timeScale( 1.f ),
	m_interpolation( LINEAR ),
	m_startTangent( 0 ),
	m_endTangent( 0 ),
	PARENTLOCK( m_keys )
{
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-evaluates key expressions. Disguised as "Sort" function for compatibility with
//   the curve editor.
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::Sort()
{
	ReEvaluateKeys();
}

// --------------------------------------------------------------------------------------
// Description:
//   Re-evaluates key expressions.
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::ReEvaluateKeys()
{
	Tr2ScalarExprKey* previousKey = NULL;
	for( PTr2ScalarExprKeyVector::iterator it = m_keys.begin(); it != m_keys.end(); ++it )
	{
		( *it )->UpdateValues( previousKey );
		previousKey = *it;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns left tangent of specified key or the starting tangent.
// --------------------------------------------------------------------------------------
float Tr2ScalarExprKeyCurve::GetKeyLeftTangent( unsigned int idx )
{
	if ( idx < (unsigned int)m_keys.size() )
	{
		return m_keys[idx]->m_leftTangent;
	}
	return m_startTangent;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assignes a new left tangent to a key.
// Arguments:
//   idx - key index
//   tangent - new tangent value
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::SetKeyLeftTangent( unsigned int idx, float tangent )
{
	if ( idx < (unsigned int)m_keys.size() )
	{
		m_keys[idx]->m_leftTangent = tangent;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns right tangent of specified key or the ending tangent.
// Return Value:
//   right tangent of specified key or the ending tangent
// --------------------------------------------------------------------------------------
float Tr2ScalarExprKeyCurve::GetKeyRightTangent( unsigned int idx )
{
	if ( idx < (unsigned int)m_keys.size() )
	{
		return m_keys[idx]->m_rightTangent;
	}
	return m_endTangent;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assignes a new right tangent to a key.
// Arguments:
//   idx - key index
//   tangent - new tangent value
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::SetKeyRightTangent( unsigned int idx, float tangent )
{
	if ( idx < (unsigned int)m_keys.size() )
	{
		m_keys[idx]->m_rightTangent = tangent;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriCurveLength interface. Returns curve length.
// Return value:
//   returns curve length (time)
// --------------------------------------------------------------------------------------
float Tr2ScalarExprKeyCurve::Length()
{
	if( m_keys.empty() )
	{
		return 0.0f;
	}
	return m_keys.back()->m_time - m_keys.front()->m_time;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements IInitialize interface. Re-evaluates keys keys.
// Return value:
//   true always
// --------------------------------------------------------------------------------------
bool Tr2ScalarExprKeyCurve::Initialize()
{
	ReEvaluateKeys();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITriFunction interface. Updates curve current value.
// Arguments:
//   time - current time
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::UpdateValue( double time )
{
	m_currentValue = GetValueAt( time );
}

// --------------------------------------------------------------------------------------
// Description:
//   Computes curve value.
// Arguments:
//   time - time
// Return value:
//   curve value at a given time
// --------------------------------------------------------------------------------------
float Tr2ScalarExprKeyCurve::GetValueAt( double time )
{
	if ( m_keys.empty() )
	{
		return 0.0f;
	}

	ReEvaluateKeys();

	float length = Length();

	float result;

	time = time / (double)m_timeScale - (double)m_timeOffset;

	if ( length <= 0.0f || time <= 0.0 )
	{
		return m_keys.front()->m_value;
	}

	if ( time > length + m_keys.front()->m_time )
	{
		if ( m_cycle )
		{
			time = m_keys.front()->m_value + fmod( time - m_keys.front()->m_value, (double)length );
		}
		else
		{
			if ( m_reversed )
			{
				return m_keys.front()->m_value;
			}
			else
			{
				return m_keys.back()->m_value;
			}
		}
	}	

	if ( m_reversed )
	{
		time = m_keys.front()->m_value + ( (double)length - ( time - m_keys.front()->m_value ) );
	}

	Tr2ScalarExprKey* startKey = m_keys[0];
	Tr2ScalarExprKey* endKey = m_keys.back();
	if ( time <= startKey->m_time ) // We are between the start of the curve and the first key
	{
		Interpolate( &result, float( time ), NULL, startKey );
	}
	else if ( time >= endKey->m_time ) // We are between the last key and the end of the curve
	{
		Interpolate( &result, float( time ), endKey, NULL );
	}
	else
	{	
		endKey = startKey;
		for ( size_t i = 1; i < m_keys.size(); ++i )
		{
			startKey = endKey;
			endKey = m_keys[i];
			if ( endKey->m_time > time )
			{
				break;
			}
		}
		Interpolate( &result, float( time ), startKey, endKey );
	}
	return result;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns key value.
// Arguments:
//   idx - key index
// Return value:
//   value of key
// --------------------------------------------------------------------------------------
float Tr2ScalarExprKeyCurve::GetKeyValue( unsigned int idx )
{
	return m_keys[idx]->m_value;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns new key value.
// Arguments:
//   idx - key index
//   value - new key value
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::SetKeyValue( unsigned int idx, float value )
{
	m_keys[idx]->m_value = value;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns key time.
// Arguments:
//   idx - key index
// Return value:
//   key time
// --------------------------------------------------------------------------------------
float Tr2ScalarExprKeyCurve::GetKeyTime( unsigned int idx )
{
	return m_keys[idx]->m_time;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns new key time.
// Arguments:
//   idx - key index
//   tim - new key time
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::SetKeyTime( unsigned int idx, float time )
{
	m_keys[idx]->m_time = time;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns key interpolation type.
// Arguments:
//   idx - key index
// Return value:
//   key interpolation type
// --------------------------------------------------------------------------------------
unsigned int Tr2ScalarExprKeyCurve::GetKeyInterpolation( unsigned int idx )
{
	return m_keys[idx]->m_interpolation;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns new key interpolation type.
// Arguments:
//   idx - key index
//   interp - new interpolation type
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::SetKeyInterpolation( unsigned int idx, unsigned int interp )
{
	m_keys[idx]->m_interpolation = Interpolation( interp );
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a new key.
// Arguments:
//   time - new key time
//   value - new key value
//   leftTangent - new key left tangent value
//   rightTangent - new key right tangent value
//   interpolation - new key interpolation value
// Return value:
//   new key index in keys list
// --------------------------------------------------------------------------------------
int Tr2ScalarExprKeyCurve::AddKey( float time, float value, float leftTangent, float rightTangent, unsigned int interpolation )
{
	int index = 0;
	while( index < int( m_keys.size() ) )
	{
		if( m_keys[index]->m_time > time )
		{
			break;
		}
		index++;
	}

	Tr2ScalarExprKeyPtr key;
	if( !key.CreateInstance() )
	{
		return -1;
	}

	key->m_time = time;
	key->m_value = value;
	key->m_leftTangent = leftTangent;
	key->m_rightTangent = rightTangent;
	key->m_interpolation = (Interpolation)interpolation;
	m_keys.Insert( index, key->GetRawRoot() );
	return index;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a key.
// Arguments:
//   idx - index of the key to remove
// --------------------------------------------------------------------------------------
void Tr2ScalarExprKeyCurve::RemoveKey( unsigned int idx )
{
	m_keys.Remove( idx );
}


// --------------------------------------------------------------------------------------
// Description:
//   Implements Tr2Curve method. Returns curve value at current time.
// Arguments:
//   out - (out) Curve value
//   lastKey - key with the largest time less than current time (or NULL)
//   nextKey - key with the smallest time greater than current time (or NULL)
// Return Value:
//   curve value at current time
// --------------------------------------------------------------------------------------
float* Tr2ScalarExprKeyCurve::Interpolate( float* out, float time, Tr2ScalarExprKey* lastKey, Tr2ScalarExprKey* nextKey )
{
	*out = m_keys.front()->m_value;
	float deltaTime = Length();
	float startValue = m_keys.front()->m_value;
	float endValue = m_keys.back()->m_value;
	unsigned int interp = m_interpolation;
	if ( lastKey )
	{
		interp = lastKey->m_interpolation;
		time -=  lastKey->m_time;
	}

	// The tr2 curves have by default a start and end point
	switch( interp )
	{
	case LINEAR:
		{	// We are in between two keys start------x---0----x------end
			if ( lastKey && nextKey  )
			{
				startValue = lastKey->m_value;
				endValue = nextKey->m_value;
				deltaTime = nextKey->m_time - lastKey->m_time;
			}
			// We are in between the start of the curve and the next key
			// start--0---x-------x------end
			else if ( lastKey == NULL && nextKey != NULL )
			{
				startValue = endValue = nextKey->m_value;
				deltaTime = nextKey->m_time;
			}
			// We are in between the end of the curve and the last key
			// start------x-------x--0---end
			else if ( lastKey != NULL && nextKey == NULL )
			{
				startValue = endValue = lastKey->m_value;
				deltaTime = Length() - lastKey->m_time;
			}// else there are no keys, just the start and end point . start---0---end
			*out = startValue + ( endValue - startValue)*(time/deltaTime);
			break;
		}
	case HERMITE:
		{
			float inTangent = 0.0f;
			float outTangent = 0.0f;
			// We are in between two keys start------x---0----x------end
			if ( lastKey && nextKey  )
			{
				startValue = lastKey->m_value;
				inTangent = lastKey->m_rightTangent;
				endValue = nextKey->m_value;
				outTangent = nextKey->m_leftTangent;
				deltaTime = nextKey->m_time - lastKey->m_time;
			}
			// We are in between the start of the curve and the next key
			// start--0---x-------x------end
			else if ( lastKey == NULL && nextKey != NULL )
			{
				startValue = endValue = nextKey->m_value;
				outTangent = nextKey->m_leftTangent;
				deltaTime = nextKey->m_time;
			}
			// We are in between the end of the curve and the last key
			// start------x-------x--0---end
			else if ( lastKey != NULL && nextKey == NULL )
			{
				startValue = endValue = lastKey->m_value;
				inTangent = lastKey->m_rightTangent;
				deltaTime = Length() - lastKey->m_time;
			}// else there are no keys, just the start and end point . start---0---end

			float s = time/deltaTime;
			float s2 = s*s;
			float s3 = s2*s;

			float c2 = -2.0f*s3 + 3.0f*s2;
			float c1 =  1.0f - c2;
			float c4 = s3 - s2;
			float c3 = s + c4 - s2;

			*out = startValue*c1 + endValue*c2 + inTangent * c3 + outTangent * c4;
			break;
		}
	default:
		break;
	}

	return out;
}

