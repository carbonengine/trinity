#pragma once
#ifndef Tr2PyValueBinding_h
#define Tr2PyValueBinding_h


#include "include/ITr2ValueBinding.h"

BLUE_DECLARE( Tr2PyValueBinding );

class Tr2PyValueBinding:
	 public INotify,
	 public ITr2ValueBinding
{
public:
    EXPOSE_TO_BLUE();

    Tr2PyValueBinding( IRoot* lockobj = NULL );
	~Tr2PyValueBinding();

	//////////////////////////////////////////////////////////////////////////
	// INotify
	bool OnModified( Be::Var* val );

	//////////////////////////////////////////////////////////////////////////
	// ITr2ValueBinding
	void CopyValue();

private:
	std::string m_name;
	bool m_isValid;
	PyObject* m_sourceObject;
	PyObject* m_destinationObject;

	std::string m_sourceAttribute;
	std::string m_destinationAttribute;

private:
	void Initialize();
};

TYPEDEF_BLUECLASS( Tr2PyValueBinding );
#endif //Tr2PyValueBinding_h
