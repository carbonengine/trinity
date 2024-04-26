////////////////////////////////////////////////////////////////////////////////
//
// Created:		April 2024
// Copyright:	CCP 2024
//
#pragma once

#include "Tr2UpscalingAL.h"

class Tr2Fsr1UpscalingTechnique : public Tr2UpscalingTechniqueAL
{
public:
	Tr2Fsr1UpscalingTechnique( Tr2UpscalingAL::Technique technique, Tr2UpscalingAL::Setting setting, bool frameGeneration );
	~Tr2Fsr1UpscalingTechnique();
	virtual std::vector<Tr2UpscalingAL::Setting> GetAvailableSettings() const override;

	virtual Tr2UpscalingAL::Result Setup() override;
	virtual void Destroy( Tr2RenderContextAL& renderContext ) override;

private:
	virtual Tr2UpscalingContextAL* CreateContextInstance( uint32_t displayWidth, uint32_t displayHeight, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat ) override;
};

class Tr2Fsr1UpscalingContext : public Tr2UpscalingContextAL
{
public:
	Tr2Fsr1UpscalingContext( uint32_t displayWidth, uint32_t displayHeight, Tr2UpscalingAL::Setting setting, bool frameGeneration, Tr2RenderContextEnum::PixelFormat sourceFormat, Tr2RenderContextEnum::DepthStencilFormat depthFormat );
	~Tr2Fsr1UpscalingContext();

	virtual Tr2UpscalingAL::Result Setup( Tr2RenderContextAL& renderContext ) override;
	void Destroy( Tr2RenderContextAL& renderContex );
	virtual bool IsTemporal() const override;
	virtual void UpdateJitter() override;
	virtual uint32_t GetDispatchRequirements() const override;

	virtual Tr2UpscalingAL::Result Dispatch( Tr2RenderContextAL& renderContext, Tr2UpscalingAL::DispatchParameters& dispatchParameters ) override;

private:
	//Tr2EffectAL* m_fidelityFXFsrEASUShader;
	//Tr2EffectPtr m_fidelityFXFsrRCASShader;

	//AMDUpscaling::FSRConstants m_easuConst;
	//AMDUpscaling::FSRConstants m_rcasConst;
};
