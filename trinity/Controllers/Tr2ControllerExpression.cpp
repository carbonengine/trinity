////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#include "StdAfx.h"
#include "Tr2ControllerExpression.h"
#include "Tr2Controller.h"
#include "Tr2StateMachine.h"
#include "Tr2ControllerFloatVariable.h"
#include "Eve/SpaceObject/EveSpaceObject2.h"
#include "Eve/SpaceObject/EveShip2.h"
#include "Eve/SpaceObject/Children/EveChildContainer.h"
#include "Eve/SpaceObject/Children/EveChildInstanceContainer.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2ExpressionTermInfo.h"
#include "TriSettingsRegistrar.h"
#include <regex>


bool g_controllerFunctionOverrideEnabled = false;
float g_controllerShipSpeed = 0;
float g_controllerShipMaxSpeed = 1;
Be::Time g_controllerServerTime = 0;

TRI_REGISTER_SETTING( "controllerFunctionOverrideEnabled", g_controllerFunctionOverrideEnabled );
TRI_REGISTER_SETTING( "controllerShipSpeed", g_controllerShipSpeed );
TRI_REGISTER_SETTING( "controllerShipMaxSpeed", g_controllerShipMaxSpeed );
TRI_REGISTER_SETTING( "controllerServerTime", g_controllerServerTime );


namespace
{
	float StateTime( const Tr2StateMachine* stateMachine )
	{
		return stateMachine ? stateMachine->GetStateRunTime() : 0;
	}

	float CurveSetTime( IRoot* owner, const char* name )
	{
		ITr2CurveSetOwnerPtr csOwner = BlueCastPtr( owner );
		if( !csOwner )
		{
			return 0;
		}
		if( auto slash = strchr( name, '/' ) )
		{
			std::string csName = name;
			std::string rangeName = csName.substr( slash - name + 1 );
			csName = csName.substr( 0, slash - name );
			return csOwner->GetRangeDuration( csName, rangeName );
		}
		else
		{
			return csOwner->GetCurveSetDuration( name );
		}
	}

	float GetExternalControllerVariable( IRoot* ctx, const char* name, float defaultVal )
	{
		ITr2ControllerOwnerPtr owner = BlueCastPtr( ctx );
		if( !owner )
		{
			return defaultVal;
		}

		float value = 0.0;
		if( owner->GetControllerValueByName( name, value ) )
		{
			return value;
		}

		return defaultVal;
	}

	float AnimationTime( IRoot* ctx, const char* name )
	{
		EveSpaceObject2Ptr spaceObject = BlueCastPtr( ctx );
		if( !spaceObject )
		{
			return 0;
		}
		auto ac = spaceObject->GetAnimationController();
		if( !ac )
		{
			return 0;
		}
		auto animation = ac->FindAnimationByName( name );
		if( !animation )
		{
			return 0;
		}
		return animation->Duration;
	}

	float Random( float min, float max)
	{
		float result = min + rand() % int( max - min );
		return result;
	}

	float Mod( float number, float mod )
	{
		if( mod == 0 )
		{
			return 0;
		}
		return fmod( number, mod );
	}

	float IsAnimationPlaying( IRoot* ctx, const char* layerName )
	{
		EveSpaceObject2Ptr spaceObject = BlueCastPtr( ctx );
		if( !spaceObject )
		{
			return 0;
		}
		auto ac = spaceObject->GetAnimationController();
		if( !ac )
		{
			return 0;
		}
		auto layer = ac->GetAnimationLayer( layerName && !*layerName ? nullptr : layerName );
		if( !layer )
		{
			return 0;
		}
		auto remaining = layer->GetAnimationRemainingTime();
		return remaining > 0;
	}

	float KillCount( IRoot* ctx )
	{
		if( EveShip2Ptr ship = BlueCastPtr( ctx ) )
		{
			return ship->GetKillCounterValue();
		}
		return 0.0f;
	}

	float BoundingSphereRadius( IRoot* ctx )
	{
		if( EveSpaceObject2Ptr spaceObject = BlueCastPtr( ctx ) )
		{
			return spaceObject->GetRadius();
		}
		return 0.0f;
	}

	float ShipSpeed( IRoot* ctx )
	{
		if( g_controllerFunctionOverrideEnabled )
		{
			return g_controllerShipSpeed;
		}
		if( IEveSpaceObject2Ptr spaceObject = BlueCastPtr( ctx ) )
		{
			Vector3 velocity;
			spaceObject->GetWorldVelocity( velocity );
			return Length( velocity );
		}
		else if( EveChildContainerPtr child = BlueCastPtr( ctx ) )
		{
			Vector3 velocity;
			child->GetWorldVelocity( velocity );
			return Length( velocity );
		}
		return 0;
	}

	float ShipMaxSpeed( IRoot* ctx )
	{
		if( g_controllerFunctionOverrideEnabled )
		{
			return g_controllerShipMaxSpeed;
		}
		if( EveShip2Ptr ship = BlueCastPtr( ctx ) )
		{
			auto speed = ship->GetMaxSpeed();
			return speed > 0 ? speed : 1;
		}
		else if( EveChildContainerPtr child = BlueCastPtr( ctx ) )
		{
			auto speed = child->GetOwnerMaxSpeed();
			return speed > 0 ? speed : 1;
		}
		else if( EveChildInstanceContainerPtr child = BlueCastPtr( ctx ) )
		{
			auto speed = child->GetOwnerMaxSpeed();
			return speed > 0 ? speed : 1;
		}
		return 1;
	}

	float BoosterIntensity( void* ctx )
	{
		if( EveShip2Ptr ship = BlueCastPtr( static_cast<IRoot*>( ctx ) ) )
		{
			auto intensity = ship->GetBoosterIntensity();
			return intensity > 0 ? intensity: 0;
		}
		return 0.0f;
	}

	bool IsValidVariableName( const char* name )
	{
		auto isLetter = []( char x ) {
			return ( x >= 'a' && x <= 'z' ) || ( x >= 'A' && x <= 'Z' ) || ( x == '_' );
		};
		auto isDigit = []( char x ) {
			return x >= '0' && x <= '9';
		};
		if( !isLetter( *name ) )
		{
			return false;
		}
		++name;
		while( *name )
		{
			if( !isLetter( *name ) && !isDigit( *name) )
			{
				return false;
			}
			++name;
		}
		return true;
	}

#ifdef _WIN32
	// We really need something like FileTimeToSystemTime in blue and platform-independent

	SYSTEMTIME GetServerTimeParts()
	{
		Be::Time time;
		if( g_controllerFunctionOverrideEnabled )
		{
			time = g_controllerServerTime;
		}
		else
		{
			time = BeOS->GetCurrentFrameTime();
		}

		SYSTEMTIME st = {};
		FileTimeToSystemTime( (FILETIME*)&time, &st );
		return st;
	}

	float GetServerYear()
	{
		return float( GetServerTimeParts().wYear );
	}

	float GetServerMonth()
	{
		return float( GetServerTimeParts().wMonth );
	}

	float GetServerDay()
	{
		return float( GetServerTimeParts().wDay );
	}

	float GetServerDayOfWeek()
	{
		return float( GetServerTimeParts().wDayOfWeek );
	}

	float GetServerHour()
	{
		return float( GetServerTimeParts().wHour );
	}

	float GetServerMinute()
	{
		return float( GetServerTimeParts().wMinute );
	}

	float GetServerSecond()
	{
		return float( GetServerTimeParts().wSecond );
	}

#else
    struct tm GetServerTimeParts()
    {
        Be::Time time;
        if( g_controllerFunctionOverrideEnabled )
        {
            time = g_controllerServerTime;
        }
        else
        {
            time = BeOS->GetCurrentFrameTime();
        }

        time_t t = time_t( time / 10000000 );
        return *localtime( &t );
    }
    float GetServerYear()
    {
        return float( 1900 + GetServerTimeParts().tm_year );
    }

    float GetServerMonth()
    {
        return float( GetServerTimeParts().tm_mon + 1 );
    }

    float GetServerDay()
    {
        return float( GetServerTimeParts().tm_mday );
    }

    float GetServerDayOfWeek()
    {
        return float( GetServerTimeParts().tm_wday );
    }

    float GetServerHour()
    {
        return float( GetServerTimeParts().tm_hour );
    }

    float GetServerMinute()
    {
        return float( GetServerTimeParts().tm_min );
    }

    float GetServerSecond()
    {
        return float( GetServerTimeParts().tm_sec );
    }
#endif

	float IsWeekend()
	{
		if( int( GetServerDayOfWeek() ) % 6 == 0 )
		{
			return 1.f;
		}
		else
		{
			return 0.f;
		}
	}

	float TimePhase( float period )
	{
		auto p = TimeFromDouble( period );
		if( !p )
		{
			return 0;
		}
		if( p < 0 )
		{
			p = -p;
		}
		Be::Time time;
		if( g_controllerFunctionOverrideEnabled )
		{
			time = g_controllerServerTime;
		}
		else
		{
			time = BeOS->GetCurrentFrameTime();
		}
		return TimeAsFloat( time % p );
	}

	// Helper function to reduce nesting in f: ServerTimeGreaterThan() and ServerTimeLessThan()
	float CompareTimeFloats(float arg1, float arg2)
	{
		if ( arg1 == -1 || arg2 == -1 )
		{
			return 1.f;	
		}
		
		if( arg1 >= arg2 )
		{
			return 1.f;
		}
		
		return 0.f;		
	}

	float ServerTimeGreaterThan( float year, float month, float day, float hour, float minute, float second )
	{
		float isTrue = 1.f;
		isTrue *= CompareTimeFloats( GetServerYear(), year );
		isTrue *= CompareTimeFloats( GetServerMonth(), month );
		isTrue *= CompareTimeFloats( GetServerDay(), day );
		isTrue *= CompareTimeFloats( GetServerHour(), hour );
		isTrue *= CompareTimeFloats( GetServerMinute(), minute );
		isTrue *= CompareTimeFloats( GetServerSecond(), second );
		return isTrue;
	}

	float ServerTimeLessThanOrEqual( float year, float month, float day, float hour, float minute, float second )
	{
		float isTrue = 1.f;
		isTrue *= CompareTimeFloats( year, GetServerYear() );
		isTrue *= CompareTimeFloats( month, GetServerMonth() );
		isTrue *= CompareTimeFloats( day, GetServerDay() );
		isTrue *= CompareTimeFloats( hour, GetServerHour() );
		isTrue *= CompareTimeFloats( minute, GetServerMinute() );
		isTrue *= CompareTimeFloats( second, GetServerSecond() );
		return isTrue;
	}

	CcpParser::Function s_functions[] = {
		CcpParser::Function( "StateTime", StateTime, Tr2ControllerExpression::STATE_MACHINE_BUFFER_INDEX, 0 ),
		CcpParser::Function( "AnimationTime", AnimationTime, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "IsAnimationPlaying", IsAnimationPlaying, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "CurveSetTime", CurveSetTime, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "GetExternalControllerVariable", GetExternalControllerVariable, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "ShipSpeed", ShipSpeed, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "ShipMaxSpeed", ShipMaxSpeed, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "Random", Random ),
		CcpParser::Function( "ShipBoosterIntensity", BoosterIntensity, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "KillCount", KillCount, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),
		CcpParser::Function( "BoundingSphereRadius", BoundingSphereRadius, Tr2ControllerExpression::OWNER_BUFFER_INDEX, 0 ),

		CcpParser::Function( "IsWeekend", IsWeekend ),
		CcpParser::Function( "ServerYear", GetServerYear ),
		CcpParser::Function( "ServerMonth", GetServerMonth ),
		CcpParser::Function( "ServerDay", GetServerDay ),
		CcpParser::Function( "ServerDayOfWeek", GetServerDayOfWeek ),
		CcpParser::Function( "ServerHour", GetServerHour ),
		CcpParser::Function( "ServerMinute", GetServerMinute ),
		CcpParser::Function( "ServerSecond", GetServerSecond ),
		CcpParser::Function( "ServerTimePhase", TimePhase ),
		CcpParser::Function( "ServerTimeGreaterThan", ServerTimeGreaterThan ),
		CcpParser::Function( "ServerTimeLessThanOrEqual", ServerTimeLessThanOrEqual ),
	};


	struct ParserObserver: public CcpParser::Observer
	{
		CcpParser::VariableView m_variables = {};
		uint64_t m_mask = 0;
		bool m_maskOverflow = false;
		bool m_hasNonPureFunctions = false;

		void OnVariable( const CcpParser::Variable* variable ) override
		{
			auto offset = variable - m_variables.data;
			if( offset >= 0 && offset < ptrdiff_t( m_variables.count ) )
			{
				if( offset >= 64 )
				{
					m_maskOverflow = true;
				}
				else
				{
					m_mask |= 1ull << offset;
				}
			}
		}
		
		void OnFunction( const CcpParser::Function* func ) override
		{
			if( !HasFlag( func->flags, CcpParser::FunctionFlags::PURE_FUNC ) )
			{
				m_hasNonPureFunctions = true;
			}
		}
	};
}


Tr2ControllerExpression::Tr2ControllerExpression()
	:m_stateMachine( nullptr ),
	m_controller( nullptr ),
	m_variableMask( 0 )
{
}

std::string Tr2ControllerExpression::SetExpr( const char* expression, const Tr2StateMachine& stateMachine )
{
	Clear();
	m_stateMachine = &stateMachine;
	m_controller = stateMachine.GetController();
	return CreateParser( expression, {} );
}

std::string Tr2ControllerExpression::SetExpr( const char* expression, const Tr2Controller& controller, const CcpParser::FunctionView& extraFunctions )
{
	Clear();
	m_stateMachine = nullptr;
	m_controller = &controller;
	return CreateParser( expression, extraFunctions );
}

std::string Tr2ControllerExpression::CreateParser( const char* expression, const CcpParser::FunctionView& extraFunctions )
{
	CcpParser::Externals externals;
	CcpParser::VariableView varViews[] = { m_controller->GetVariableView() };
	externals.variables = varViews;
	CcpParser::FunctionView funcViews[2] = { extraFunctions, s_functions };
	externals.functions = { funcViews, 2 };
	ParserObserver observer;
	observer.m_variables = varViews[0];
	auto parsed = CcpParser::Parse( expression, externals, m_program, &observer );
	if( parsed )
	{
		m_controller->EnsureTempArenaSize( m_program.GetTempArenaSize() );
		m_variableMask = observer.m_maskOverflow || observer.m_hasNonPureFunctions ? 0ull : observer.m_mask;
		return std::string();
	}
	else
	{
		return ToString( parsed, expression );
	}
}

std::pair<bool, float> Tr2ControllerExpression::Eval( void* extraBuffer ) const
{
	if( !m_controller || !m_program )
	{
		return std::make_pair( false, 0.f );
	}
	auto owner = m_controller->GetOwner();
	void* externals[] = { m_controller->GetVariableBuffer(), &owner, (void*)&m_stateMachine, extraBuffer };
	float result = m_program.Eval( externals, m_controller->GetTempArena() );
	return std::make_pair( true, result );
}

void Tr2ControllerExpression::Clear()
{
	if( m_controller )
	{
		m_program = CcpParser::Program();
	}
	m_stateMachine = nullptr;
	m_controller = nullptr;
}

bool Tr2ControllerExpression::IsExpressionValid() const
{
	return m_program;
}

uint64_t Tr2ControllerExpression::GetVariableMask() const
{
	return m_variableMask;
}

void Tr2ControllerExpression::GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& info ) const
{
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "StateTime", "time in seconds the current state is running" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "AnimationTime", "name", "geometry animation duration (in seconds) for the given name" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "CurveSetTime", "name", "duration (in seconds) of the curve set with the given name" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "GetExternalControllerVariable", "name", "defaultValue", "Peek into another controller and get the value, if value is not found returns the default value" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "IsAnimationPlaying", "name", "return 1 if the geometry animation in the given layer is playing; 0 otheriwise" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "ShipSpeed", "owning ship speed" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "ShipMaxSpeed", "owning ship maximum speed" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "ShipBoosterIntensity", "Owning ships boosters intensity" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "Random", "min", "max", "returns a random from 'min'-'max-1' " ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "KillCount", "Owning ship kill count" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "BoundingSphereRadius", "Owning object's bounding sphere radius" ) );


	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "IsWeekend", "is it the weekend (output: 1.0 = yes, 0.0 = no)" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerYear", "returns a year for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerMonth", "returns a month (1-12) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerDay", "returns a day of month (1-31) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerDayOfWeek", "returns a day of week (1-7) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerHour", "returns an hour (0-23) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerMinute", "returns a minute (0-60) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerSecond", "returns a second (0-60) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerTimePhase", "period", "returns seconds phase in a server time period of the given length" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerTimeGreaterThan", "year", "month", "day", "hour","minute", "second","args are: ( yyyy, mm, dd, hh, mm, ss )  use '-1' to ignore specific args" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerTimeLessThanOrEqual", "year", "month", "day", "hour", "minute", "second", "args are: ( yyyy, mm, dd, hh, mm, ss )  use '-1' to ignore specific args" ) );
}

