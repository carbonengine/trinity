// Copyright © 2023 CCP ehf.

#pragma once

#include "../include/Tr2ShaderProgramAL.h"
#include "../include/Tr2ShaderAL.h"
#include "../include/Tr2ResourceSetAL.h"

#if ( TRINITY_PLATFORM == TRINITY_DIRECTX11 )

namespace TrinityALImpl
{
class Tr2ShaderProgramAL : public Tr2DeviceResourceAL<Tr2ShaderProgramAL>
{
public:
	Tr2ShaderProgramAL();

	ALResult Create( ::Tr2ShaderAL* shaders, size_t count, Tr2PrimaryRenderContextAL& renderContext );
	void Destroy();

	bool IsValid() const;

	Tr2ALMemoryType GetMemoryClass() const;

	void Describe( Tr2DeviceResourceDescriptionAL& description ) const;

	const Tr2RegisterMapAL& GetRegisterMap() const;
	ALResult SetName( const char* name );

private:
	struct Shaders
	{
		CComPtr<ID3D11VertexShader> vertexShader;
		CComPtr<ID3D11PixelShader> pixelShader;
		CComPtr<ID3D11ComputeShader> computeShader;
		CComPtr<ID3D11GeometryShader> geometryShader;
		CComPtr<ID3D11HullShader> hullShader;
		CComPtr<ID3D11DomainShader> domainShader;
	};

	::Tr2ShaderAL m_vertexShader;
	Shaders m_shaders;
	Tr2RegisterMapAL m_registerMap;
	std::string m_name;

	bool m_isValid;

	friend class ::Tr2RenderContextAL;
};
}

#endif