#include "StdAfx.h"

#if INTERIORS_ENABLED

#include "Tr2InteriorCell.h"
#include "umbraTypes.h"
#include "Utilities/BoundingSphere.h"
#include "Resources/TriGeometryRes.h"
#include "Tr2InteriorProbeVisualizer.h"

#include "Tr2SHProbeRes.h"
#include "Tr2IntSkinnedObject.h"
#include "Tr2InteriorEnlightenSystem.h"
#include "Tr2InteriorPortalSocket.h"
#include "Tr2InteriorOccluder.h"
#include "Tr2InteriorBoundingBox.h"
#include "TriLineSet.h"

extern bool g_enlightenBreakOnErrors;
extern bool g_outputEnlightenDebugBuildInfo;

//////////////////////////////////////////////////////////////////////////
// Enlighten Counters
CCP_STATS_DECLARE( triEnlightenTextureMemory, "Trinity/Tr2InteriorCell/TextureMemory", false, CST_MEMORY, "The amount of texture memory allocated for Enlighten" );
CCP_STATS_DECLARE( triEnlightenObjectMemory, "Trinity/Tr2InteriorCell/ObjecteMemory", false, CST_MEMORY, "The amount of memory allocated for Enlighten objects" );
CCP_STATS_DECLARE( triEnlightenActiveSystems, "Trinity/Tr2InteriorCell/ActiveSystems", true, CST_COUNTER_LOW, "The number of Enlighten Systems updated this frame" );

extern CcpLogChannel_t g_enlightenBuildChannel;

BLUE_DEFINE_INTERFACE( ITr2InteriorDynamic );

// Define a workaround for crashes in Geo::VConstruct when it's not inlined (compiler error?)
#define GEO_VCONSTRUCT( x, y, z, w ) (_mm_setr_ps((x), (y), (z), (w)))
//#define GEO_VCONSTRUCT( x, y, z, w ) (Geo::VConstruct((x), (y), (z), (w)))

// ------------------------------------------------------------------------------------------------------
Tr2InteriorCell::Tr2InteriorCell( IRoot* lockobj ) :
	PARENTLOCK( m_systems ),
	PARENTLOCK( m_lights ),
	PARENTLOCK( m_dynamics ),
	PARENTLOCK( m_skinnedObjects ),
	PARENTLOCK( m_probeVolumes ),
	PARENTLOCK( m_occluders ),
	PARENTLOCK( m_boundingBoxes ),
	PARENTLOCK( m_defaultProbeVolume ),
	PARENTLOCK( m_portalSockets ),
	m_cellNeighbors(),
	m_enlightenEnvironmentWorld( NULL ),
	m_umbraCell( NULL ),
	m_isDirty( true ),
	m_position( 0.0f, 0.0f, 0.0f ),
	m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ),
	m_drawBoundingBox( false ),
	m_shScale( 1.0f ),
	m_drawSocketProbes( false ),
	m_drawSocketProbeCulling( false ),
	m_reflectionMapPath( "" )
{
	// default size
	m_minBounds = Vector3( 0.0f, 0.0f, 0.0f );
	m_maxBounds = Vector3( 0.0f, 0.0f, 0.0f );
	m_minBoxGutter = Vector3( 0.0f, 0.0f, 0.0f );
	m_maxBoxGutter = Vector3( 0.0f, 0.0f, 0.0f );
	m_boundingBoxReady = false;
	m_isUnbounded = false;

	D3DXMatrixIdentity( &m_worldTransform );

	// create umbra objects
	m_umbraCell = Umbra::Cell::create();
	m_umbraCell->set( Umbra::Cell::REPORT_IMMEDIATELY, true );

	// if our list changes, we get a notification!
	m_systems.SetNotify( this );
	m_shProbeResPath = "";

	// Initialize visibility depth
	m_isVisible = false;
	m_cellDepth = -1;

	m_portalSocketSamples.m_volumeBox = NULL;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	m_portalSocketSamples.m_precomputedProbeSet = NULL;
#endif
	m_portalSocketSamples.m_debugProbeLighting = NULL;

	m_socketSystem.m_systemInCellIdx = -1;
	m_socketSystem.SetRadNotificationTarget( this );
	m_socketOutputBuffer = NULL;

	m_portalSockets.SetNotify( this );
	m_probeVolumes.SetNotify( this );
	m_occluders.SetNotify( this );
	m_boundingBoxes.SetNotify( this );

	m_enlightenEnvironment = GEO_NEW( Enlighten::EmissiveEnvironment );

	m_variableStore.CreateInstance();
	m_variableStore->RegisterVariable( "ReflectionMap", (TriTextureRes*)NULL );

	m_enlightenEnvironmentWorld = GEO_NEW_ARRAY( Geo::v128, 24 );
	memset( m_enlightenEnvironmentWorld, 0, sizeof( Geo::v128 ) * 24 );
}

// ------------------------------------------------------------------------------------------------------
Tr2InteriorCell::~Tr2InteriorCell()
{
	// Release the Enlighten light probe output structure
	DeleteSampleVolumes();

	{
		using namespace Enlighten;
		GEO_DELETE( EmissiveEnvironment, m_enlightenEnvironment );
	}

	if( m_shProbeResource )
	{
		m_shProbeResource->RemoveNotifyTarget( this );
		m_shProbeResource.Unlock();
	}

	if( m_reflectionMapRes )
	{
		m_reflectionMapRes->RemoveNotifyTarget( this );
	}

	if( m_umbraCell )
	{
		m_umbraCell->release();
	}

	// Clear out the cell neighbors
	m_cellNeighbors.clear();

	// Clear out the lights
	m_lights.Clear();

	// Clear out the dynamics
	m_dynamics.Clear();

	// Clear out the skinned objects
	m_skinnedObjects.Clear();

	// Clear out the bounding boxes
	m_boundingBoxes.Clear();

	// Clear out the occluders
	for( PTr2InteriorOccluderVector::iterator it = m_occluders.begin(); it != m_occluders.end(); ++it )
	{
		(*it)->SetParentCell( NULL );
	}
	m_occluders.Clear();

	// Clear out the probe volumes
	for( PTr2InteriorProbeVolumeVector::iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it )
	{
		(*it)->SetParentCell( NULL );
	}
	m_probeVolumes.Clear();

	// Explicitly clear the parent pointers
	for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->SetParentCell( NULL );
	}

	for( PTr2InteriorPortalSocketVector::iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
	{
		(*it)->AddToCell( NULL );
	}

	GEO_DELETE_ARRAY( Geo::v128, m_enlightenEnvironmentWorld );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::Initialize()
{
	// position & orientation modify: calc new transform matrix...
	D3DXMatrixTransformation( &m_worldTransform, NULL, NULL, NULL, NULL, &m_rotation, &m_position );
 	m_umbraCell->setCellToWorldMatrix( AS_UMBRA_MATRIX( m_worldTransform ) );

	SetSHProbeResource();
	SetReflectionMapPath();

	m_socketSystem.Initialize();

	for( PTr2InteriorPortalSocketVector::iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
	{
		(*it)->AddToCell( this );
	}
	for( PTr2InteriorProbeVolumeVector::iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it )
	{
		(*it)->SetParentCell( this );
	}
	for( PTr2InteriorOccluderVector::iterator it = m_occluders.begin(); it != m_occluders.end(); ++it)
	{
		(*it)->SetParentCell( this );
	}
	for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); it != m_systems.end(); ++it)
	{
		(*it)->RebuildBoundingBox();
		(*it)->SetParentCell( this );
		m_isDirty = true;
	}

	RebuildInternalData();
	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetReflectionMapPath()
// ------------------------------------------------------------------------------------------------------
{
	if( m_reflectionMapRes )
	{
		m_reflectionMapRes->RemoveNotifyTarget( this );
		m_reflectionMapRes.Unlock();
	}

	BeResMan->GetResource( m_reflectionMapPath.c_str(), "", m_reflectionMapRes );

	if( m_reflectionMapRes )
	{
		m_reflectionMapRes->AddNotifyTarget( this );
		m_reflectionMapRes->SetName( "Tr2InteriorCell ReflectionMap" );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Calculates 2x2x2 cubemap values by "rotating" the original 2x2x2 cubemap by the 
//   given matrix. The function does an approximation of this rotation by projecting
//   rotated texels onto a convex hull of all original cubemap texel centers.
// Arguments:
//   cube - Original cubemap values (array of 24 colors)
//   transform - Rotation transform (from resulting rotated space to the space where
//               the original cubemap is specified)
//   result - New rotated cubemap (output array of 24 colors)
// --------------------------------------------------------------------------------------
static void RotateEnvironmentCube( const Geo::v128* cube, const Matrix& transform, Geo::v128* result )
{
	// Centers of cubemap texels in 3D space
	static const Vector3 pixelCenters[] = {
		Vector3( 2.f, 1.f, 1.f ),		// 0
		Vector3( 2.f, 1.f, -1.f ),
		Vector3( 2.f, -1.f, 1.f ),
		Vector3( 2.f, -1.f, -1.f ),

		Vector3( -2.f, 1.f, -1.f ),		// 4
		Vector3( -2.f, 1.f, 1.f ),
		Vector3( -2.f, -1.f, -1.f ),
		Vector3( -2.f, -1.f, 1.f ),

		Vector3( -1.f, 2.f, 1.f ),		// 8
		Vector3( 1.f, 2.f, 1.f ),
		Vector3( -1.f, 2.f, -1.f ),
		Vector3( 1.f, 2.f, -1.f ),

		Vector3( -1.f, -2.f, 1.f ),		// 12
		Vector3( 1.f, -2.f, 1.f ),
		Vector3( -1.f, -2.f, -1.f ),
		Vector3( 1.f, -2.f, -1.f ),

		Vector3( -1.f, 1.f, 2.f ),		// 16
		Vector3( 1.f, 1.f, 2.f ),
		Vector3( -1.f, -1.f, 2.f ),
		Vector3( 1.f, -1.f, 2.f ),

		Vector3( 1.f, 1.f, -2.f ),		// 20
		Vector3( -1.f, 1.f, -2.f ),
		Vector3( 1.f, -1.f, -2.f ),
		Vector3( -1.f, -1.f, -2.f ),
	};

	// Triangle indexes for a mesh connecting closes cubemap texel centers
	int triangles[] = {
		0, 1, 2,
		3, 2, 1,

		4, 5, 6,
		7, 6, 5,

		8, 9, 10, 
		11, 10, 9,

		12, 13, 14,
		15, 14, 13,

		16, 17, 18, 
		19, 18, 17,

		20, 21, 22,
		23, 22, 21,


		20, 9, 21, 
		8, 21, 9, 

		8, 10, 4, 
		5, 4, 10, 

		1, 0, 9, 
		11, 9, 0, 

		11, 17, 10, 
		16, 10, 17, 

		4, 6, 21, 
		23, 21, 6, 

		16, 18, 5, 
		7, 5, 18, 

		0, 2, 17, 
		19, 17, 2, 

		20, 22, 1, 
		3, 1, 22, 

		19, 13, 18, 
		12, 18, 13, 

		12, 14, 7, 
		6, 7, 14, 

		2, 3, 13, 
		15, 13, 3, 

		15, 22, 14, 
		23, 14, 22, 

		0, 17, 11,
		1, 9, 20,
		2, 13, 19,
		3, 22, 15,
		5, 10, 16,
		7, 18, 12,
		6, 14, 23,
		4, 21, 8,
	};

	Matrix transformInv;
	D3DXMatrixInverse( &transformInv, NULL, &transform );
	D3DXMatrixTranspose( &transformInv, &transformInv );

	Vector3 pixelTransformed[24];
	for( int i=0; i<24; ++i ) 
	{
		D3DXVec3TransformNormal( &pixelTransformed[i], &pixelCenters[i], &transformInv );
		D3DXVec3Normalize( &pixelTransformed[i], &pixelTransformed[i] );
	}

	// For each rotated texel find a triangle that intersects a ray from
	// origin to rotated texel center. 
	for( int i = 0; i < 24; ++i )
	{
		bool found = false;
		for( int j = 0; j < sizeof( triangles ) / sizeof( unsigned ); j += 3 )
		{
			Vector3 a = pixelCenters[triangles[j + 0]];
			Vector3 b = pixelCenters[triangles[j + 1]] - a;
			Vector3 c = pixelCenters[triangles[j + 2]] - a;
			Vector3 n = pixelTransformed[i];

			Matrix fromTriangle(
				b.x, b.y, b.z, 0.f,
				c.x, c.y, c.z, 0.f,
				n.x, n.y, n.z, 0.f,
				a.x, a.y, a.z, 1.f );
			Matrix toTriangle;
			D3DXMatrixInverse( &toTriangle, NULL, &fromTriangle );

			Vector3 point( 0.f, 0.f, 0.f );
			D3DXVec3TransformCoord( &point, &point, &toTriangle );

			float s = point.x;
			float t = point.y;
			if( s >= -0.001f && t >= -0.001f && s + t <= 1.001f && point.z < 0.f )
			{
				result[i] = XMVectorAdd( 
						XMVectorAdd(
							XMVectorScale( cube[triangles[j + 0]], ( 1.f - s ) * ( 1.f - t ) ),
							XMVectorScale( cube[triangles[j + 1]], s * ( 1.f - t ) ) ),
						XMVectorScale( cube[triangles[j + 2]], (1.f - s ) * t ) );
				found = true;
				break;
			}
		}
		if( !found )
		{
			// This should never happen...
			result[i] = XMVectorReplicate( 100.f ); 
		}

	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::OnModified( Be::Var* value )
{
	// Update the world transform
	if( IsMatch( value, m_position ) || 
		IsMatch( value, m_rotation ) )
	{
		// Calculate new world-space transformation matrix
		D3DXMatrixTransformation( &m_worldTransform, NULL, NULL, NULL, NULL, &m_rotation, &m_position );

		m_umbraCell->setCellToWorldMatrix( AS_UMBRA_MATRIX( m_worldTransform ) );

		for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			( *it )->InvalidateLightCache();
		}

		if( IsMatch( value, m_rotation ) )
		{
			Geo::v128 cube[24];
			RotateEnvironmentCube( m_enlightenEnvironmentWorld, m_worldTransform, cube );
			m_enlightenEnvironment->SetValues( cube );
		}

		m_isDirty = true;
	}
	else if( value == ( Be::Var* )&m_reflectionMapPath )
	{
		SetReflectionMapPath();
	}
	else if( value == ( Be::Var* )&m_shProbeResPath )
	{
		SetSHProbeResource();
	}
	else if( value == (Be::Var*)&m_isUnbounded )
	{
		m_isDirty = true;
	}
	else
	{
		m_socketSystem.OnModified( value );
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
	if( (event & BELIST_LOADING) == 0  )
	{
		switch( event & BELIST_EVENTMASK )
		{
		case BELIST_INSERTED:
			if( theList == &m_portalSockets && value )
			{
				Tr2InteriorPortalSocket* socket = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorPortalSocket>(), (void**)&socket ) )
				{
					socket->AddToCell( this );
					socket->Unlock();
				}
			}
			else if( theList == &m_probeVolumes && value )
			{
				Tr2InteriorProbeVolume* volume = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorProbeVolume>(), (void**)&volume ) )
				{
					volume->SetParentCell( this );
					volume->Unlock();
				}
			}
			else if( theList == &m_occluders && value )
			{
				Tr2InteriorOccluder* occluder = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorOccluder>(), (void**)&occluder ) )
				{
					occluder->SetParentCell( this );
					occluder->Unlock();
				}
			}
			else if( theList == &m_systems && value )
			{
				Tr2InteriorEnlightenSystem* system = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorEnlightenSystem>(), (void**)&system ) )
				{
					// tell the new system about it's parent
					system->SetParentCell( this );

					// rebuild cell
					RebuildInternalData();
					m_isDirty = true;

					system->Unlock();
				}
			}
			break;
		case BELIST_REMOVED:
			if( theList == &m_portalSockets && value )
			{
				Tr2InteriorPortalSocket* socket = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorPortalSocket>(), (void**)&socket ) )
				{
					socket->AddToCell( NULL );
					socket->Unlock();
				}
			}
			else if( theList == &m_occluders && value )
			{
				Tr2InteriorOccluder* occluder = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorOccluder>(), (void**)&occluder ) )
				{
					occluder->SetParentCell( NULL );
					occluder->Unlock();
				}
			}
			else if( theList == &m_probeVolumes && value )
			{
				Tr2InteriorProbeVolume* volume = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorProbeVolume>(), (void**)&volume ) )
				{
					volume->SetParentCell( NULL );
					volume->Unlock();
				}
			}
			else if( theList == &m_systems && value )
			{
				Tr2InteriorEnlightenSystem* system = NULL;
				if( value->QueryInterface( BlueInterfaceIID<Tr2InteriorEnlightenSystem>(), (void**)&system ) )
				{
					// tell the new system about it's parent
					system->SetParentCell( NULL );

					// rebuild cell
					RebuildInternalData();
					m_isDirty = true;

					system->Unlock();
				}
			}
			break;
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::ReleaseCachedData( BlueAsyncRes* p )
{
	if( p == m_shProbeResource )
	{
		DeleteSampleVolumes();
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RebuildCachedData( BlueAsyncRes* p )
{
	if ( p->IsGood() && p == m_shProbeResource )
	{
		RebuildInternalData();

		DeleteSampleVolumes();

		Vector3 center = (m_minBounds + m_maxBounds) / 2.0f;
		Vector3 scale = m_maxBounds - m_minBounds;
		Matrix transform(
			scale.x, 0.0f, 0.0f, 0.0f,
			0.0f, scale.y, 0.0f, 0.0f,
			0.0f, 0.0f, scale.z, 0.0f,
			center.x, center.y, center.z, 1.0f );

		CreateSampleVolume( 
			m_shProbeResource->GetXResolution(),
			m_shProbeResource->GetYResolution(),
			m_shProbeResource->GetZResolution(),
			transform,
			m_shProbeResource->GetSHLightProbes() );

		for (size_t i = 0; i < m_shProbeResource->GetAdditionalProbeSetCount(); i++)
		{
			Enlighten::RadProbeSetCore *set;
			int resX, resY, resZ;
			Matrix transform;
			if (!m_shProbeResource->GetAdditionalProbeSet( i, set, resX, resY, resZ, transform ))
			{
				CCP_LOGERR( "Tr2InteriorCell::RebuildCachedData. Incorrect number of probe volumes in the file\n" );	
				break;
			}
			if( resX == 0 || resY == 0 || resZ == 0)
			{
				break;
			}

			CreateSampleVolume( resX, resY, resZ, transform, set );
		}
		if( m_shProbeResource->GetAdditionalProbeSetCount() > 0 )
		{
			Enlighten::RadProbeSetCore *set;
			int resX, resY, resZ;
			Matrix transform;
			if( m_shProbeResource->GetAdditionalProbeSet( m_shProbeResource->GetAdditionalProbeSetCount() - 1, set, resX, resY, resZ, transform ) )
			{
				if( resX == 0 || resY == 0 || resZ == 0)
				{
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
					m_portalSocketSamples.m_precomputedProbeSet = NULL;
#endif
					m_portalSocketSamples.m_debugProbeLighting = NULL;
					CreateProbeTask( m_portalSocketSamples, set, set->m_MetaData.m_NumProbes );

					int index = 0;
					for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it, ++index )
					{
						if( index < set->m_MetaData.m_NumProbes )
						{
							(*it)->SetHasValidProbe( true );

							Geo::v128 position;
							(*it)->GetInputProbePosition( position );
							m_portalSocketSamples.m_samples.Push( position );
							m_portalSocketIndexes[*it] = m_portalSocketSamples.m_samples.GetSize() - 1;
						}
						else
						{
							(*it)->SetHasValidProbe( false );
						}
					}
					for( int j = 0; j < m_portalSocketSamples.m_task.m_NumIndicesToSolve; ++j )
					{
						m_portalSocketSamples.m_task.m_IndicesToSolve[j] = j;
					}
				}
			}
			else
			{
				CCP_LOGERR( "Tr2InteriorCell::RebuildCachedData. Incorrect number of probe volumes in the file\n" );	
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Returns true if the cell is dirty or any of its systems are dirty.  Returns false
//   otherwise.
// Return Value:
//   true, if the cell or any of its systems have changed
//   false, otherwise
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsDirty( void )
{
	// Return true if the cell is dirty
	if( m_isDirty )
	{
		return true;
	}

	// Now check our systems
	for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin();
		 it != m_systems.end(); ++it )
	{
		if( (*it)->IsDirty() )
		{
			return true;
		}
	}

	// Cell isn't dirty, none of the systems are either, so return false
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Resets the dirty flag on the cell and all of the contained systems.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::ResetDirtyFlag( void )
{
	m_isDirty = false;

	for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin();
		it != m_systems.end(); ++it )
	{
		(*it)->ResetDirtyFlag();
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::GetBoundingBox( Vector3& minBounds, Vector3& maxBounds )
{
	minBounds = m_minBounds + m_minBoxGutter;
	maxBounds = m_maxBounds + m_maxBoxGutter;

	return m_boundingBoxReady;
}

// --------------------------------------------------------------------------------------
void Tr2InteriorCell::UpdateBoundingBox( void )
{
	if( !m_boundingBoxReady )
	{
		RebuildBoundingBox();
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsBoundingBoxReady( void ) const
{
	return m_boundingBoxReady;
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsUnbounded( void ) const
{
	return m_isUnbounded;
}

void Tr2InteriorCell::UpdateEnlightenWorkspaceList( int cutoffDepth )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Clear workspaces from last frame
	m_inputLightingBuffers.clear();

	// Update systems and gather workspaces from systems
	{
		for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			if( (cutoffDepth == -1) || (m_cellDepth <= cutoffDepth) )
			{
				Enlighten::InputLightingBuffer* inputLighting = (*it)->GetEnlightenInputLightingBuffer();
				if( inputLighting != NULL )
				{
					m_inputLightingBuffers.push_back( inputLighting );
				}
			}
		}

		if( m_socketSystem.GetCurrentInputLightingBuffer() )
		{
			m_inputLightingBuffers.push_back( m_socketSystem.GetCurrentInputLightingBuffer() );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::UpdateEnlightenWorkspace( int cutoffDepth )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Clear workspaces from last frame
	m_inputLightingBuffers.clear();

	// Update systems and gather workspaces from systems
	{
		for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			(*it)->ClearVisibility();
			if( (cutoffDepth == -1) || (m_cellDepth <= cutoffDepth) )
			{
				(*it)->UpdateEnlightenWorkspace( m_lights );
				Enlighten::InputLightingBuffer* inputLighting = (*it)->GetEnlightenInputLightingBuffer();
				if( inputLighting != NULL )
				{
					m_inputLightingBuffers.push_back( inputLighting );
				}
			}
		}
	}

	if( m_socketSystem.GetEnlightenWorkspace() )
	{
		for( PTr2InteriorPortalSocketVector::iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
		{
			Color color = TriLinearToGamma( (*it)->GetEmissiveColor() );
			color.a = 1.f;

			Vector2 offset = (*it)->GetTexturePixelPosition();

			int width, height;
			m_socketSystem.GetAlbedoTextureSize( width, height );

			m_socketSystem.GetEmissiveTexture()[ int( offset.x * ( width - 1 ) ) + int( offset.y * ( height - 1 ) ) * width] = color;
		}

		m_socketSystem.EndInputWorkspace();

		m_inputLightingBuffers.push_back( m_socketSystem.GetCurrentInputLightingBuffer() );
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Sets Enlighten environment light values. 
// Arguments:
//   cube - Array of environment light values (2x2x6 for a cube oriented in scene space)
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetEnvironmentCube( const Geo::v128* cube )
{
	memcpy( m_enlightenEnvironmentWorld, cube, sizeof( Geo::v128 ) * 24 );
	Geo::v128 result[24];
	RotateEnvironmentCube( m_enlightenEnvironmentWorld, m_worldTransform, result );
	m_enlightenEnvironment->SetValues( const_cast<Geo::v128*>( result ) );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::PrepareUpdateEnlighten( int cutoffDepth, Tr2IntEnlightenTaskManager &updateTask )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	// Temp vector to hold workspaces from systems and neighboring cells
	std::vector<const Enlighten::InputLightingBuffer*> mergedLightingBuffers;

	// Gather workspaces from neighboring cells
	for( std::map<Tr2InteriorCellPtr, int>::iterator it = m_cellNeighbors.begin(); it != m_cellNeighbors.end(); ++it )
	{
		it->first->GetInputLightingBuffers( mergedLightingBuffers );
	}

	// Add in the workspaces from systems
	mergedLightingBuffers.insert( mergedLightingBuffers.end(), m_inputLightingBuffers.begin(), m_inputLightingBuffers.end() );

	// Update systems
	if( !m_inputLightingBuffers.empty() && ((cutoffDepth == -1) || (m_cellDepth <= cutoffDepth)) )
	{
		bool hasSystems = false;
		for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			if( (*it)->PrepareUpdateEnlighten( mergedLightingBuffers, m_enlightenEnvironment, updateTask ) )
			{
				m_updateTaskInfo.SetTaskManager( &updateTask );
				hasSystems = true;
			}
		}
		if( hasSystems && !m_sampleVolumes.empty() && !m_dynamics.empty() )
		{
			for( std::vector<SampleVolume>::iterator it = m_sampleVolumes.begin(); it != m_sampleVolumes.end(); ++it )
			{
				if( !it->m_task.m_InputLighting )
				{
					unsigned int inputWorkspaceLength = Enlighten::GetInputWorkspaceListLength( it->m_task.m_CoreProbeSet );
					it->m_task.m_InputLighting = GEO_NEW_ARRAY( const Enlighten::InputLightingBuffer*, inputWorkspaceLength );
				}

				Enlighten::PrepareInputLightingList(
					it->m_task.m_CoreProbeSet,
					&mergedLightingBuffers[0],			// Input
					(Geo::s32)mergedLightingBuffers.size(),
					it->m_task.m_InputLighting );

				updateTask.AddSHVolume( this, it->m_volumeBox, it->m_task.m_CoreProbeSet, it->m_task.m_InputLighting );
			}

			updateTask.SetCellInputLighting( this, mergedLightingBuffers, m_enlightenEnvironment );

			Vector3 center;
			for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
			{
				ITr2InteriorDynamic* pDynamic = *it;

				pDynamic->GetShProbePosition( center );

				unsigned int index = updateTask.AddSHProbeSample( pDynamic->GetSHSampleIndex(), this, center );
				pDynamic->SetSHSampleIndex( index );
			}
		}
	}

	if( m_socketSystem.GetRadSystem() && m_socketSystem.GetEnlightenWorkspace() )
	{
		unsigned int control_word;
		_controlfp_s(&control_word, _DN_FLUSH, _MCW_DN);

		Enlighten::WriteInputLightingTask writeTask;
		writeTask.m_InputWorkspace = m_socketSystem.GetEnlightenWorkspace();
		writeTask.m_LightArray = NULL;
		writeTask.m_LightingBuffer = m_socketSystem.GetCurrentInputLightingBuffer();
		writeTask.m_NumLights = 0;

		int size = Enlighten::CalcRequiredWorkspaceMemory( &writeTask );
		void* workspace = GEO_ALIGNED_MALLOC( size, 16 );

		Geo::u32 ms;
		if( !Enlighten::DoWriteInputLightingTask( &writeTask, workspace, ms ) )
		{
			CCP_LOGERR( "Failed to run Enlighten DoWriteInputLightingTask:");
			CCP_LOGERR( Tr2Renderer::GetEnlightenErrorBuffer() );
			Enlighten::ClearErrorBuffer();
			GEO_ALIGNED_FREE( workspace );
			return;
		}
		GEO_ALIGNED_FREE( workspace );

		Enlighten::EndInputLightingTask endTask;
		endTask.m_LightingBuffer = m_socketSystem.GetCurrentInputLightingBuffer();
		endTask.m_PreviousLightingBuffer = NULL;
		endTask.m_InputWorkspace = m_socketSystem.GetEnlightenWorkspace();
		endTask.m_BounceData = m_socketSystem.GetBounceData();
		endTask.m_BounceScale = Geo::VBroadcast( 1.0f );
		endTask.m_ClusterAlbedoWorkspace = NULL;
		endTask.m_AlbedoTextureData = m_socketSystem.GetAlbedoTexture();
		endTask.m_EmissiveTextureData = m_socketSystem.GetEmissiveTexture();
		endTask.m_EmissiveScale = Geo::VBroadcast( 1.f );

		if( !Enlighten::DoEndInputLightingTask( &endTask, ms ) )
		{
			CCP_LOGERR( "Failed to run Enlighten DoEndInputLightingTask:");
			CCP_LOGERR( Tr2Renderer::GetEnlightenErrorBuffer() );
			Enlighten::ClearErrorBuffer();
		}

		// Restore it
		_controlfp_s(&control_word, control_word, _MCW_DN);
	}
}

// -------------------------------------------------------------
// Description:
//   Convert SH coeficients to channel matrices.
// Arguments:
//   finalCoeff - Array of 9x3 SH indices
//   redMat, greenMat, blueMat (out) - Resulting matrices for each channel
// -------------------------------------------------------------
static void GetSHMatrices( const Vector3* finalCoeff, Matrix& redMat, Matrix& greenMat, Matrix& blueMat )
{
	const float c1 = 0.429043f;
	const float c2 = 0.511664f;
	const float c3 = 0.743125f;
	const float c4 = 0.886227f;
	const float c5 = 0.247708f;

	// Set the red matrix
	{
		Vector4 r0( c1 * finalCoeff[8].x,
					c1 * finalCoeff[4].x,
					c1 * finalCoeff[7].x,
					c2 * finalCoeff[3].x );
		Vector4 r1( c1 * finalCoeff[4].x,
				   -c1 * finalCoeff[8].x,
					c1 * finalCoeff[5].x,
					c2 * finalCoeff[1].x );
		Vector4 r2( c1 * finalCoeff[7].x,
					c1 * finalCoeff[5].x,
					c3 * finalCoeff[6].x,
					c2 * finalCoeff[2].x );
		Vector4 r3( c2 * finalCoeff[3].x,
					c2 * finalCoeff[1].x,
					c2 * finalCoeff[2].x,
					c4 * finalCoeff[0].x - c5 * finalCoeff[6].x );

		redMat = Matrix( r0.x, r0.y, r0.z, r0.w, 
			             r1.x, r1.y, r1.z, r1.w, 
						 r2.x, r2.y, r2.z, r2.w, 
						 r3.x, r3.y, r3.z, r3.w );
	}

	// Set the green matrix
	{
		Vector4 r0( c1 * finalCoeff[8].y,
					c1 * finalCoeff[4].y,
					c1 * finalCoeff[7].y,
					c2 * finalCoeff[3].y );
		Vector4 r1( c1 * finalCoeff[4].y,
				   -c1 * finalCoeff[8].y,
					c1 * finalCoeff[5].y,
					c2 * finalCoeff[1].y );
		Vector4 r2( c1 * finalCoeff[7].y,
					c1 * finalCoeff[5].y,
					c3 * finalCoeff[6].y,
					c2 * finalCoeff[2].y );
		Vector4 r3( c2 * finalCoeff[3].y,
					c2 * finalCoeff[1].y,
					c2 * finalCoeff[2].y,
					c4 * finalCoeff[0].y - c5 * finalCoeff[6].y );

		greenMat = Matrix( r0.x, r0.y, r0.z, r0.w, 
			               r1.x, r1.y, r1.z, r1.w, 
						   r2.x, r2.y, r2.z, r2.w, 
						   r3.x, r3.y, r3.z, r3.w );
	}

	// Set the blue matrix
	{
		Vector4 r0( c1 * finalCoeff[8].z,
					c1 * finalCoeff[4].z,
					c1 * finalCoeff[7].z,
					c2 * finalCoeff[3].z );
		Vector4 r1( c1 * finalCoeff[4].z,
				   -c1 * finalCoeff[8].z,
					c1 * finalCoeff[5].z,
					c2 * finalCoeff[1].z );
		Vector4 r2( c1 * finalCoeff[7].z,
					c1 * finalCoeff[5].z,
					c3 * finalCoeff[6].z,
					c2 * finalCoeff[2].z );
		Vector4 r3( c2 * finalCoeff[3].z,
					c2 * finalCoeff[1].z,
					c2 * finalCoeff[2].z,
					c4 * finalCoeff[0].z - c5 * finalCoeff[6].z );

		blueMat = Matrix( r0.x, r0.y, r0.z, r0.w, 
			              r1.x, r1.y, r1.z, r1.w, 
						  r2.x, r2.y, r2.z, r2.w, 
						  r3.x, r3.y, r3.z, r3.w );
	}
}


// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::ProcessEnlightenResults( Tr2IntEnlightenTaskManager &updateTask )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	{
		for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
		{
			(*it)->UpdateEnlightenTextures();
		}
		m_updateTaskInfo.SetTaskManager( NULL );
	}

	std::vector<const Enlighten::InputLightingBuffer*> mergedLighting;
	{
		// Gather workspaces from neighboring cells
		for( std::map<Tr2InteriorCellPtr, int>::iterator it = m_cellNeighbors.begin(); it != m_cellNeighbors.end(); ++it )
		{
			it->first->GetInputLightingBuffers( mergedLighting );
		}

		// Add in the workspaces from systems
		mergedLighting.insert( mergedLighting.end(), m_inputLightingBuffers.begin(), m_inputLightingBuffers.end() );
	}

	for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		const Vector3* coefficients = updateTask.GetSHProbeResult( (*it)->GetSHSampleIndex() );
		if( coefficients )
		{
			GetSHMatrices( coefficients, 
						   (*it)->GetRedLightProbeMatrix(), 
						   (*it)->GetGreenLightProbeMatrix(), 
						   (*it)->GetBlueLightProbeMatrix() );
		}
		else
		{
			static const Matrix noLighting( 0.0f, 0.0f, 0.0f, 0.0f, 
											0.0f, 0.0f, 0.0f, 0.0f, 
											0.0f, 0.0f, 0.0f, 0.0f, 
											0.0f, 0.0f, 0.0f, 0.0f );
			(*it)->GetRedLightProbeMatrix() = noLighting;
			(*it)->GetGreenLightProbeMatrix() = noLighting;
			(*it)->GetBlueLightProbeMatrix() = noLighting;
		}
	}

	// Update the light probes
	UpdateLightProbes( mergedLighting );
}

// -------------------------------------------------------------
// Description:
//   Updates (solves) all SH probe volumes (default, additional and
//   portal socket).
// Arguments:
//   mergedWorkspaces - List of all (including neighbor) system workspaces
// -------------------------------------------------------------
void Tr2InteriorCell::UpdateLightProbes( std::vector<const Enlighten::InputLightingBuffer*>& mergedInputLighting )
{
	if( m_boundingBoxReady && !m_inputLightingBuffers.empty() )
	{
		if( !m_sampleVolumes.empty() )
		{
			if( m_defaultProbeVolume.HasValidProbes() && m_defaultProbeVolume.GetSphereProbeVisualization())
			{
				CreateDebugProbeData( m_sampleVolumes[0], mergedInputLighting );
			}
			for( size_t i = 0; i != m_probeVolumes.size(); ++i )
			{
				if( m_probeVolumes[i]->HasValidProbes() && m_probeVolumes[i]->GetSphereProbeVisualization())
				{
					CreateDebugProbeData( m_sampleVolumes[i + 1], mergedInputLighting );
				}
			}
		}
		if( !m_portalSocketSamples.m_samples.IsEmpty() && m_portalSocketSamples.m_task.m_CoreProbeSet != NULL && m_socketSystem.GetEnlightenWorkspace() != NULL )
		{
			if( !m_portalSocketSamples.m_task.m_InputLighting )
			{
				unsigned int inputWorkspaceLength = Enlighten::GetInputWorkspaceListLength( m_portalSocketSamples.m_task.m_CoreProbeSet );
				m_portalSocketSamples.m_task.m_InputLighting = GEO_NEW_ARRAY( const Enlighten::InputLightingBuffer*, inputWorkspaceLength );
			}

			Geo::u32 probeTaskSolveTime;
			Enlighten::PrepareInputLightingList(
				m_portalSocketSamples.m_task.m_CoreProbeSet,
				const_cast<const Enlighten::InputLightingBuffer**>( &m_inputLightingBuffers[0] ),			// Input
				(Geo::s32)m_inputLightingBuffers.size() - 1,
				m_portalSocketSamples.m_task.m_InputLighting );
			Enlighten::SolveProbeTaskL2( &m_portalSocketSamples.m_task, probeTaskSolveTime );
		}

		if( m_drawSocketProbes && m_portalSocketSamples.m_debugProbeLighting == NULL )
		{
			m_portalSocketSamples.m_debugProbeLighting = new Matrix[3 * m_portalSocketSamples.m_samples.GetSize()];
		}

		Vector3 finalCoeff[9];
		Matrix redMat, greenMat, blueMat;
		for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
		{
			std::map<Tr2InteriorPortalSocket*, int>::iterator mit = m_portalSocketIndexes.find( *it );
			if( mit == m_portalSocketIndexes.end() || !(*it)->HasValidProbe() )
			{
				static const Matrix blackChannel(
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0,
					0, 0, 0, 0 );
				(*it)->SetInputColor( blackChannel, blackChannel, blackChannel );
			}
			else
			{
				int index = mit->second;
				for( int i = 0; i < 9; ++i )
				{
					finalCoeff[i] = Vector3( 0.0f, 0.0f, 0.0f );
				}

				float* out = m_portalSocketSamples.m_task.m_OutputPointers[index];
				for( int c = 0; c < 3; ++c )
				{
					for( int i = 0; i < 9; ++i )
					{
						float v = (*out++);

						if( c == 0 )
						{
							finalCoeff[i].x += v;
						}
						else if( c == 1 )
						{
							finalCoeff[i].y += v;
						}
						else
						{
							finalCoeff[i].z += v;
						}
					}
				}

				GetSHMatrices( finalCoeff, redMat, greenMat, blueMat );

				(*it)->SetInputColor( redMat, greenMat, blueMat );
				if( m_drawSocketProbes )
				{
					m_portalSocketSamples.m_debugProbeLighting[index * 3 + 0] = redMat;
					m_portalSocketSamples.m_debugProbeLighting[index * 3 + 1] = greenMat;
					m_portalSocketSamples.m_debugProbeLighting[index * 3 + 2] = blueMat;
				}
			}
		}
	}
	else
	{
		for( PITr2InteriorDynamicVector::iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
		{
			ITr2InteriorDynamic* pDynamic = *it;

			// Initialize matrices to black (to prevent artifacts when rendering without a light volume)
			pDynamic->GetRedLightProbeMatrix() = Tr2Renderer::GetNullTransform();
			pDynamic->GetGreenLightProbeMatrix() = Tr2Renderer::GetNullTransform();
			pDynamic->GetBlueLightProbeMatrix() = Tr2Renderer::GetNullTransform();
		}
		for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
		{
			static const Matrix blackChannel(
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0,
				0, 0, 0, 0 );
			(*it)->SetInputColor( blackChannel, blackChannel, blackChannel );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::Update( Be::Time time, int cutoffDepth )
{
	// Set the sub-cell visibility to false
	for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->SetVisibility( false );
		(*it)->Update( time );
	}

	m_isVisible = false;
	m_cellDepth = -1;

	m_variableStore->RegisterVariable( "ReflectionMap", m_reflectionMapRes );
}

// ------------------------------------------------------------------------------------------------------
TriTextureRes* Tr2InteriorCell::GetReflectionMap()
{
	return m_reflectionMapRes;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RebuildInternalData()
{
	RebuildBoundingBox();
}

// --------------------------------------------------------------------------------------
// Description:
//   Enlarges cell's bounding box to enclose the given bounding box. 
// Arguments:
//   minBounds - min bounds of the box to enclose
//   maxBounds - max bounds of the box to enclose
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::UpdateBoundingBox( const Vector3& minBounds, const Vector3& maxBounds )
{
	Vector3 oldMinBounds = m_minBounds;
	Vector3 oldMaxBounds = m_maxBounds;
	BoundingBoxUpdate( m_minBounds, m_maxBounds, minBounds, maxBounds );

	m_boundingBoxReady = true;

	if ( ( oldMinBounds != m_minBounds || oldMaxBounds != m_maxBounds ) && 
		abs( m_minBounds.x ) != FLT_MAX )
	{
		Vector3 center = (m_minBounds + m_maxBounds) / 2.0f;
		Vector3 scale = m_maxBounds - m_minBounds;
		m_defaultProbeVolume.SetPosition( center );
		m_defaultProbeVolume.SetScaling( scale );

		m_isDirty = true;

		if( !m_sampleVolumes.empty() )
		{
			Matrix transform(
				scale.x, 0.0f, 0.0f, 0.0f,
				0.0f, scale.y, 0.0f, 0.0f,
				0.0f, 0.0f, scale.z, 0.0f,
				center.x, center.y, center.z, 1.0f );
			Vector3 position = Vector3( -0.5f, -0.5f, -0.5f );
			m_sampleVolumes[0].m_transform = transform;
			D3DXVec3TransformCoord( &position, &position, &m_sampleVolumes[0].m_transform );
			Geo::v128 pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 0.0f );

			Vector3 x( 1.0f, 0.0f, 0.0f );
			Vector3 y( 0.0f, 1.0f, 0.0f );
			Vector3 z( 0.0f, 0.0f, 1.0f );
			D3DXVec3TransformNormal( &x, &x, &m_sampleVolumes[0].m_transform );
			D3DXVec3TransformNormal( &y, &y, &m_sampleVolumes[0].m_transform );
			D3DXVec3TransformNormal( &z, &z, &m_sampleVolumes[0].m_transform );
			Vector3 axis;
			axis.x = D3DXVec3Length( &x );
			axis.y = D3DXVec3Length( &y );
			axis.z = D3DXVec3Length( &z );
			x /= axis.x;
			y /= axis.y;
			z /= axis.z;
			Geo::v128 s = GEO_VCONSTRUCT( axis.x, axis.y, axis.z, 0.0f );

			Geo::v128 mat[3];
			mat[0] = GEO_VCONSTRUCT( x.x, x.y, x.z, 0.0f );
			mat[1] = GEO_VCONSTRUCT( y.x, y.y, y.z, 0.0f );
			mat[2] = GEO_VCONSTRUCT( z.x, z.y, z.z, 0.0f );

			static_cast<GeoEngine::NonAABoundingBox*>( m_sampleVolumes[0].m_volumeBox )->Initialise( mat, pos, s );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds an interior Enlighten system to the cell.  If the add succeeds, the cell's Umbra
//   representation is rebuilt.  It is an error to add a NULL system, and a log message 
//   is printed saying so.
// Arguments:
//   system - The Enlighten system to add to the cell (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::AddSystem( Tr2InteriorEnlightenSystem* system )
{
	// Bail out early if the system is NULL
	if( !system )
	{
		CCP_LOGERR( "Attempt to add a NULL Enlighten system to interior cell!" );
		return;
	}

	// Put it in our list of systems
	m_systems.Insert( -1, system->GetRawRoot() );

	// Tell the new system about it's parent
	system->SetParentCell( this );

	// Rebuild cell
	RebuildInternalData();
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes an Enlighten system from the cell.  It is an error to remove a NULL system, 
//   and a log message is printed saying so.
// Arguments:
//   system - The Enlighten system to remove from the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::RemoveSystem( Tr2InteriorEnlightenSystem* system )
{
	// Bail out early if the Enlighten system is NULL
	if( !system )
	{
		CCP_LOGERR( "Attempt to remove a NULL system from interior cell!" );
		return;
	}

	// find this one
	ssize_t pos = m_systems.FindKey( system->GetRawRoot() );
	if( pos == -1 )
	{
		CCP_LOGERR("Tr2InteriorCell::RemoveSystem() - system not found in this cell!");
		return;
	}

	// and clean-up system
	system->SetParentCell( NULL );

	// remove it
	m_systems.Remove( pos );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::ClearSystems()
{
	// clean-up all sub cells
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->SetParentCell( NULL );
	}
	// clear vector
	while( m_systems.size() )
	{
		m_systems.Remove( 0 );
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds a dynamic object to the cell.  If the dynamic is already in the cell's
//   internal list, it is not added again.  In this case, shadows are marked as dirty.
//   This function should be called by the Tr2InteriorScene containing the cell, and
//   not by any other caller.  Adding a dynamic to a cell ensures it receives correct
//   indirect lighting from the Enlighten light probes, which are managed by the cell.
//   It is an error to add a NULL dynamic to a cell, and a log message is printed if that
//   occurs.
// Arguments:
//   dynamic - The dynamic to add to the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::AddDynamic( ITr2InteriorDynamic* dynamic )
{
	// Bail out early if
	if( !dynamic )
	{
		CCP_LOGERR( "Attempt to add a NULL dynamic to interior cell!" );
		return false;
	}

	// put it in our list of dynamics
	bool added = false;
	ssize_t pos = m_dynamics.FindKey( dynamic );
	if( pos == -1 )
	{
		m_dynamics.Insert( -1, dynamic );
		added = true;
	}

	// See if it's a skinned object - we maintain a separate list of those for
	// performance reasons
	Tr2IntSkinnedObject* skinned = dynamic_cast<Tr2IntSkinnedObject*>( dynamic );
	if( skinned )
	{
		pos = m_skinnedObjects.FindKey( dynamic );
		if( pos == -1 )
		{
			m_skinnedObjects.Insert( -1, dynamic );
		}
	}

	MarkShadowsDirtyForDynamic( dynamic );

	return added;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes a dynamic object from the cell.  If the dynamic is in the list prior to
//   removal, shadows are flagged as dirty.  It is an error to remove a NULL dynamic, and 
//   a log message is printed if that occurs.
// Arguments:
//   dynamic - The dynamic object to remove from the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::RemoveDynamic( ITr2InteriorDynamic* dynamic )
{
	// Bail out early if the dynamic is NULL
	if( !dynamic )
	{
		CCP_LOGERR( "Attempt to remove NULL dynamic from interior cell!" );
		return false;
	}

	bool removed = false;

	// find this one
	ssize_t pos = m_dynamics.FindKey( dynamic );
	if( pos != -1 )
	{
		MarkShadowsDirtyForDynamic( dynamic );

		// remove it
		m_dynamics.Remove( pos );

		removed = true;

		pos = m_skinnedObjects.FindKey( dynamic );
		if( pos != -1 )
		{
			m_skinnedObjects.Remove( pos );
		}
	}

	return removed;
}

// --------------------------------------------------------------------------------------
// Description:
//   Adds an interior light source to the cell.  An individual light source can only be 
//   added to a cell once.  If the light is static, then all Enlighten systems contained
//   in the cell invalidate their lighting caches.  It is an error to add a NULL light 
//   source, and a log message is printed if that occurs.
// Arguments:
//   light - The light source to add to the interior cell (should not be NULL)
// --------------------------------------------------------------------------------------
bool Tr2InteriorCell::AddLight( ITr2InteriorLight* light )
{
	// Bail out early if the light source is NULL
	if( !light )
	{
		CCP_LOGERR( "Attempt to add NULL light source to interior cell!" );
		return false;
	}

	// put it in our list of lights
	ssize_t key = m_lights.FindKey( light );
	if( key == -1 )
	{
		m_lights.Insert( -1, light );

		// If this light is static, notify the systems to invalidate their caches
		if( light->IsStatic() )
		{
			for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); 
				 it != m_systems.end(); ++it )
			{
				(*it)->InvalidateLightCache();
			}
		}

		// Added the light, return success
		return true;
	}
	
	// Didn't add the light (because it's already in our list), return failure
	return false;
}

// --------------------------------------------------------------------------------------
// Description:
//   Removes an interior light source from the cell.  If the light source is static, then
//   all Enlighten systems in the cell invalidate their lighting caches.  It is an error 
//   to remove a NULL light source, and a log message is printed if that occurs.
// Arguments:
//   lightSource - The light source to remove from the interior scene (should not be NULL)
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::RemoveLight( ITr2InteriorLight* light )
{
	// Bail out early if the light source is NULL
	if( !light )
	{
		CCP_LOGERR( "Attempt to remove NULL light from interior cell!" );
		return;
	}

	ssize_t key = m_lights.FindKey( light );
	if( key != -1 )
	{
		m_lights.Remove( key );

		// If this light is static, notify the systems to invalidate their caches
		if( light->IsStatic() )
		{
			for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); 
				 it != m_systems.end(); ++it )
			{
				(*it)->InvalidateLightCache();
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   Mark spotlight shadows as dirty for lights that affect the given dynamic object.
// Arguments:
//   dynamic - Dynamic object that has moved/changed.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::MarkShadowsDirtyForDynamic( ITr2InteriorDynamic* dynamic )
{
	if( !dynamic->CastsShadows() )
	{
		return;
	}
	for( ITr2InteriorLightVector::iterator it = m_lights.begin(); it != m_lights.end(); ++it )
	{
		if( (*it)->IsDynamicContributingToShadows( dynamic ) )
		{
			Vector3 min, max;
			dynamic->GetWorldBoundingBox( min, max );
			(*it)->MarkShadowsDirtyForBounds( min, max );
		}
	}
}

// --------------------------------------------------------------------------------------
// Description
//   Mark spotlight shadows as dirty for lights that affect any of the cell's skinned 
//   objects.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::MarkShadowsDirtyForSkinnedObjects()
{
	for( ITr2InteriorDynamicVector::iterator it = m_skinnedObjects.begin(); 
		it != m_skinnedObjects.end(); ++it )
	{
		MarkShadowsDirtyForDynamic( *it );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RenderDebugInfo( TriLineSetPtr lines )
{
	// bounding boxes?
	if( m_drawBoundingBox )
	{
		lines->AddOrientedBox( m_worldTransform, m_minBounds, m_maxBounds, 0x800000ff );

		for( Tr2InteriorBoundingBoxVector::const_iterator it = m_boundingBoxes.begin(); it != m_boundingBoxes.end(); ++it)
		{
			lines->AddOrientedBox( m_worldTransform, (*it)->m_minBounds, (*it)->m_maxBounds, 0x400000ff );
		}
	}

	// Loop over the dynamics & draw any necessary probe contributions
	for( PITr2InteriorDynamicVector::const_iterator it = m_dynamics.begin(); it != m_dynamics.end(); ++it )
	{
		const ITr2InteriorDynamic* dynamic = *it;

		if( dynamic->DoVisualizeLightProbes() )
		{
			// Get the center of the dynamic's bounding box
			Vector3 min, max;
			dynamic->GetWorldBoundingBox( min, max );
			Vector3 center = 0.5f * (min + max);

			float totalWeight = 0.0f;
			for (PTr2InteriorProbeVolumeVector::const_iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it)
			{
				totalWeight += (*it)->GetMixWeight( center );
			}

			if (totalWeight > 0.0f)
			{
				for (PTr2InteriorProbeVolumeVector::const_iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it)
				{
					float weight = (*it)->GetMixWeight( center );
					if (weight > 0)
					{
						Vector3 probes[GeoEngine::VOL_INTERP_TRI];
						float weights[GeoEngine::VOL_INTERP_TRI];
						(*it)->GetProbePositions( center, probes, weights );

						for( int i = 0; i < GeoEngine::VOL_INTERP_TRI; ++i )
						{
							const Vector3& probePos = probes[i];
							float weight = weights[i];
							if (totalWeight > 1.0f)
							{
								weight /= totalWeight;
							}
							else
							{
								weight *= totalWeight;
							}
							
							weight *= 2.0f;
							weight = weight > 1.0f ? 1.0f : weight;

							unsigned int color = 0xff000000;
							unsigned short channel = static_cast<unsigned short>( weight * static_cast<float>(0xff) );
							color |= channel << 16;
							color |= channel << 8;
							color |= channel;

							Vector3 probePosWorld;
							D3DXVec3TransformCoord( &probePosWorld, &probePos, &m_worldTransform );

							lines->Add( center, color, probePosWorld, color );
						}
					}
				}
			}


			if( totalWeight < 1.0f )
			{
				// Get the indices and weights of the 8 nearest light probes
				Vector3 probes[GeoEngine::VOL_INTERP_TRI];
				float weights[GeoEngine::VOL_INTERP_TRI];
				m_defaultProbeVolume.GetProbePositions( center, probes, weights );

				for( int i = 0; i < GeoEngine::VOL_INTERP_TRI; ++i )
				{
					const Vector3& probePos = probes[i];
					float weight = weights[i];
					weight *= 1.0f - totalWeight;
					
					weight *= 2.0f;
					weight = weight > 1.0f ? 1.0f : weight;

					unsigned int color = 0xff000000;
					unsigned short channel = static_cast<unsigned short>( weight * static_cast<float>(0xff) );
					color |= channel << 16;
					color |= channel << 8;
					color |= channel;

					Vector3 probePosWorld;
					D3DXVec3TransformCoord( &probePosWorld, &probePos, &m_worldTransform );

					lines->Add( center, color, probePos, color );
				}
			}
		}
	}


	// pass down to sub-cells
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->RenderDebugInfo( lines );
	}

	for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
	{
		(*it)->RenderDebugInfo( lines );
	}

	// pass down to probe volumes
	m_defaultProbeVolume.RenderDebugInfo( lines );
	for( PTr2InteriorProbeVolumeVector::const_iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it )
	{
		(*it)->RenderDebugInfo( lines );
	}

	if( !m_sampleVolumes.empty() )
	{
		if( m_defaultProbeVolume.HasValidProbes() && m_defaultProbeVolume.GetSphereProbeVisualization() )
		{
			if( m_defaultProbeVolume.m_drawProbeCulling )
			{
				RenderDebugProbeCullStatus( m_sampleVolumes[0] );
			}
			else
			{
				RenderDebugProbeSpheres( m_sampleVolumes[0] );
			}
		}
		for( size_t i = 0; i != m_probeVolumes.size(); ++i )
		{
			if( m_probeVolumes[i]->HasValidProbes() && m_probeVolumes[i]->GetSphereProbeVisualization() )
			{
				if( m_probeVolumes[i]->m_drawProbeCulling )
				{
					RenderDebugProbeCullStatus( m_sampleVolumes[i + 1] );
				}
				else
				{
					RenderDebugProbeSpheres( m_sampleVolumes[i + 1] );
				}
			}
		}
	}

	if( m_drawSocketProbes && m_portalSocketSamples.m_task.m_CoreProbeSet != NULL && m_portalSocketSamples.m_debugProbeLighting != NULL )
	{
		if( m_probeVisualizer == NULL )
		{
			m_probeVisualizer.CreateInstance();
		}
		for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
		{
			std::map<Tr2InteriorPortalSocket*, int>::const_iterator mit = m_portalSocketIndexes.find( *it );
			if( mit != m_portalSocketIndexes.end() && (*it)->HasValidProbe() )
			{
				int j = mit->second;

				Vector3 pos = Vector3( Geo::VGetX( m_portalSocketSamples.m_samples[j] ), Geo::VGetY( m_portalSocketSamples.m_samples[j] ), Geo::VGetZ( m_portalSocketSamples.m_samples[j] ) );
				D3DXVec3TransformCoord( &pos, &pos, &m_worldTransform );

				if( m_drawSocketProbeCulling )
				{
					static const Matrix fullChannel(
						0, 0, 0, 0, 
						0, 0, 0, 0, 
						0, 0, 0, 0, 
						0, 0, 0, D3DX_PI );
					static const Matrix emptyChannel(
						0, 0, 0, 0, 
						0, 0, 0, 0, 
						0, 0, 0, 0, 
						0, 0, 0, 0 );

					if( Enlighten::IsProbeCulled( j, m_portalSocketSamples.m_task.m_CoreProbeSet ) )
					{
						m_probeVisualizer->Render( pos, fullChannel, emptyChannel, emptyChannel, 1.0f );
					}
					else
					{
						m_probeVisualizer->Render( pos, emptyChannel, fullChannel, emptyChannel, 1.0f );
					}
				}
				else
				{
					m_probeVisualizer->Render( 
						pos, 
						m_portalSocketSamples.m_debugProbeLighting[j * 3], m_portalSocketSamples.m_debugProbeLighting[j * 3 + 1], m_portalSocketSamples.m_debugProbeLighting[j * 3 + 2],
						m_shScale );
				}
			}
		}
	}
}

// --------------------------------------------------------------------------------------
// Description:
//   Renders occlusion geometry of all statics in all Enlighten systems.
// --------------------------------------------------------------------------------------
void Tr2InteriorCell::RenderOcclusionGeometry() const
{
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		( *it )->RenderOcclusionGeometry();
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetSHScale( float scale )
{
	m_shScale = scale;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::CreateDebugProbeData( SampleVolume &volume, std::vector<const Enlighten::InputLightingBuffer*>& inputLightingBuffers )
{
	if( m_probeVisualizer == NULL )
	{
		m_probeVisualizer.CreateInstance();
	}

	if( !volume.m_task.m_InputLighting )
	{
		unsigned int inputWorkspaceLength = Enlighten::GetInputWorkspaceListLength( volume.m_task.m_CoreProbeSet );
		volume.m_task.m_InputLighting = GEO_NEW_ARRAY( const Enlighten::InputLightingBuffer*, inputWorkspaceLength );
	}

	if( volume.m_debugProbeLighting == NULL )
	{
		volume.m_debugProbeLighting = new Matrix[3 * volume.m_samples.GetSize()];
	}

	Enlighten::PrepareInputLightingList(
		volume.m_task.m_CoreProbeSet,
		&inputLightingBuffers[0],			// Input
		(Geo::s32)inputLightingBuffers.size(),
		volume.m_task.m_InputLighting );
	volume.m_task.m_NumIndicesToSolve = GeoEngine::VOL_INTERP_NN;

	Vector3 finalCoeff[9];
	Matrix redMat, greenMat, blueMat;

	for( int j = 0; j < volume.m_samples.GetSize(); ++j )
	{
		volume.m_task.m_IndicesToSolve[0] = j;

		// Run the task
		Geo::u32 probeTaskSolveTime;
		Enlighten::SolveProbeTaskL2( &volume.m_task, probeTaskSolveTime );

		for( int i = 0; i < 9; ++i )
		{
			finalCoeff[i] = Vector3( 0.0f, 0.0f, 0.0f );
		}

		float* out = volume.m_task.m_OutputPointers[0];

		for( int c = 0; c < 3; ++c )
		{
			for( int i = 0; i < 9; ++i )
			{
				float v = (*out++);

				if( c == 0 )
				{
					finalCoeff[i].x += v;
				}
				else if( c == 1 )
				{
					finalCoeff[i].y += v;
				}
				else
				{
					finalCoeff[i].z += v;
				}
			}
		}

		GetSHMatrices( finalCoeff, volume.m_debugProbeLighting[j * 3], volume.m_debugProbeLighting[j * 3 + 1], volume.m_debugProbeLighting[j * 3 + 2] );
	}

	volume.m_task.m_NumIndicesToSolve = GeoEngine::VOL_INTERP_TRI;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RenderDebugProbeSpheres( const SampleVolume &volume ) const
{
	if( m_probeVisualizer == NULL )
	{
		return;
	}

	if( volume.m_debugProbeLighting == NULL )
	{
		return;
	}

	for( int j = 0; j < volume.m_samples.GetSize(); ++j )
	{
		Vector3 pos = Vector3( Geo::VGetX( volume.m_samples[j] ), Geo::VGetY( volume.m_samples[j] ), Geo::VGetZ( volume.m_samples[j] ) );
		D3DXVec3TransformCoord( &pos, &pos, &m_worldTransform );

		m_probeVisualizer->Render( 
			pos, 
			volume.m_debugProbeLighting[j * 3], volume.m_debugProbeLighting[j * 3 + 1], volume.m_debugProbeLighting[j * 3 + 2],
			m_shScale );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RenderDebugProbeCullStatus( const SampleVolume &volume ) const
{
	if( m_probeVisualizer == NULL )
	{
		return;
	}

	if( volume.m_task.m_CoreProbeSet == NULL )
	{
		return;
	}

	static const Matrix fullChannel(
		0, 0, 0, 0, 
		0, 0, 0, 0, 
		0, 0, 0, 0, 
		0, 0, 0, D3DX_PI );
	static const Matrix emptyChannel(
		0, 0, 0, 0, 
		0, 0, 0, 0, 
		0, 0, 0, 0, 
		0, 0, 0, 0 );

	for( int j = 0; j < volume.m_samples.GetSize(); ++j )
	{
		Vector3 pos = Vector3( Geo::VGetX( volume.m_samples[j] ), Geo::VGetY( volume.m_samples[j] ), Geo::VGetZ( volume.m_samples[j] ) );
		D3DXVec3TransformCoord( &pos, &pos, &m_worldTransform );

		if( Enlighten::IsProbeCulled( j, volume.m_task.m_CoreProbeSet ) )
		{
			m_probeVisualizer->Render( 
				pos, 
				fullChannel, emptyChannel, emptyChannel,
				1.0f );
		}
		else
		{
			m_probeVisualizer->Render( 
				pos, 
				emptyChannel, fullChannel, emptyChannel,
				1.0f );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetVisualizeMethod( VisualizeMethod method )
{
	// Pass down to sub-cells
	for( PTr2InteriorEnlightenSystemVector::iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->SetVisualizeMethod( method );
	}
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
// Description:
//   Packs geometries in all systems into Enlighten systems (one for each system). Also do the same for
//   portal socket system.
// Arguments:
//   quality - quality level for the built
//   pPrecompute - pointer to Enlighten precompute object
//   prog - progress log object
// Return Value:
//   true If successfully packed all systems
//   false If there was an error during packing of one of the systems
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::BuildPackedSystems( Tr2InteriorEnlightenSystemImpl::Quality quality,
                                          Enlighten::IPrecompute* pPrecompute, 
                                          TriEnlightenProgressBar& prog )
{
	// Clear out all the scratch-pad storage
	m_packedSystems.clear();
	m_packedSystemsNeighbors.clear();
	m_systemPreClusters.clear();
	m_systemPreClustersNeighbors.clear();
	m_systemClusters.clear();
	m_systemClustersNeighbors.clear();
	m_systemIDs.Empty();

	// Pack all the systems
	unsigned int systemInCellIdx = 0;
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it, ++systemInCellIdx )
	{
		(*it)->PackEnlightenSystem( quality, pPrecompute, prog, systemInCellIdx );
		Enlighten::IPrecompPackedSystem* packedSystem = (*it)->GetPackedSystem();
		if( !packedSystem )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "PackEnlightenSystem() failed to pack system %i. Halting system build.", systemInCellIdx );
			return false;
		}
		m_packedSystems.push_back( packedSystem );
		m_systemIDs.Push( packedSystem->GetId() );
	}

	m_packedPortalSockets.clear();
	if( m_socketSystem.PackEnlightenSystem( quality, pPrecompute, prog, -1, this ) )
	{
		const Enlighten::IPrecompPackedInstance** packedInstances = m_socketSystem.GetPackedSystem()->GetPackedInstances();
		for( unsigned index = 0; index < m_packedPortalSockets.size(); ++index )
		{
			Geo::Geo2DTransform tf = packedInstances[index]->GetUvTransform();
			m_packedPortalSockets[index]->SetTexturePixelPosition( Vector2( tf.m_Translation ) );
		}
		m_packedSystems.push_back( m_socketSystem.GetPackedSystem() );
		m_systemIDs.Push( m_socketSystem.GetPackedSystem()->GetId() );
	}
	return true;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Builds Enlighten pre-clusters for all systems into Enlighten systems. Also do the same for
//   portal socket system.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   prog - progress log object
// Return Value:
//   true If successfully built pre-clusters for all systems
//   false If there was an error when building pre-clusters
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::BuildSystemPreClusters( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog )
{
	// Merge the packed systems from our systems and neighboring cells into one big list
	std::vector<Enlighten::IPrecompPackedSystem*> m_mergedSystems;
	m_mergedSystems.insert( m_mergedSystems.end(), m_packedSystems.begin(), m_packedSystems.end() );
	m_mergedSystems.insert( m_mergedSystems.end(), m_packedSystemsNeighbors.begin(), m_packedSystemsNeighbors.end() );

	unsigned int systemInCellIdx = 0;
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it, ++systemInCellIdx )
	{
		(*it)->BuildEnlightenPreClustering( pPrecompute, m_mergedSystems, prog  );
		Enlighten::IPrecompSystemPreClustering* systemPreClustering = (*it)->GetSystemPreClustering();
		if( !systemPreClustering )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "BuildEnlightenPreClustering() failed to get system preclustering from system %i.", systemInCellIdx );
			return false;
		}
		m_systemPreClusters.push_back( systemPreClustering );
	}

	if( m_socketSystem.GetPackedSystem() )
	{
		std::vector<Enlighten::IPrecompPackedSystem*> neighbours( 1, m_socketSystem.GetPackedSystem() );
		m_socketSystem.BuildPreClustering( pPrecompute, neighbours, prog );
		m_systemPreClusters.push_back( m_socketSystem.GetSystemPreClustering() );
	}

	return true;
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Builds Enlighten clusters for all systems into Enlighten systems. Also do the same for
//   portal socket system.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   prog - progress log object
// Return Value:
//   true If successfully built clusters for all systems
//   false If there was an error when building clusters
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::BuildSystemClusters( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog )
{
	// Merge the packed systems from our systems and neighboring cells into one big list
	std::vector<Enlighten::IPrecompSystemPreClustering*> mergedPreClusters;
	mergedPreClusters.insert( mergedPreClusters.end(), m_systemPreClusters.begin(), m_systemPreClusters.end() );
	mergedPreClusters.insert( mergedPreClusters.end(), m_systemPreClustersNeighbors.begin(), m_systemPreClustersNeighbors.end() );

	unsigned int systemInCellIdx = 0;
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it, ++systemInCellIdx )
	{
		(*it)->BuildEnlightenClustering( pPrecompute, mergedPreClusters, prog  );
		Enlighten::IPrecompSystemClustering* systemClustering = (*it)->GetSystemClustering();
		if( !systemClustering )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "BuildEnlightenClustering() failed to get system clustering from system %i.", systemInCellIdx );
			return false;
		}
		m_systemClusters.push_back( systemClustering );
	}

	if( m_socketSystem.GetPackedSystem() )
	{
		std::vector<Enlighten::IPrecompSystemPreClustering*> neighbours( 1, m_socketSystem.GetSystemPreClustering() );
		m_socketSystem.BuildClustering( pPrecompute, neighbours, prog );
		m_systemClusters.push_back( m_socketSystem.GetSystemClustering() );
	}

	return true;
}
#endif

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::DeleteSampleVolumes()
{
	m_updateTaskInfo.WaitToComplete();
	m_updateTaskInfo.InvalidateResult();

	for ( std::vector<SampleVolume>::iterator it = m_sampleVolumes.begin(); it != m_sampleVolumes.end(); ++it )
	{
		DeleteSampleVolume( *it );
	}
	m_sampleVolumes.clear();
	DeleteSampleVolume( m_portalSocketSamples, m_portalSocketSamples.m_samples.GetSize() );
	m_portalSocketIndexes.clear();

	m_defaultProbeVolume.SetValidProbes( false );
	for( PTr2InteriorProbeVolumeVector::iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it )
	{
		(*it)->SetValidProbes( false );
	}
	for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
	{
		(*it)->SetHasValidProbe( false );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::DeleteSampleVolume( SampleVolume& volume, int pointerCount )
{
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	if( volume.m_precomputedProbeSet )
	{
		volume.m_precomputedProbeSet->Release();
		volume.m_precomputedProbeSet = NULL;
	}
#endif

	if( volume.m_volumeBox )
	{
		using namespace GeoEngine;
		GEO_DELETE(NonAAVolume, volume.m_volumeBox);
	}

	if( volume.m_task.m_InputLighting )
	{
		GEO_DELETE_ARRAY( const Enlighten::InputLightingBuffer*, volume.m_task.m_InputLighting );
		volume.m_task.m_InputLighting = NULL;
	}

	if( volume.m_task.m_IndicesToSolve )
	{
		GEO_DELETE_ARRAY( Geo::s32, volume.m_task.m_IndicesToSolve );
		volume.m_task.m_IndicesToSolve = NULL;
	}

	if( volume.m_task.m_OutputPointers )
	{
		for( Geo::s32 i=0; i < pointerCount; ++i )
		{
			GEO_DELETE_ARRAY( float, volume.m_task.m_OutputPointers[i] );
		}
		GEO_DELETE_ARRAY( float*, volume.m_task.m_OutputPointers );
		volume.m_task.m_OutputPointers = NULL;
	}

	if( volume.m_debugProbeLighting )
	{
		delete []volume.m_debugProbeLighting;
		volume.m_debugProbeLighting = NULL;
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::CreateSampleVolume( const Tr2InteriorProbeVolume &volume)
{
	SampleVolume result;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	result.m_precomputedProbeSet = NULL;
#endif

	result.m_debugProbeLighting = NULL;

	Vector3 axis = volume.GetScaling();
	Vector3 position = Vector3( -0.5f, -0.5f, -0.5f );
	result.m_transform = volume.GetTransform();
	D3DXVec3TransformCoord( &position, &position, &result.m_transform );
 	Geo::v128 pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 0.0f );

	Geo::v128 s = GEO_VCONSTRUCT( axis.x, axis.y, axis.z, 0.0f );

	Vector3 x( 1.0f, 0.0f, 0.0f );
	Vector3 y( 0.0f, 1.0f, 0.0f );
	Vector3 z( 0.0f, 0.0f, 1.0f );
	D3DXVec3TransformNormal( &x, &x, &result.m_transform );
	D3DXVec3TransformNormal( &y, &y, &result.m_transform );
	D3DXVec3TransformNormal( &z, &z, &result.m_transform );
	x /= axis.x;
	y /= axis.y;
	z /= axis.z;

	Geo::v128 mat[3];
	mat[0] = GEO_VCONSTRUCT( x.x, x.y, x.z, 0.0f );
	mat[1] = GEO_VCONSTRUCT( y.x, y.y, y.z, 0.0f );
	mat[2] = GEO_VCONSTRUCT( z.x, z.y, z.z, 0.0f );

	GeoEngine::NonAABoundingBox boundingBox;
	boundingBox.Initialise( mat, pos, s );

	result.m_volumeBox = GEO_NEW(GeoEngine::NonAAVolume);
	result.m_volumeBox->Initialise( &boundingBox, volume.GetResolutionX(), volume.GetResolutionY(), volume.GetResolutionZ() );

	for (std::vector<Vector3>::const_iterator it = volume.m_lightProbes.begin(); it != volume.m_lightProbes.end(); ++it)
	{
		result.m_samples.Push( GEO_VCONSTRUCT( it->x, it->y, it->z, 1.0f ) );
	}

	result.m_task.m_CoreProbeSet = NULL;

	m_sampleVolumes.push_back( result );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::CreateSampleVolume( int resX, int resY, int resZ, const Matrix &transform, Enlighten::RadProbeSetCore* core )
{
	SampleVolume result;

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
	result.m_precomputedProbeSet = NULL;
#endif
	result.m_debugProbeLighting = NULL;

	Vector3 position = Vector3( -0.5f, -0.5f, -0.5f );
	result.m_transform = transform;
	D3DXVec3TransformCoord( &position, &position, &result.m_transform );
	Geo::v128 pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 0.0f );

	Vector3 x( 1.0f, 0.0f, 0.0f );
	Vector3 y( 0.0f, 1.0f, 0.0f );
	Vector3 z( 0.0f, 0.0f, 1.0f );
	D3DXVec3TransformNormal( &x, &x, &result.m_transform );
	D3DXVec3TransformNormal( &y, &y, &result.m_transform );
	D3DXVec3TransformNormal( &z, &z, &result.m_transform );
	Vector3 axis;
	axis.x = D3DXVec3Length( &x );
	axis.y = D3DXVec3Length( &y );
	axis.z = D3DXVec3Length( &z );
	x /= axis.x;
	y /= axis.y;
	z /= axis.z;
	Geo::v128 s = GEO_VCONSTRUCT( axis.x, axis.y, axis.z, 0.0f );

	Geo::v128 mat[3];
	mat[0] = GEO_VCONSTRUCT( x.x, x.y, x.z, 0.0f );
	mat[1] = GEO_VCONSTRUCT( y.x, y.y, y.z, 0.0f );
	mat[2] = GEO_VCONSTRUCT( z.x, z.y, z.z, 0.0f );

	GeoEngine::NonAABoundingBox boundingBox;
	boundingBox.Initialise( mat, pos, s );

	result.m_volumeBox = GEO_NEW(GeoEngine::NonAAVolume);
	result.m_volumeBox->Initialise( &boundingBox, resX, resY, resZ );

	for( int z = 0; z < resZ; ++z )
	{
		for( int y = 0; y < resY; ++y )
		{
			for( int x = 0; x < resX; ++x )
			{
				// Get the sample location in unit-cube space.
				Geo::v128 sample = GEO_VCONSTRUCT( 
					(float( x ) + 0.5f) / float( resX ),
					(float( y ) + 0.5f) / float( resY ),
					(float( z ) + 0.5f) / float( resZ ),
					1.0f );
				// Convert from unit cube to worldspace coordinates, and store.
				sample = result.m_volumeBox->MapFromLocalUnitCube( sample );
				result.m_samples.Push( sample );
			}
		}
	}

	CreateProbeTask( result, core );
	result.m_task.m_Environment = m_enlightenEnvironment;

	m_sampleVolumes.push_back( result );
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::CanBuildLightProbes() const
{
	return m_systemClusters.size() > 0 && m_systemIDs.GetSize() > 0;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::BuildLightProbes( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog )
{
	DeleteSampleVolumes();
	CreateSampleVolume( m_defaultProbeVolume );
	for (PTr2InteriorProbeVolumeVector::iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it)
	{
		CreateSampleVolume( *(*it) );
	}

	for ( std::vector<SampleVolume>::iterator it = m_sampleVolumes.begin(); it != m_sampleVolumes.end(); ++it )
	{
		BuildLightProbe( pPrecompute, prog, *it, m_systemIDs.Begin(), m_systemIDs.End(), &m_systemClusters[0], (int)m_systemClusters.size() );
		// Finally, create the probe task
		if( it->m_precomputedProbeSet == NULL )
		{
			CCP_LOGWARN( "BuildLightProbes: probe volume has no probe set" );
		}
		else
		{
			CreateProbeTask( *it, it->m_precomputedProbeSet->GetProbeSetCore() );
		}
		it->m_task.m_Environment = m_enlightenEnvironment;
	}

	m_defaultProbeVolume.SetValidProbes( true );
	for( PTr2InteriorProbeVolumeVector::iterator it = m_probeVolumes.begin(); it != m_probeVolumes.end(); ++it )
	{
		(*it)->SetValidProbes( true );
	}

	// Build portal samples
	m_portalSocketIndexes.clear();

	m_portalSocketSamples.m_samples.Resize( 0 );
	for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
	{
		Geo::v128 position;
		(*it)->GetInputProbePosition( position );
		m_portalSocketSamples.m_samples.Push( position );
		m_portalSocketIndexes[*it] = m_portalSocketSamples.m_samples.GetSize() - 1;
	}
	if( !m_portalSocketSamples.m_samples.IsEmpty() )
	{
		BuildLightProbe( pPrecompute, 
				 prog, 
				 m_portalSocketSamples, 
				 m_systemIDs.Begin(), 
				 m_systemIDs.End() - 1, // Ignore the last system with socket geometry (http://core/wiki/Tr2InteriorPortalSocket#Recursive_light_bouncing)
				 &m_systemClusters[0], 
				 (int)m_systemClusters.size() - 1 );

		if( m_portalSocketSamples.m_precomputedProbeSet == NULL )
		{
			CCP_LOGWARN( "BuildLightProbes: portal socket samples has no probe set" );
		}
		else
		{
			CreateProbeTask( m_portalSocketSamples, m_portalSocketSamples.m_precomputedProbeSet->GetProbeSetCore(), m_portalSocketSamples.m_samples.GetSize() );
		}

		for( int j = 0; j < m_portalSocketSamples.m_task.m_NumIndicesToSolve; ++j )
		{
			m_portalSocketSamples.m_task.m_IndicesToSolve[j] = j;
		}
		for( PTr2InteriorPortalSocketVector::const_iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
		{
			(*it)->SetHasValidProbe( true );
		}
	}
}

// -------------------------------------------------------------
// Description:
//   Builds (compiles) a single SH probe volume.
// Arguments:
//   pPrecompute - Enlighten precompute object
//   prog - Progress notification object
//   volume - The volume to be computed
//   systemIDBegin - Pointer to the first system ID in the list
//       of neighbor system IDs
//   systemIDBegin - Pointer to the end of the list
//       of neighbor system IDs
//   clusterings - Pointer to the array of neighbor clusters
//   numClusterings - Number of elements in the array of neighbor clusterss
// -------------------------------------------------------------
void Tr2InteriorCell::BuildLightProbe( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog, SampleVolume& volume, const Geo::GeoGuid* systemIDBegin, const Geo::GeoGuid* systemIDEnd, const Enlighten::IPrecompSystemClustering* const* clusterings, int numClusterings )
{
	// Create the light probe input structure
	Enlighten::IPrecompInputProbeSet* pProbeInput = Enlighten::IPrecompInputProbeSet::Create();

	Enlighten::IPrecompProbeSetBuildParameters* params = pProbeInput->GetBuildParameters();

	// Use L2 Light probes (9 coefficients)
	// L1 Light probes are smaller (4 coeffs) & faster, but the pre-compute time is about the same
	// L1 probes are easier to pack into a texture (you can test L1 by putting coeffs 5-9 to zero in an L2 shader)
	// L0 non-directional ambient is the first coefficient (might be useful for particle systems)
	params->SetNumSHCoefficients( Enlighten::SH_ORDER_L2 );

	// Determines if the probe is culled when it sees back-facing polygons
	params->SetUseCulling( true );

	// Threshold value for probe culling. If the probe visibility is less than this value the probe is rejected. 
	// Probe visibility is the proportion of valid (i.e. front-facing) geometry seen by the probe, 
	// where 0 means no valid geometry was seen and 1 means only valid geometry was seen. 
	// A lower threshold means more probes are culled. 
	// The threshold is compared against the visibility of the probe and if it is lower the probe is culled. 
	// A threshold of, say, 0.90 will cull probes that sees 10% backfaces or more. 
	// Probe culling does not affect pre-compute time
	// params->SetCullingThreshold( 0.96 ); // Default

	// Other build parameters are roughly equivalent to those in Tr2InteriorEnlightenSystem::PackEnlightenSystem

	// Populate the input structure
	pProbeInput->SetId( Geo::GeoGuid::Create( 1, TriGeometryRes::s_currentEnlightenGeometryGuid++ ) );
	pProbeInput->AddProbePositions( volume.m_samples.Begin(), volume.m_samples.End() );
	pProbeInput->AddSystemIDs( systemIDBegin,  systemIDEnd );

	Enlighten::IPrecompOutputProbeSet* outputProbeSet = Enlighten::IPrecompOutputProbeSet::Create();

	// Now precompute the light probes
	pPrecompute->CreateProbeSet( 
		pProbeInput, 
		clusterings, 
		numClusterings,
		&prog, 
		outputProbeSet );

	pPrecompute->CompileProbeSet(
		outputProbeSet,
		Geo::GEO_PLATFORM_WINDOWS,
		&prog,
		volume.m_precomputedProbeSet
		);

	pProbeInput->Release();
	outputProbeSet->Release();
}
#endif

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::CreateProbeTask( SampleVolume& volume, const Enlighten::RadProbeSetCore* core, int solveCount )
{
	volume.m_task.m_CoreProbeSet = core;

	volume.m_task.m_NumIndicesToSolve = solveCount;
	volume.m_task.m_IndicesToSolve = GEO_NEW_ARRAY( Geo::s32, solveCount );

	CCP_ASSERT( NULL != volume.m_task.m_IndicesToSolve );

	volume.m_task.m_OutputPointers = GEO_NEW_ARRAY(float*, solveCount);
	CCP_ASSERT( NULL != volume.m_task.m_OutputPointers );
	for( Geo::s32 i=0; i < solveCount; ++i )
	{
		volume.m_task.m_OutputPointers[i] = GEO_NEW_ARRAY( float, /*RGB*/3*Enlighten::SH_ORDER_L2 );
		CCP_ASSERT( NULL != volume.m_task.m_OutputPointers[i] );
	}
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::PopulateProbeVolumes()
{
	if (m_sampleVolumes.empty())
	{
		return false;
	}
	m_probeVolumes.Clear();
	m_defaultProbeVolume.BuildLightVolume( 
		m_sampleVolumes[0].m_volumeBox->GetXRes(), 
		m_sampleVolumes[0].m_volumeBox->GetYRes(), 
		m_sampleVolumes[0].m_volumeBox->GetZRes() );
	m_defaultProbeVolume.SetValidProbes( true );
	for (size_t i = 1; i < m_sampleVolumes.size(); ++i)
	{
		Tr2InteriorProbeVolumePtr pv;
		pv.CreateInstance();
		pv->SetTransform( m_sampleVolumes[i].m_transform );
		pv->BuildLightVolume( 
			m_sampleVolumes[i].m_volumeBox->GetXRes(), 
			m_sampleVolumes[i].m_volumeBox->GetYRes(), 
			m_sampleVolumes[i].m_volumeBox->GetZRes() );
		pv->SetValidProbes( true );
		m_probeVolumes.Append( pv );
	}
	return true;
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::BuildEnlightenSolution( Enlighten::IPrecompute* pPrecompute, TriEnlightenProgressBar& prog )
{
	m_updateTaskInfo.WaitToComplete();
	m_updateTaskInfo.InvalidateResult();

	// Merge the packed systems from our systems and neighboring cells into one big list
	std::vector<Enlighten::IPrecompPackedSystem*> m_mergedSystems;
	m_mergedSystems.insert( m_mergedSystems.end(), m_packedSystems.begin(), m_packedSystems.end() );
	m_mergedSystems.insert( m_mergedSystems.end(), m_packedSystemsNeighbors.begin(), m_packedSystemsNeighbors.end() );

	// Merge the system clusters from our systems and neighboring cells into one big list
	std::vector<Enlighten::IPrecompSystemClustering*> m_mergedClusters;
	m_mergedClusters.insert( m_mergedClusters.end(), m_systemClusters.begin(), m_systemClusters.end() );
	m_mergedClusters.insert( m_mergedClusters.end(), m_systemClustersNeighbors.begin(), m_systemClustersNeighbors.end() );

	// Do the build
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->BuildEnlightenSystem( pPrecompute, m_mergedSystems, m_mergedClusters, prog );
	}

	if( m_socketSystem.GetPackedSystem() )
	{
		std::vector<Enlighten::IPrecompPackedSystem*> neighbours( 1, m_socketSystem.GetPackedSystem() );
		std::vector<Enlighten::IPrecompSystemClustering*> clusters( 1, m_socketSystem.GetSystemClustering() );
		m_socketSystem.BuildEnlightenSystem( pPrecompute, neighbours, clusters, prog );
	}
	// Clean up all the scratch-pad storage
	m_packedSystems.clear();
	m_packedSystemsNeighbors.clear();
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Deletes all temporary data used during Enlighten precompute for all systems and a portal socket
//   system.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::DeletePrecompData()
{
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->DeletePrecompData();
	}
	m_socketSystem.DeletePrecompData();
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Gathers packed systems from cell's neighbors. Used during Enlighten precompute.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GatherPackedSystems( void )
{
	for( std::map<Tr2InteriorCellPtr, int>::const_iterator it = m_cellNeighbors.begin(); it != m_cellNeighbors.end(); ++it )
	{
		it->first->GetPackedSystems( m_packedSystemsNeighbors );
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Gathers built pre-clusters from cell's neighbors. Used during Enlighten precompute.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GatherSystemPreClusters( void )
{
	for( std::map<Tr2InteriorCellPtr, int>::const_iterator it = m_cellNeighbors.begin(); it != m_cellNeighbors.end(); ++it )
	{
		it->first->GetSystemPreClusters( m_systemPreClustersNeighbors );
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Gathers built clusters from cell's neighbors. Used during Enlighten precompute.
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GatherSystemClusters( void )
{
	for( std::map<Tr2InteriorCellPtr, int>::const_iterator it = m_cellNeighbors.begin(); it != m_cellNeighbors.end(); ++it )
	{
		it->first->GetSystemClusters( m_systemClustersNeighbors );
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns a vector of packed systems for all cell's systems. Used during Enlighten precompute.
// Arguments:
//   packedSystems (out) - vector of all cell's packed systems
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GetPackedSystems( std::vector<Enlighten::IPrecompPackedSystem*>& packedSystems ) const
{
	for( std::vector<Enlighten::IPrecompPackedSystem*>::const_iterator it = m_packedSystems.begin(); it != m_packedSystems.end(); ++it )
	{
		packedSystems.push_back( *it );
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns a vector of built pre-clusters for all cell's systems. Used during Enlighten precompute.
// Arguments:
//   systemPreClusters (out) - vector of all cell's built pre-clusters
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GetSystemPreClusters( std::vector<Enlighten::IPrecompSystemPreClustering*>& systemPreClusters ) const
{
	for( std::vector<Enlighten::IPrecompSystemPreClustering*>::const_iterator it = m_systemPreClusters.begin(); it != m_systemPreClusters.end(); ++it )
	{
		systemPreClusters.push_back( *it );
	}
}

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns a vector of built clusters for all cell's systems. Used during Enlighten precompute.
// Arguments:
//   systemClusters (out) - vector of all cell's built clusters
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GetSystemClusters( std::vector<Enlighten::IPrecompSystemClustering*>& systemClusters ) const
{
	for( std::vector<Enlighten::IPrecompSystemClustering*>::const_iterator it = m_systemClusters.begin(); it != m_systemClusters.end(); ++it )
	{
		systemClusters.push_back( *it );
	}
}
#endif

// ------------------------------------------------------------------------------------------------------
// Description:
//   Returns a vector of input lighting buffers for all cell's systems. Used during Enlighten update.
// Arguments:
//   inputLightingBuffers (out) - vector of all cell's input lighting buffers
// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GetInputLightingBuffers( std::vector<const Enlighten::InputLightingBuffer*>& inputLightingBuffers ) const
{
	for( std::vector<const Enlighten::InputLightingBuffer*>::const_iterator it = m_inputLightingBuffers.begin(); it != m_inputLightingBuffers.end(); ++it )
	{
		inputLightingBuffers.push_back( *it );
	}
}

#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::BuildEnlightenSystems()
{
	return BuildEnlightenSystemsImpl( Tr2InteriorEnlightenSystemImpl::QUALITY_RELEASE );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::PreviewEnlightenSystems()
{
	return BuildEnlightenSystemsImpl( Tr2InteriorEnlightenSystemImpl::QUALITY_PREVIEW );
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::BuildEnlightenSystemsImpl( Tr2InteriorEnlightenSystemImpl::Quality quality )
{
	m_updateTaskInfo.WaitToComplete();
	m_updateTaskInfo.InvalidateResult();

	// CCP_LOG 'progress bar'
	TriEnlightenProgressBar prog;

	Enlighten::IPrecompute* pPrecompute = Enlighten::CreatePrecompute();
	ON_BLOCK_EXIT( &Enlighten::IPrecompute::Release, pPrecompute );// Can't use CComPtr

	if( g_outputEnlightenDebugBuildInfo )
	{
		_mkdir( "C:\\EnlightenDebug" );
		CCP_LOGWARN_CH( g_enlightenBuildChannel, "Dumping Enlighten Precompute state to C:\\EnlightenDebug");
		pPrecompute->SetStateDumpFolder( L"C:\\EnlightenDebug" );
		pPrecompute->SetStateDump( pPrecompute->esdAll );
	}

	m_packedSystems.clear();
	m_packedSystemsNeighbors.clear();
	m_systemPreClusters.clear();
	m_systemPreClustersNeighbors.clear();
	m_systemClusters.clear();
	m_systemClustersNeighbors.clear();
	m_systemIDs.Empty();

	// Pack all the Systems
	unsigned int systemInCellIdx = 0;
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it, ++systemInCellIdx )
	{
		(*it)->PackEnlightenSystem( quality, pPrecompute, prog, systemInCellIdx );
		Enlighten::IPrecompPackedSystem* packedSystem = (*it)->GetPackedSystem();
		if( !packedSystem )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "BuildEnlightenSystems() failed to pack system %i. Halting system build.", systemInCellIdx );
			return false;
		}
		m_packedSystems.push_back( packedSystem );
		m_systemIDs.Push( packedSystem->GetId() );
	}
	if( m_socketSystem.PackEnlightenSystem( Tr2InteriorEnlightenSystemImpl::QUALITY_RELEASE, pPrecompute, prog, -1, this ) )
	{
		m_packedSystems.push_back( m_socketSystem.GetPackedSystem() );
		m_systemIDs.Push( m_socketSystem.GetPackedSystem()->GetId() );
	}

	// Build all the system preclusters
	systemInCellIdx = 0;
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it, ++systemInCellIdx )
	{
		(*it)->BuildEnlightenPreClustering( pPrecompute, m_packedSystems, prog  );
		Enlighten::IPrecompSystemPreClustering* systemPreClustering = (*it)->GetSystemPreClustering();
		if( !systemPreClustering )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "BuildEnlightenSystems() failed to get system preclustering from system %i.", systemInCellIdx );
			return false;
		}
		m_systemPreClusters.push_back( systemPreClustering );
	}
	if( m_socketSystem.GetPackedSystem() )
	{
		std::vector<Enlighten::IPrecompPackedSystem*> neighbours( 1, m_socketSystem.GetPackedSystem() );
		m_socketSystem.BuildPreClustering( pPrecompute, neighbours, prog );
		m_systemPreClusters.push_back( m_socketSystem.GetSystemPreClustering() );
	}

	// Build all the system clusters
	systemInCellIdx = 0;
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it, ++systemInCellIdx )
	{
		(*it)->BuildEnlightenClustering( pPrecompute, m_systemPreClusters, prog  );
		Enlighten::IPrecompSystemClustering* systemClustering = (*it)->GetSystemClustering();
		if( !systemClustering )
		{
			CCP_LOGERR_CH( g_enlightenBuildChannel, "BuildEnlightenSystems() failed to get system clustering from system %i.", systemInCellIdx );
			return false;
		}
		m_systemClusters.push_back( systemClustering );
	}
	if( m_socketSystem.GetPackedSystem() )
	{
		std::vector<Enlighten::IPrecompSystemPreClustering*> neighbours( 1, m_socketSystem.GetSystemPreClustering() );
		m_socketSystem.BuildClustering( pPrecompute, neighbours, prog );
		m_systemClusters.push_back( m_socketSystem.GetSystemClustering() );
	}

	// Build the light probe system for the volume
	BuildLightProbes( pPrecompute, prog );

	// Finally, do the build
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->BuildEnlightenSystem( pPrecompute, m_packedSystems, m_systemClusters, prog );
	}
	if( m_socketSystem.GetPackedSystem() )
	{
		std::vector<Enlighten::IPrecompPackedSystem*> neighbours( 1, m_socketSystem.GetPackedSystem() );
		std::vector<Enlighten::IPrecompSystemClustering*> clusters( 1, m_socketSystem.GetSystemClustering() );
		m_socketSystem.BuildEnlightenSystem( pPrecompute, neighbours, clusters, prog );
	}

	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->DeletePrecompData();
	}
	m_socketSystem.DeletePrecompData();

	return true;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SaveLightProbes()
{
	if( !m_shProbeResPath.empty() )
	{
		if( !WriteSHProbeDataToDisk())
		{
			CCP_LOGERR( "Could not write out the spherical harmonic data" );	
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SaveEnlightenSystems()
{
	if( !m_shProbeResPath.empty() )
	{
		if( !WriteSHProbeDataToDisk())
		{
			CCP_LOGERR( "Could not write out the spherical harmonic data" );	
		}
	}

	// Finally, do the build
	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		(*it)->SaveEnlightenSystem();
	}

	std::vector<const TriGeometryRes*> geometries;
	if( m_socketSystem.GetRadSystem() )
	{
		m_socketSystem.SaveEnlightenSystem( geometries );
	}
}
#endif

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetSHProbeResource()
{
	if( m_shProbeResource )
	{
		m_shProbeResource->RemoveNotifyTarget( this );
		m_shProbeResource.Unlock();
	}

	if( !m_shProbeResPath.empty() )
	{
		BeResMan->GetResource( m_shProbeResPath.c_str(), "", m_shProbeResource );
	}

	if( m_shProbeResource )
	{
		m_shProbeResource->AddNotifyTarget( this );
	}	
}
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::WriteSHProbeDataToDisk()
{
	std::wstring shProbeResPathW = CA2W( m_shProbeResPath.c_str() );
	std::wstring fullPath = BePaths->ResolvePathW( shProbeResPathW );
	if( !m_sampleVolumes.empty() )
	{
		if( m_sampleVolumes[0].m_precomputedProbeSet == NULL )
		{
			CCP_LOGERR( "Tr2InteriorCell::WriteSHProbeDataToDisk. No valid probe data to write for default probe volume\n" );	
			return false;
		}
		for (size_t i = 1; i < m_sampleVolumes.size(); ++i)
		{
			if( m_sampleVolumes[i].m_precomputedProbeSet == NULL )
			{
				CCP_LOGERR( "Tr2InteriorCell::WriteSHProbeDataToDisk. No valid probe data to write for default additional probe volume %i\n", i );	
				return false;
			}
		}

		Geo::GeoFileStream stream = Geo::GeoFileStream();
		if ( !stream.Open( fullPath.c_str(), Geo::GeoFileStream::esmWrite ) )
		{
			CCP_LOGERR( "Tr2InteriorCell::WriteSHProbeDataToDisk. Could not open %S for writing\n", fullPath.c_str() );	
			return false;
		}
		else
		{
			if (!Tr2SHProbeRes::WriteSHProbeSetToDisk( 
				m_sampleVolumes[0].m_precomputedProbeSet->GetProbeSetCore(),
				stream,
				m_sampleVolumes[0].m_volumeBox->GetXRes(),
				m_sampleVolumes[0].m_volumeBox->GetYRes(),
				m_sampleVolumes[0].m_volumeBox->GetZRes() ))
			{
				return false;
			}
			for (size_t i = 1; i < m_sampleVolumes.size(); ++i)
			{
				stream.Write( m_sampleVolumes[i].m_transform, sizeof(Matrix), 1 );
				if (!Tr2SHProbeRes::WriteSHProbeSetToDisk( 
					m_sampleVolumes[i].m_precomputedProbeSet->GetProbeSetCore(),
					stream,
					m_sampleVolumes[i].m_volumeBox->GetXRes(),
					m_sampleVolumes[i].m_volumeBox->GetYRes(),
					m_sampleVolumes[i].m_volumeBox->GetZRes() ))
				{
					return false;
				}
			}
			if( m_portalSocketSamples.m_precomputedProbeSet )
			{
				Matrix transform;
				ZeroMemory( &transform, sizeof( transform ) );
				stream.Write( transform, sizeof(Matrix), 1 );
				if( !Tr2SHProbeRes::WriteSHProbeSetToDisk( 
					m_portalSocketSamples.m_precomputedProbeSet->GetProbeSetCore(),
					stream,
					0,
					0,
					0 ) )
				{
					return false;
				}
			}
			return true;
		}
	}
	return false;
}
#endif

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::GetLightProbeResolution(int* x, int* y, int* z) const
{
	*x = m_defaultProbeVolume.GetResolutionX();
	*y = m_defaultProbeVolume.GetResolutionY();
	*z = m_defaultProbeVolume.GetResolutionZ();
}

// ------------------------------------------------------------------------------------------------------
bool Tr2InteriorCell::IsVolumeReady() const
{
	return m_boundingBoxReady;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::BuildLightVolume( int xRes, int yRes, int zRes )
{
	m_defaultProbeVolume.BuildLightVolume( xRes, yRes, zRes );
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::DetermineVisibility( int curDepth )
{
	// We've already determined the depth of this cell, so bail out
	if( curDepth > m_cellDepth && m_cellDepth != -1 )
		return;

	// Set the cell depth (minimum number of portal traversals from a visible cell)
	m_cellDepth = curDepth;

	// Set the depth for all our neighbors
	for( std::map<Tr2InteriorCellPtr, int>::iterator it = m_cellNeighbors.begin(); it != m_cellNeighbors.end(); ++it )
	{
		(*it).first->DetermineVisibility( curDepth + 1 );
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetVisibility( bool bVisible )
{
	m_isVisible = bVisible;
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::AddCellNeighbor( Tr2InteriorCell* neighbor )
{
	if( !neighbor )
	{
		CCP_LOGERR( "Attempted to add a NULL cell neighbor" );
		return;
	}

	if( m_cellNeighbors.find( neighbor ) == m_cellNeighbors.end() )
	{
		m_cellNeighbors[neighbor] = 1;
	}
	else
	{
		++m_cellNeighbors[neighbor];
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RemoveCellNeighbor( Tr2InteriorCell* neighbor )
{
	if( !neighbor )
	{
		CCP_LOGERR( "Attempted to remove a NULL cell neighbor" );
		return;
	}

	if( m_cellNeighbors.find( neighbor ) == m_cellNeighbors.end() )
	{
		CCP_LOGWARN( "Attempted to remove a cell which is not a neighbor" );
	}
	else
	{
		--m_cellNeighbors[neighbor];
		if( m_cellNeighbors[neighbor] == 0 )
		{
			m_cellNeighbors.erase( neighbor );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::RebuildBoundingBox( void )
{
	Vector3 oldMinBounds = m_minBounds;
	Vector3 oldMaxBounds = m_maxBounds;

	m_minBounds = Vector3( FLT_MAX, FLT_MAX, FLT_MAX );
	m_maxBounds = Vector3( -FLT_MAX, -FLT_MAX, -FLT_MAX );
	
	m_boundingBoxReady = false;

	// add all sub-cells
	if( m_systems.empty() )
	{
		return;
	}

	for( PTr2InteriorEnlightenSystemVector::const_iterator it = m_systems.begin(); it != m_systems.end(); ++it )
	{
		// add sub-cell's boundingbox to this one
		Vector3 mn, mx;
		// make sure, sub-cell's boundingbox is up-to-date
		if( (*it)->GetBoundingBox( mn, mx ) )
		{
			BoundingBoxUpdate( m_minBounds, m_maxBounds, mn, mx );
			m_boundingBoxReady = true;
		}
	}


	if( ( oldMinBounds != m_minBounds || oldMaxBounds != m_maxBounds ) && 
		abs( m_minBounds.x ) != FLT_MAX )
	{
		Vector3 center = (m_minBounds + m_maxBounds) / 2.0f;
		Vector3 scale = m_maxBounds - m_minBounds;
		m_defaultProbeVolume.SetPosition( center );
		m_defaultProbeVolume.SetScaling( scale );

		if( !m_sampleVolumes.empty() )
		{
			Matrix transform(
				scale.x, 0.0f, 0.0f, 0.0f,
				0.0f, scale.y, 0.0f, 0.0f,
				0.0f, 0.0f, scale.z, 0.0f,
				center.x, center.y, center.z, 1.0f );
			Vector3 position = Vector3( -0.5f, -0.5f, -0.5f );
			m_sampleVolumes[0].m_transform = transform;
			D3DXVec3TransformCoord( &position, &position, &m_sampleVolumes[0].m_transform );
			Geo::v128 pos = GEO_VCONSTRUCT( position.x, position.y, position.z, 0.0f );

			Vector3 x( 1.0f, 0.0f, 0.0f );
			Vector3 y( 0.0f, 1.0f, 0.0f );
			Vector3 z( 0.0f, 0.0f, 1.0f );
			D3DXVec3TransformNormal( &x, &x, &m_sampleVolumes[0].m_transform );
			D3DXVec3TransformNormal( &y, &y, &m_sampleVolumes[0].m_transform );
			D3DXVec3TransformNormal( &z, &z, &m_sampleVolumes[0].m_transform );
			Vector3 axis;
			axis.x = D3DXVec3Length( &x );
			axis.y = D3DXVec3Length( &y );
			axis.z = D3DXVec3Length( &z );
			x /= axis.x;
			y /= axis.y;
			z /= axis.z;
			Geo::v128 s = GEO_VCONSTRUCT( axis.x, axis.y, axis.z, 0.0f );

			Geo::v128 mat[3];
			mat[0] = GEO_VCONSTRUCT( x.x, x.y, x.z, 0.0f );
			mat[1] = GEO_VCONSTRUCT( y.x, y.y, y.z, 0.0f );
			mat[2] = GEO_VCONSTRUCT( z.x, z.y, z.z, 0.0f );

			static_cast<GeoEngine::NonAABoundingBox*>( m_sampleVolumes[0].m_volumeBox )->Initialise( mat, pos, s );
		}
	}
}

// ------------------------------------------------------------------------------------------------------
int Tr2InteriorCell::GetDefaultProbeVolumeResolutionX() const
{
	return m_defaultProbeVolume.GetResolutionX();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetDefaultProbeVolumeResolutionX(int res)
{
	m_defaultProbeVolume.SetResolutionX( res );
}

// ------------------------------------------------------------------------------------------------------
int Tr2InteriorCell::GetDefaultProbeVolumeResolutionY() const
{
	return m_defaultProbeVolume.GetResolutionY();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetDefaultProbeVolumeResolutionY(int res)
{
	m_defaultProbeVolume.SetResolutionY( res );
}

// ------------------------------------------------------------------------------------------------------
int Tr2InteriorCell::GetDefaultProbeVolumeResolutionZ() const
{
	return m_defaultProbeVolume.GetResolutionZ();
}

// ------------------------------------------------------------------------------------------------------
void Tr2InteriorCell::SetDefaultProbeVolumeResolutionZ(int res)
{
	m_defaultProbeVolume.SetResolutionZ( res );
}
#if defined(ENLIGHTEN_PRECOMPUTE_ENABLED)
// -------------------------------------------------------------
// Description:
//   Fills a list of portal socket packed geometries. 
//   Called by m_enlightenSystem.PackEnlightenSystem.
// Arguments:
//   pPrecompute - pointer to Enlighten precompute object
//   inputSystem - Enlighten input system
//   outputPixelSize - Enlighten map pixel size (in meters)
//   geometries (out) - List of packed geometries for the system
// Return Value:
//   true If successfully filled the list (has portal sockets)
//   false If there are not portal sockets in this cell and the 
//         the Enlighten build should be aborted
// -------------------------------------------------------------
bool Tr2InteriorCell::GetEnlightenPackedGeometry( Enlighten::IPrecompute* precompute, 
												  Enlighten::IPrecompInputSystem* inputSystem, 
												  TriEnlightenProgressBar& prog, 
												  float outputPixelSize, 
												  std::vector<Enlighten::IPrecompPackedGeometry*>& geometries )
{
	unsigned int instanceIDinSystem = 0;
	for( PTr2InteriorPortalSocketVector::iterator it = m_portalSockets.begin(); it != m_portalSockets.end(); ++it )
	{
		Enlighten::IPrecompPackedGeometry* packedGeometry = (*it)->GetEnlightenPackedGeometry( precompute, prog, m_socketSystem.m_enlightenPixelSize );
		if( packedGeometry )
		{
			geometries.push_back( packedGeometry );
			m_packedPortalSockets.push_back( *it );

			Enlighten::PrecompInputInstance inputInstance;
			inputInstance.m_Location = Geo::MIdentity();
			inputInstance.m_GeometryId = packedGeometry->GetId();
			CCP_LOG_CH( g_enlightenBuildChannel, "Adding GeometryID %u,%u to portal socket system", inputInstance.m_GeometryId.High(), inputInstance.m_GeometryId.Low() );

			// Set the instanceID so that the order can change in the list
			(*it)->SetInstanceInSystemIdx( instanceIDinSystem );

			inputSystem->AddInstances( &inputInstance, &inputInstance + 1 );

			++instanceIDinSystem;
		}
	}
	return !geometries.empty();
}
#endif

// -------------------------------------------------------------
// Description:
//   Adds a given portal socket to the list of owned portal sockets,
//   sets socket's parent.
// Arguments:
//   socket - Portal socket to add to the list
// -------------------------------------------------------------
void Tr2InteriorCell::AddPortalSocket( Tr2InteriorPortalSocket* socket )
{
	m_portalSockets.Insert( -1, socket->GetRawRoot() );
	socket->AddToCell( this );
}

// -------------------------------------------------------------
// Description:
//   Tests if a point is inside the cell's union of bounding boxes
// Arguments:
//   testPoint - the point to test in worldspace
// -------------------------------------------------------------
bool Tr2InteriorCell::ContainsPoint( const Vector3& testPoint, float epsilon )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
	//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
	//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	//	Transform the point to get it into the cell's local space
	XMVECTOR det;
	Matrix inv;
	inv = XMMatrixInverse( &det, GetWorldTransform() );
	
	Vector3 newPoint( XMVector3TransformCoord( testPoint, inv ) );

	bool intersects = BoundingBoxIsInside( cellMinBounds, cellMaxBounds, newPoint, epsilon );

	//	If we don't intersect, then don't test the children
	if( !intersects )
	{
		return false;
	}
	
	//	If we don't have children, we're done!
	if( m_boundingBoxes.empty() )
	{
		return true;
	}

	//	Finally, test the children
	for( Tr2InteriorBoundingBoxVector::const_iterator it = m_boundingBoxes.begin(); it!=m_boundingBoxes.end(); ++it )
	{
		const Vector3& minBound = ( *it )->m_minBounds;
		const Vector3& maxBound = ( *it )->m_maxBounds;

		if( BoundingBoxIsInside( minBound, maxBound, newPoint, epsilon ) )
		{
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//   Tests if an oriented bounding box intersects the cell's union of bounding boxes
// Arguments:
//   boxCenter, boxExtents, boxOrientation - the definition of the oriented bounding box
// -------------------------------------------------------------
bool Tr2InteriorCell::IntersectsOBB( const Vector3& boxCenter, const Vector3& boxExtents, const Quaternion& boxOrientation )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
		//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
		//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	Vector3 scale, translation;
	Quaternion rotation;
	D3DXMatrixDecompose( &scale, &rotation, &translation, &GetWorldTransform() );
	Vector3 center = ( cellMinBounds + cellMaxBounds ) / 2 + translation;
	Vector3 extents = cellMaxBounds - center;

	bool intersects = IntersectOrientedBoxOrientedBox( 
		boxCenter, 
		boxExtents, 
		boxOrientation, 
		center, 
		extents,
		rotation );


	//	If we don't intersect, then don't test the children
	if( !intersects )
	{
		return false;
	}
	
	//	If we don't have children, we're done!
	if( m_boundingBoxes.empty() )
	{
		return true;
	}

	//	Finally, test the children
	for( Tr2InteriorBoundingBoxVector::const_iterator it = m_boundingBoxes.begin(); it!=m_boundingBoxes.end(); ++it )
	{
		const Vector3& subCellMinBound = ( *it )->m_minBounds;
		const Vector3& subCellMaxBound = ( *it )->m_maxBounds;

		Vector3 subCenter = ( subCellMinBound + subCellMaxBound ) / 2 + translation;
		Vector3 subExtents = subCellMaxBound - center;

		if( IntersectOrientedBoxOrientedBox( 
			boxCenter, 
			boxExtents, 
			boxOrientation, 
			subCenter, 
			subExtents,
			rotation ) )
		{
			return true;
		}
	}
	return false;

}

// -------------------------------------------------------------
// Description:
//   Tests if an axisaligned bounding box intersects the cell's union of bounding boxes
// Arguments:
//   boxMin, boxMax - the axis-aligned bounding box extents of the tested object in worldspace
// -------------------------------------------------------------
bool Tr2InteriorCell::IntersectsAABB( const Vector3& minBounds, const Vector3& maxBounds )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
	//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
	//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	//	Get the cell's rotation and offset
	XMVECTOR xmScale, xmRot, xmTrans;
	XMMatrixDecompose( &xmScale, &xmRot, &xmTrans, GetWorldTransform() );

	
	Vector3 center = 0.5f * ( cellMinBounds + cellMaxBounds ) + Vector3( xmTrans );
	Vector3 extents = cellMaxBounds - center;

	//	TODO: Test against each bbox
	bool intersects = IntersectOrientedBoxAxisAlignedBox( center, extents, Quaternion( xmRot ), minBounds, maxBounds );


	//	If we don't intersect, then don't test the children
	if( !intersects )
	{
		return false;
	}
	
	//	If we don't have children, we're done!
	if( m_boundingBoxes.empty() )
	{
		return true;
	}

	//	Finally, test the children
	for( Tr2InteriorBoundingBoxVector::const_iterator it = m_boundingBoxes.begin(); it!=m_boundingBoxes.end(); ++it )
	{
		const Vector3& subCellminBound = ( *it )->m_minBounds;
		const Vector3& subCellmaxBound = ( *it )->m_maxBounds;

		Vector3 subCenter = 0.5f * ( subCellminBound + subCellmaxBound ) + Vector3( xmTrans );
		Vector3 subExtents = subCellmaxBound - center;

		if( IntersectOrientedBoxAxisAlignedBox( subCenter, subExtents, Quaternion( xmRot ), minBounds, maxBounds ) )
		{
			return true;
		}
	}
	return false;
}


// -------------------------------------------------------------
// Description:
//   Tests if a sphere intersects the cell's union of bounding boxes
// Arguments:
//   center - Sphere center in worldspace
//	 radius - Sphere radius in worldspace units
// -------------------------------------------------------------
bool Tr2InteriorCell::IntersectsSphere( const Vector3& center, float radius )
{
	Vector3 cellMinBounds, cellMaxBounds;

	if( IsUnbounded() )
	{
	//	Infinite cells contain everything
		return true;
	}
	else if( !GetBoundingBox( cellMinBounds, cellMaxBounds ) )
	{
	//	If the bboxes aren't ready, then we don't contain this object
		return false;
	}

	XMVECTOR det;
	Matrix inv;
	inv = XMMatrixInverse( &det, GetWorldTransform() );
	
	Vector3 position( XMVector3TransformCoord( center, inv ) );

	Vector4 sphere( position.x, position.y, position.z, radius );
	bool intersects = IntersectSphereAxisAlignedBox( sphere, cellMinBounds, cellMaxBounds );

	//	If we don't intersect, then don't test the children
	if( !intersects )
	{
		return false;
	}
	
	//	If we don't have children, we're done!
	if( m_boundingBoxes.empty() )
	{
		return true;
	}

	//	Finally, test the children
	for( Tr2InteriorBoundingBoxVector::const_iterator it = m_boundingBoxes.begin(); it!=m_boundingBoxes.end(); ++it )
	{
		const Vector3& minBound = ( *it )->m_minBounds;
		const Vector3& maxBound = ( *it )->m_maxBounds;

		if( IntersectSphereAxisAlignedBox( sphere, minBound, maxBound))
		{
			return true;
		}
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//   Blue-exposed function that returns a list of light probe
//   positions in the cell's default volume.
// Return value:
//   List of light probe positions
// -------------------------------------------------------------
const std::vector<Vector3>& Tr2InteriorCell::GetLightProbeList()
{
	return m_defaultProbeVolume.m_lightProbes;
}

#endif
