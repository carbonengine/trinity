////////////////////////////////////////////////////////////
//
//    Created:   June 2010
//    Copyright: CCP 2010
//

#pragma once
#ifndef Tr2ShaderMaterial_H
#define Tr2ShaderMaterial_H

#include "include/ITriEffectParameter.h"
#include "ITr2ShaderMaterial.h"
#include "Resources/Tr2EffectRes.h"

BLUE_DECLARE( Tr2HighLevelShader );
BLUE_DECLARE( Tr2LowLevelShader );
BLUE_DECLARE( Tr2VariableStore );

class Tr2VertexDefinition;

class Tr2ShaderSituation
{
public:
	// Constructors
	Tr2ShaderSituation() : m_situations() {}
	explicit Tr2ShaderSituation( const std::vector<unsigned int>& flags ) : 
		m_situations( flags ) {}

	// Invalid situation code
	static const unsigned int INVALID_SITUATION = 0xFFFFFFFF;

	// Clear the situation
	void Reset( void );

	// Add a situation from a string
	void AddSituationString( const char* str );
	// Add a situation from a hash code (generated from a string)
	void AddSituationCode( unsigned int prehashedString );

	// Append a vector of situation flags to this situation
	void AppendFlags( const std::vector<unsigned int>& flags );

	// Add situations from a vertex declaration
	void AddVertexFormat( const Tr2VertexDefinition& definition );
	// Add situation for a non-null or non-zero shader parameter
	void AddParameter( const ITriEffectParameter* param );

	// Get the situation vector
	const std::vector<unsigned int>& GetSituation( void ) const { return m_situations; }

private:
	// The vector of situation codes
	std::vector<unsigned int> m_situations;
};

// --------------------------------------------------------------------------------------
// Description:
//   Tr2ShaderMaterial holds the material data, a loose pointer to Tr2HighLevelShader.
//   It contains a list of Tr2ShaderParameters. It also implements the ITr2ShaderMaterial
//   interface.  Tr2ShaderMaterial adds in non-zero parameters to the situation on the 
//   way down to binding a low level shader.
// SeeAlso:
//   Tr2HighLevelShader, Tr2LowLevelShader, ITr2ShaderMaterial, ITr2ShaderState
// --------------------------------------------------------------------------------------
BLUE_CLASS( Tr2ShaderMaterial ) :
	public IInitialize,
	public INotify,
	public ITr2ShaderMaterial,
	public Tr2DeviceResource
{
public:
	Tr2ShaderMaterial(IRoot* lockobj = NULL);	
	virtual ~Tr2ShaderMaterial();

	EXPOSE_TO_BLUE();

	/////////////////////////////////////////////////////////////////////////////////////
	// INotify
	virtual bool OnModified( Be::Var* val );

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	virtual bool Initialize();

	/////////////////////////////////////////////////////////////////////////////////////
	// ITr2ShaderMaterial
	uint32_t ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext );
	void ApplyShaderInputs( unsigned int passIndex, Tr2RenderContextEnum::ShaderType shaderType, Tr2RenderContext& renderContext );
	unsigned int GetSortValue( void ) const;
	ITr2ShaderState* GetShaderStateInterface( void ) const;
	ITriEffectParameter* GetParameterByName( const char* name ) const { return FindParameterByName( name ); }
	const Tr2ConstantEffectParameter* GetConstParameters( size_t& count ) const;
	void UnloadResources();
	void LoadResources();

	// Set the high-level shader name
	void SetHighLevelShaderName( const std::string& shaderName );
	virtual std::string& GetHighLevelShaderName() { return m_highLevelShaderName; }

	// Applies material parameters to the current situation
	virtual void ApplyMaterialToSituation( Tr2ShaderSituation& situation, bool overrideDefaultSituation );

	// Bind the low-level shader with the given situation
	void BindLowLevelShader( Tr2ShaderSituation situation );
	void BindLowLevelShaderMaterialOnly(std::vector<unsigned int> hashedSituations);

	//  Get the permuteIndex from the low level shader, if any
	unsigned GetLowLevelPermuteIndex() const;

	// Rebinds low-level shader (typically after a change to the global situation)
	void RebindLowLevelShader( void );
	
	void SetHighLevelShaderName( const char* name );

	void RebuildCachedDataInternal( void );
	void SetVariableStore( Tr2VariableStore* variableStore );

	// An accessor for getting the default situation string
	const std::string& GetDefaultSituationString() const { return m_defaultSituation; };
	void SetDefaultSituationString( const char* situation );

	bool AreParametersPrepared() const;

	// Populates parameter list with defaults from the parameter descriptions
	void PopulateDefaultParameters( void );
	// Resets the current parameter values with defaults from the parameter descriptions
	void ResetDefaultParameters( void );
protected:
	// Tokenizes the default situation string & computes the hash
	void TokenizeAndHashDefaultSituation( void );

	// Builds the list of parameters for the effect
	bool PopulateParameters( void );
	// Removes any unused parameters from the parameter list
	bool PruneParameters( void );
	// Builds the parameter mirror

	// Maps the samplers to registers
	virtual void MapPassResources( 
		const Tr2EffectResourceMap& resources, 
		Tr2EffectParamVector &pv, 
		uint32_t resourceFlags );

	// Utility function to find a parameter by name
	virtual ITriEffectParameter* FindParameterByName( const char* name ) const;

	Tr2VariableStore& GetVariableStore();

	virtual void ReleaseResources( TriStorage s );
	virtual bool OnPrepareResources();

protected:
	void PyBindLowLevelShader( const std::vector<std::string>& situations );

	PITriEffectParameterDict m_parameters;
	Tr2EffectPassParametersVector m_parametersForPasses;

	std::string m_highLevelShaderName; // persisted

	std::string m_materialName;

	//	The last bound low-level shader.  Since this is a function of this material AND 
	//  the applied Situation, it should ultimately live somewhere else
	Tr2LowLevelShaderPtr m_lowLevelShader;	

	//	The high level shader that builds the specific low-level shaders
	Tr2HighLevelShaderPtr m_highLevelShader;

	// Default situation string for this material
	std::string m_defaultSituation;
	std::vector<unsigned int> m_defaultSituationHashCodes;

	//	The unique hash of this shader's parameter values
	unsigned int m_parameterHash;

	// Current situation
	Tr2ShaderSituation m_currentSituation;

	// Variable store to use for binding variables
	Tr2VariableStorePtr m_variableStore;

};


TYPEDEF_BLUECLASS( Tr2ShaderMaterial );

#endif

