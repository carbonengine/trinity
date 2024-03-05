////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once


BLUE_CLASS( Tr2ControllerFloatVariable ) : public IInitialize, public INotify
{
public:
	enum Type
	{
		FLOAT,
		INTEGER,
		BOOLEAN,
		ENUM,
	};

	Tr2ControllerFloatVariable( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	virtual bool Initialize();
	bool OnModified( Be::Var * value ) override;

	const std::string& GetName() const;
	float GetValue() const;
	void SetValue( float value );
	void SetDestinationBuffer( float* buffer );
	void SetDirtyMask( uint64_t* maskDestination, uint64_t mask );

private:
	std::string m_name;
	std::string m_enumValues;
	float* m_destination;
	uint64_t* m_dirtyMaskDestination;
	uint64_t m_dirtyMask;
	Type m_type;
	float m_value;
	float m_defaultValue;
};

TYPEDEF_BLUECLASS( Tr2ControllerFloatVariable );
BLUE_DECLARE_VECTOR( Tr2ControllerFloatVariable );
