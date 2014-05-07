#pragma once

#ifndef ITr2ShaderMaterial_h
#define ITr2ShaderMaterial_h

#include "Tr2EffectDescription.h"

BLUE_DECLARE_INTERFACE( ITr2ShaderMaterial );
BLUE_DECLARE_INTERFACE( ITr2ShaderState );
BLUE_DECLARE_INTERFACE( ITriEffectParameter );
BLUE_DECLARE_INTERFACE( ITriReroutable );

BLUE_DECLARE( Tr2EffectParam );
typedef std::vector<Tr2EffectParam>		Tr2EffectParamVector;
typedef std::vector<ITriReroutable*>	ITriReroutableVector;

BLUE_DECLARE( Tr2VariableStore );


struct Tr2ConstantEffectParameter
{
	BlueSharedString name;
	Vector4 value;
};

BLUE_DECLARE_STRUCTURE_LIST( Tr2ConstantEffectParameter );


BLUE_INTERFACE( ITr2ShaderMaterial ) : public  IRoot
{
public:
	virtual uint32_t ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext ) = 0;
	virtual void ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext ) = 0;
	virtual unsigned int GetSortValue() const = 0;
	virtual ITr2ShaderState* GetShaderStateInterface() const = 0;

	virtual void SetVariableStore( Tr2VariableStore* variableStore ) = 0;
	virtual Tr2VariableStore& GetVariableStore() = 0;

	virtual void MapPassResources( 
		const Tr2EffectResourceMap& resources, 
		Tr2EffectParamVector &pv,
		uint32_t resourceFlags ) = 0;

	virtual ITriEffectParameter* GetParameterByName( const char* name ) const = 0;
	virtual ITriEffectParameter* FindParameterByName( const char* name ) const = 0;
	virtual const Tr2ConstantEffectParameter* GetConstParameters( size_t& count ) const = 0;

	virtual void UnloadResources() = 0;
	virtual void LoadResources() = 0;};

#endif