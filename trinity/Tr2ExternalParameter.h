////////////////////////////////////////////////////////////
//
//    Created:   March 2016
//    Copyright: CCP 2016
//

#pragma once
#ifndef Tr2ExternalParameter_H
#define Tr2ExternalParameter_H


BLUE_DECLARE( TriValueBinding );


// --------------------------------------------------------------------------------------
// Description:
//   Tr2ExternalParameter is a special binding that doesn't have a source, but only a
//   destination. The SetValue method can be used to change destination value from 
//   script. Tr2ExternalParameter can be used to expose internal values in an object tree 
//   to the script in a nice way.
// See Also:
//   EveSpaceScene, EveSpaceObject2
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ExternalParameter ): 
	public IInitialize,
	public INotify
{
public:
	Tr2ExternalParameter( IRoot* lockobj = nullptr );
	~Tr2ExternalParameter();

	EXPOSE_TO_BLUE();

	virtual bool Initialize();
	virtual bool OnModified( Be::Var* value );
	
	const char* GetName() const;
	void SetName( const std::string& name );
	void SetDestinationObject( IRoot* destination );
	void SetDestinationAttribute( const std::string& attributeName );

	BlueStdResult SetValue( BlueScriptValue value );
	BlueStdResult GetValue( BlueScriptValue& value ) const;
	Be::Var* GetDestination() const;
	const Be::VarEntry* GetDestinationEntry() const;

	bool IsValid() const;

	// Creates a binding from this Tr2ExternalParameter. The new binding has the same destination object/attribute.
	TriValueBindingPtr CreateBinding() const;
private:
	// parameter name
	std::string m_name;
	// destination object
	IRootPtr m_destinationObject;
	// destination attribute name
	std::string m_destinationAttribute;
	// pointer do destination value
	Be::Var* m_destination;
	// blue class description entry for destination attribute
	const Be::VarEntry* m_destinationEntry;
	// offset into the destination array for attribute names containing .xyzw
	uint32_t m_destItemOffset;
	// INotify interface for destination object
	INotifyPtr m_notifyPtr;
	// is parameter valid
	bool m_valid;
	// if m_destItemOffset used
	bool m_useOffset;
};

TYPEDEF_BLUECLASS( Tr2ExternalParameter );
BLUE_DECLARE_VECTOR( Tr2ExternalParameter );

#endif