// Copyright © 2023 CCP ehf.

#pragma once

#include "Tr2CurveVector3.h"
#include "ccpparser.h"

#include <ITriFunction.h>
#include <ITriCurveLength.h>

BLUE_DECLARE_IVECTOR( ITriScalarFunction );
BLUE_DECLARE( Tr2ExpressionTermInfo );

BLUE_CLASS( Tr2CurveVector3Expression ) :
	public ITriColorFunction, public ITriVectorFunction, public IInitialize
{
public:
	Tr2CurveVector3Expression( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool Initialize();

	virtual void UpdateValue( double time );

	Vector3 GetValue( double time ) const;

	std::string GetExpression( size_t index ) const;
	void SetExpression( size_t index, const std::string& expression );

	std::string GetExpressionX() const;
	void SetExpressionX( const std::string& expression );

	std::string GetExpressionY() const;
	void SetExpressionY( const std::string& expression );

	std::string GetExpressionZ() const;
	void SetExpressionZ( const std::string& expression );

	float GetRandomConstant() const;
	float GetInputValue( int index ) const;
	float GetInputValue( int index, float time ) const;

	virtual Color* Update( Color * in, Be::Time time );
	virtual Color* Update( Color * in, double time );
	virtual Color* GetValueAt( Color * in, Be::Time time );
	virtual Color* GetValueAt( Color * in, double time );

	virtual Vector3* Update( Vector3 * in, Be::Time time );
	virtual Vector3* Update( Vector3 * in, double time );
	virtual Vector3* GetValueAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueAt( Vector3 * in, double time );
	virtual Vector3* GetValueDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDotAt( Vector3 * in, double time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, Be::Time time );
	virtual Vector3* GetValueDoubleDotAt( Vector3 * in, double time );
	virtual Vector3d* InterpolatedPosition( Vector3d * out, Be::Time time );

	void ResetRandomConstant();
	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
	BlueStdResult EvaluateExpression( const char* expression, float& value ) const;

private:
	std::string m_name;
	std::string m_expressions[3];
	CcpParser::Program m_programs[3];
	std::unique_ptr<uint8_t[]> m_tempArena;

	PITriScalarFunctionVector m_inputs;

	Vector3 m_currentValue;
	float m_timeScale;
	float m_randomConstant;

	struct Arguments
	{
		mutable float m_time = 0;

		float m_input1 = 0;
		float m_input2 = 0;
		float m_input3 = 0;
		float m_input4 = 0;
	} m_arguments;
};

TYPEDEF_BLUECLASS( Tr2CurveVector3Expression );
