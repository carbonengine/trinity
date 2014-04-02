#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorEnlightenSystem.h"

#include "include/ITr2DebugRenderer.h"
#include "Resources/TriGeometryRes.h"
#include "TriLineSet.h"
#include "TriSettingsRegistrar.h"
#include "Tr2EnlightenArea.h"
#include "Tr2InteriorCell.h"
#include "Tr2InteriorStatic.h"
#include "Tr2PushPopDS.h"
#include "Tr2PushPopRT.h"

using namespace Tr2RenderContextEnum;

static unsigned s_currentEnlightenSystemGuid = 1;
CCP_STATS_DECLARED_ELSEWHERE( triEnlightenActiveSystems );
CCP_STATS_DECLARED_ELSEWHERE( triEnlightenTextureMemory );
CCP_STATS_DECLARED_ELSEWHERE( triEnlightenObjectMemory );
CCP_STATS_DECLARE( enlightenSystemUpdates, "Trinity/Tr2InteriorEnlightenSystem/enlightenSystemUpdates", true, CST_COUNTER_LOW, "Number of SolveTask calls per frame" );
CCP_STATS_DECLARE( enlightenInputLightingUpdates, "Trinity/Tr2InteriorEnlightenSystem/enlightenInputLightingUpdates", true, CST_COUNTER_LOW, "Number of input lighting update calls per frame" );
CCP_STATS_DECLARE( enlightenPixelUpdates, "Trinity/Tr2InteriorEnlightenSystem/enlightenPixelUpdates", true, CST_COUNTER_LOW, "Number of pixels being updated per frame" );
CCP_STATS_DECLARE( wiscAddStaticCount, "Trinity/Tr2InteriorEnlightenSystem/AddStaticCalls", true, CST_COUNTER_LOW, "Number of calls to Tr2InteriorEnlightenSystem::AddStatic per frame" );
CCP_STATS_DECLARE( wiscRemoveStaticCount, "Trinity/Tr2InteriorEnlightenSystem/RemoveStaticCalls", true, CST_COUNTER_LOW, "Number of calls to Tr2InteriorEnlightenSystem::RemoveStatic per frame" );

extern CcpLogChannel_t g_enlightenBuildChannel;

// Define a workaround for crashes in Geo::VConstruct when it's not inlined (compiler error?)
#define GEO_VCONSTRUCT( x, y, z, w ) (_mm_setr_ps((x), (y), (z), (w)))
//#define GEO_VCONSTRUCT( x, y, z, w ) (Geo::VConstruct((x), (y), (z), (w)))

// Scaling factor for Enlighten albedo/emissive render targets
static int s_albedoTextureSupersamplingScale = 4;

// Situation names for generating albedo and emissive textures.
static const char* s_generateAlbedoSituation = "GenerateEnlightenAlbedo";
static const char* s_generateEmissiveSituation = "GenerateEnlightenEmissive";

// ------------------------------------------------------------------------------------------------------
Tr2InteriorEnlightenSystem::Tr2InteriorEnlightenSystem( IRoot* lockobj ) :
	PARENTLOCK( m_statics ),
	m_parentCell( NULL ),
	m_isVisible( false ),
	m_drawBoundingBox( true ),
	m_updateInputLighting( true ),
	m_isInputLightingCacheDirty( true ),
	m_updateRadiosity( true ),
	m_debugRenderDusterLighting( false ),
	m_visualizeMethod( VM_NONE ),
	m_useTextureAlbedo( true ),
	m_albedoTextureState( TEXTURES_DIRTY ),
	m_enlightenQuality( DIRECTIONAL_IRRADIANCE ),
	m_enlightenQualityChanged( true ),
	m_enlightenTexturesDirty( true )
{
	// default size
	m_minBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	m_maxBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	m_solveData.state = SolveData::INITIALIZED;
	m_solveData.redSphericalOutput = NULL;
	m_solveData.greenSphericalOutput = NULL;
	m_solveData.blueSphericalOutput = NULL;
	m_solveData.irradianceOutput = NULL;
	m_solveData.directionalIrradianceOutput = NULL;
	m_solveData.bounceScale = -1.0f;
	m_solveData.outputScale = -1.0f;

	// if our list changes, we get a notification!
	m_statics.SetNotify( this );

	// Set the dirty flag to true
	m_isDirty = true;

	//////////////////////////////////////////////////////////////////////////
	m_inputLightingCache = NULL;

	m_bounceScale = 1.0f;
	m_irradianceScale = 1.0f;
	m_inputLightingCacheSize = 0;
	m_enlightenSystem.m_systemID = s_currentEnlightenSystemGuid++;
	m_enlightenSystem.SetRadNotificationTarget( this );

	m_variableStore.CreateInstance();

	m_variableStore->RegisterPlaceholderTextureVariable( "SHRedMap" );
	m_variableStore->RegisterPlaceholderTextureVariable( "SHGreenMap" );
	m_variableStore->RegisterPlaceholderTextureVariable( "SHBlueMap" );
	m_variableStore->RegisterPlaceholderTextureVariable( "IrradianceMap" );
	m_variableStore->RegisterPlaceholderTextureVariable( "DirectionalIrradianceMap" );
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorEnlightenSystem::~Tr2InteriorEnlightenSystem()
{
	m_enlightenMaterialCache.clear();
	ReleaseInputLightingCache();

	// Explicitly clear the parent pointers
	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->SetParentSystem( NULL );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::SetSystemInCellIdx( unsigned int idx )
{
	m_enlightenSystem.m_systemInCellIdx = idx;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns current quality of Enlighten textures.
// Return Value:
//   current quality of Enlighten textures
// ------------------------------------------------------------------------------------------------------
Tr2InteriorEnlightenSystem::EnlightenQuality Tr2InteriorEnlightenSystem::GetEnlightenQuality() const
{
	return m_enlightenQuality;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::ReleaseInputLightingCache( void )
{
	if( m_inputLightingCache )
	{
		Enlighten::DeleteInputLightingBuffer( m_inputLightingCache );
		m_inputLightingCache = NULL;

		CCP_STATS_ADD( triEnlightenObjectMemory, -static_cast<int>( m_inputLightingCacheSize ) );
		m_inputLightingCacheSize = 0;
		m_isInputLightingCacheDirty = true;
	}
	m_solveData.state = SolveData::INITIALIZED;
	CCP_DELETE[] m_solveData.redSphericalOutput;
	CCP_DELETE[] m_solveData.greenSphericalOutput;
	CCP_DELETE[] m_solveData.blueSphericalOutput;
	CCP_DELETE[] m_solveData.irradianceOutput;
	CCP_DELETE[] m_solveData.directionalIrradianceOutput;

	m_solveData.redSphericalOutput = NULL;
	m_solveData.greenSphericalOutput = NULL;
	m_solveData.blueSphericalOutput = NULL;
	m_solveData.irradianceOutput = NULL;
	m_solveData.directionalIrradianceOutput = NULL;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::AllocateInputLightingCache( void )
{
	if( m_enlightenSystem.GetEnlightenWorkspace() )
	{
		m_inputLightingCacheSize = Enlighten::CalcInputLightingBufferSize( m_enlightenSystem.GetEnlightenWorkspace() );
		void* dataBlock = GEO_ALIGNED_MALLOC( m_inputLightingCacheSize, 16 );
		m_inputLightingCache = Enlighten::CreateInputLightingBuffer( dataBlock, m_enlightenSystem.GetEnlightenWorkspace() );
		CCP_STATS_ADD( triEnlightenObjectMemory, static_cast<int>( m_inputLightingCacheSize ) );
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorEnlightenSystem::NeedToRebuildInputLightingCache( const PITr2InteriorLightVector& lightSources )
{
	// Just check the static changed flag on each light
	for( PITr2InteriorLightVector::const_iterator it = lightSources.begin(); it != lightSources.end(); ++it )
	{
		if( (*it)->StaticFlagChanged() )
		{
			return true;
		}
	}

	// No lights change their static status, so don't need to rebuild cache
	return false;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Adds a light to Enlighten input lighting buffer (the buffer could be a direct input to Enlighten
//   SolveTask or a lighting cache for static lights).
// Arguments:
//   light - light to add to lighting buffer
//   inputLightingCache - lighting buffer to add light to
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::AddLightToInputWorkspace( const ITr2InteriorLight* light, 
												   Enlighten::InputLightingBuffer* inputLightingCache )
{
	Matrix transformInv;
	D3DXMatrixInverse( &transformInv, NULL, &GetParentTransform() );
	if( light->AddToEnlightenSystem( m_enlightenSystem.GetDusterCache(),
									 transformInv ) )
	{
		CCP_STATS_ADD( enlightenInputLightingUpdates, 1 );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::SetParentCell( Tr2InteriorCell* parentCell )
{
	// if valid parent cell, just remember, otherwise "remove" this whole system from parent
	if( parentCell )
	{
		m_parentCell = parentCell;
		m_variableStore->SetParentVariableStore( &parentCell->GetVariableStore() );

		for( PTr2InteriorStaticVector::const_iterator it = m_statics.begin(); it != m_statics.end(); ++it )
		{
			(*it)->SetParentSystem( this );
		}
	}
	else
	{
		// remove all statics - and why would we want to do that?
		ClearStatics();
		// then clear the pointer
		m_parentCell = NULL;
		m_variableStore->SetParentVariableStore( &GlobalStore() );
	}
}

// ------------------------------------------------------------------------------------------------------
const Matrix& Tr2InteriorEnlightenSystem::GetParentTransform() const
{
	if( m_parentCell )
	{
		return m_parentCell->GetWorldTransform();
	}
	else
	{
		return Tr2Renderer::GetIdentityTransform();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns system's bounding box in parent cell coordinate space. 
// Arguments:
//   minBounds - (out) min bounds
//   maxBounds - (out) max bounds
// Return value:
//   true - If the system has valid bounding box
//   false - If the system does not have valid bounding box (and minBounds, maxBounds 
//           arguments are not assigned)
// --------------------------------------------------------------------------------------
bool Tr2InteriorEnlightenSystem::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds ) const
{
	if( m_minBounds.x < m_maxBounds.x )
	{
		minBounds = m_minBounds;
		maxBounds = m_maxBounds;
		return true;
	}
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Checks all the statics in the system to see if they are dirty.
// Return Value:
//   true, if any static in the system is dirty
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorEnlightenSystem::IsDirty( void )
{
	// If we know we're dirty, return true immediately
	if( m_isDirty )
	{
		return true;
	}

	// Check the dirty flag of all statics
	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end();
		 ++it )
	{
		// Check the dirty flag
		if( (*it)->IsDirty() )
		{
			m_albedoTextureState = TEXTURES_DIRTY;
			return true;
		}
	}

	// We're not dirty, none of the statics are either, so return false
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Resets the dirty flag on the system and all its contained statics.
// --------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::ResetDirtyFlag( void )
{
	m_isDirty = false;

	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end();
		 ++it )
	{
		(*it)->ResetDirtyFlag();
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::Update( Be::Time time )
{
	// time all statics in this cell
	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->Update( time );
	}

	RenderEnlightenMaterialTextures();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::RenderDebugInfo( TriLineSetPtr lines ) const
{
	// bounding box?
	if( m_drawBoundingBox )
	{
		lines->AddOrientedBox( GetParentTransform(), m_minBounds, m_maxBounds, 0x80ff0000 );
	}
	for( PTr2InteriorStaticVector::const_iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->RenderDebugInfo( lines );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders occlusion geometry of all statics in the system.
// --------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::RenderOcclusionGeometry() const
{
	for( PTr2InteriorStaticVector::const_iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		( *it )->RenderOcclusionGeometry();
	}
}


// -------------------------------------------------------------
// Description:
//   Builds Enlighten debug textures (naughty pixels, charts,
//   output density).
// -------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateEnlightenDebugTextures()
{
	if( !m_enlightenSystem.GetRadSystem() )
	{
		return;
	}

	unsigned debugTexture = DebugMaxCount;

	switch( m_visualizeMethod )
	{
	case VM_EN_OUTPUT_DENSITY: 
		debugTexture = DebugDensity; break;
			
	case VM_EN_NAUGHTY_PIXELS:
		debugTexture = DebugNaughtyPixels; break;
		
	case VM_EN_CHARTS:
	case VM_EN_TARGET_CHARTS:
		debugTexture = DebugChart; break;
		
	case VM_EN_ALBEDO:
		debugTexture = DebugAlbedo; break;
		
	case VM_EN_EMISSIVE:
		debugTexture = DebugEmissive; break;
		
	default:		
		m_variableStore->GetLocalVariable( "SHRedMap" )->SetValue( &m_enlightenSHRTexture );
		m_variableStore->GetLocalVariable( "SHGreenMap" )->SetValue( &m_enlightenSHGTexture );
		m_variableStore->GetLocalVariable( "SHBlueMap" )->SetValue( &m_enlightenSHBTexture );
		m_variableStore->GetLocalVariable( "IrradianceMap" )->SetValue( &m_enlightenIrradianceTexture );
		m_variableStore->GetLocalVariable( "DirectionalIrradianceMap" )->SetValue( &m_enlightenDirectionalIrradianceTexture );
		return;
	}

	if( m_enlightenDebugTexture[debugTexture].IsValid() || !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return;
	}

	int width = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
	int height = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;

	std::vector<unsigned char>  data8bpp;
	std::vector<unsigned short> data16bpp;
	bool float16 = true;

	switch( m_visualizeMethod )
	{
	case VM_EN_OUTPUT_DENSITY: 
		{
			data8bpp.resize( width * height * 4 );
			std::generate( data8bpp.begin(), data8bpp.end(), []{ return ( rand() & 0xff ) + 127; } );
			break;
		}
		
	case VM_EN_NAUGHTY_PIXELS:
		{
			data16bpp.resize( width * height * 4 );
			float16 = false;

			Enlighten::RadDebugColouringTask debugTask;
			debugTask.m_CoreSystem = m_enlightenSystem.GetRadSystem();

			const unsigned short bright( (unsigned short)((1<<16) * 0.7f) );
			const unsigned short dark( (unsigned short)((1<<16) * 0.3f) );

			debugTask.m_IrradianceOutput = &data16bpp[0];
			debugTask.m_OutputFormat = Enlighten::IRRADIANCE_FORMAT_16;

			for( int y = 0; y < height; ++y ) 
			{
				__int64 *row = (__int64*)(((char*)debugTask.m_IrradianceOutput) + y * width * 8);
				for( int x = 0; x < width; ++x ) 
				{
					unsigned short component = (x + y) % 2 == 0 ? dark : bright;
					unsigned short *pixel = (unsigned short*)(row+x);
					pixel[0] = component;
					pixel[1] = component;
					pixel[2] = component;
					pixel[3] = component;
				}
			}

			debugTask.m_OutputStride = width;
					
			// Colours to ouput to the debug texture
			debugTask.m_FailedProjectionColour = Geo::VConstruct( 1.0f, 0.0f, 0.0f, 1.0f);
			debugTask.m_DegenerateColour =		 Geo::VConstruct( 1.0f, 1.0f, 0.0f, 1.0f);
			debugTask.m_WingColour =			 Geo::VConstruct( 0.0f, 1.0f, 1.0f, 1.0f);

			Enlighten::DoDebugColouring( &debugTask );

			break;
		}

	case VM_EN_CHARTS:
	case VM_EN_TARGET_CHARTS:
		{
			data16bpp.resize( width * height * 4 );
					
			D3DXFLOAT16 *pixels =  reinterpret_cast<D3DXFLOAT16*>( &data16bpp[0] );
			if( height != this->GetChartHeight() || width != this->GetChartWidth() )
			{
				CCP_LOGERR( "Chart/output size mismatch: (%d x %d) vs (%d x %d)", 
					this->GetChartWidth(), this->GetChartHeight(),
					width, height );

				for( int y = 0; y < height; ++y )
				{
					for( int x = 0; x < width; ++x )
					{
						bool isCheck = (x+y) & 1;

						D3DXFLOAT16 *pixel = (D3DXFLOAT16*)(pixels + ( y * width + x) * 4 );
						pixel[0] = D3DXFLOAT16( isCheck ? 1.f : 0.f );
						pixel[1] = D3DXFLOAT16( isCheck ? 0.f : 1.f );
						pixel[2] = D3DXFLOAT16( 0.f );
						pixel[3] = D3DXFLOAT16( 1.f );
					}
				}
			}
			else
			{
				//stole these from GeoRadiosity, so should be easy to compare
				float r[5] = { 1.f, 0.8f, 0.6f, 0.4f, 0.2f };
				float g[7] = { 0.2f, 0.33f, 0.46f, 0.6f, 0.73f, 0.86f, 1.f };
				float b[3] = { 0.6f, 1.0f, 0.2f };
						
				for( int y = 0; y < height; ++y )
				{
					for( int x = 0; x < width; ++x )
					{
						bool isCheck = (x+y) & 1;
						float check = isCheck ? 0.7f : 0.3f;
						int chartIndex =  this->GetChartIndex( x, y );
						if( chartIndex >= 0 ) //valid chart
						{
							D3DXFLOAT16 *pixel = (D3DXFLOAT16*)(pixels + ( y * width + x) * 4 );
							pixel[0] = D3DXFLOAT16( r[chartIndex % 5] * check );
							pixel[1] = D3DXFLOAT16( g[chartIndex % 7] * check );
							pixel[2] = D3DXFLOAT16( b[chartIndex % 3] * check );
							pixel[3] = D3DXFLOAT16( 1.f );
						}
						else //not in a chart, use something distinctive
						{
							D3DXFLOAT16 *pixel = (D3DXFLOAT16*)(pixels + ( y * width + x) * 4 );
							pixel[0] = D3DXFLOAT16( 1.f );
							pixel[1] = D3DXFLOAT16( isCheck ? 1.f : 0.f );
							pixel[2] = D3DXFLOAT16( isCheck ? 0.f : 1.f );
							pixel[3] = D3DXFLOAT16( 1.f );
						}
					}
				}
			}
			
			break;
		}
	case VM_EN_ALBEDO:
		{
			m_enlightenSystem.GetAlbedoTextureSize( width, height );
			data8bpp.resize( width * height * 4 );
			
			uint8_t* row = &data8bpp[0];
			for( int j = 0; j < height; ++j )
			{
				for( int i = 0; i < width; ++i )
				{
					Geo::u32 pixel = m_enlightenSystem.GetAlbedoTexture()[i + j * width];
					reinterpret_cast<Geo::u32*>( row )[i] = ( ( pixel & 0xff ) << 16 ) |
						( pixel & 0xff00 ) |
						( ( pixel & 0xff0000 ) >> 16 ) |
						( pixel & 0xff000000 );
				}
				row += width * 4;
			}
			
			break;
		}
	case VM_EN_EMISSIVE:
		{
			m_enlightenSystem.GetAlbedoTextureSize( width, height );
			data8bpp.resize( width * height * 4 );

			uint8_t* row = &data8bpp[0];
			for( int j = 0; j < height; ++j )
			{
				for( int i = 0; i < width; ++i )
				{
					Geo::u32 pixel = m_enlightenSystem.GetEmissiveTexture()[i + j * width];
					reinterpret_cast<Geo::u32*>( row )[i] = ( ( pixel & 0xff ) << 16 ) |
						( pixel & 0xff00 ) |
						( ( pixel & 0xff0000 ) >> 16 ) |
						( pixel & 0xff000000 );
				}
				row += width * 4;
			}
			break;
		}
	}

	Tr2TextureAL &texture = m_enlightenDebugTexture[debugTexture];

	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( !data8bpp.empty() )
	{
		Tr2SubresourceData initialData = { &data8bpp[0], width * 4, width * height * 4, height };

		texture.Create2D( width, height, 1, PIXEL_FORMAT_B8G8R8A8_UNORM,
						  USAGE_IMMUTABLE, &initialData, renderContext );
	}
	else
	{
		Tr2SubresourceData initialData = { &data16bpp[0], width * 8, width * height * 8, height };

		texture.Create2D( width, height, 1, 
						  float16 ? PIXEL_FORMAT_R16G16B16A16_FLOAT 
								  : PIXEL_FORMAT_R16G16B16A16_UNORM,
						  USAGE_IMMUTABLE, &initialData, renderContext );
	}

	m_variableStore->GetLocalVariable( "SHRedMap"      )->SetValue( &texture );
	m_variableStore->GetLocalVariable( "SHGreenMap"    )->SetValue( &texture );
	m_variableStore->GetLocalVariable( "SHBlueMap"     )->SetValue( &texture );
	m_variableStore->GetLocalVariable( "IrradianceMap" )->SetValue( &texture );

	CCP_STATS_ADD( triEnlightenTextureMemory, width * height * ( data8bpp.empty() ? 8 : 4 ) );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::SetVisualizeMethod( VisualizeMethod method )
{
	m_visualizeMethod = method;
	UpdateEnlightenDebugTextures();

	// Pass down to statics
	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->SetVisualizeMethod( method );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateLightCount( const PITr2InteriorLightVector& lights )
{
	for( PTr2InteriorStaticVector::iterator staticIt = m_statics.begin(); staticIt != m_statics.end(); ++ staticIt )
	{
		Vector3 staticBBMin, staticBBMax;
		(*staticIt)->GetBoundingBox( staticBBMin, staticBBMax );
		Matrix transMat = (*staticIt)->GetWorldTransform();
		D3DXVec3TransformCoord( &staticBBMin, &staticBBMin, &transMat );
		D3DXVec3TransformCoord( &staticBBMax, &staticBBMax, &transMat );

		int lightCount = 0;

		for( PITr2InteriorLightVector::const_iterator lightIt = lights.begin(); lightIt != lights.end(); ++lightIt )
		{
			Vector3 lightBBMin, lightBBMax;
			(*lightIt)->GetBoundingBox( lightBBMin, lightBBMax );

			if( IntersectAxisAlignedBoxAxisAlignedBox( staticBBMin, staticBBMax, lightBBMin, lightBBMax ) )
			{
				++lightCount;
			}
		}

		(*staticIt)->SetTotalLightCount( lightCount );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( theList == &m_statics )
	{
		if( (event & BELIST_LOADING) == 0  )
		{
			switch( event & BELIST_EVENTMASK )
			{
			case BELIST_INSERTED:
				{
					m_isDirty = true;
					m_albedoTextureState = TEXTURES_DIRTY;

					Tr2InteriorStatic* object = NULL;
					if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorStatic>(), ( void** )&object ) )
					{
						object->SetParentSystem( this );
						object->BindLowLevelShaders();
						object->Unlock();
					}
				}
				break;
			case BELIST_REMOVED:
				{
					//	If an entry is being removed from the statics list, ensure that it has its parent system cleared
					Tr2InteriorStatic* object = NULL;
					if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorStatic>(), ( void** )&object ) )
					{
						object->SetParentSystem( NULL );
						object->Unlock();
					}

					RebuildBoundingBox();
					m_isDirty = true;
					m_albedoTextureState = TEXTURES_DIRTY;
				}
				break;
			case BELIST_SWAPPED:
			case BELIST_MOVED:
				//				rebuildUmbraCell();
				break;
			}
		}
	}
}

bool Tr2InteriorEnlightenSystem::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_enlightenQuality ) )
	{
		m_updateTaskInfo.WaitToComplete();
		m_updateTaskInfo.InvalidateResult();
		for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
		{
			(*it)->BindLowLevelShaders();
		}
		m_enlightenQualityChanged = true;
		return true;
	}

	m_enlightenSystem.OnModified( value );

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
	if( m_isVisible && m_parentCell )
	{
		m_parentCell->SetVisibility( true );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::ClearVisibility( void )
{
	m_isVisible = false;
	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->SetVisibility( false );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Enlarges system's bounding box to enclose the given bounding box. Propagates the
//   update to the parent cell.
// Arguments:
//   minBounds - min bounds of the box to enclose
//   maxBounds - max bounds of the box to enclose
// --------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateBoundingBox( const Vector3& minBounds, const Vector3& maxBounds )
{
	BoundingBoxUpdate( m_minBounds, m_maxBounds, minBounds, maxBounds );
	if( m_parentCell )
	{
		m_parentCell->UpdateBoundingBox( m_minBounds, m_maxBounds );
	}
	m_isDirty = true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::RebuildBoundingBox()
{
	Vector3 oldMinBounds = m_minBounds;
	Vector3 oldMaxBounds = m_maxBounds;

	// rest bbox
	m_minBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	m_maxBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	// add all sub-cells
	for( PTr2InteriorStaticVector::const_iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		Vector3 mn, mx;
		if( (*it)->GetBoundingBox( mn, mx ) )
		{
			BoundingBoxTransform( mn, mx, (*it)->GetTransform() );
			BoundingBoxUpdate( m_minBounds, m_maxBounds, mn, mx );
		}
	}

	if( m_parentCell && ( oldMinBounds != m_minBounds || oldMaxBounds != m_maxBounds ) )
	{
		m_parentCell->RebuildBoundingBox();
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a static object to the interior Enlighten system.  If the system has a valid
//   parent cell, the cell recomputes its Enlighten bounding box after the static is
//   added.  It is an error to add a NULL static, and a log message is printed if that
//   occurs
// Arguments:
//   interiorStatic - The static object to add to the system (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::AddStatic( Tr2InteriorStatic* interiorStatic )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Bail out early if the static is NULL
	if( !interiorStatic )
	{
		CCP_LOGERR( "Attempt to add a NULL static object to interior Enlighten system!" );
		return;
	}

	CCP_STATS_INC( wiscAddStaticCount );

	// put in our statics list
	m_statics.Insert( -1, interiorStatic->GetRawRoot() );
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a static object from the Enlighten system.  It is an error to remove a NULL
//   static, and a log message is printed if that occurs.
// Arguments:
//   interiorStatic - The static object to remove
// --------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::RemoveStatic( Tr2InteriorStatic* interiorStatic )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Bail out early if the static is NULL
	if( !interiorStatic )
	{
		CCP_LOGERR( "Attempt to remove a NULL static object from interior Enlighten system!" );
		return;
	}

	CCP_STATS_INC( wiscRemoveStaticCount );

	// find this one

	IRoot *key =  interiorStatic->GetRawRoot();
	ssize_t pos = m_statics.FindKey( key );

	if( pos == -1 )
	{
		CCP_LOGERR("Tr2InteriorEnlightenSystem::RemoveStatic() - interiorStatic %p not found in this system %s!", ( void* )key, m_name.c_str() );
		return;
	}

	m_statics.Remove( pos );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::ClearStatics()
{
	// clean-up all statics
	for( PTr2InteriorStaticVector::const_iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		(*it)->RemoveFromCell();
	}
	// clear vector
	while( m_statics.size() )
	{
		m_statics.Remove( 0 );
	}

	RebuildBoundingBox();

	m_isDirty = true;
	m_albedoTextureState = TEXTURES_DIRTY;
}

bool Tr2InteriorEnlightenSystem::Initialize()
{
	m_enlightenSystem.Initialize();

	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		CCP_STATS_INC( wiscAddStaticCount );

		(* it )->SetParentSystem( this );
	}

	m_isDirty = true;
	m_albedoTextureState = TEXTURES_DIRTY;

	return true;
}

void Tr2InteriorEnlightenSystem::ReleaseCachedData( BlueAsyncRes* p )
{
	m_updateTaskInfo.WaitToComplete();
	m_updateTaskInfo.InvalidateResult();

	m_enlightenMaterialCache.clear();
	for( unsigned i = 0; i != DebugMaxCount; ++i )
	{
		m_enlightenDebugTexture[i].Destroy();
	}
	
	for( unsigned i = 0; i != 2; ++i )
	{
		auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;
		if( rt.IsValid() )
		{
			int width, height;
			m_enlightenSystem.GetAlbedoTextureSize( width, height );
			rt.Destroy();
			CCP_STATS_ADD( triEnlightenTextureMemory, -int( width * height * s_albedoTextureSupersamplingScale * s_albedoTextureSupersamplingScale ) * 4 );
		}
	}
	
	ReleaseInputLightingCache();
}

void Tr2InteriorEnlightenSystem::RebuildCachedData( BlueAsyncRes* p )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if ( p->IsGood() )
	{
		m_enlightenMaterialCache.clear();
		ReleaseInputLightingCache();
		UpdateEnlightenDebugTextures();

		int width, height;
		if( m_enlightenSystem.GetAlbedoTextureSize( width, height ) && Tr2Renderer::IsResourceCreationAllowed() )
		{
			for( unsigned i = 0; i != 2; ++i )
			{
				auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;

				rt.Destroy();			
				CR_RETURN( rt.Create(	width  * s_albedoTextureSupersamplingScale, 
										height * s_albedoTextureSupersamplingScale, 
										1, 
										PIXEL_FORMAT_B8G8R8A8_UNORM, 
										renderContext ) );
				CCP_STATS_ADD( triEnlightenTextureMemory, width * height * s_albedoTextureSupersamplingScale * s_albedoTextureSupersamplingScale * 4 );
			}
		}
		m_albedoTextureState = TEXTURES_DIRTY;
	}
}
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::BuildEnlightenSystem( Enlighten::IPrecompute* pPrecompute, std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, std::vector< Enlighten::IPrecompSystemClustering* >& systemClusters, TriEnlightenProgressBar& prog )
{
	m_enlightenMaterialCache.clear();
	ReleaseInputLightingCache();
	for( unsigned i = 0; i != 2; ++i )
	{
		auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;
		if( rt.IsValid() )
		{
			int width, height;
			m_enlightenSystem.GetAlbedoTextureSize( width, height );
			rt.Destroy();
			CCP_STATS_ADD( triEnlightenTextureMemory, -int( width * height * s_albedoTextureSupersamplingScale * s_albedoTextureSupersamplingScale ) * 4 );
		}
	}

	m_enlightenSystem.BuildEnlightenSystem( pPrecompute, neighbours, systemClusters, prog );

	RebuildEnlightenTextures();

	int width, height;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( m_enlightenSystem.GetAlbedoTextureSize( width, height ) && Tr2Renderer::IsResourceCreationAllowed() )
	{
		for( unsigned i = 0; i != 2; ++i )
		{
			auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;
			rt.Destroy();
			CR_RETURN( rt.Create(	width  * s_albedoTextureSupersamplingScale, 
									height * s_albedoTextureSupersamplingScale, 
									1, 
									PIXEL_FORMAT_B8G8R8A8_UNORM,
									renderContext ) );
			CCP_STATS_ADD( triEnlightenTextureMemory, width * height * s_albedoTextureSupersamplingScale * s_albedoTextureSupersamplingScale * 4 );
		}
	}
	m_albedoTextureState = TEXTURES_DIRTY;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Deletes all temporary data used during Enlighten precompute.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::DeletePrecompData()
{
	//Before we delete the precomp data, save the mapping from texels to charts
	CaptureChartMapping();
	m_enlightenSystem.DeletePrecompData();
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Saves the mapping from lightmap texels to chart indices.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::CaptureChartMapping()
{
	if( !m_enlightenSystem.CaptureChartMapping( ) )
	{
		CCP_LOGWARN_CH( g_enlightenBuildChannel, "Failed to capture Enlighten chart mapping" );
	}
}
#endif
// ------------------------------------------------------------------------------------------------------
// Description:
//   Gets the chart index for a given texel
// ------------------------------------------------------------------------------------------------------
int Tr2InteriorEnlightenSystem::GetChartIndex( int x, int y ) const
{
	return m_enlightenSystem.GetChartIndex( x, y );
}

int Tr2InteriorEnlightenSystem::GetChartHeight() const
{
	return m_enlightenSystem.GetChartHeight();
}

int Tr2InteriorEnlightenSystem::GetChartWidth() const
{
	return m_enlightenSystem.GetChartWidth();
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
void Tr2InteriorEnlightenSystem::SaveEnlightenSystem()
{
	CCP_LOG( "Writing out system %i ", m_enlightenSystem.m_systemID );

	std::vector<const TriGeometryRes*> geometries;
	for( PTr2InteriorStaticVector::iterator i = m_statics.begin(); i != m_statics.end(); ++i )
	{
		TriGeometryRes* g = (*i)->GetGeometryResource();
		if( g )
		{
			geometries.push_back( g );
		}
	}

	if( !m_enlightenSystem.SaveEnlightenSystem( geometries ) )
	{
		CCP_LOGERR( "Could not write out the radiosity data" );		
	}
}

// -------------------------------------------------------------
// Description:
//   An ugly way to check if packed geometry stored in .gr2 file
//   is valid (it might be invalid if it was built with previous
//   version of Enlighten SDK). This function might provide false
//   positive answers, but is the only way to check this.
// Arguments:
//   packedGeometry - Enlighten packed geometry to test
// Return Value:
//   true If geometry seems valid
//   false If geometry is definitely invalid
// -------------------------------------------------------------
static bool TestPackedGeometry(Enlighten::IPrecompPackedGeometry* packedGeometry) 
{
	__try
	{
		packedGeometry->GetId();
	}
	__except( EXCEPTION_EXECUTE_HANDLER )
	{
		return false;
	}
	return true;
}

// -------------------------------------------------------------
// Description:
//   Fills a list of packed geometries for the system.
//   Called by m_enlightenSystem.PackEnlightenSystem.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   inputSystem - Enlighten input system
//   outputPixelSize - Enlighten map pixel size (in meters)
//   geometries (out) - List of packed geometries for the system
// Return Value:
//   true If successfully filled the list (always returns true)
//   false If the the Enlighten build should be aborted
// -------------------------------------------------------------
bool Tr2InteriorEnlightenSystem::GetEnlightenPackedGeometry( Enlighten::IPrecompute* precompute, 
													 Enlighten::IPrecompInputSystem* inputSystem, 
													 TriEnlightenProgressBar& prog, 
													 float outputPixelSize, 
													 std::vector<Enlighten::IPrecompPackedGeometry*>& geometries )
{
	m_packedGeometry.clear();

	// Build up our list of unique geometries and instances of those geometries within the system
	unsigned int instanceIDinSystem = 0;
	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it, ++instanceIDinSystem)
	{
		Matrix staticWorldTf = ((*it)->GetTransform());

		TriGeometryRes* geometry = NULL;
		geometry = (*it)->GetGeometryResource();
		if( !geometry )
		{
			continue;
		}

		Enlighten::IPrecompPackedGeometry* packedGeometry;
		PackedGeometryMap::iterator found = m_packedGeometry.find( geometry );
		if( found != m_packedGeometry.end() )
		{
			packedGeometry = found->second;
		}
		else
		{
			packedGeometry = geometry->GetEnlightenPackedGeometry( precompute, prog, m_enlightenSystem.m_enlightenPixelSize );
			m_packedGeometry[geometry] = packedGeometry;
		}
		if( !packedGeometry )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "Got NULL Packed Geometry back from TriGeometryRes file\"%S\". Either the granny file does not contain packed"
						" geometry or packed geometry was build with previous version of Enlighten", geometry->GetPath() );
			continue;
		}
		else
		{
			if( !TestPackedGeometry( packedGeometry ) )
			{
				CCP_LOGERR_CH( g_enlightenBuildChannel, "Packed geometry for %S was built with old Enlighten SDK. Full build is required.", geometry->GetPath() );
				return false;
			}
			geometries.push_back( packedGeometry );
		}

		if( prog.ShouldCancel() )
		{
			return false;
		}

		Enlighten::PrecompInputInstance inputInstance;
		// Convert to column major here
		inputInstance.m_Location = Geo::MConstruct(
			staticWorldTf._11, staticWorldTf._21, staticWorldTf._31, staticWorldTf._41,
			staticWorldTf._12, staticWorldTf._22, staticWorldTf._32, staticWorldTf._42,
			staticWorldTf._13, staticWorldTf._23, staticWorldTf._33, staticWorldTf._43,
			staticWorldTf._14, staticWorldTf._24, staticWorldTf._34, staticWorldTf._44);


		inputInstance.m_GeometryId = packedGeometry->GetId();
		CCP_LOG_CH( g_enlightenBuildChannel, "Adding GeometryID %u,%u to system", inputInstance.m_GeometryId.High(), inputInstance.m_GeometryId.Low() );

		// Set the instanceID so that the order can change in the list
		(*it)->SetInstanceInSystemIdx( instanceIDinSystem );
		inputInstance.m_Id = instanceIDinSystem;

		inputSystem->AddInstances( &inputInstance, &inputInstance + 1 );
	}

	return true;
}

void Tr2InteriorEnlightenSystem::PackEnlightenSystem( Tr2InteriorEnlightenSystemImpl::Quality quality, Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog, unsigned int systemInCellIdx )
{
	if( m_statics.empty() )
	{
		return;
	}

	if( m_enlightenSystem.PackEnlightenSystem( quality, pPrecompute, prog, systemInCellIdx, this ) )
	{
		unsigned int instanceIdx = 0;
		for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it)
		{
			TriGeometryRes* geometry = (*it)->GetGeometryResource();
			PackedGeometryMap::iterator found = m_packedGeometry.find( geometry );
			if( found != m_packedGeometry.end() && found->second != NULL )
			{
				const Enlighten::IPrecompPackedInstance** packedInstances = m_enlightenSystem.GetPackedSystem()->GetPackedInstances();
				Geo::GeoGuid currentInstanceGuid = packedInstances[instanceIdx]->GetGeometryId();
				CCP_LOG_CH( g_enlightenBuildChannel, "Setting uv transform info for Geometry ID %u,%u to instance %i (static: \"%s\")",
						 currentInstanceGuid.High(),
						 currentInstanceGuid.Low(),
						 instanceIdx, 
						 (*it)->GetName().c_str() );
				Geo::Geo2DTransform tf = packedInstances[instanceIdx]->GetUvTransform();

				Geo::GeoGuid currentStaticGuid = found->second->GetId();

				if( currentInstanceGuid != currentStaticGuid )
				{
					CCP_LOGERR_CH( g_enlightenBuildChannel, "Guids of the instance and the static do not match!?" );
				}

				(*it)->SetEnlightenInstanceTransform( tf.m_LinearTransform, tf.m_Translation );

				++instanceIdx;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns packed Enlighten system. Used during Enlighten precompute.
// Return Value:
//   Packed Enlighten system.
// ------------------------------------------------------------------------------------------------------
Enlighten::IPrecompPackedSystem* Tr2InteriorEnlightenSystem::GetPackedSystem()
{
	return m_enlightenSystem.GetPackedSystem();
}
#endif

void Tr2InteriorEnlightenSystem::UpdateEnlightenWorkspace( const PITr2InteriorLightVector& lightSources )
{
	if( !m_enlightenSystem.GetEnlightenWorkspace() )
	{
		return;
	}

	UpdateEnlightenMaterials();
	UpdateEnlightenInputLighting( lightSources );
	m_enlightenSystem.EndInputWorkspace();

	if( m_debugRenderDusterLighting )
	{
		extern ITr2DebugRendererPtr g_debugRenderer;
		if( g_debugRenderer )
		{
			for( int i = 0; i < Enlighten::GetNumberOfPointsInInputWorkspace( m_enlightenSystem.GetEnlightenWorkspace() ); ++i )
			{
				Enlighten::InputWorkspaceDebugPoint point;
				Enlighten::GetInputWorkspaceDebugPoint( m_enlightenSystem.GetEnlightenWorkspace(), &point, i );
				const Vector3 from = Vector3(&point.m_Position[0]);
				const Vector3 to = Vector3(&point.m_Position[0])+(0.025f*Vector3(&point.m_Normal[0]));
				const Color lineColor = Color(point.m_LightValue[0],point.m_LightValue[1],point.m_LightValue[2],1.0f);
				g_debugRenderer->DrawLine( from, to, lineColor );
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Copies Enlighten input texture (albedo or emissive) into Enlighten memory block. Performs blur for 
//   unfilled areas.
// Arguments:
//   texture - Source texture
//   enlightenData - Enlighten data memory block
//   width - Width of Enlighten texture
//   height - Height of Enlighten texture
// Return Value:
//   Packed Enlighten system.
// ------------------------------------------------------------------------------------------------------
static void UpdateEnlightenInputTexture( Tr2RenderTargetAL& renderTarget, Geo::u32* enlightenData, int width, int height )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	USE_MAIN_THREAD_RENDER_CONTEXT();

	void* pBits = nullptr;
	unsigned Pitch = 0;
	CR_RETURN( renderTarget.Lock( 0, nullptr, pBits, Pitch, renderContext ) );
	ON_BLOCK_EXIT( [&]{ renderTarget.Unlock( renderContext ); } );
	
	const byte* data = static_cast<const byte*>( pBits );

	Geo::u32* destination = enlightenData;

	for( int j = 0; j < height; ++j )
	{
		for( int i = 0; i < width; ++i )
		{
			unsigned int r = 0;
			unsigned int g = 0;
			unsigned int b = 0;
			unsigned int a = 0;
			unsigned int count = 0;
			for( int dj = 0; dj < s_albedoTextureSupersamplingScale; ++dj )
			{
				for( int di = 0; di < s_albedoTextureSupersamplingScale; ++di )
				{
					unsigned int pixel = *( reinterpret_cast<const Geo::u32*>( data + 
						Pitch * ( j * s_albedoTextureSupersamplingScale + dj ) ) + i * s_albedoTextureSupersamplingScale + di );
					if( pixel & 0xff000000 )
					{
						b += pixel & 0xff;
						g += ( pixel & 0xff00 ) >> 8;
						r += ( pixel & 0xff0000 ) >> 16;
						a += ( pixel & 0xff000000 ) >> 24;
						++count;
					}
				}
			}
			if( count > 0 )
			{
				*destination++ = ( a / count << 24 ) | ( r / count << 16 ) | ( g / count << 8 ) | b / count;
			}
			else
			{
				*destination++ = 0;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Prepares data for multithreaded Enlighten update. Fills Enlighten::RadTask structure and creates/
//   recreates Enlighten output arrays. Is called only for multithreaded Enlighten update.
// Arguments:
//   inputLighting - Vector of input lighting buffers for all neighbor Enlighten systems (including this 
//                   one).
//   environment - Environment cube lighting.
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorEnlightenSystem::PrepareUpdateEnlighten( std::vector<const Enlighten::InputLightingBuffer*>& inputLighting, 
												 const Enlighten::EmissiveEnvironment* environment,
												 Tr2IntEnlightenTaskManager &updateTask )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_enlightenSystem.GetEnlightenWorkspace() || !m_updateRadiosity )
	{
		return false;
	}

	TR2SHADERMODEL shaderModel = Tr2Renderer::GetShaderModel();

	switch( m_enlightenQuality )
	{
	case SPHERICAL_HARMONICS:
		if( !m_enlightenSHRTexture.IsValid() || !m_enlightenSHGTexture.IsValid() || !m_enlightenSHBTexture.IsValid() )
		{
			if( !RebuildEnlightenTextures() )
			{
				return false;
			}
		}
		break;
	case DIRECTIONAL_IRRADIANCE:
		if( !m_enlightenIrradianceTexture.IsValid() || !m_enlightenDirectionalIrradianceTexture.IsValid() )
		{
			if( !RebuildEnlightenTextures() )
			{
				return false;
			}
		}
		break;
	default:
		if( !m_enlightenIrradianceTexture.IsValid() )
		{
			if( !RebuildEnlightenTextures() )
			{
				return false;
			}
		}
		break;
	}

	int width, height;
	if( !m_enlightenSystem.GetAlbedoTextureSize( width, height ) )
	{
		return false;
	}

	// Download albedo textures
	if( m_albedoTextureState == TEXTURES_RENDERED && 
		( m_albedoTextureRenderTarget.IsValid() || m_emissiveTextureRenderTarget.IsValid() ) && 
		Tr2Renderer::IsResourceCreationAllowed() )
	{
		if( m_albedoTextureRenderTarget.IsValid() )
		{
			UpdateEnlightenInputTexture( m_albedoTextureRenderTarget, m_enlightenSystem.GetAlbedoTexture(), width, height );
			
			if( m_enlightenDebugTexture[DebugAlbedo].IsValid() )
			{
				int width, height;
				m_enlightenSystem.GetAlbedoTextureSize( width, height );
				m_enlightenDebugTexture[DebugAlbedo].Destroy();
				CCP_STATS_ADD( triEnlightenTextureMemory, -int( width * height ) * 4 );
			}
			if( m_visualizeMethod == VM_EN_ALBEDO )
			{
				UpdateEnlightenDebugTextures();
			}
		}

		if( m_emissiveTextureRenderTarget.IsValid() )
		{
			UpdateEnlightenInputTexture( m_emissiveTextureRenderTarget, m_enlightenSystem.GetEmissiveTexture(), width, height );
			
			if( m_enlightenDebugTexture[DebugEmissive].IsValid() )
			{
				int width, height;
				m_enlightenSystem.GetAlbedoTextureSize( width, height );
				m_enlightenDebugTexture[DebugEmissive].Destroy();
				CCP_STATS_ADD( triEnlightenTextureMemory, -int( width * height ) * 4 );
			}
			if( m_visualizeMethod == VM_EN_EMISSIVE )
			{
				UpdateEnlightenDebugTextures();
			}
		}

		m_albedoTextureState = TEXTURES_UPDATED;
	}

	// Enlighten ignores changes to irradiance or bounce scale
	// when doing temporal coherent solves, so whenever these values
	// change we need to disable temporal coherence for one frame
	m_solveData.enableTemporalCoherence = abs( m_solveData.bounceScale - m_bounceScale ) < 0.001f;
	m_solveData.enableTemporalCoherence &= abs( m_solveData.outputScale - m_irradianceScale ) < 0.001f;
	m_solveData.enableTemporalCoherence &= !m_enlightenQualityChanged;
	m_enlightenQualityChanged = false;
	
	m_solveData.inputWorkspace = m_enlightenSystem.GetEnlightenWorkspace();
	m_solveData.lightingBuffer = m_enlightenSystem.GetNextInputLightingBuffer();
	m_solveData.lightArray = m_enlightenSystem.GetDusterCache()->GetLights();
	m_solveData.numLights = m_enlightenSystem.GetDusterCache()->GetLightCount();

	m_solveData.dusters = m_enlightenSystem.GetDusterCache()->GetDusterMemory();
	m_solveData.lightingCache = m_inputLightingCache;

	m_solveData.bounceScale = m_bounceScale;
	m_solveData.previousLightingBuffer = m_enlightenSystem.GetCurrentInputLightingBuffer();
	if( m_useTextureAlbedo )
	{
		m_solveData.clusterAlbedoWorkspace = NULL;
	}
	else
	{
		m_solveData.clusterAlbedoWorkspace = m_enlightenSystem.GetAlbedoWorkspace();
	}
	m_solveData.albedoTextureData = m_enlightenSystem.GetAlbedoTexture();
	m_solveData.emissiveTextureData = m_enlightenSystem.GetEmissiveTexture();

	m_solveData.coreSystem = m_enlightenSystem.GetRadSystem();

	m_solveData.inputLighting = m_enlightenSystem.GetInputLightingList();
	Enlighten::PrepareInputLightingList(
		m_enlightenSystem.GetRadSystem(),
		&inputLighting[0],			// Input
		(Geo::s32)inputLighting.size(),
		m_solveData.inputLighting );
	m_solveData.environment = environment;
	m_solveData.outputScale = m_irradianceScale;
	switch( m_enlightenQuality )
	{
	case SPHERICAL_HARMONICS:
		if( !m_solveData.redSphericalOutput )
		{
			unsigned outputWidth = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
			unsigned outputHeight = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;
			m_solveData.redSphericalOutput = CCP_NEW( "Tr2InteriorEnlightenSystem/m_RedSphericalOutput" ) short[outputWidth * outputHeight * 4];
			memset( m_solveData.redSphericalOutput, 0, outputWidth * outputHeight * 4 * sizeof( short ) );
			m_solveData.greenSphericalOutput = CCP_NEW( "Tr2InteriorEnlightenSystem/m_GreenSphericalOutput" ) short[outputWidth * outputHeight * 4];
			memset( m_solveData.greenSphericalOutput, 0, outputWidth * outputHeight * 4 * sizeof( short ) );
			m_solveData.blueSphericalOutput = CCP_NEW( "Tr2InteriorEnlightenSystem/m_BlueSphericalOutput" ) short[outputWidth * outputHeight * 4];
			memset( m_solveData.blueSphericalOutput, 0, outputWidth * outputHeight * 4 * sizeof( short ) );
		}
		CCP_DELETE m_solveData.irradianceOutput;
		m_solveData.irradianceOutput = NULL;
		CCP_DELETE m_solveData.directionalIrradianceOutput;
		m_solveData.directionalIrradianceOutput = NULL;
		break;
	case DIRECTIONAL_IRRADIANCE:
		if( !m_solveData.directionalIrradianceOutput )
		{
			unsigned outputWidth = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
			unsigned outputHeight = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;
			m_solveData.directionalIrradianceOutput = CCP_NEW( "Tr2InteriorEnlightenSystem/m_DirectionalIrradianceOutput" ) char[outputWidth * outputHeight * 4];
			memset( m_solveData.directionalIrradianceOutput, 0, outputWidth * outputHeight * 4 );
		}
	default:
		if( !m_solveData.irradianceOutput )
		{
			unsigned outputWidth = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
			unsigned outputHeight = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;
			m_solveData.irradianceOutput = CCP_NEW( "Tr2InteriorEnlightenSystem/m_IrradianceOutput" ) short[outputWidth * outputHeight * 4];
			memset( m_solveData.irradianceOutput, 0, outputWidth * outputHeight * 4 * sizeof( short ) );
		}
		CCP_DELETE m_solveData.redSphericalOutput;
		CCP_DELETE m_solveData.greenSphericalOutput;
		CCP_DELETE m_solveData.blueSphericalOutput;

		m_solveData.redSphericalOutput = NULL;
		m_solveData.greenSphericalOutput = NULL;
		m_solveData.blueSphericalOutput = NULL;
	}

	m_solveData.bounceData = m_enlightenSystem.GetBounceData();

	m_solveData.state = SolveData::INITIALIZED;


	updateTask.AddSystem( this );
	m_updateTaskInfo.SetTaskManager( &updateTask );


	return true;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Helper function to fill a texture with data from array.
// Arguments:
//   texture - Texture to be filled.
//   data - Array of data to fill the texture with. Assumed to be the same pixel format as the texture.
//   pitch - Pitch for data array.
//   height - Height of matrix for data array.
// ------------------------------------------------------------------------------------------------------
static void CopyTextureFromMemory( Tr2TextureAL& texture, const void *data, const unsigned srcPitch, const unsigned srcHeight )
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	texture.UpdateSubresource( 0, 0, texture.GetWidth(), texture.GetHeight(), data, srcPitch, renderContext );
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Updates Enlighten textures with computed data from multithreaded Enlighten update. Is called only 
//   for multithreaded Enlighten update (when g_useThreadedEnlighten is set to true).
// Arguments:
//   task - Enlighten solve task structure. Filled in Tr2InteriorEnlightenSystem::PrepareUpdateEnlighten.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateEnlightenTextures()
{
	if( m_solveData.state != SolveData::SOLVED && !( m_enlightenTexturesDirty && m_solveData.state == SolveData::UNCHANGED ) )
	{
		m_updateTaskInfo.SetTaskManager( NULL );
		return; 
	}

	TR2SHADERMODEL shaderModel = Tr2Renderer::GetShaderModel();

	unsigned outputWidth = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
	unsigned outputHeight = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;

	switch( m_enlightenQuality )
	{
	case SPHERICAL_HARMONICS:
		if( !m_enlightenSHRTexture.IsValid() )
		{
			if( !RebuildEnlightenTextures() )
			{
				return;
			}
		}

		if( m_solveData.redSphericalOutput )
		{
			CopyTextureFromMemory( m_enlightenSHRTexture, m_solveData.redSphericalOutput, outputWidth * 8, outputHeight );
			CopyTextureFromMemory( m_enlightenSHGTexture, m_solveData.greenSphericalOutput, outputWidth * 8, outputHeight );
			CopyTextureFromMemory( m_enlightenSHBTexture, m_solveData.blueSphericalOutput, outputWidth * 8, outputHeight );
		}
		break;
	case DIRECTIONAL_IRRADIANCE:
		if( !m_enlightenIrradianceTexture.IsValid() || !m_enlightenDirectionalIrradianceTexture.IsValid() )
		{
			if( !RebuildEnlightenTextures() )
			{
				return;
			}
		}

		if( m_solveData.irradianceOutput )
		{
			CopyTextureFromMemory( m_enlightenIrradianceTexture, m_solveData.irradianceOutput, outputWidth * 8, outputHeight );
		}
		if( m_solveData.directionalIrradianceOutput )
		{
			CopyTextureFromMemory( m_enlightenDirectionalIrradianceTexture, m_solveData.directionalIrradianceOutput, outputWidth * 4, outputHeight );
		}
		break;
	default:
		if( !m_enlightenIrradianceTexture.IsValid() )
		{
			if( !RebuildEnlightenTextures() )
			{
				return;
			}
		}

		if( m_solveData.irradianceOutput )
		{
			CopyTextureFromMemory( m_enlightenIrradianceTexture, m_solveData.irradianceOutput, outputWidth * 8, outputHeight );
		}
	}

	if( m_visualizeMethod != VM_EN_OUTPUT_DENSITY && m_visualizeMethod != VM_EN_NAUGHTY_PIXELS &&
		m_visualizeMethod != VM_EN_CHARTS && m_visualizeMethod != VM_EN_TARGET_CHARTS && 
		m_visualizeMethod != VM_EN_ALBEDO && m_visualizeMethod != VM_EN_EMISSIVE )
	{
		m_variableStore->GetLocalVariable( "SHRedMap"					)->SetValue( &m_enlightenSHRTexture );
		m_variableStore->GetLocalVariable( "SHGreenMap"					)->SetValue( &m_enlightenSHGTexture );
		m_variableStore->GetLocalVariable( "SHBlueMap"					)->SetValue( &m_enlightenSHBTexture );
		m_variableStore->GetLocalVariable( "IrradianceMap"				)->SetValue( &m_enlightenIrradianceTexture );
		m_variableStore->GetLocalVariable( "DirectionalIrradianceMap"	)->SetValue( &m_enlightenDirectionalIrradianceTexture );
	}

	m_enlightenTexturesDirty = false;

	m_updateTaskInfo.SetTaskManager( NULL );
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Runs Enlighten lighting update. Is called from different threads parallel to the main thread.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateEnlightenLightingThreaded()
{
	CCP_STATS_ZONE(__FUNCTION__);

	if( m_solveData.state != SolveData::INITIALIZED )
	{
		return;
	}

	// Set the floating point control that Enlighten Requires
	unsigned int control_word;
	_controlfp_s( &control_word, _DN_FLUSH, _MCW_DN );
	ON_BLOCK_EXIT( &_controlfp_s, &control_word, control_word, _MCW_DN );

	Enlighten::WriteInputLightingTask writeTask;
	writeTask.m_InputWorkspace = m_solveData.inputWorkspace;
	writeTask.m_LightArray = m_solveData.lightArray;
	writeTask.m_LightingBuffer = m_solveData.lightingBuffer;
	writeTask.m_NumLights = m_solveData.numLights;

	int size = Enlighten::CalcRequiredWorkspaceMemory( &writeTask );
	void* workspace = GEO_ALIGNED_MALLOC( size, 16 );

	Geo::u32 ms;
	if( !Enlighten::DoWriteInputLightingTask( &writeTask, workspace, ms ) )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to run Enlighten DoWriteInputLightingTask:");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
		GEO_ALIGNED_FREE( workspace );
		return;
	}
	GEO_ALIGNED_FREE( workspace );

	Enlighten::AddCachedValuesToInputWorkspace( m_solveData.inputWorkspace, m_solveData.lightingBuffer, m_solveData.lightingCache );
	Enlighten::AddDusterValuesToInputWorkspace( m_solveData.inputWorkspace, m_solveData.lightingBuffer, m_solveData.dusters );

	Enlighten::EndInputLightingTask endTask;
	endTask.m_AlbedoTextureData = m_solveData.albedoTextureData;
	endTask.m_LightingBuffer = m_solveData.lightingBuffer;
	endTask.m_PreviousLightingBuffer = m_solveData.previousLightingBuffer;
	endTask.m_InputWorkspace = m_solveData.inputWorkspace;
	endTask.m_BounceData = m_solveData.bounceData;
	endTask.m_BounceScale = Geo::VBroadcast( m_solveData.bounceScale );
	endTask.m_ClusterAlbedoWorkspace = m_solveData.clusterAlbedoWorkspace;
	endTask.m_AlbedoTextureData = m_solveData.albedoTextureData;
	endTask.m_EmissiveTextureData = m_solveData.emissiveTextureData;
	endTask.m_EmissiveScale = Geo::VBroadcast( 1.f );

	if( !Enlighten::DoEndInputLightingTask( &endTask, ms ) )
	{
		CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to run Enlighten DoWriteInputLightingTask:");
		CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
		Enlighten::ClearErrorBuffer();
		return;
	}

	m_solveData.state = SolveData::LIGHTING_SOLVED;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Runs Enlighten update. Is called from different threads parallel to the main thread.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateEnlightenSolutionThreaded()
{
	CCP_STATS_ZONE(__FUNCTION__);

	if( m_solveData.state != SolveData::LIGHTING_SOLVED )
	{
		return;
	}

	// Set the floating point control that Enlighten Requires
	unsigned int control_word;
	_controlfp_s( &control_word, _DN_FLUSH, _MCW_DN );
	ON_BLOCK_EXIT( &_controlfp_s, &control_word, control_word, _MCW_DN );

	Geo::u32 ms, pixels;

	if( m_solveData.redSphericalOutput )
	{
		Enlighten::RadShTask task;
		task.m_CoreSystem = m_solveData.coreSystem;
		task.m_InputLighting = m_solveData.inputLighting;
		task.m_Environment = m_solveData.environment;
		task.m_OutputFormat = Enlighten::ENLIGHTEN_FORMAT_FP16;
		task.m_OutputScale = m_solveData.outputScale;
		task.m_OutputStride = m_solveData.coreSystem->m_MetaData.m_OutputWidth;
		task.m_RedSphericalOutput = m_solveData.redSphericalOutput;
		task.m_GreenSphericalOutput = m_solveData.greenSphericalOutput;
		task.m_BlueSphericalOutput = m_solveData.blueSphericalOutput;
		task.m_BounceData = m_solveData.bounceData;
		if( m_solveData.enableTemporalCoherence )
		{
			task.m_TemporalCoherenceThreshold = 0.01f;
		}

		int size = Enlighten::CalcRequiredShTaskWorkspaceMemory( m_solveData.coreSystem );
		void* workspace = GEO_ALIGNED_MALLOC( size, 16 );

		if( !Enlighten::SolveShTask( &task, workspace, ms, pixels ) )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to run Enlighten SolveShTask:");
			CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
			Enlighten::ClearErrorBuffer();
		}

		GEO_ALIGNED_FREE( workspace );
	}
	else
	{
		Enlighten::RadIrradianceTask task;
		task.m_CoreSystem = m_solveData.coreSystem;
		task.m_InputLighting = m_solveData.inputLighting;
		task.m_Environment = m_solveData.environment;
		task.m_OutputFormat = Enlighten::ENLIGHTEN_FORMAT_U16;
		task.m_OutputScale = m_solveData.outputScale;
		task.m_OutputStride = m_solveData.coreSystem->m_MetaData.m_OutputWidth;
		task.m_IrradianceOutput = m_solveData.irradianceOutput;
		task.m_DirectionalOutput = m_solveData.directionalIrradianceOutput;
		task.m_DirectionalOutputStride = m_solveData.coreSystem->m_MetaData.m_OutputWidth;
		task.m_BounceData = m_solveData.bounceData;
		if( m_solveData.enableTemporalCoherence )
		{
			task.m_TemporalCoherenceThreshold = 0.01f;
		}

		int size = Enlighten::CalcRequiredIrradianceTaskWorkspaceMemory( m_solveData.coreSystem );
		void* workspace = GEO_ALIGNED_MALLOC( size, 16 );

		if( !Enlighten::SolveIrradianceTask( &task, workspace, ms, pixels ) )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "Failed to run Enlighten SolveIrradianceTask:");
			CCP_LOGERR_CH( g_enlightenBuildChannel, Tr2Renderer::GetEnlightenErrorBuffer() );
			Enlighten::ClearErrorBuffer();
		}

		GEO_ALIGNED_FREE( workspace );
	}

	CCP_STATS_ADD( enlightenSystemUpdates, 1 );
	CCP_STATS_ADD( enlightenPixelUpdates, pixels );

	if( pixels > 0 )
	{
		m_solveData.state = SolveData::SOLVED;
	}
	else
	{
		m_solveData.state = SolveData::UNCHANGED;
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Renders statics into Enlighten albedo/emissive textures when needed.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::RenderEnlightenMaterialTextures()
{
	int width, height;
	if( !m_enlightenSystem.GetAlbedoTextureSize( width, height ) )
	{
		return;
	}

	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( m_albedoTextureState == TEXTURES_DIRTY && ( m_albedoTextureRenderTarget.IsValid() || m_emissiveTextureRenderTarget.IsValid() ) )
	{
		for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
		{
			if( !(*it)->CanRenderEnlightenMaterialTextures() )
			{
				return;
			}
		}

		Tr2PushPopDS pushPopDS( nullDS, renderContext );

		renderContext.m_esm.BeginManagedRendering();
		renderContext.m_esm.ApplyStandardStates( Tr2EffectStateManager::RM_FULLSCREEN );

		for( unsigned i = 0; i != 2; ++i )
		{
			auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;
			if( rt.IsValid() )
			{
				Tr2PushPopRT pushPopRT( rt, renderContext );
				
				CR( renderContext.Clear( CLEARFLAGS_TARGET, 0, 1.f, 0 ) );

				for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
				{
					(*it)->RenderEnlightenMaterialTexture( i ? s_generateEmissiveSituation : s_generateAlbedoSituation, renderContext );
				}
			}
		}
		
		renderContext.m_esm.EndManagedRendering();

		m_albedoTextureState = TEXTURES_RENDERED;
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Marks Enlighten albedo/emissive textures as dirty. They will be updated on the next frame. This 
//   function is supposed to be called from Python whenever areas/shaders/textures for static objects
//   are changed.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::UpdateEnlightenMaterialTextures()
{
	m_albedoTextureState = TEXTURES_DIRTY;
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
// Description:
//   Builds pre-clusters for Enlighten system. Used during Enlighten precompute.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   neighbours - vector of all neighboring packed systems
//   prog - progress log object
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::BuildEnlightenPreClustering( Enlighten::IPrecompute* pPrecompute, 
													  std::vector< Enlighten::IPrecompPackedSystem* >& neighbours, 
													  TriEnlightenProgressBar& prog )
{
	m_enlightenSystem.BuildPreClustering( pPrecompute, neighbours, prog );
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Builds clusters for Enlighten system. Used during Enlighten precompute.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   neighbours - vector of all pre-clusterings for neighboring systems
//   prog - progress log object
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorEnlightenSystem::BuildEnlightenClustering( Enlighten::IPrecompute* pPrecompute, 
												   std::vector< Enlighten::IPrecompSystemPreClustering* >& neighbours, 
												   TriEnlightenProgressBar& prog )
{
	m_enlightenSystem.BuildClustering( pPrecompute, neighbours, prog );
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns built pre-clusters for Enlighten system. Used during Enlighten precompute.
// Return Value:
//   Built pre-clusters for Enlighten system.
// ------------------------------------------------------------------------------------------------------
Enlighten::IPrecompSystemPreClustering* Tr2InteriorEnlightenSystem::GetSystemPreClustering()
{
	return m_enlightenSystem.GetSystemPreClustering();
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns built clusters for Enlighten system. Used during Enlighten precompute.
// Return Value:
//   Built clusters for Enlighten system.
// ------------------------------------------------------------------------------------------------------
Enlighten::IPrecompSystemClustering* Tr2InteriorEnlightenSystem::GetSystemClustering()
{
	return m_enlightenSystem.GetSystemClustering();
}
#endif

bool Tr2InteriorEnlightenSystem::RebuildEnlightenTextures()
{
	USE_MAIN_THREAD_RENDER_CONTEXT();

	if( !m_enlightenSystem.GetRadSystem() )
	{
		return false;
	}

	ReleaseEnlightenD3DResources();

	if( !Tr2Renderer::IsResourceCreationAllowed() )
	{
		return false;
	}

	// This is not a fully populated radLimits structure, it's just a convenient store
	unsigned outputWidth = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
	unsigned outputHeight = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;

	m_pixelDimensions.x = (float)outputWidth;
	m_pixelDimensions.y = (float)outputHeight;

	// Create the textures we'll need
	switch( m_enlightenQuality )
	{
	case SPHERICAL_HARMONICS:
		{
			CR_RETURN_VAL( m_enlightenSHRTexture.Create2D( outputWidth, outputHeight, 1, PIXEL_FORMAT_R16G16B16A16_FLOAT, 0, nullptr, renderContext ), false );
			
			// If we return due to a further Create2D calls failing, destroy the texture
			ScopeGuard SHRGuard = MakeGuard( [&]{ m_enlightenSHRTexture.Destroy(); } );

			CR_RETURN_VAL( m_enlightenSHGTexture.Create2D( outputWidth, outputHeight, 1, PIXEL_FORMAT_R16G16B16A16_FLOAT, 0, nullptr, renderContext ), false );
			ScopeGuard SHGGuard = MakeGuard( [&]{ m_enlightenSHGTexture.Destroy(); } );

			CR_RETURN_VAL( m_enlightenSHBTexture.Create2D( outputWidth, outputHeight, 1, PIXEL_FORMAT_R16G16B16A16_FLOAT, 0, nullptr, renderContext ), false );

			// All CreateTexture calls succeeded.
			SHRGuard.Dismiss();
			SHGGuard.Dismiss();

			CCP_STATS_ADD( triEnlightenTextureMemory, 3 * outputWidth * outputHeight * 8 );
		}
		break;
	case DIRECTIONAL_IRRADIANCE:
		{
			CR_RETURN_VAL( m_enlightenIrradianceTexture.Create2D( outputWidth, outputHeight, 1, PIXEL_FORMAT_R16G16B16A16_UNORM, 0, nullptr, renderContext ), false );

			ScopeGuard IrradianceGuard = MakeGuard( [&]{ m_enlightenIrradianceTexture.Destroy(); } );

			CR_RETURN_VAL( m_enlightenDirectionalIrradianceTexture.Create2D( outputWidth, outputHeight, 1, PIXEL_FORMAT_B8G8R8A8_UNORM, 0, nullptr, renderContext ), false );
			
			IrradianceGuard.Dismiss();

			CCP_STATS_ADD( triEnlightenTextureMemory, outputWidth * outputHeight * 12 );
		}
		break;
	default:
		CR_RETURN_VAL( m_enlightenIrradianceTexture.Create2D( outputWidth, outputHeight, 1, PIXEL_FORMAT_R16G16B16A16_FLOAT, 0, nullptr, renderContext ), false );
		CCP_STATS_ADD( triEnlightenTextureMemory, outputWidth * outputHeight * 8 );
	}
	m_enlightenTexturesDirty = true;
	return true;
}

void Tr2InteriorEnlightenSystem::UpdateEnlightenMaterials()
{
	// We make heavy use of caching to try and make sure that we don't call relatively expansive functions in the enlighten API
	// where we don't need to.
	if( m_useTextureAlbedo )
	{
		return;
	}

	if( !m_enlightenSystem.GetAlbedoWorkspace() )
	{
		CCP_LOGWARN("Radiosity system albedo workspace was not initialized!");
		return;
	}

	for( PTr2InteriorStaticVector::iterator it = m_statics.begin(); it != m_statics.end(); ++it )
	{
		Tr2InteriorStatic* staticObject = (*it);

		TriGeometryRes* geometry = staticObject->GetGeometryResource();
		if( !geometry )
		{
			continue;
		}

		const Geo::u64 instanceAlbedoMappingOffset = (Geo::u64( staticObject->GetInstanceInSystemIdx() ) << 48);
		const PTr2EnlightenAreaVector& enlightenAreas = staticObject->GetEnlightenAreas();

		for( PTr2EnlightenAreaVector::const_iterator areaIt = enlightenAreas.begin(); areaIt != enlightenAreas.end(); ++areaIt )
		{
			Tr2EnlightenArea* area = (*areaIt);
			MaterialCacheMap::iterator cacheLookup = m_enlightenMaterialCache.find( area );
			const bool isDirtyArea = area->IsDirty();
			const bool isNewArea = cacheLookup == m_enlightenMaterialCache.end();

			// Only update enlighten, if the area is not already in the cache, or it's flagged as dirty
			if( isDirtyArea || isNewArea )
			{
				m_enlightenSystem.GetDusterCache()->InvalidateAlbedo();

				// Grab the newest values from the area
				const Color& areaColor = area->GetAlbedoColor();
				Geo::v128 enlightenAlbedoColor = GEO_VCONSTRUCT( areaColor.r, areaColor.g, areaColor.b, 0.0f );

				const Color& areaEmissiveColor = area->GetEmissiveColor();
				Geo::v128 enlightenEmissiveColor = GEO_VCONSTRUCT( areaEmissiveColor.r, areaEmissiveColor.g, areaEmissiveColor.b, 0.0f );

				const bool isEmissive = area->IsEmissive();

				EnlightenMaterialProperties* cachedValues = NULL;
				// Create the cache value if we don't have one, or get the existing one
				if( isNewArea )
				{
					EnlightenMaterialProperties newEntry;

					// most of these entries will be set anyway
					newEntry.m_albedoColor = areaColor;
					newEntry.m_areaLightID = 0;
					newEntry.m_emissiveColor = areaEmissiveColor;
					newEntry.m_isEmissive = isEmissive;

					m_enlightenMaterialCache[area] = newEntry;
					cachedValues = &m_enlightenMaterialCache[area];
				}
				else
				{
					cachedValues = &((*cacheLookup).second);
				}

				// work out what we need to do
				const bool createAreaLight = (isNewArea && isEmissive) || (isDirtyArea && isEmissive && !cachedValues->m_isEmissive);
				const bool destroyAreaLight = (isDirtyArea && !isEmissive && cachedValues->m_isEmissive);
				const bool setAreaLightColor = createAreaLight || (isDirtyArea && (areaEmissiveColor != cachedValues->m_emissiveColor));
				const bool setAreaAlbedoColor = isNewArea || (isDirtyArea && (areaColor != cachedValues->m_albedoColor));

				const unsigned int areaStartID = area->GetIndex();
				
				// in case we're about to create a light, the current lightID will be the one we're about to assign
				unsigned int currentLightID;

				// otherwise, the current lightID should be the one from the cache
				if( !createAreaLight )
				{
					currentLightID = cachedValues->m_areaLightID;
				}

				// Each "area" can render multiple areas for a count > 1, so deal with that
				for( int areaIdx = 0; areaIdx < area->GetCount(); ++areaIdx )
				{
					const Geo::u64 materialID = instanceAlbedoMappingOffset + areaStartID + areaIdx;

					if( createAreaLight )
					{
						currentLightID = Enlighten::GetAreaLightId( m_enlightenSystem.GetAlbedoWorkspaceData(), materialID );

						// Set the color later, and only once
					}
					if( destroyAreaLight )
					{
						Geo::v128 blackColor = Geo::VBroadcast( 0.f );
						Enlighten::SetClusterAlbedoWorkspaceConstantColour(
							m_enlightenSystem.GetAlbedoWorkspaceData(), 
							materialID,
							blackColor );
					}
					if( setAreaAlbedoColor )
					{
						Enlighten::SetClusterAlbedoWorkspaceConstantColour(
							m_enlightenSystem.GetAlbedoWorkspaceData(), 
							materialID,
							enlightenAlbedoColor);
					}
				}

				// Don't set lightID 0 (black) to any other color!
				if( setAreaLightColor )
				{
					Enlighten::SetAreaLightColour(	
						m_enlightenSystem.GetAlbedoWorkspaceData(),
						currentLightID,
						enlightenEmissiveColor 
						);
				}

				area->ClearDirtyFlags();

				// Update the cache
				if( setAreaLightColor )
				{
					cachedValues->m_emissiveColor = areaEmissiveColor;
				}
				if( setAreaAlbedoColor )
				{
					cachedValues->m_albedoColor = areaColor;
				}
				if( destroyAreaLight )
				{
					cachedValues->m_areaLightID = 0;
					cachedValues->m_isEmissive = isEmissive;
				}

				if( createAreaLight && currentLightID != 0 )
				{
					cachedValues->m_areaLightID = currentLightID;
					cachedValues->m_isEmissive = isEmissive;
				}
			}
		}
	}
}

void Tr2InteriorEnlightenSystem::UpdateEnlightenInputLighting( const PITr2InteriorLightVector& lightSources )
{
	if( !m_updateInputLighting || !m_enlightenSystem.GetEnlightenWorkspace() )
	{
		return;
	}

	// Check if the input lighting cache is dirty
	if( m_isInputLightingCacheDirty || NeedToRebuildInputLightingCache( lightSources ) )
	{
		// If the cache hasn't been allocated, do that now
		if( !m_inputLightingCache )
		{
			AllocateInputLightingCache();

			if( !m_inputLightingCache )
			{
				return;
			}
		}

		m_enlightenSystem.GetDusterCache()->ClearLightData();

		// Clear the cache memory
		Enlighten::ClearInputWorkspace( m_enlightenSystem.GetEnlightenWorkspace(), m_inputLightingCache );

		// Now update the lighting cache
		for( PITr2InteriorLightVector::const_iterator it = lightSources.begin(); it != lightSources.end(); ++it )
		{
			if( (*it)->IsStatic() )
			{
				AddLightToInputWorkspace( *it, m_inputLightingCache );
			}
		}

		Enlighten::WriteInputLightingTask writeTask;
		writeTask.m_InputWorkspace = m_enlightenSystem.GetEnlightenWorkspace();
		writeTask.m_LightArray = m_enlightenSystem.GetDusterCache()->GetLights();
		writeTask.m_NumLights = m_enlightenSystem.GetDusterCache()->GetLightCount();
		writeTask.m_LightingBuffer = m_inputLightingCache;

		int size = Enlighten::CalcRequiredWorkspaceMemory( &writeTask );
		void* workspace = GEO_ALIGNED_MALLOC( size, 16 );

		Geo::u32 ms;
		if( !Enlighten::DoWriteInputLightingTask( &writeTask, workspace, ms ) )
		{
			CCP_LOGERR( "Failed to run Enlighten DoWriteInputLightingTask:");
			CCP_LOGERR( Tr2Renderer::GetEnlightenErrorBuffer() );
			Enlighten::ClearErrorBuffer();
		}
		else
		{
			Enlighten::AddDusterValuesToInputWorkspace( m_enlightenSystem.GetEnlightenWorkspace(), m_inputLightingCache, m_enlightenSystem.GetDusterCache()->GetDusterMemory() );

			// Finally, clear the dirty flag
			m_isInputLightingCacheDirty = false;
		}
		GEO_ALIGNED_FREE( workspace );
	}

	m_enlightenSystem.GetDusterCache()->ClearLightData();

	// Update dynamic input lighting
	for( PITr2InteriorLightVector::const_iterator it = lightSources.begin(); it != lightSources.end(); ++it )
	{
		if( !(*it)->IsStatic() )
		{
			AddLightToInputWorkspace( *it, m_enlightenSystem.GetNextInputLightingBuffer() );
		}
	}
}

void Tr2InteriorEnlightenSystem::ReleaseEnlightenD3DResources()
{

	if( !m_enlightenSystem.GetRadSystem() )
	{
		return;
	}

	int outputWidth = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputWidth;
	int outputHeight = m_enlightenSystem.GetRadSystem()->m_MetaData.m_OutputHeight;

	m_pixelDimensions.x = (float)outputWidth;
	m_pixelDimensions.y = (float)outputHeight;

	
	Tr2TextureAL* textures[] = { &m_enlightenIrradianceTexture, &m_enlightenSHRTexture, &m_enlightenSHBTexture, &m_enlightenSHGTexture, &m_enlightenDirectionalIrradianceTexture };

	for( auto it = std::begin( textures ); it != std::end( textures ); ++it )
	{
		auto& tex = **it;
		if( tex.IsValid() )
		{
			tex.Destroy();
			CCP_STATS_ADD( triEnlightenTextureMemory, -(int)( tex.GetWidth() * tex.GetHeight() * GetBytesPerPixel( tex.GetFormat() ) ) );;
		}
	}
	
	for( auto it = std::begin( m_enlightenDebugTexture ); it != std::end( m_enlightenDebugTexture ); ++it )
	{
		if( it->IsValid() )
		{
			it->Destroy();
			CCP_STATS_ADD( triEnlightenTextureMemory, -(int)(it->GetWidth() * it->GetHeight() * GetBytesPerPixel( it->GetFormat() )) );
		}
	}
}

void Tr2InteriorEnlightenSystem::ReleaseResources( TriStorage s )
{
	if( s & TRISTORAGE_VIDEOMEMORY )
	{
		ReleaseEnlightenD3DResources();
		for( unsigned i = 0; i != 2; ++i )
		{
			auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;		
			if( rt.IsValid() )
			{
				int width, height;
				m_enlightenSystem.GetAlbedoTextureSize( width, height );
				rt.Destroy();
				CCP_STATS_ADD( triEnlightenTextureMemory, -int( width * height * s_albedoTextureSupersamplingScale * s_albedoTextureSupersamplingScale ) * 4 );
			}
		}

		if( m_albedoTextureState == TEXTURES_RENDERED )
		{
			m_albedoTextureState = TEXTURES_DIRTY;
		}
	}
}

bool Tr2InteriorEnlightenSystem::OnPrepareResources()
{
	RebuildEnlightenTextures();
	UpdateEnlightenDebugTextures();

	int width, height;
	USE_MAIN_THREAD_RENDER_CONTEXT();
	if( m_enlightenSystem.GetAlbedoTextureSize( width, height ) )
	{
		for( unsigned i = 0; i != 2; ++i )
		{
			auto& rt = i ? m_emissiveTextureRenderTarget : m_albedoTextureRenderTarget;
			rt.Create(	width  * s_albedoTextureSupersamplingScale, 
						height * s_albedoTextureSupersamplingScale, 
						1, 
						PIXEL_FORMAT_B8G8R8A8_UNORM,
						renderContext );
									 
			CCP_STATS_ADD( triEnlightenTextureMemory, width * height * s_albedoTextureSupersamplingScale * s_albedoTextureSupersamplingScale * 4 );
		}
	}
	return true;
}

#endif
