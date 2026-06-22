// Copyright © 2020 CCP ehf.

#pragma once
#ifndef Tr2GStateParameter_h
#define Tr2GStateParameter_h

#if GSTATE_ENABLED

// --------------------------------------------------------------------------------------
// Description:
//   A Blue-exposed wrapper for GState parameters. This class in meant to be used by
//   Tr2GStateAnimation in the form of a list with one instance of this Tr2GStateParameter
//   per actual parameter in the GState file.
//   The need for this is to be able to create bindings between GState Parameters and
//   curvesets. So the animation of such parameters can be driven by a curveset.
// See Also:
//   Tr2GStateAnimation
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2GStateParameter ) :
	public IInitialize
{
public:
	Tr2GStateParameter( IRoot* lockobj = NULL );
	~Tr2GStateParameter();

	EXPOSE_TO_BLUE();

	bool Initialize();
	void SetValue( float value );
	void SetName( std::string name );
	void SetNodeName( std::string name );
	const float GetValue() const;
	const std::string& GetName() const;
	const std::string& GetNodeName() const;

private:
	bool IsInitialized() const;
	// float value of the parameter
	float m_value;
	// name of the parameter
	std::string m_name;
	// name of the parameter node (parent)
	std::string m_nodeName;
};

TYPEDEF_BLUECLASS( Tr2GStateParameter );

#endif

#endif //Tr2GStateParameter_h