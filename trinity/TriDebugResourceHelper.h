#pragma once

#ifndef TRIDEBUGRESOURCEHELPER_H
#define TRIDEBUGRESOURCEHELPER_H

#include "Tr2DeviceResource.h"

BLUE_DECLARE( Tr2Effect );

class TriDebugResourceHelper :
	public Tr2DeviceResource
{
public:
	TriDebugResourceHelper();
	~TriDebugResourceHelper();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// ITriDeviceResource
	//////////////////////////////////////////////////////////////////////////////////////////////
	void ReleaseResources( TriStorage s );
private:
	bool OnPrepareResources();
public:
#ifdef TRINITYDEV
	void GetDescription( std::string& desc ) { desc = "TriDebugResourceHelper"; }
#endif

	Tr2Effect* GetEffect();
	Tr2Effect* GetLineEffect();
	unsigned int GetVertexPosDecl();
	unsigned int GetVertexPosColorDecl();

	struct VertexPos
	{
		Vector3 m_pos;
	};

	struct VertexPosColor
	{
		Vector3 m_pos;
		uint32_t m_color;
	};

private:
	unsigned int m_vertexPosColorDecl;
	unsigned int m_vertexPosDecl;
	Tr2EffectPtr m_effect;
	Tr2EffectPtr m_lineEffect;
};

extern TriDebugResourceHelper g_debugResourceHelper;
#endif