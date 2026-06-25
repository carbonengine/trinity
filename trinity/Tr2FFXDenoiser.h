// Copyright © 2023 CCP ehf.

#pragma once


BLUE_CLASS( Tr2FidelityFXDenoiser ) :
{
public:
	EXPOSE_TO_BLUE();

	Tr2FidelityFXDenoiser();
	~Tr2FidelityFXDenoiser();

private:
	bool m_enable;
};

TYPEDEF_BLUECLASS( Tr2FidelityFXDenoiser );