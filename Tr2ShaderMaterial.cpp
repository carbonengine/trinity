////////////////////////////////////////////////////////////
//
// Created:  June 2010
// Copyright: CCP 2010-2011
//

#include "StdAfx.h"

#include "Tr2ShaderMaterial.h"

#include "Tr2Renderer.h"
#include "Tr2ShaderManager.h"
#include "Tr2ShaderParameterDescription.h"
#include "Tr2Effect.h"
#include "EffectParameter/TriTexture2DParameter.h"
#include "Tr2HighLevelShader.h"


#define INVALID_PARAMETER_HASH (~0)

using namespace Tr2RenderContextEnum;

// --------------------------------------------------------------------------------------
// Description:
//   Constructor.  Initializes members to defaults and registers the shader material with
//   the Tr2Renderer.
// --------------------------------------------------------------------------------------
Tr2ShaderMaterial::Tr2ShaderMaterial( IRoot* lockobj ) :
	PARENTLOCK( m_parameters ),
	m_parameterHash( INVALID_PARAMETER_HASH )
{
	// Register the material with the Tr2Renderer so it can receive global situation
	// updates
	Tr2Renderer::RegisterMaterial( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Destructor.  Unregisters the material with the Tr2Renderer.
// --------------------------------------------------------------------------------------
Tr2ShaderMaterial::~Tr2ShaderMaterial()
{
	// Unregister the material with the Tr2Renderer
	if( m_highLevelShader )
	{
		m_highLevelShader->UnregisterShader( this );
	}

	Tr2Renderer::UnregisterMaterial( this );
}

// --------------------------------------------------------------------------------------
// Description:
//   Responds to changes to member variables out in Python.
// Arguments:
//   value - The Blue-exposed value that changed.
// Return Value:
//   true, always
// See Also: SetHighLevelShaderName
// --------------------------------------------------------------------------------------
bool Tr2ShaderMaterial::OnModified( Be::Var* value )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( IsMatch( value, m_highLevelShaderName ) ) // this is a pointer test.
	{
		// call the common setup code for switching the HLS name
		SetHighLevelShaderName( m_highLevelShaderName.c_str() );
	}
	else if( IsMatch( value, m_defaultSituation ) )
	{
		// Reprocess the default situation
		TokenizeAndHashDefaultSituation();
	}

	return true;
}



// --------------------------------------------------------------------------------------
// Description:
//   Sets the high level shader name, and makes sure all registration /un registratioon takes place.
// Arguments:
//   name - the new name of the HLS to bind, can also be null.
// 
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::SetHighLevelShaderName( const char* name )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2ShaderManager* manager = GetShaderManager();
	Tr2HighLevelShader* newShader = NULL;
	
	if( !manager )
	{
		return;
	}
	// This is redundant for OnModified calls
	m_highLevelShaderName = name;

	if( !m_highLevelShaderName.empty() )
	{
		// try to go get the new HLS from the library.
		newShader = manager->FindShaderByName( m_highLevelShaderName.c_str() );
	}

	// If the new shader is not the same as the existing one
	if( newShader != m_highLevelShader )
	{
		// does an HLS already exist? if so, unregister ourselves with it.
		if( m_highLevelShader )
		{
			m_highLevelShader->UnregisterShader( this );
		}

		// set this to null, in case name binding fails in the next step.
		m_highLevelShader = newShader;

		// if we got a new valid shader back, register myself with it.
		if( m_highLevelShader )
		{
			m_highLevelShader->RegisterShader( this );
		}
	}
}

void Tr2ShaderMaterial::ReleaseResources( TriStorage s )
{
	if( ( s & TRISTORAGE_ALL ) == TRISTORAGE_ALL )
	{
		for( auto it = m_parametersForPasses.begin(); it != m_parametersForPasses.end(); ++it )
		{
			for( unsigned i = 0; i != SHADER_TYPE_COUNT; ++i )
			{
				if( ( *it )->m_stageInput[i].m_constantBuffer )
				{
					( *it )->m_stageInput[i].m_constantBuffer->Destroy();
				}
			}
		}
	}
}

bool Tr2ShaderMaterial::OnPrepareResources()
{
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Does some initialization after loading object from a .red file.
// Return Value:
//   true, always
// --------------------------------------------------------------------------------------
bool Tr2ShaderMaterial::Initialize()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	SetHighLevelShaderName(  m_highLevelShaderName.c_str()  );

	// Process the default situation
	TokenizeAndHashDefaultSituation();

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies vertex and pixel shader inputs for this given pass.
// Arguments:
//   passIndex - The index of the pass for which to apply vertex and pixel shader inputs
//   context - Current render context
// --------------------------------------------------------------------------------------
uint32_t Tr2ShaderMaterial::ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext )
{
	return ::ApplyMaterialDataForPass( m_parametersForPasses, m_lowLevelShader, passIndex, renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies shader inputs for the given pass.  
// Arguments:
//   passIndex - The index of the pass for which to apply vertex shader inputs.
//   shaderType - Stage input type (vertex shader, pixel shader, etc.)
//   context - Current render context
// Return value:
//   A mask containing bits for each stage that has modified sampler state
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::ApplyShaderInputs( const unsigned passIndex, 
										   const Tr2RenderContextEnum::ShaderType shaderType, 
										   Tr2RenderContext& renderContext )
{
	bool samplersChanged;
	::ApplyShaderInputs( *m_parametersForPasses[ passIndex ], shaderType, samplersChanged, renderContext );
}

// --------------------------------------------------------------------------------------
// Description:
//   Gets the sort value for the currently bound low-level shader (or 0, if no shader is
//   bound).
// Return Value:
//   The sort value for the currently bound low-level shader, or 0 if no shader is bound
// --------------------------------------------------------------------------------------
unsigned Tr2ShaderMaterial::GetSortValue() const
{
	return m_lowLevelShader ? m_lowLevelShader->GetSortValue() : 0;	
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the currently bound low-level shader (or NULL).
// Return Value:
//   The currently bound low-level shader (or NULL)
// --------------------------------------------------------------------------------------
ITr2ShaderState* Tr2ShaderMaterial::GetShaderStateInterface() const
{
	return m_lowLevelShader;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2ShaderMaterial interface. Returns empty constant parameter set.
// Arguments:
//   count - (out) Number of constant parameters
// Return Value:
//   NULL always
// --------------------------------------------------------------------------------------
const Tr2ConstantEffectParameter* Tr2ShaderMaterial::GetConstParameters( size_t& count ) const
{
	count = 0;
	return nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Sets the high-level shader name and attempts to acquire a pointer to the high-level
//   shader from the shader manager.
// Arguments:
//   shaderName - The name of the high-level shader
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::SetHighLevelShaderName( const std::string& shaderName )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_highLevelShaderName = shaderName;
	// Get the high-level shader from the library
	Tr2ShaderManager* manager = GetShaderManager();
	m_highLevelShader = manager->FindShaderByName( m_highLevelShaderName.c_str() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies the material parameters to the current situation.  This turns on any permute
//   bits for non-zero, non-NULL parameters, and turns off any permute bits for zero 
//   or NULL parameters.
// Arguments:
//   situation - The current situation (used as output)
//   overrideDefaultSituation - Boolean flag allowing an optional override of the default
//                              situation
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::ApplyMaterialToSituation( 
	Tr2ShaderSituation& situation, 
	bool overrideDefaultSituation )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( PITriEffectParameterDict::const_iterator it = m_parameters.begin();
		 it != m_parameters.end(); ++it )
	{
		situation.AddParameter( it->second );
	}

	// Add default situation (if we aren't overriding it)
	if( !overrideDefaultSituation )
	{
		for( std::vector<unsigned int>::iterator it = m_defaultSituationHashCodes.begin();
			it != m_defaultSituationHashCodes.end(); ++it )
		{
			situation.AddSituationCode( *it );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Binds a low-level shader using the specified situation.  A cached copy of the 
//   situation is made before applying the global situation.  Note that the situation is
//   intentionally passed by value to this function, so that the global situation is not
//   repeatedly applied to the current situation.  Parameters are re-bound after calling
//   this function.
// Arguments:
//   situation - The current situation (passed by value, so global situation isn't
//               repeatedly applied)
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::BindLowLevelShader( Tr2ShaderSituation situation )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// high level shader wasn't bound, so I can't do anything. report error and exit.
	if( !m_highLevelShader )
	{
		CCP_LOGERR( "High level shader %s failed to bind, so low level binding will fail", 
			m_highLevelShaderName.c_str() );
		return;
	}

	// Cache the current situation, before we get the global situation from Tr2Renderer
	m_currentSituation = situation;

	// Append the global situation flags to the current situation
	situation.AppendFlags( Tr2Renderer::GetGlobalSituation() );

	// Get the low level shader
	m_lowLevelShader = 	m_highLevelShader->GetOrCreateLowLevelShader( situation );

	// Rebuild the parameter mirror
	RebuildCachedDataInternal();
}


// for post process or other 'meshless' shaders.
void Tr2ShaderMaterial::BindLowLevelShaderMaterialOnly( std::vector<unsigned int> hashedSituations )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2ShaderSituation sitch;
	sitch.Reset();
	
	std::vector<unsigned int>::iterator walker( hashedSituations.begin() ), endOfList( hashedSituations.end() );

	while( walker != endOfList )
	{
		unsigned int code = *walker;
		sitch.AddSituationCode( code );
		++walker;
	}
	
	// apply material now.
	ApplyMaterialToSituation( sitch , false );

	// finish the process by calling the main function above
	BindLowLevelShader( sitch );

}

// --------------------------------------------------------------------------------------
// Description:
//   Returns the permuteIndex of the low level shader, if any, else zero.
// --------------------------------------------------------------------------------------
unsigned Tr2ShaderMaterial::GetLowLevelPermuteIndex() const
{
	return m_lowLevelShader ? m_lowLevelShader->GetPermuteIndex() : 0;
}

// --------------------------------------------------------------------------------------
// Description:
//   Rebinds the current low level shader, using the cached situation plus the global
//   situation, acquired from Tr2Renderer.
// See Also:
//   BindLowLevelShaders
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::RebindLowLevelShader()
{
	BindLowLevelShader( m_currentSituation );
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns a variable store to use when binding shader variables.
// Arguments:
//   variableStore - Variable store to use for binding shader variables.
// See Also:
//   BindLowLevelShaders
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::SetVariableStore( Tr2VariableStore* variableStore )
{
	m_variableStore = variableStore;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns a new default situation string to this material.
// Arguments:
//   situation - String containing situation names (space delimited).
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::SetDefaultSituationString( const char* situation )
{
	m_defaultSituation = situation;
	TokenizeAndHashDefaultSituation();
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks if all material parameters are ready to be used.
// Return value:
//   true If all material parameters are ready to be used
//   false Otherwise
// --------------------------------------------------------------------------------------
bool Tr2ShaderMaterial::AreParametersPrepared() const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( PITriEffectParameterDict::const_iterator it = m_parameters.begin();
		it != m_parameters.end();
		++it )
	{
		if( !it->second->IsPrepared() )
		{
			return false;
		}
	}
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   This function splits a space-delimited string of situation strings into individual
//   tokens, then hashes and stores the extracted situation tokens.
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::TokenizeAndHashDefaultSituation()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Space delimiter string
	const std::string delimiter = " ";
	const std::string& defaultSituationString = GetDefaultSituationString();

	// Clear the default situation hash code vector
	m_defaultSituationHashCodes.clear();

	// First non-delimiter character
	std::string::size_type lastPos = defaultSituationString.find_first_not_of( delimiter, 0 );
	// First delimiter character
	std::string::size_type pos = defaultSituationString.find_first_of( delimiter, lastPos );

	// Iterate until we hit the end of the string
	while( std::string::npos != pos || std::string::npos != lastPos )
	{
		// Get the situation token
		std::string situationToken = defaultSituationString.substr( lastPos, pos - lastPos );

		// Compute and store the hash code for this situation token
		unsigned int hashCode = CcpHashFNV1( situationToken.c_str(), 
			situationToken.length() );
		m_defaultSituationHashCodes.push_back( hashCode );

		// Get the next delimiter and non-delimiter positions
		lastPos = defaultSituationString.find_first_not_of( delimiter, pos );
		pos = defaultSituationString.find_first_of( delimiter, lastPos );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   This function populates the parameters list based on 
// --------------------------------------------------------------------------------------
bool Tr2ShaderMaterial::PopulateParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if (!m_lowLevelShader)
	{
		CCP_LOGERR( "No effect resource loaded." );
		return false;
	}

	auto paramAdder = [&]( ITriEffectParameter* p )
							{
								m_parameters.AssignSubscript( p->GetParameterName(), p );
							};

	for( unsigned passIx = 0; passIx < m_lowLevelShader->GetPassCount(); ++passIx )
	{
		const Tr2Pass& pass = m_lowLevelShader->GetPass( passIx );
		for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			const auto& input = pass.stageInputs[i];
			for( auto constant = input.constants.cbegin(); constant != input.constants.end();  ++constant )
			{
				if( !GetBool( m_lowLevelShader, constant->name.c_str(), "SasUiVisible" ) )
				{
					continue;
				}

				if( m_parameters.find( constant->name.c_str() ) != m_parameters.end() )
				{
					continue;
				}

				ConvertEffectConstant(	*constant, input.constantValues, paramAdder );				
			}

			for( auto sampler = input.resources.cbegin(); sampler != input.resources.cend(); ++sampler )
			{
				if( !GetBool( m_lowLevelShader, sampler->second.name, "SasUiVisible" ) )
				{
					continue;
				}

				if( m_parameters.find( sampler->second.name ) != m_parameters.end() )
				{
					continue;
				}

				ConvertEffectResource( sampler->second, paramAdder, paramAdder );				
			}
		}
	}

	// Rebuild the parameter mirror
	RebuildCachedDataInternal();
	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes any parameters that aren't actually used by the bound low-level shader.
// Return Value:
//   true, If the low-level shader is bound & the parameters can be evaluated.
//   false, If the low-level shader isn't bound or the parameters can't be evaluated for
//      some other reason.
// --------------------------------------------------------------------------------------
bool Tr2ShaderMaterial::PruneParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_lowLevelShader )
	{
		CCP_LOGERR( "No effect resource loaded." );
		return false;
	}

	std::vector<std::string> keysToRemove;
	for( auto it = m_parameters.begin(); it != m_parameters.end(); ++it )
	{
		bool removeParameter = !GetBool( m_lowLevelShader, it->first.c_str(), "SasUiVisible" );
		const Tr2EffectConstant *constant = m_lowLevelShader->GetConstant( it->first.c_str() );
		if( constant == nullptr && !removeParameter )
		{
			removeParameter = true;
			for( unsigned passIx = 0; passIx < m_lowLevelShader->GetPassCount() && removeParameter; ++passIx )
			{
				const Tr2Pass& pass = m_lowLevelShader->GetPass( passIx );
				for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT && removeParameter; ++i )
				{
					for( auto sampler = pass.stageInputs[i].resources.begin(); sampler != pass.stageInputs[i].resources.end(); ++sampler )
					{
						if( strcmp( sampler->second.name, it->first.c_str() ) == 0 )
						{
							removeParameter = false;
							break;
						}
					}
				}
			}
		}
		if (removeParameter)
		{
			keysToRemove.push_back( it->first );
		}
	}

	// Now remove the keys
	for( std::vector<std::string>::iterator it = keysToRemove.begin();
		 it != keysToRemove.end(); ++it )
	{
		m_parameters.AssignSubscript( ( *it ).c_str(), NULL );
	}

	return true;
}

// --------------------------------------------------------------------------------------
// Description:
//   Populate the parameter list with defaults from the high-level shader parameter
//   descriptions.  Existing parameters are not modified - it's assumed that any change
//   to the parameter list prior to calling this function is intentional & should take
//   precedence.
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::PopulateDefaultParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_highLevelShader )
	{
		const Tr2ShaderParameterDescriptionVector& descriptions = 
			m_highLevelShader->GetParameterDescriptions();

		// Iterate over the parameter descriptions from the HLS
		for( auto it = descriptions.cbegin(); it != descriptions.cend(); ++it )
		{
			const std::string& pName = ( *it )->GetShaderParameterName();
			if( m_parameters.find( pName.c_str() ) == m_parameters.end() )
			{
				ITriEffectParameter* param = ( *it )->CreateDefaultParameter();
				m_parameters.AssignSubscript( pName.c_str(), param );
				param->Unlock();
			}
		}	
	}
	else
	{
		CCP_LOGERR( "PopulateDefaultParameters failed: material does not have a high-level shader!" );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Clears the parameter list and then re-populates the parameter dict with defaults 
//   from the high-level shader parameter descriptions.
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::ResetDefaultParameters()
{
	if( m_highLevelShader )
	{
		// Clear the old parameter dictionary
		m_parameters.clear();

		// Populate defaults
		PopulateDefaultParameters();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Something
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::RebuildCachedDataInternal()
{
	//	Clear the parameter hash so it will be rebuilt when next we need it
	m_parameterHash = INVALID_PARAMETER_HASH;

	if( m_lowLevelShader )
	{
		RebuildCachedDataForEffect( *m_lowLevelShader, *this, m_parametersForPasses );
	}

	// It's ok to pass in NULL values to these functions so that the parameters
	// realize that they're not in use by a NULL technique etc
	for( auto it = m_parameters.begin(); it != m_parameters.end(); ++it )
	{
		it->second->RebuildEffectHandles( m_lowLevelShader );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Maps the samplers to registers
// Arguments:
//   samplers - Map of sampler state setups
//   pv - Vector of sampler parameters
// --------------------------------------------------------------------------------------
void Tr2ShaderMaterial::MapPassResources( const Tr2EffectResourceMap& resources, Tr2EffectParamVector &pv, uint32_t resourceFlags )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = resources.begin(); it != resources.end(); ++it )
	{
		const Tr2EffectResource& ss = it->second;
		const char* name = ss.name;

		Tr2EffectParam param;
		param.m_sourceValue.Unlock();

		// First search in effect parameter list
		ITriEffectParameter* p = FindParameterByName( name );

		//	See if we have a valid resource
		if( p && !p->IsZeroOrNull() )
		{
			if( p->IsTextureParameter() )
			{
				param.m_sourceValue = p;
			}
			else
			{
				CCP_LOGWARN( "Parameter %s was requested as a sampler, but isn't a texture", name );
			}
		}
		// Fallback to variable store if we didn't find a result above
		else
		{
			if( TriVariable* v = GetVariableStore().FindVariable( name ) )
			{
				switch( v->GetType() )
				{
				// All of the following cases are valid for mapping texture samplers
				case TRIVARIABLE_TEXTURE_RES:
				case TRIVARIABLE_UNKNOWN_TEXTURE:
				case TRIVARIABLE_TEXTURE_AL:
					param.m_sourceValue = v;
					break;

				default:
					break;
				}
			}
			else
			{
				if( it->second.isAutoregister )
				{
					// We don't choose the type here, just reserve a variable
					param.m_sourceValue = GlobalStore().RegisterPlaceholderTextureVariable( name );
				}
			}
		}

		if( param.m_sourceValue )
		{
			param.m_sourceName = ss.name;
			param.m_registerIndex = it->first;
			param.m_registerCount = resourceFlags | ( it->second.isSRGB ? ITr2EffectValue::RESOURCE_FLAG_SRGB : 0 );

			pv.push_back( param );
		}
	}
}

void Tr2ShaderMaterial::UnloadResources()
{

}

void Tr2ShaderMaterial::LoadResources()
{

}

// --------------------------------------------------------------------------------------
// Description:
//   Utility function to fund a parameter by name
// Arguments:
//   name - Name of the parameter to find
// Return Value:
//   The parameter or NULL if not found
// --------------------------------------------------------------------------------------
ITriEffectParameter* Tr2ShaderMaterial::FindParameterByName(  const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	auto it = m_parameters.find( name );
	return it != m_parameters.end() ? it->second : nullptr;
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a variable store used by material to bind shader variables.
// Return value:
//   Variable store used by this material.
// --------------------------------------------------------------------------------------
Tr2VariableStore& Tr2ShaderMaterial::GetVariableStore()
{
	if( m_variableStore )
	{
		return *m_variableStore;
	}
	return GlobalStore();
}

void Tr2ShaderMaterial::PyBindLowLevelShader( const std::vector<std::string>& situations )
{
	std::vector<unsigned int> situationList;
	for( auto it = situations.begin(); it != situations.end(); ++it )
	{
		unsigned int h = CcpHashFNV1( it->c_str(), it->length() );
		situationList.push_back(h);
	}
	BindLowLevelShaderMaterialOnly( situationList );
}


/////////////////////////////////////////////
// Situation Member functions
/////////////////////////////////////////
void Tr2ShaderSituation::AddParameter( const ITriEffectParameter* param )
{
	// Add situation code if the parameter is not zero or null
	if( !param->IsZeroOrNull() )
	{
		const char* paramName = param->GetParameterName();

		if( paramName )
		{
			AddSituationString( paramName );
		}
		else
		{
			CCP_ASSERT( !"Tr2ShaderSituation::AddParameter() - Param doesn't have a name!" );
		}
	}
	//	TODO: See if there is a valid binding in the variable store?
}

const char* s_usageStrings[ Tr2VertexDefinition::NUM_USAGE_CODE ] =
{
	"POSITION",
	"COLOR",
	"NORMAL",
	"TANGENT",
	"BITANGENT",
	"TEXCOORD",
	"BLENDINDICES",
	"BLENDWEIGHTS"
};

void Tr2ShaderSituation::AddVertexFormat( const Tr2VertexDefinition& definition )
{
	char tempBuf[80];

	m_situations.reserve( m_situations.size() + definition.m_items.size() );
	for( auto it = begin( definition.m_items ); it != end( definition.m_items ); ++it )
	{
		sprintf_s( tempBuf, "%s%d", s_usageStrings[ it->m_usage ], it->m_usageIndex );
		unsigned int hash = CcpHashFNV1( tempBuf, strlen( tempBuf ) );
		m_situations.push_back( hash );
	}
}

void Tr2ShaderSituation::AddSituationCode( unsigned int prehashedString )
{
	m_situations.push_back( prehashedString );
}

// --------------------------------------------------------------------------------------
// Description:
//   Appends a vector of situations flags to this situation.
// Arguments:
//   flags - The vector of situation flags to append
// --------------------------------------------------------------------------------------
void Tr2ShaderSituation::AppendFlags( const std::vector<unsigned int>& flags )
{
	m_situations.insert( m_situations.end(), flags.begin(), flags.end() );
}

void Tr2ShaderSituation::AddSituationString( const char* str )
{
	// hash string first into a DWORD, and call Add code
	unsigned int hashedString = CcpHashFNV1( str, strlen( str ) );

	AddSituationCode( hashedString );
}

void Tr2ShaderSituation::Reset()
{
	m_situations.clear();
}
