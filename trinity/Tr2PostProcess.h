// Copyright © 2023 CCP ehf.

#pragma once
#ifndef Tr2PostProcess_H
#define Tr2PostProcess_H


BLUE_DECLARE( Tr2PostProcess );
BLUE_DECLARE( Tr2Effect );
BLUE_DECLARE_VECTOR( Tr2Effect );
class TriVariable;

class Tr2PostProcess : public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	Tr2PostProcess( IRoot* lockobj = NULL );
	~Tr2PostProcess();

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

private:
	std::string m_name;

	// list off effects which define the post-processing
	PTr2EffectVector m_stages;
};
TYPEDEF_BLUECLASS( Tr2PostProcess );

#endif // Tr2PostProcess_H