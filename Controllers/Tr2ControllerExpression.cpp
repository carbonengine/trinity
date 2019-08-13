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
	const Tr2StateMachine* s_stateMachine = nullptr;
	IRoot* s_owner = nullptr;

	float StateTime()
	{
		return s_stateMachine ? s_stateMachine->GetStateRunTime() : 0;
	}

	float CurveSetTime( const char* name )
	{
		ITr2CurveSetOwnerPtr csOwner = BlueCastPtr( s_owner );
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

	float AnimationTime( const char* name )
	{
		EveSpaceObject2Ptr spaceObject = BlueCastPtr( s_owner );
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

	float IsAnimationPlaying( const char* layerName )
	{
		EveSpaceObject2Ptr spaceObject = BlueCastPtr( s_owner );
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

	float ShipSpeed()
	{
		if( g_controllerFunctionOverrideEnabled )
		{
			return g_controllerShipSpeed;
		}
		if( IEveSpaceObject2Ptr spaceObject = BlueCastPtr( s_owner ) )
		{
			Vector3 velocity;
			spaceObject->GetWorldVelocity( velocity );
			return Length( velocity );
		}
		else if( EveChildContainerPtr child = BlueCastPtr( s_owner ) )
		{
			Vector3 velocity;
			child->GetWorldVelocity( velocity );
			return Length( velocity );
		}
		return 0;
	}

	float ShipMaxSpeed()
	{
		if( g_controllerFunctionOverrideEnabled )
		{
			return g_controllerShipMaxSpeed;
		}
		if( EveShip2Ptr ship = BlueCastPtr( s_owner ) )
		{
			auto speed = ship->GetMaxSpeed();
			return speed > 0 ? speed : 1;
		}
		return 1;
	}

	bool IsValidVariableName( const char* name )
	{
		static std::regex namePattern( "[a-zA-Z_][a-zA-Z_0-9]*" );
		return std::regex_match( name, namePattern );
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
	float GetServerYear()
	{
		return 0;
	}

	float GetServerMonth()
	{
		return 0;
	}

	float GetServerDay()
	{
		return 0;
	}

	float GetServerDayOfWeek()
	{
		return 0;
	}

	float GetServerHour()
	{
		return 0;
	}

	float GetServerMinute()
	{
		return 0;
	}

	float GetServerSecond()
	{
		return 0;
	}
#endif

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
	float CompareTimeFloats(float arg, float arg2)
	{
		if ( arg != -1 )
		{
			if ( arg2 > arg )
			{
				return 1;
			}
	
			if ( arg2 < arg )
			{
				return 0;
			}
		}
		return -1;
	}

	float ServerTimeGreaterThan( float year, float month, float day, float hour, float minute, float second )
	{
		float ret = -1;
		
		ret = CompareTimeFloats( year, GetServerYear() );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( month, GetServerMonth() );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( day, GetServerDay() );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( hour, GetServerHour() );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( minute, GetServerMinute() );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( second, GetServerSecond() );
		if ( ret != -1 ) return ret;
		return 1;
	}

	float ServerTimeLessThan( float year, float month, float day, float hour, float minute, float second )
	{
		float ret = -1;

		ret = CompareTimeFloats(  GetServerYear(), year );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( GetServerMonth(), month );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats(  GetServerDay(), day );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats( GetServerHour(), hour );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats(  GetServerMinute(),  minute );
		if ( ret != -1 ) return ret;
		ret = CompareTimeFloats(  GetServerSecond(), second );
		if ( ret != -1 ) return ret;
		return 1;
	}
}


Tr2ControllerExpression::Tr2ControllerExpression()
	:m_stateMachine( nullptr ),
	m_controller( nullptr )
{
}

std::string Tr2ControllerExpression::SetExpr( const char* expression, const Tr2StateMachine& stateMachine )
{
	m_stateMachine = &stateMachine;
	m_controller = stateMachine.GetController();
	return CreateParser( expression, nullptr );
}

std::string Tr2ControllerExpression::SetExpr( const char* expression, const Tr2Controller& controller, ModifyParser modifyParser )
{
	m_stateMachine = nullptr;
	m_controller = &controller;
	return CreateParser( expression, modifyParser );
}

std::string Tr2ControllerExpression::CreateParser( const char* expression, ModifyParser modifyParser )
{
	m_expressionParser = mu::Parser();
	auto& variables = m_controller->GetVariables();
	for( auto it = begin( variables ); it != end( variables ); ++it )
	{
		if( !IsValidVariableName( ( *it )->GetName().c_str() ) )
		{
			continue;
		}
		m_expressionParser.DefineVar( ( *it )->GetName(), ( *it )->GetPointerForParser() );
	}

	m_expressionParser.DefineFun( "StateTime", StateTime, false );
	m_expressionParser.DefineFun( "AnimationTime", AnimationTime, false );
	m_expressionParser.DefineFun( "IsAnimationPlaying", IsAnimationPlaying, false );
	m_expressionParser.DefineFun( "CurveSetTime", CurveSetTime, false );
	m_expressionParser.DefineFun( "ShipSpeed", ShipSpeed, false );
	m_expressionParser.DefineFun( "ShipMaxSpeed", ShipMaxSpeed, false );
	m_expressionParser.DefineFun( "Random", Random, false );

	m_expressionParser.DefineFun( "ServerYear", GetServerYear, false );
	m_expressionParser.DefineFun( "ServerMonth", GetServerMonth, false );
	m_expressionParser.DefineFun( "ServerDay", GetServerDay, false );
	m_expressionParser.DefineFun( "ServerDayOfWeek", GetServerDayOfWeek, false );
	m_expressionParser.DefineFun( "ServerHour", GetServerHour, false );
	m_expressionParser.DefineFun( "ServerMinute", GetServerMinute, false );
	m_expressionParser.DefineFun( "ServerSecond", GetServerSecond, false );
	m_expressionParser.DefineFun( "ServerTimePhase", TimePhase, false );
	m_expressionParser.DefineFun( "ServerTimeGreaterThan", ServerTimeGreaterThan, false );
	m_expressionParser.DefineFun( "ServerTimeLessThan", ServerTimeLessThan, false );


	if( modifyParser )
	{
		( *modifyParser )( m_expressionParser );
	}
	
	m_expressionParser.SetExpr( expression );
	s_stateMachine = m_stateMachine;
	s_owner = m_controller->GetOwner();
	try
	{
		m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& e )
	{
		s_stateMachine = nullptr;
		s_owner = nullptr;
		return e.GetMsg();
	}
	s_stateMachine = nullptr;
	s_owner = nullptr;
	return std::string();
}

std::pair<bool, float> Tr2ControllerExpression::Eval() const
{
	if( !m_controller )
	{
		return std::make_pair( false, 0.f );
	}
	s_stateMachine = m_stateMachine;
	s_owner = m_controller->GetOwner();
	bool success = true;
	float result = 0.f;
	try
	{
		result = m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& )
	{
		success = false;
	}
	s_stateMachine = nullptr;
	s_owner = nullptr;
	return std::make_pair( success, result );
}

void Tr2ControllerExpression::Clear()
{
	m_stateMachine = nullptr;
	m_controller = nullptr;
	m_expressionParser = mu::Parser();
}

bool Tr2ControllerExpression::IsExpressionValid() const
{
	try
	{
		m_expressionParser.Eval();
	}
	catch( const mu::Parser::exception_type& )
	{
		return false;
	}
	return true;
}

void Tr2ControllerExpression::GetExpressionTermInfo( std::vector<Tr2ExpressionTermInfoPtr>& info ) const
{
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "StateTime", "time in seconds the current state is running" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "AnimationTime", "name", "geometry animation duration (in seconds) for the given name" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "CurveSetTime", "name", "duration (in seconds) of the curve set with the given name" ) );
	info.push_back( Tr2ExpressionTermInfo::StringFunction( "Controller", "IsAnimationPlaying", "name", "return 1 if the geometry animation in the given layer is playing; 0 otheriwise" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "ShipSpeed", "owning ship speed" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "ShipMaxSpeed", "owning ship maximum speed" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "Controller", "Random", "min", "max", "returns a random from 'min'-'max-1' " ) );

	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerYear", "returns a year for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerMonth", "returns a month (1-12) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerDay", "returns a day of month (1-31) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerDayOfWeek", "returns a day of week (1-7) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerHour", "returns an hour (0-23) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerMinute", "returns a minute (0-60) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerSecond", "returns a second (0-60) for current server time" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerTimePhase", "period", "returns seconds phase in a server time period of the given length" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerTimeGreaterThan", "year", "month", "day", "hour","minute", "second","args are: ( yyyy, mm, dd, hh, mm, ss )  use '-1' to ignore specific args" ) );
	info.push_back( Tr2ExpressionTermInfo::Function( "DateTime", "ServerTimeLessThan", "year", "month", "day", "hour", "minute", "second", "args are: ( yyyy, mm, dd, hh, mm, ss )  use '-1' to ignore specific args" ) );
}

