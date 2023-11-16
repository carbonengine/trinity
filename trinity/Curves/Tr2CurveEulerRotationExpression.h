////////////////////////////////////////////////////////////
//
//    Created:   June 2017
//    Copyright: CCP 2017
//

#pragma once


#include "Include/ITriFunction.h"
#include "Include/ITriCurveLength.h"
#include "ccpparser.h"

BLUE_DECLARE_IVECTOR( ITriScalarFunction );
BLUE_DECLARE( Tr2ExpressionTermInfo );

BLUE_CLASS( Tr2CurveEulerRotationExpression ) : public ITriQuaternionFunction, public IInitialize
{
public:
	Tr2CurveEulerRotationExpression( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool Initialize();

	virtual void UpdateValue( double time );

	Quaternion GetValue( double time ) const;

	std::string GetExpression( size_t index ) const;
	void SetExpression( size_t index, const std::string& expression );

	std::string GetExpressionYaw() const;
	void SetExpressionYaw( const std::string& expression );

	std::string GetExpressionPitch() const;
	void SetExpressionPitch( const std::string& expression );

	std::string GetExpressionRoll() const;
	void SetExpressionRoll( const std::string& expression );

	float GetRandomConstant() const;
	float GetInputValue( int index ) const;
	float GetInputValue( int index, float time ) const;


	virtual Quaternion* Update( Quaternion* in, Be::Time time );
	virtual Quaternion* Update( Quaternion* in, double time );
	virtual Quaternion* GetValueAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueAt( Quaternion* in, double time );
	virtual Quaternion* GetValueDotAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueDotAt( Quaternion* in, double time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion* in, Be::Time time );
	virtual Quaternion* GetValueDoubleDotAt( Quaternion* in, double time );

	void ResetRandomConstant();
	std::vector<Tr2ExpressionTermInfoPtr> GetExpressionTermInfo() const;
	BlueStdResult EvaluateExpression( const char* expression, float& value ) const;
private:
	std::string m_name;
	std::string m_expressions[3];
	CcpParser::Program m_programs[3];
	std::unique_ptr<uint8_t[]> m_tempArena;

	PITriScalarFunctionVector m_inputs;

	Quaternion m_currentValue;
	float m_timeScale;
	float m_randomConstant;
	mutable float m_time;

	float m_input1;
	float m_input2;
	float m_input3;
	float m_input4;
};

TYPEDEF_BLUECLASS( Tr2CurveEulerRotationExpression );