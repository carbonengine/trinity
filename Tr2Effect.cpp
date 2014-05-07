#include "StdAfx.h"


#include "Tr2Effect.h"
#include "Tr2Renderer.h"
#include "EffectParameter/Tr2FloatParameter.h"
#include "EffectParameter/Tr2Vector2Parameter.h"
#include "EffectParameter/Tr2Vector3Parameter.h"
#include "EffectParameter/Tr2Vector4Parameter.h"
#include "EffectParameter/Tr2Matrix4Parameter.h"
#include "EffectParameter/TriFloatArrayParameter.h"
#include "EffectParameter/TriTexture2DParameter.h"
#include "EffectParameter/TriTextureCubeParameter.h"
#include "EffectParameter/Tr2GeometryBufferParameter.h"
#include "EffectParameter/TriVariableParameter.h"

BLUE_DEFINE_INTERFACE( ITr2EffectValue );

#define INVALID_PARAMETER_HASH (~0)

static unsigned int s_effectId = 0;

using namespace Tr2RenderContextEnum;

CCP_STATS_DECLARE( effectCBLocks, "Trinity/effectCBLocks", true, CST_COUNTER_LOW, "number of CB locks for effect parameters" );
CCP_STATS_DECLARE( effectCBSkippedLocks, "Trinity/effectCBSkippedLocks", true, CST_COUNTER_LOW, "number of CB locks for effect parameters skipped due to optimization" );
CCP_STATS_DECLARE( effectCBSkippedEmptyLocks, "Trinity/effectCBSkippedEmptyLocks", true, CST_COUNTER_LOW, "number of CB locks for effect parameters skipped due to optimization with empty param list" );

static BlueStructureDefinition Tr2EffectParameterStructureDef[] =
{ 
	{ "name", Be::SHAREDSTRING_1, 0 }, 
	{ "value", Be::FLOAT32_4, 8 }, 
	{0} 
};

// ---------------------------------------------------------------
Tr2Effect::Tr2Effect(IRoot* lockobj) :
	#if TRINITYDEV
		m_insideBegin( false ),
		m_insideBeginPass( false ),
	#endif
	m_insideStartUpdate( false ),
	PARENTLOCK(m_parameters),
	PARENTLOCK(m_resources),
	PARENTLOCK( m_constParameters ),
	m_display( true ),
	m_parameterHash( INVALID_PARAMETER_HASH )
{	
	m_parameters.SetNotify( this );
	m_resources.SetNotify( this );
	m_constParameters.SetStructureDefinition( Tr2EffectParameterStructureDef );

	m_sortValue = s_effectId++;

	Tr2Renderer::RegisterEffect( this );
}

// ---------------------------------------------------------------
Tr2Effect::~Tr2Effect()
{
	Tr2Renderer::UnregisterEffect( this );

	if( m_effectResource )
	{
		ReleaseCachedData( m_effectResource );
		m_effectResource->RemoveNotifyTarget( this );
	}
}

void Tr2Effect::ReleaseResources( TriStorage s )
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

bool Tr2Effect::OnPrepareResources()
{
	return true;
}

static bool ConvertEffectPath( const std::string path, std::string& actualPath )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	actualPath = std::string( path.size() + 8 + 10, 0 );
	const char* str = actualPath.c_str();
	size_t i = 0;
	size_t dot = -1;
	bool foundEffect = false;
	for( auto it = std::begin( path ); it != std::end( path ); ++it )
	{
		char c = actualPath[i] = tolower( *it );
		if( c == '/' )
		{
			if( i > 8 && strcmp( str + i - 7, "/effect/" ) == 0 )
			{
				--i;
				static const char* s_platformName = "."  TRINITY_PLATFORM_NAME "/";
				for( const char* src = s_platformName; *src; ++src )
				{
					actualPath[++i] = *src;
				}
				foundEffect = true;
			}
			dot = -1;
		}
		else if( c == '.' )
		{
			dot = i;
		}
		++i;
	}
	if( !foundEffect )
	{
		CCP_LOGERR( "Effect '%s' needs to be under /effect/", path.c_str() );
		return false;
	}
	if( !dot )
	{
		CCP_LOGWARN( "Invalid effect path '%s', no extension", path.c_str() );
		dot = i;
	}
	actualPath[dot++] = '.';
	const char* sm;
	switch( Tr2Renderer::GetShaderModel() )
	{
	case TR2SM_3_0_DEPTH:
		sm = "sm_depth";
		break;
	case TR2SM_3_0_HI:
		sm = "sm_hi";
		break;
	default:
		sm = "sm_lo";
	}
	while( *sm )
	{
		actualPath[dot++] = *sm++;
	}
	return true;
}

// ---------------------------------------------------------------
bool Tr2Effect::Initialize()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_effectResource )
	{
		ReleaseCachedData( m_effectResource );
		m_effectResource->RemoveNotifyTarget( this );
		m_effectResource.Unlock();
	}

	if( m_effectFilePath.size() > 0 )
	{
		if( !ConvertEffectPath( m_effectFilePath, m_actualEffectFilePath ) )
		{
			m_actualEffectFilePath = "";
			return true;
		}
		BeResMan->GetResource( m_actualEffectFilePath.c_str(), "", BlueInterfaceIID<Tr2EffectRes>(), (void**)&m_effectResource );

		if( m_effectResource )
		{
			m_effectResource->AddNotifyTarget( this );
		}
	}
	else
	{
		m_actualEffectFilePath = "";
	}

	return true;
}

// ---------------------------------------------------------------
void Tr2Effect::SetEffectPathName( const char* path )
{
	m_effectFilePath = path ? path : "";
	Initialize();
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a texture 2d resource to this effect's list with creating
//   it
// --------------------------------------------------------------------------------
void Tr2Effect::AddResourceTexture2D( const char* name, const char* resPath )
{
	// alloc and init the texture parameter
	TriTexture2DParameterPtr texture2d;
	texture2d.CreateInstance();
	texture2d->SetParameterName( name );
	texture2d->SetResourcePath( resPath );
	// add it to this effect's resources
	m_resources.Append( texture2d->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a color parameter to this effect's list with creating
//   it
// --------------------------------------------------------------------------------
void Tr2Effect::AddParameterVector4( const char* name, const Vector4* value )
{
	// alloc and init the vec4 parameter
	Tr2Vector4ParameterPtr param;
	param.CreateInstance();
	param->m_name = BlueSharedString( name );
	param->m_value = *value;
	// add it to this effect's parameters
	m_parameters.Append( param->GetRawRoot() );
}

// --------------------------------------------------------------------------------
// Description:
//   Manually adding a vector4 parameter to this effect's list with creating
//   it
// --------------------------------------------------------------------------------
void Tr2Effect::AddParameterColor( const char* name, const Color* value )
{
	// alloc and init the vec4 parameter
	Tr2Vector4ParameterPtr param;
	param.CreateInstance();
	param->m_name = BlueSharedString( name );
	param->m_value = *reinterpret_cast<const Vector4*>( value );
	// add it to this effect's parameters
	m_parameters.Append( param->GetRawRoot() );
}

// ---------------------------------------------------------------
void Tr2Effect::RebuildCachedDataInternal()
{
	m_parameterHash = INVALID_PARAMETER_HASH;

	if( m_effectResource )
	{		
		RebuildCachedDataForEffect( *m_effectResource, *this, m_parametersForPasses );
	}

	// It's ok to pass in NULL values to these functions so that the parameters
	// realize that they're not in use by a NULL technique etc

	for( size_t i = 0; i != m_parameters.size(); ++i )
	{
		m_parameters[i]->RebuildEffectHandles( m_effectResource );
	}

	for( size_t i = 0; i != m_resources.size(); ++i)
	{
		m_resources[i]->RebuildEffectHandles( m_effectResource );
	}

}

void Tr2Effect::RebuildCachedData( BlueAsyncRes* p )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( p == m_effectResource );
	if( p == m_effectResource )
	{
		RebuildCachedDataInternal();
	}
}

void Tr2Effect::ReleaseCachedData( BlueAsyncRes* p )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	CCP_ASSERT( p == m_effectResource );
	if( p == m_effectResource )
	{
		const int numResources = (unsigned int)m_resources.size();
		const int numParameters = (unsigned int)m_parameters.size();

		// It's ok to pass in NULL values to these functions so that the parameters
		// realize that they're not in use by a NULL technique etc.
		// It's important to notify parameters of the fact that the effect
		// has been reset
		for (int i = 0; i < numParameters; i++)
		{
			m_parameters[i]->RebuildEffectHandles( nullptr );
		}

		for (int i = 0; i < numResources; i++)
		{
			m_resources[i]->RebuildEffectHandles( nullptr );
		}
	}
}

// ---------------------------------------------------------------
bool Tr2Effect::OnModified( Be::Var* value )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	// m_effectFilePath is the only attribute with a notify flag

	Initialize();

	// Immediately invalidate the cached information
	RebuildCachedDataInternal();

	return true;
}

// ---------------------------------------------------------------
void Tr2Effect::StartUpdate()
{
	m_insideStartUpdate = true;
}

// ---------------------------------------------------------------
void Tr2Effect::EndUpdate()
{
	if( m_insideStartUpdate )
	{
		m_insideStartUpdate = false;
		RebuildCachedDataInternal();
	}
}

// ---------------------------------------------------------------
void Tr2Effect::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* currvalue, const IList* theList )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_insideStartUpdate )
	{
		return;
	}

	if( (event & BELIST_LOADING) == 0  )
	{
		switch( event & BELIST_EVENTMASK )
		{
			case BELIST_REMOVED:
				{
					ITriReroutablePtr rp( BlueCastPtr( currvalue ) );
					if( rp )
					{
						rp->SetDestination( NULL, 0 );
					}
				}
				RebuildCachedDataInternal();
				break;

			case BELIST_INSERTED:
			case BELIST_SWAPPED:
			case BELIST_MOVED:
				RebuildCachedDataInternal();
				break;
		}
	}
}

// ---------------------------------------------------------------
// PopulateParameters
// 
// Populates the list of parameters on the mesh using the effect file
// Visibility annotations default to false, so you need to specifically
// expose stuff.
// ---------------------------------------------------------------
bool Tr2Effect::PopulateParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if (!m_effectResource)
	{
		CCP_LOGERR( "No effect resource loaded." );
		return false;
	}

	auto paramAdder = [&]( ITriEffectParameter* p )
						{
							m_parameters.Insert( -1, p );
						};

	auto resourceAdder = [&]( ITriEffectResourceParameter* p )
						{
							m_resources.Insert( -1, p );
						};

	for( unsigned passIx = 0; passIx < m_effectResource->GetPassCount(); ++passIx )
	{
		const Tr2Pass& pass = m_effectResource->GetPass( passIx );
		for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			const auto& input = pass.stageInputs[i];

			for( auto constant = input.constants.cbegin(); constant != input.constants.cend(); ++constant )
			{
				if( !GetBool( m_effectResource, constant->name.c_str(), "SasUiVisible" ) )
				{
					continue;
				}

				if( GetParameterByName( constant->name.c_str() ) )
				{
					continue;
				}
				
				ConvertEffectConstant( *constant, input.constantValues, paramAdder );										
			}

			for( auto sampler = input.resources.begin(); sampler != input.resources.end(); ++sampler )
			{
				if( !GetBool( m_effectResource, sampler->second.name, "SasUiVisible" ) )
				{
					continue;
				}

				if( GetParameterByName( sampler->second.name ) )
				{
					continue;
				}
				
				ConvertEffectResource( sampler->second, paramAdder, resourceAdder );
			}

			for( auto uav = input.uavs.begin(); uav != input.uavs.end(); ++uav )
			{
				if( !GetBool( m_effectResource, uav->second.name, "SasUiVisible" ) )
				{
					continue;
				}

				if( GetParameterByName( uav->second.name ) )
				{
					continue;
				}
				
				ConvertEffectResource( uav->second, paramAdder, resourceAdder );
			}
		}
	}

	RebuildCachedDataInternal();
	return true;
}


// ---------------------------------------------------------------
// PruneParameters
// 
// Removes any parameters from the object that do not exist in the
// effect file
// ---------------------------------------------------------------
bool Tr2Effect::PruneParameters()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if (!m_effectResource)
	{
		CCP_LOGERR( "No effect resource loaded." );
		return false;
	}

	// Prune Effects Parameters
	int y = 0;
	while (y < m_parameters.GetSize())
	{
		std::string pName = m_parameters[y]->GetParameterName();
		bool removeParameter = !GetBool( m_effectResource, pName.c_str(), "SasUiVisible" );
		const Tr2EffectConstant *constant = m_effectResource->GetConstant( pName.c_str() );
		if( constant == nullptr && !removeParameter )
		{
			removeParameter = true;
			for( unsigned passIx = 0; passIx < m_effectResource->GetPassCount() && removeParameter; ++passIx )
			{
				const Tr2Pass& pass = m_effectResource->GetPass( passIx );
				for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
				{
					for( auto sampler = pass.stageInputs[i].resources.begin(); sampler != pass.stageInputs[i].resources.end(); ++sampler )
					{
						if( strcmp( sampler->second.name, pName.c_str() ) == 0 )
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
			m_parameters.Remove(y);
		}
		else
		{
			++y;
		}
	}

	// Prune Effects Resources
	y = 0;
	while (y < m_resources.GetSize())
	{
		std::string pName = m_resources[y]->GetParameterName();
		bool removeParameter = !GetBool( m_effectResource, pName.c_str(), "SasUiVisible" );
		const Tr2EffectConstant *constant = m_effectResource->GetConstant( pName.c_str() );
		if( constant == nullptr && !removeParameter )
		{
			removeParameter = true;
			for( unsigned passIx = 0; passIx < m_effectResource->GetPassCount() && removeParameter; ++passIx )
			{
				const Tr2Pass& pass = m_effectResource->GetPass( passIx );
				for( unsigned i = 0; i < Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
				{
					for( auto sampler = pass.stageInputs[i].resources.begin(); sampler != pass.stageInputs[i].resources.end(); ++sampler )
					{
						if( strcmp( sampler->second.name, pName.c_str() ) == 0 )
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
			m_resources.Remove(y);
		}
		else
		{
			++y;
		}
	}

	return true;
}

bool Tr2Effect::IsParameterUsedByTechnique( const std::string& parameterName )
{
	if (!GetEffectRes())
	{
		CCP_LOGERR( "No effect resource loaded." );
		return false;
	}

	return GetEffectRes()->GetConstant( parameterName.c_str() ) != nullptr;
}

unsigned int Tr2Effect::GetSortValue() const
{
	if( m_effectResource )
	{
		return m_effectResource->GetSortValue();
	}
	else
	{
		return m_sortValue;
	}
}

bool Tr2Effect::IsEqual( Tr2Effect* other )
{
	if( other == NULL )
	{
		return false;
	}

	if( other == this )
	{
		return true;
	}

	if( GetEffectRes() != other->GetEffectRes() )
	{
		return false;
	}

	if( m_resources.GetSize() != other->m_resources.GetSize() )
	{
		return false;
	}

	for( EffectResourceList::iterator it = m_resources.begin(), itOther = other->m_resources.begin(); it != m_resources.end(); ++it, ++itOther )
	{
	    ITriEffectResourceParameter* mine = *it;
		ITriEffectResourceParameter* theirs = *itOther;

		if( mine->GetResourcePointer() != theirs->GetResourcePointer() )
		{
			return false;
		}
	}

	// todo: parameters

	return true;
}

Tr2EffectRes* Tr2Effect::GetEffectRes() const
{
	return m_effectResource;
}

ITriEffectParameter* Tr2Effect::GetParameterByName( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	ITriEffectParameter* result = FindParameterByName( name );

	if ( !result )
	{
		result = FindResourceByName( name );
	}

	return result;
}

// --------------------------------------------------------------------------------------
// Description:
//   Assigns a variable store to use when binding shader variables.
// Arguments:
//   variableStore - Variable store to use for binding shader variables.
// See Also:
//   BindLowLevelShaders
// --------------------------------------------------------------------------------------
void Tr2Effect::SetVariableStore( Tr2VariableStore* variableStore )
{
	m_variableStore = variableStore;
}

// --------------------------------------------------------------------------------------
// Description:
//   Implements ITr2ShaderMaterial interface. Returns an array of constant effect 
//   parameters.
// Arguments:
//   count - (out) Number of constant parameters
// Return Value:
//   Pointer to the first element in constant parameter array
// --------------------------------------------------------------------------------------
const Tr2ConstantEffectParameter* Tr2Effect::GetConstParameters( size_t& count ) const
{
	count = m_constParameters.size();
	if( count )
	{
		return &m_constParameters[0];
	}
	else
	{
		return nullptr;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns a variable store used by material to bind shader variables.
// Return value:
//   Variable store used by this material.
// --------------------------------------------------------------------------------------
Tr2VariableStore& Tr2Effect::GetVariableStore()
{
	if( m_variableStore )
	{
		return *m_variableStore;
	}
	return GlobalStore();
}

void Tr2Effect::MapPassResources( const Tr2EffectResourceMap& resources, Tr2EffectParamVector &pv, uint32_t resourceFlags )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = resources.begin(); it != resources.end(); ++it )
	{
		const Tr2EffectResource& ss = it->second;
		const char* name = ss.name;

		Tr2EffectParam param;

		// First search in effect resource list 
		if( ITriEffectParameter* p = FindResourceByName( name ) )
		{
			param.m_sourceValue = p;
		}
		// Secondly search in effect parameter list
		else if( ITriEffectParameter* p = FindParameterByName( name ) )
		{
			// The only parameter that can point to a resource is the TriVariableParameter
			TriVariableParameter* vp = dynamic_cast<TriVariableParameter*>(p);
			if( vp 
				&& vp->m_variable
				&& ( vp->m_variable->GetType() == TRIVARIABLE_UNKNOWN_TEXTURE || 
					 vp->m_variable->GetType() == TRIVARIABLE_TEXTURE_RES || 
					 vp->m_variable->GetType() == TRIVARIABLE_TEXTURE_AL  ) )
			{
				param.m_sourceValue = p;
			}

		}
		// Fallback to variable store
		else if( TriVariable* v = GetVariableStore().FindVariable( name ) )
		{
			if( v->GetType() == TRIVARIABLE_UNKNOWN_TEXTURE || 
				v->GetType() == TRIVARIABLE_TEXTURE_RES 	|| 
				v->GetType() == TRIVARIABLE_TEXTURE_AL )
			{
				param.m_sourceValue = v;
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


void Tr2Effect::ApplyShaderInputs(	unsigned passIndex, 
									Tr2RenderContextEnum::ShaderType shaderType, 
									Tr2RenderContext& renderContext )
{
	bool samplersChanged;
	::ApplyShaderInputs( *m_parametersForPasses[ passIndex ], shaderType, samplersChanged, renderContext );
}

ITr2ShaderState* Tr2Effect::GetShaderStateInterface() const
{
	Tr2EffectRes* res = GetEffectRes();
	if( res && res->IsGood() )
	{
		return res;
	}

	return NULL;
}

uint32_t Tr2Effect::ApplyMaterialDataForPass( unsigned int passIndex, Tr2RenderContext& renderContext )
{
	return ::ApplyMaterialDataForPass( m_parametersForPasses, GetEffectRes(), passIndex, renderContext );
}

void Tr2Effect::Render( IRenderCallback* cb, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2EffectRes* effectResource = GetEffectRes();

	if( !effectResource || !effectResource->IsGood() )
	{
		return;
	}

	CCP_ASSERT( cb );

	unsigned int passCount = effectResource->GetPassCount();

	for( uint32_t passIx = 0; passIx < passCount; ++passIx )
	{
		effectResource->ApplyAllStateForPass( passIx, renderContext );
		ApplyMaterialDataForPass( passIx, renderContext );
		cb->SubmitGeometry( renderContext );
	}

}

void Tr2Effect::RenderForPicking( IRenderCallback* cb, int objId, Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Tr2EffectRes* effectResource = GetEffectRes();

	if( !effectResource || !effectResource->IsGood() )
	{
		return;
	}

	CCP_ASSERT( cb );

	TriVariable* objIdVariable = renderContext.m_esm.GetObjectIdVariable();
	if( objIdVariable )
	{
		objIdVariable->SetValue( (float)objId );
	}

	unsigned int passCount = effectResource->GetPassCount();

	for( uint32_t passIx = 0; passIx < passCount; ++passIx )
	{
		effectResource->ApplyAllStateForPass( passIx, renderContext );
		ApplyMaterialDataForPass( passIx, renderContext );
		cb->SubmitGeometry( renderContext );
	}
}

ITriEffectParameter* Tr2Effect::FindParameterByName( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_parameters.cbegin(); it != m_parameters.cend(); ++it )
	{
		ITriEffectParameter* p = *it;
		const char* candidate = p->GetParameterName();
		if( candidate && strcmp( candidate, name ) == 0 )
		{
			return p;
		}
	}

	return nullptr;
}

ITriEffectParameter* Tr2Effect::FindResourceByName( const char* name ) const
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_resources.cbegin(); it != m_resources.cend(); ++it )
	{
		ITriEffectParameter* p = *it;
		const char* candidate = p->GetParameterName();
		if( candidate && strcmp( candidate, name ) == 0 )
		{
			return p;
		}
	}

	return nullptr;
}


// --------------------------------------------------------------------------------------
// Description:
//	Find an annotation called annotationName in the map, and return its value. If not found, or it's not a
//	bool, or map is nullptr, return defaultValue instead.
// Arguments:
//   map - annotation map, may be nullptr
//   annotationName - name of the annotation, may be nullptr
//	 defaultValue - what to return if not found or the other arguments are nullptr
// Return Value:
//   the value of this annotation, if it can be found and is of type BOOL.
// -------------------------------------------------------------------------------------
bool GetBool( const Tr2EffectParameterAnnotationMap* map, const char* annotationName, bool defaultValue )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !map || !annotationName )
	{
		return defaultValue;
	}

	auto value = std::find_if( map->begin(), map->end(), [&]( const Tr2EffectParameterAnnotation& a ) { return strcmp( a.name, annotationName ) == 0; } );
	if( value != map->end() && value->type == Tr2EffectParameterAnnotation::BOOL )
	{
		return value->boolValue;
	}

	return defaultValue;
}

// --------------------------------------------------------------------------------------
// Description:
//	Find an annotation called annotationName in the annotation map of shaderState, belonging to a
//  parameter called paramName.  Return its value if found and bool.
// Arguments:
//   shaderState - shader state, may be nullptr
//   paramName - name of the parameters of which the annotations need to be scanned, may be nullptr
//   annotationName - name of the annotation, may be nullptr
//	 defaultValue - what to return if not found or the other arguments are nullptr
// Return Value:
//   the value of this annotation on this parameter, if both can be found and the annotation is of type BOOL.
// -------------------------------------------------------------------------------------
bool GetBool( const ITr2ShaderState* shaderState, const char* paramName, const char* annotationName, bool defaultValue )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	return	( shaderState && paramName )
			? GetBool( shaderState->GetParameterAnnotations( paramName ), annotationName, defaultValue )
			: defaultValue;
}

void RebuildCachedDataForEffect(	ITr2ShaderState &effectResource, 
									ITr2ShaderMaterial &owner,
									Tr2EffectPassParametersVector& parametersForPasses )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	USE_MAIN_THREAD_RENDER_CONTEXT();

	parametersForPasses.clear();

	auto& desc = effectResource.GetEffectDescription();
	const unsigned passCount = unsigned( desc.passes.size() );

	if( !passCount )
	{
		return;
	}

	parametersForPasses.resize( passCount );

	for( unsigned passIx = 0; passIx != passCount; ++passIx )
	{
		parametersForPasses[passIx].reset( CCP_NEW( "Tr2EffectPassParameters" ) Tr2EffectPassParameters() );
		Tr2EffectPassParameters& pp = *parametersForPasses[passIx];

		for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT; ++i )
		{
			auto& stage = desc.passes[passIx].stageInputs[i];
			if( !stage.m_exists )
			{
				continue;
			}

			MapPassParameters( 
				passIx,
				pp,
				Tr2RenderContextEnum::ShaderType( i ),
				stage.constants, 
				effectResource,
				desc,
				owner,
				renderContext );

			auto& input = pp.m_stageInput[i];
			if( !stage.resources.empty() )
			{
				owner.MapPassResources( stage.resources, input.m_samplers, 0 );
			}
			if( !stage.uavs.empty() )
			{
				owner.MapPassResources( stage.uavs, input.m_uavs, ITr2EffectValue::RESOURCE_FLAG_UAV );
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies shader inputs for the given pass.  
// Arguments:
//   pp - parameters for the pass that's being applied
//   shaderType - Stage input type (vertex shader, pixel shader, etc.)
//   samplersChanged - (out) will be set to true if any of the bound texture parameters
//     changed its sampler state
//   context - Current render context
// --------------------------------------------------------------------------------------
void ApplyShaderInputs( Tr2EffectPassParameters& pp,
						Tr2RenderContextEnum::ShaderType shaderType, 
						bool& samplersChanged,
						Tr2RenderContext& renderContext )
{
	auto& input = pp.m_stageInput[shaderType];

	auto cb = input.m_constantBuffer.get();
	if( cb && cb->GetSize() )
	{
		CCP_STATS_INC( effectCBLocks );
		uint8_t* const mirror = reinterpret_cast<uint8_t*>( cb->GetBufferMirror( renderContext ) );
		if( cb->IsValid() && mirror )
		{
			const auto endVS = input.m_shaderParameters.cend();
			for( auto it = input.m_shaderParameters.cbegin(); it != endVS; ++it )
			{
				size_t size = it->m_registerCount;
				uint8_t* const dst = mirror + it->m_registerIndex;
				it->m_sourceValue->CopyValueToEffect( shaderType, dst, size, renderContext );
			}
			cb->UpdateFromMirror( renderContext );

			renderContext.SetConstants( *cb, shaderType, CONSTANT_BUFFER_FOR_EFFECT_PARAMETERS );
		}
	}

	samplersChanged = false;
	for( auto it = input.m_samplers.cbegin(); it != input.m_samplers.cend(); ++it )
	{
		unsigned char destHandle[2] = { static_cast<unsigned char>( it->m_registerIndex ), 0 };
		it->m_sourceValue->CopyValueToEffect( shaderType, destHandle, it->m_registerCount, renderContext );
		if( destHandle[1] )
		{
			samplersChanged = true;
		}
	}

	for( auto it = input.m_uavs.cbegin(); it != input.m_uavs.cend(); ++it )
	{
		unsigned char destHandle[2] = { static_cast<unsigned char>( it->m_registerIndex ), 0 };
		it->m_sourceValue->CopyValueToEffect( shaderType, destHandle, it->m_registerCount, renderContext );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Look at the constant and build an ITriEffectParameter out of it
// Arguments:
//   constant - constant being analyzed
//   constantValues - blob with the default value for the constant
//   adder - callback function that's in charge of actually adding the newly created parameter
// --------------------------------------------------------------------------------------
void ConvertEffectConstant( const Tr2EffectConstant& constant, 
							const char* constantValues,
							std::function<void(ITriEffectParameter*)> adder )
{
	if( constant.type != Tr2EffectConstant::FLOAT )
    {
        return;
    }
    if( constant.elements > 1 )
    {
        // create new paramater
        OTriFloatArrayParameter* newFloatArray = new OTriFloatArrayParameter();  // Creates object with 1 lock
		newFloatArray->m_name = BlueSharedString( constant.name );
        // create an instance for each vector4
        for( unsigned int e = 0; e < constant.elements; ++e )
        {
            TriVector4Ptr newVector4;
			newVector4.CreateInstance();
            newVector4->m_data = *reinterpret_cast<const Vector4*>( constantValues + constant.offset + e * sizeof( Vector4 ) );
            newFloatArray->m_value.Insert( -1, newVector4 );
        }
        adder( newFloatArray );
        newFloatArray->Unlock();
    }
    else if( constant.dimension == 16 )
    {
		if( strstr( constant.name.c_str(), "Reflection" ) )
        {
            OTriVariableParameter* p = new OTriVariableParameter();
            p->m_name = BlueSharedString( constant.name );
            p->m_variableName = BlueSharedString( "EnvMapTransform" );
            p->Initialize();
            adder( p );
            p->Unlock(); // Remove the original lock created by 'new'.
        }
        else
        {
            OTr2Matrix4Parameter* p = new OTr2Matrix4Parameter();
            p->m_name = BlueSharedString( constant.name );
            p->m_value = *reinterpret_cast<const Matrix*>( constantValues + constant.offset );
            adder( p );
            p->Unlock();
        }
    }
    else if( constant.dimension == 2 )
    {
        OTr2Vector2Parameter* newVector2 = new OTr2Vector2Parameter();  // Creates object with 1 lock
        newVector2->m_name = BlueSharedString( constant.name );
        newVector2->m_value = *reinterpret_cast<const Vector2*>( constantValues + constant.offset );
        adder( newVector2 );
        newVector2->Unlock();
    }
    else if( constant.dimension == 3 )
    {
        OTr2Vector3Parameter* newVector3 = new OTr2Vector3Parameter();  // Creates object with 1 lock
        newVector3->m_name = BlueSharedString( constant.name );
        newVector3->m_value = *reinterpret_cast<const Vector3*>( constantValues + constant.offset );
        adder( newVector3 );
        newVector3->Unlock();
    }
    else if( constant.dimension > 1 )
    {
        OTr2Vector4Parameter* newVector4 = new OTr2Vector4Parameter();  // Creates object with 1 lock
        newVector4->m_name = BlueSharedString( constant.name );
        newVector4->m_value = *reinterpret_cast<const Vector4*>( constantValues + constant.offset );
        adder( newVector4 );
        newVector4->Unlock();
    }
    else
    {
        OTr2FloatParameter* newFloat = new OTr2FloatParameter();
        newFloat->m_name = BlueSharedString( constant.name );
        newFloat->m_value = *reinterpret_cast<const float*>( constantValues + constant.offset );
        adder( newFloat );
        newFloat->Unlock(); // Remove the original lock created by 'new'.
    }
}

// --------------------------------------------------------------------------------------
// Description:
//   Look at the constant and build an ITriEffectParameter and/or ITriEffectResourceParameter out of it
// Arguments:
//   texture - texture being analyzed
//   paramAdder - callback function that's in charge of actually adding any newly created parameters
//   resourceAdder - callback function that's in charge of actually adding any newly created resources
// --------------------------------------------------------------------------------------
void ConvertEffectResource(	const Tr2EffectResource& resource, 
							std::function<void(ITriEffectParameter*)> paramAdder,
							std::function<void(ITriEffectResourceParameter*)> resourceAdder )
{
	switch( resource.type )
	{
	case Tr2EffectResource::TEXTURE_CUBE:
		if( strstr( resource.name, "Reflection" ) )
		{
			OTriVariableParameter* p = new OTriVariableParameter;
			p->m_name = BlueSharedString( resource.name );
			p->m_variableName = BlueSharedString( "EnvMap1" );
			p->Initialize();
			paramAdder( p );
			p->Unlock(); // Remove the original lock created by 'new'.
		}
		else
		{
			OTriTextureCubeParameter* newTexCube = new OTriTextureCubeParameter();
			newTexCube->SetParameterName( resource.name );
			resourceAdder( newTexCube );
			newTexCube->Unlock(); // Remove the original lock created by 'new'.
		}
		break;
	case Tr2EffectResource::TEXTURE_1D:
	case Tr2EffectResource::TEXTURE_2D:
	case Tr2EffectResource::TEXTURE_3D:
	case Tr2EffectResource::TEXTURE_TYPELESS:
		{
			OTriTexture2DParameter* newTex2D = new OTriTexture2DParameter();
			newTex2D->SetParameterName( resource.name );
			resourceAdder( newTex2D );
			newTex2D->Unlock(); // Remove the original lock created by 'new'.
		}
		break;
	default:
		{
			OTr2GeometryBufferParameter* newBuffer = new OTr2GeometryBufferParameter();
			newBuffer->m_name = resource.name;
			resourceAdder( newBuffer );
			newBuffer->Unlock(); // Remove the original lock created by 'new'.
		}
		break;
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Applies vertex and pixel shader inputs for this given pass.
// Arguments:
//	 vec - all parameters for all passes
//	 resource - effect resource from which the parameters are being applied
//   passIndex - The index of the pass for which to apply vertex and pixel shader inputs
//   context - Current render context
// Return value:
//   A mask containing bits for each stage that has modified sampler state
// --------------------------------------------------------------------------------------
uint32_t ApplyMaterialDataForPass( Tr2EffectPassParametersVector& vec, ITr2ShaderState* resource, unsigned passIndex, Tr2RenderContext& renderContext )
{
	unsigned mask = resource ? resource->GetShaderTypeMask() : 0xffFFffFFu;
	uint32_t samplersChangedMask = 0;
	for( unsigned i = 0; i != Tr2RenderContextEnum::SHADER_TYPE_COUNT && mask; ++i )
	{
		if( mask & ( 1 << i ) )
		{
			bool samplersChanged = false;
			ApplyShaderInputs( *vec[passIndex], Tr2RenderContextEnum::ShaderType( i ), samplersChanged, renderContext );
			mask &= ~( 1 << i );
			if( samplersChanged )
			{
				samplersChangedMask |= 1 << i;
			}
		}
	}
	return samplersChangedMask;
}

// --------------------------------------------------------------------------------------
// Description:
//   Maps the parameters for a pass to indices in the constant mirror.
// Arguments:
//   passIx - Pass index
//   pp - Pass parameters
//   stage - Stage type
//   constants - The constant table
//	 resource - effect being mapped
//   owner - shaderState being mapped
//	 renderContext - render context
// --------------------------------------------------------------------------------------
void MapPassParameters( 
			unsigned passIx,
			Tr2EffectPassParameters& pp,
			Tr2RenderContextEnum::ShaderType stage,
			const Tr2EffectConstantVector& constants, 
			ITr2ShaderState& resource,
			const Tr2EffectDescription& desc,
			ITr2ShaderMaterial& owner,
			Tr2RenderContext& renderContext )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	static const size_t MAX_PARAMS = 64;

	Tr2EffectParamVector &pv = pp.m_stageInput[stage].m_shaderParameters;
	ITriReroutableVector& reroutables = pp.m_reroutedParameters;
	Tr2VariableStore& variableStore = owner.GetVariableStore();

	unsigned int perObjectStart = 0xffffffff;

#if( TRINITY_PLATFORM==TRINITY_DIRECTX9 )
	// First find perObjectStart if the effect has it:
	const char* perObjectName = stage == PIXEL_SHADER ? "PerObjectPS" : "PerObjectVS";
	for( auto constantIx = constants.begin(); constantIx != constants.end(); ++constantIx )
	{
		if( strcmp( constantIx->name.c_str(), perObjectName ) == 0 )
		{
			perObjectStart = constantIx->offset;
			break;
		}
	}
#endif

	CCP_ASSERT( constants.size() < MAX_PARAMS );
	ITr2EffectValue* foundParams[MAX_PARAMS];
	unsigned constantSize = 0;
	unsigned index = 0;
	bool hasConstantParams = false;
	bool hasVariableParams = false;

	size_t constParamCount;
	auto constParams = owner.GetConstParameters( constParamCount );
	uint32_t constIndexes[MAX_PARAMS];

	// Fist pass: determine the size of the constant buffer
	for( auto constantIx = constants.begin(); constantIx != constants.end(); ++constantIx )
	{
		constIndexes[index] = -1;
		bool foundConstant = false;
		for( size_t i = 0; i < constParamCount; ++i )
		{
			if( constantIx->name == constParams[i].name )
			{
				constIndexes[index] = uint32_t( i );
				foundConstant = true;
				hasConstantParams = true;
				break;
			}
		}
		ITr2EffectValue* paramAsEffectValue = NULL;
		if( !foundConstant )
		{

			// First search in effect parameter list and see if we have a match
			if( ITriEffectParameter* p = owner.FindParameterByName( constantIx->name.c_str() ) )
			{
				paramAsEffectValue = p;
			}
			// Fallback to variable store
			else if( TriVariable* v = variableStore.FindVariable( constantIx->name.c_str() ) )
			{
				paramAsEffectValue = v;
			}
			else
			{
				if( constantIx->isAutoregister )
				{
					paramAsEffectValue = GlobalStore().GetVariable( constantIx->name.c_str() );
				}
			}
			hasVariableParams |= paramAsEffectValue != nullptr;
			foundConstant = paramAsEffectValue != nullptr;
		}
		foundParams[index++] = paramAsEffectValue;

		if( foundConstant )
		{
			// It's illegal to pass the perObjectStart if the effect has the block
			if( constantIx->offset >= perObjectStart )
			{
				CCP_ASSERT_M( false, "Register is mapped beyond valid range" );
				CCP_LOGERR( "Effect maps parameter '%s' beyond limit (target is c%d, limit is c%d)", 
					constantIx->name.c_str(), constantIx->offset, perObjectStart );

				// We must ignore this parameter!
				constIndexes[index - 1] = -1;
				foundParams[index - 1] = nullptr;
				continue;
			}

			unsigned int constantIndex = constantIx->offset + constantIx->size;
			if( constantIndex > constantSize )
			{
				constantSize = constantIndex;
			}
		}
	}

	unsigned int constantDefaultValueSize = desc.passes[passIx].stageInputs[stage].m_constantValueSize;
	const void* constantDefaultValues = desc.passes[passIx].stageInputs[stage].constantValues;

	constantSize = std::max( constantSize, constantDefaultValueSize );
	// Allocate constant buffer
	pp.AllocateConstantMirror( stage, constantSize );
	if( constantSize == 0 || !pp.m_stageInput[stage].m_constantBuffer )
	{
		return;
	}

	void* mirror = pp.m_stageInput[stage].m_constantBuffer->GetBufferMirror( renderContext );
	if( !mirror )
	{
		return;
	}

	memcpy( mirror, constantDefaultValues, constantDefaultValueSize );

	if( hasConstantParams )
	{
		for( size_t i = 0; i < constants.size(); ++i )
		{
			if( constIndexes[i] != -1 )
			{
				auto& c = constants[i];
				memcpy( static_cast<uint8_t*>( mirror ) + c.offset, &constParams[constIndexes[i]].value, sizeof( float ) * c.dimension );
			}
		}
	}

	if( hasVariableParams )
	{
		index = 0;
		// Now iterate again over the list and process the parameters:
		for( auto constantIx = constants.begin(); constantIx != constants.end(); ++constantIx )
		{
			ITriReroutablePtr paramAsReroutable;
			ITr2EffectValuePtr paramAsEffectValue = foundParams[index++];

			ITriEffectParameterPtr param = ITriEffectParameterPtr( BlueCastPtr( paramAsEffectValue ) );
			if( param )
			{
				// Notify a parameter of its future binding. Here
				// the parameter might check for sRGB flags, etc.
				param->RebuildEffectHandles( &resource );

				paramAsReroutable = ITriReroutablePtr( BlueCastPtr( param ) );
				if( paramAsReroutable )
				{
					paramAsEffectValue.Unlock();
				}
			}

			if( paramAsEffectValue || paramAsReroutable )
			{
				// It's illegal to pass the perObjectStart if the effect has the block
				if( constantIx->offset >= perObjectStart )
				{
					// We must ignore this parameter! ValuePassParameters will have
					// spit out an error message.
					continue;
				}

				if( paramAsReroutable )
				{
					// Parameter is reroutable - this means we can hook it up directly
					// and won't have to copy its value every frame

					if( paramAsReroutable->IsRerouted() )
					{
						// Parameter is already rerouted - likely used in a previous pass
						paramAsEffectValue = param;
					}
					else
					{
						CCP_ASSERT( constantIx->offset + constantIx->size <= constantSize );

						void* dest = (void*)((uint8_t*)mirror + constantIx->offset);
						paramAsReroutable->SetDestination( dest, constantIx->size );

						// check that it actually worked; ie the param may be reroutable in theory, but not for
						// this specific instance.
						if( !paramAsReroutable->IsRerouted() )
						{
							paramAsEffectValue = param;
						}
						else
						{
							reroutables.push_back( paramAsReroutable.Detach() );				
						}
					}
				}
			
				if( paramAsEffectValue )
				{

					Tr2EffectParam param;
					param.m_sourceValue = paramAsEffectValue;

					// Size is now passed down when copying value from parameter so
					// we don't need to check for the size here. In fact, it is normal
					// for the destination to be smaller than the value, in particular for
					// TriTransformParameter where a portion of a 4x4 matrix is used.

					param.m_sourceName = constantIx->name.c_str();
					param.m_registerIndex = constantIx->offset;
					param.m_registerCount = constantIx->size;

					pv.push_back( param );
				}
			}
		}
	}
}

void Tr2Effect::UnloadResources()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_resources.cbegin(); it != m_resources.cend(); ++it )
	{
		(*it)->UnloadResources();
	}
}

void Tr2Effect::LoadResources()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( auto it = m_resources.cbegin(); it != m_resources.cend(); ++it )
	{
		(*it)->LoadResources();
	}
}
