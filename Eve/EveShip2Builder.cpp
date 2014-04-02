#include "StdAfx.h"

#include "EveShip2Builder.h"
#include "SpaceObject/EveShip2.h"
#include "SpaceObject/Attachments/EveSpriteSet.h"
#include "SpaceObject/Attachments/EveSpotlightSet.h"
#include "SpaceObject/Attachments/EveSpaceObjectDecal.h"
#include "Tr2Mesh.h"
#include "Tr2Effect.h"
#include "Miniball3.h"
#include "EveLocator2.h"
#include "Utilities/BoundingSphere.h"

IBlueCallbackManPtr s_backgroundWelder;
CcpMutex s_writeGrannyFileMutex( "EveShip2Builder", "s_writeGrannyFileMutex" );

namespace
{

bool StartsWith( const std::string& string, const char* prefix )
{
	return strncmp( string.c_str(), prefix, strlen( prefix ) ) == 0;
}

}

EveShip2Builder::EveShip2Builder( IRoot* lockobj ) :
	m_turretLocatorCount( 0 ),
	m_boosterLocatorCount( 0 ),
	m_areaOffset( 0 ),
	m_vertexSize( 0 ),
	m_weldThreshold( 0.03f ),
	m_buildSucceeded( false ),
	m_notifyBuildDoneId( 0 ),
	m_buildQueueID( 0 )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !s_backgroundWelder )
	{
		// Create the background welder the first time we need to build
		s_backgroundWelder = BeCallbackMan;
	}
}

EveShip2Builder::~EveShip2Builder()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( m_buildQueueID )
	{
		s_backgroundWelder->Cancel( m_buildQueueID );
	}
	if( m_notifyBuildDoneId )
	{
		BeResMan->CancelFromQueue( BRMQ_MAIN, m_notifyBuildDoneId );
	}
}

EveLocator2* const FindLocatorByName( EveLocator2Vector& locators, const char* name )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( EveLocator2Vector::const_iterator it = locators.begin(); it != locators.end(); ++it )
	{
		std::string locatorName = (*it)->GetName();
		if( StartsWith( locatorName, name ) )
		{
			return (*it);
		}
	}

	return NULL;
}

void EveShip2Builder::CopyLocators( EveLocator2Vector& locators, const Vector3& offset )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	int turretLocatorCount = 0;

	for( EveLocator2Vector::const_iterator it = locators.begin(); it != locators.end(); ++it )
	{
		std::string locatorName = (*it)->GetName();

		if( !StartsWith( locatorName, "locator" ) )
		{
			CCP_LOGWARN( "EveShip2Builder::CopyLocators: Invalid locator name '%s'\n", locatorName.c_str() );
			continue;
		}

		if( StartsWith( locatorName, "locator_attach" ) )
		{
			continue;
		}

		if( StartsWith( locatorName, "locator_booster_" ) )
		{
			if( m_boosterLocatorCount > 255 )
			{
				CCP_LOGERR( "EveShip2Builder::CopyLocators: Too many booster locators!\n" );
				continue;
			}
			
			EveLocator2Ptr p;
			p.CreateInstance();

			Matrix m = (*it)->GetTransform();
			m._41 += offset.x;
			m._42 += offset.y;
			m._43 += offset.z;
			p->SetTransform( m );

			std::string boosterName = "locator_booster_";
			char buf[32];
            sprintf_s( buf, "%i", m_boosterLocatorCount );
			boosterName += buf;
			p->SetName( boosterName.c_str() );

			m_ship->m_locators.Insert( -1, p );

			++m_boosterLocatorCount;
		}
		else if( StartsWith( locatorName, "locator_turret_" ) || StartsWith( locatorName, "locator_launcher_" ) )
		{
			if( m_turretLocatorCount + turretLocatorCount > 255 )
			{
				CCP_LOGERR( "EveShip2Builder::CopyLocators: Too many turret locators! Something is odd...\n" );
				continue;
			}

			// find the last occurence of "_" in the name: after that the number starts
			size_t pos = locatorName.find_last_of( '_' );
			if( pos == std::string::npos )
			{
				CCP_LOGERR( "EveShip2Builder::CopyLocators: There must be at least one _ in a locator name: %s\n", locatorName.c_str() );
				continue;
			}

			std::string s = locatorName.substr( pos + 1 );
			s.resize( s.size() - 1 );
			
			EveLocator2Ptr p;
			p.CreateInstance();

			Matrix m = (*it)->GetTransform();
			m._41 += offset.x;
			m._42 += offset.y;
			m._43 += offset.z;
			p->SetTransform( m );
			
			p->SetName( locatorName.c_str() );

			m_ship->m_locators.Insert( -1, p );

			++turretLocatorCount;
		}
		else if( StartsWith( locatorName, "locator_audio_booster" ) )
		{
			// just collect all incoming audio positions
			Matrix m = (*it)->GetTransform();
			Vector3 pos = Vector3( m._41, m._42, m._43 ) + offset;
			m_allAudioBoosterPositions.push_back( pos );
		}
		else
		{
			CCP_LOGWARN( "EveShip2Builder::CopyLocators: Unknown locator type '%s'\n", locatorName.c_str() );
			continue;
		}
	}

	m_turretLocatorCount += turretLocatorCount / 2;
}

#if BLUE_WITH_PYTHON
void EveShip2Builder::CopyDamageLocators( EveDamageLocatorStructureList& locators, const Vector3& offset )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( unsigned i = 0; i < locators.size(); ++i )
	{		
		EveDamageLocator& loc = locators[i];

		m_ship->m_persistedDamageLocators.Append( &loc );
	}
}
#endif


void EveShip2Builder::CopySpriteSets( EveSpriteSetVector* src, EveSpriteSetVector* dst, const Vector3& offset )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( EveSpriteSetVector::iterator srcIt = src->begin(); srcIt != src->end(); ++srcIt )
	{
		EveSpriteSetPtr ss;
		Tr2EffectPtr fx = (*srcIt)->GetEffect();
		if( !fx )
		{
			continue;
		}
		
		for( EveSpriteSetVector::iterator dstIt = dst->begin(); dstIt != dst->end(); ++dstIt )
		{
			Tr2Effect* fxCandidate = (*dstIt)->GetEffect();

			if( fx->IsEqual( fxCandidate ) )
			{
				ss = *dstIt;
				break;
			}
		}

		if( !ss )
		{
			ss.CreateInstance();
			ss->SetEffect( fx );
			ss->SetName( (*srcIt)->GetName() );
			dst->Insert( -1, ss->GetRawRoot() );
		}

		EveSpriteSetItemVector* srcSprites = (*srcIt)->GetSprites();
		EveSpriteSetItemVector* dstSprites = ss->GetSprites();

		for( EveSpriteSetItemVector::iterator it = srcSprites->begin(); it != srcSprites->end(); ++it )
		{
			EveSpriteSetItemPtr item;
			item.CreateInstance();
			*item = *(*it);
			item->m_position += offset;
			dstSprites->Insert( -1, item );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all spotlights from one ship to another. Check the existing spotlight sets
//   on the target ship if we can insert the new ones into an old one, if shaders
//   do match.
// Arguments:
//   src - vector of spotlights coming from the source ship
//   dst - vector of spotlights belonging the the ship being build
//   offset - a translation offset: all modules can have them
// --------------------------------------------------------------------------------
void EveShip2Builder::CopySpotlightSets( const EveSpotlightSetVector* src, EveSpotlightSetVector* dst, const Vector3& offset )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( EveSpotlightSetVector::const_iterator srcIt = src->begin(); srcIt != src->end(); ++srcIt )
	{
		// spotlights use two shaders
		EveSpotlightSetPtr ss = NULL;
		Tr2EffectPtr glowFx = (*srcIt)->GetGlowEffect();
		Tr2EffectPtr coneFx = (*srcIt)->GetConeEffect();
		if( !glowFx || !coneFx )
		{
			continue;
		}

		// determine if a spotlight set with the correct shaders is already there
		for( EveSpotlightSetVector::iterator dstIt = dst->begin(); dstIt != dst->end(); ++dstIt )
		{
			Tr2EffectPtr fxGlowCandidate = (*dstIt)->GetGlowEffect();
			Tr2EffectPtr fxConeCandidate = (*dstIt)->GetConeEffect();

			if( glowFx->IsEqual( fxGlowCandidate ) && coneFx->IsEqual( fxConeCandidate ) )
			{
				ss = *dstIt;
				break;
			}
		}

		if( !ss )
		{
			ss.CreateInstance();

			// setup new spotlight set
			ss->SetGlowEffect( glowFx );
			ss->SetConeEffect( coneFx );
			ss->SetName( (*srcIt)->GetName() );

			// add it to ship being build
			dst->Insert( -1, ss->GetRawRoot() );
		}

		const EveSpotlightSetItemVector* srcItems = (*srcIt)->GetSpotlightItems();

		for( EveSpotlightSetItemVector::const_iterator it = srcItems->begin(); it != srcItems->end(); ++it )
		{
			EveSpotlightSetItemPtr item;

			item.CreateInstance();

			// copy it and change the position because of the offset of this t3 module
			*item = *(*it);
			// but only the position
			Vector3 newTranslation = item->m_transform.GetTranslation() + offset;
			item->m_transform.SetTranslation( &newTranslation );

			// add it to the set
			ss->AddSpotlightItem( item );
		}
	}
}

// --------------------------------------------------------------------------------
// Description:
//   Copy all decals from one ship to another. 
// Arguments:
//   src - vector of decals coming from the source ship
//   dst - vector of decals belonging the the ship being build
//   offset - a translation offset: all modules can have them
// --------------------------------------------------------------------------------
void EveShip2Builder::CopyDecals( const EveSpaceObjectDecalVector* src, EveSpaceObjectDecalVector* dst, const Vector3& offset )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( EveSpaceObjectDecalVector::const_iterator srcIt = src->begin(); srcIt != src->end(); ++srcIt )
	{
		EveSpaceObjectDecalPtr decal;
		decal.CreateInstance();

		// copy it and change the position because of the offset of this t3 module
		BeClasses->CopyTo( (*srcIt)->GetRawRoot(), (IRoot**)&decal );
		decal->SetPosition( decal->GetPosition() + offset );

		// add it to target ship
		dst->Insert( -1, decal->GetRawRoot() );
	}
}

bool EveShip2Builder::PrepareForBuild()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	for( unsigned int i = 0; i < MODULE_MAX_COUNT; ++i )
	{
		if( m_moduleResPath[i].size() == 0 )
		{
			CCP_LOGERR( "EveShip2Builder::Build: Empty module res path\n" );
			return false;
		}

		IRootPtr p;
		p.Attach( BeResMan->LoadObject( m_moduleResPath[i].c_str(), Be::LDOBJ_DONT_INITIALIZE ) );
		if( p )
		{
			m_module[i] = nullptr;
			p->QueryInterface( BlueInterfaceIID<EveShip2>(), (void**)&m_module[i] );
		}

		if( !m_module[i] )
		{
			CCP_LOGERR( "EveShip2Builder::Build: Invalid module res path '%s'\n", m_moduleResPath->c_str() );
			return false;
		}

		Tr2MeshPtr mesh = m_module[i]->GetMesh();
		if( !mesh )
		{
			CCP_LOGERR( "EveShip2Builder::Build: Module '%s' has no mesh\n", m_moduleResPath->c_str() );
			return false;
		}

		const char* grName = mesh->GetMeshResPath();

		m_grannies[i] = nullptr;
		BeResMan->GetResource( grName, "raw", BlueInterfaceIID<TriGrannyRes>(), (void**)&m_grannies[i] );
	}

	return true;
}

bool EveShip2Builder::Build()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	BeTimer time;

	m_ship.CreateInstance();

	m_boosterLocatorCount = 0;
	m_turretLocatorCount = 0;
	m_areaOffset = 0;

	Vector3 offsets[MODULE_MAX_COUNT];

	const char* moduleNames[MODULE_MAX_COUNT] = {
		"electronic",
		"defensive",
		"engineering",
		"offensive",
		"propulsion"
	};

	offsets[0] = Vector3( 0.0f, 0.0f, 0.0f );

	// Calculate offsets for geometry, using attachment locators
	for( int moduleIx = 1; moduleIx < MODULE_MAX_COUNT; ++moduleIx )
	{
		std::string locName = "locator_attach_";
		locName += moduleNames[moduleIx];
		EveLocator2* loc = FindLocatorByName( m_module[moduleIx - 1]->m_locators, locName.c_str() );
		if( !loc )
		{
			CCP_LOGERR( "EveShip2Builder::Build: Module '%s' is missing locator '%s'\n", m_module[moduleIx-1]->m_name.c_str(), locName.c_str() );
			return false;
		}

		offsets[moduleIx] = offsets[moduleIx - 1] + *(Vector3*)&(loc->GetTransform()._41);
	}

	// "merge" all audio locators we find into a single one, so we must collect them into list
	m_allAudioBoosterPositions.clear();

	// Copy all locators and sprite etc. sets over
	for( int moduleIx = 0; moduleIx < MODULE_MAX_COUNT; ++moduleIx )
	{
		EveShip2Ptr other = m_module[moduleIx];
		CopyLocators( other->m_locators, offsets[moduleIx] );
#if BLUE_WITH_PYTHON
		CopyDamageLocators( other->m_persistedDamageLocators, offsets[moduleIx] );
#endif

		CopySpriteSets( &other->m_spriteSets, &m_ship->m_spriteSets, offsets[moduleIx] );
		CopySpotlightSets( &other->m_spotlightSets, &m_ship->m_spotlightSets, offsets[moduleIx] );
		CopyDecals( &other->m_decals, &m_ship->m_decals, offsets[moduleIx] );
	}

	m_ship->RebuildDamageLocatorCache();

	CalculateAudioBooster();

	// Combine the meshes - high detail first
	Tr2MeshPtr mesh;
	mesh.CreateInstance();

	m_ship->SetHighDetailMesh( mesh );

	InitializeGrannyFile();

	for( int moduleIx = 0; moduleIx < MODULE_MAX_COUNT; ++moduleIx )
	{
		EveShip2Ptr other = m_module[moduleIx];

		if( !AddMeshToGrannyFile( m_grannies, moduleIx, other->GetMesh(), offsets[moduleIx], mesh ) )
		{
			return false;
		}
	}

	CalculateBoundingSphere( offsets );

	FinalizeGrannyFile( m_highDetailOutputName );
	mesh->SetMeshResPath( m_highDetailOutputName.c_str() );

	CCP_LOG( "EveShip2Builder::Build took %g seconds", time.GetSeconds() );

	return true;
}

EveShip2* EveShip2Builder::GetShip()
{
	return m_ship;
}

void EveShip2Builder::InitializeGrannyFile()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	memset( &m_grannyFileInfo, 0, sizeof( m_grannyFileInfo ) );
	memset( &m_finalGrannyMesh, 0, sizeof( m_finalGrannyMesh ) );
	memset( &m_grannyVertexData, 0, sizeof( m_grannyVertexData ) );
	memset( &m_grannyTopology, 0, sizeof( m_grannyTopology ) );
	m_vertexSize = 0;
}

void EveShip2Builder::Weld( granny_uint8* referenceVB, int referenceCount, granny_uint8* vb, int count )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	Vector3* referencePositions = (Vector3*)CCP_MALLOC( "Weld/referencePositions", referenceCount * sizeof( Vector3 ) );
	Vector3* positions = (Vector3*)CCP_MALLOC( "Weld/referencePositions", count * sizeof( Vector3 ) );

	{
		CCP_STATS_ZONE( "GrannyConvertVertexLayouts" );

		GrannyConvertVertexLayouts( referenceCount, m_grannyVertexData.VertexType, referenceVB, GrannyP3VertexType, referencePositions );
		GrannyConvertVertexLayouts( count, m_grannyVertexData.VertexType, vb, GrannyP3VertexType, positions );
	}

	{
		CCP_STATS_ZONE( "Weld" );

		XMVECTOR epsilon = XMVectorSet( m_weldThreshold, m_weldThreshold, m_weldThreshold, m_weldThreshold );

		for( int vertexIx = 0; vertexIx < count; ++vertexIx )
		{
			Vector3& vertexPos = positions[vertexIx];
			for( int referenceIx = 0; referenceIx < referenceCount; ++referenceIx )
			{
				Vector3& referencePos = referencePositions[referenceIx];
				if( XMVector3NearEqual( vertexPos, referencePos, epsilon ) )
				{
					vertexPos = referencePos;

					// Break out of the inner loop - we've matched the vertex to one of the 
					// reference positions and moved it to its location - no need to look
					// at the rest, it just has to match one of them.
					break;
				}
			}
		}
	}

	granny_uint8* dst = vb;
	granny_uint8* src;
	int sz;

	// Assume position is first element in vertex structure
	if( m_grannyVertexData.VertexType->Type == GrannyReal16Member )
	{
		CCP_STATS_ZONE( "FloatToHalf" );

		D3DXFLOAT16* halfPositions = (D3DXFLOAT16*)CCP_MALLOC( "Weld/halfPositions", count * sizeof( granny_uint16 ) );
		D3DXFloat32To16Array( halfPositions, (float*)positions, count );

		src = (granny_uint8*)halfPositions;
		sz = 6;
	}
	else
	{
		src = (granny_uint8*)positions;
		sz = 12;
	}

	{
		CCP_STATS_ZONE( "Copy" );

		for( int i = 0; i < count; ++i )
		{
			memcpy( dst, src, sz );
			dst += m_vertexSize;
			src += sz;
		}
	}

	CCP_FREE( positions );
	CCP_FREE( referencePositions );
}

bool EveShip2Builder::AddMeshToGrannyFile( TriGrannyResPtr* grannies, int ix, Tr2Mesh* mesh, const Vector3& offset, Tr2Mesh* dstMesh )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	int grannyFileIx = mesh->GetMeshIndex();

	if( !grannies[ix]->IsGood() )
	{
		// CCP_LOGERR( "EveShip2Builder: Mesh failed to load!\n" );
		return false;
	}
	granny_file* grannyFile = m_grannies[ix]->GetGrannyFile();
	granny_file_info* fi = GrannyGetFileInfo( grannyFile );
	granny_mesh* grannyMesh = fi->Meshes[grannyFileIx];

	if( !m_grannyVertexData.VertexType )
	{
		m_grannyVertexData.VertexType = grannyMesh->PrimaryVertexData->VertexType;
		m_vertexSize = GrannyGetTotalObjectSize( m_grannyVertexData.VertexType );
	}
	else
	{
		int vertexSize = GrannyGetTotalObjectSize( grannyMesh->PrimaryVertexData->VertexType );
		if( vertexSize != m_vertexSize )
		{
			// CCP_LOGERR( "EveShip2Builder: Meshes being merged have differing vertex formats\n" );
			return false;
		}
	}

	CopyAreas( mesh->GetAreas( TRIBATCHTYPE_OPAQUE ), dstMesh->GetAreas( TRIBATCHTYPE_OPAQUE ) );
	CopyAreas( mesh->GetAreas( TRIBATCHTYPE_DEPTH ), dstMesh->GetAreas( TRIBATCHTYPE_DEPTH ) );
	CopyAreas( mesh->GetAreas( TRIBATCHTYPE_DECAL ), dstMesh->GetAreas( TRIBATCHTYPE_DECAL ) );
	CopyAreas( mesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ), dstMesh->GetAreas( TRIBATCHTYPE_TRANSPARENT ) );
	CopyAreas( mesh->GetAreas( TRIBATCHTYPE_ADDITIVE ), dstMesh->GetAreas( TRIBATCHTYPE_ADDITIVE ) );

	for( int areaIx = 0; areaIx < grannyMesh->MaterialBindingCount; ++areaIx )
	{
		granny_material* mat = grannyMesh->MaterialBindings->Material;
		m_grannyMaterials.push_back( mat );
	}


	int prevCount = m_grannyVertexData.VertexCount;
	int currentCount = grannyMesh->PrimaryVertexData->VertexCount;
	int newCount = prevCount + currentCount;
	int vbSize = m_vertexSize * newCount;
	
	granny_uint8* newVertices = (granny_uint8*)CCP_MALLOC( "EveShip2Builder/vertices", vbSize );
	granny_uint8* dstVb = newVertices;
	
	if( m_grannyVertexData.Vertices )
	{
		memcpy( newVertices, m_grannyVertexData.Vertices, prevCount * m_vertexSize );
		dstVb += prevCount * m_vertexSize;
	}

	memcpy( dstVb, grannyMesh->PrimaryVertexData->Vertices, currentCount * m_vertexSize );

	granny_real32* affine = (granny_real32*)&offset;
	static granny_real32 id[3][3] = { {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} };

	GrannyTransformVertices( currentCount, m_grannyVertexData.VertexType, dstVb, affine, (granny_real32*)id, (granny_real32*)id, false, false );
	
	if( m_grannyVertexData.Vertices )
	{
		CCP_FREE( m_grannyVertexData.Vertices );
	}
	
	if( prevCount )
	{
		Weld( newVertices, prevCount, dstVb, currentCount );
	}
	else
	{
		Weld( dstVb, currentCount, dstVb, currentCount );
	}

	m_grannyVertexData.Vertices = newVertices;
	m_grannyVertexData.VertexCount = newCount;
	
	int prevIxCount = m_grannyTopology.IndexCount;
	bool is16Bit = false;
	int currentIxCount = grannyMesh->PrimaryTopology->IndexCount;
	if( !currentIxCount )
	{
		currentIxCount = grannyMesh->PrimaryTopology->Index16Count;
		is16Bit = true;
	}
	int newIxCount = prevIxCount + currentIxCount;

	int ibSize = newIxCount * sizeof( granny_int32 );
	granny_int32* newIndices = (granny_int32*)CCP_MALLOC( "EveShip2Builder/indices", ibSize );
	granny_int32* dstIb = newIndices;
	if( m_grannyTopology.Indices )
	{
		memcpy( newIndices, m_grannyTopology.Indices, prevIxCount * sizeof( granny_int32 ) );
		dstIb += prevIxCount;
	}
	
	for( int ixIx = 0; ixIx < currentIxCount; ++ixIx )
	{
		int currentIx;
		if( is16Bit )
		{
			currentIx = grannyMesh->PrimaryTopology->Indices16[ixIx];
		}
		else
		{
			currentIx = grannyMesh->PrimaryTopology->Indices[ixIx];
		}
		currentIx += prevCount;
		dstIb[ixIx] = currentIx;
	}
	
	if( m_grannyTopology.Indices )
	{
		CCP_FREE( m_grannyTopology.Indices );
	}

	m_grannyTopology.Indices = newIndices;
	m_grannyTopology.IndexCount = newIxCount;

	for( int groupIx = 0; groupIx < grannyMesh->PrimaryTopology->GroupCount; ++groupIx )
	{
		granny_tri_material_group grp = grannyMesh->PrimaryTopology->Groups[groupIx];
		grp.TriFirst += prevIxCount / 3;
		m_grannyGroups.push_back( grp );
	}
	
	m_areaOffset += grannyMesh->MaterialBindingCount;

	return true;
}

void EveShip2Builder::CopyAreas( Tr2MeshAreaVector* src, Tr2MeshAreaVector* dst )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CCP_ASSERT( src );
	CCP_ASSERT( dst );
	
	for( Tr2MeshAreaVector::iterator it = src->begin(); it != src->end(); ++it )
	{
		Tr2MeshAreaPtr area;
		if( area.CreateInstance() )
		{
			*area = **it;
			area->SetIndex( area->GetIndex() + m_areaOffset );

			dst->Insert( -1, area );
		}
	}
}

void EveShip2Builder::CalculateAudioBooster()
{
	// "combine" all audio boosters we have collected
	if( !m_allAudioBoosterPositions.empty() )
	{
		Vector3 newAudioBoosterPosition( 0.f, 0.f, 0.f );
		for( std::vector<Vector3>::const_iterator it = m_allAudioBoosterPositions.begin(); it != m_allAudioBoosterPositions.end(); ++it )
		{
			newAudioBoosterPosition += (*it);
		}
		newAudioBoosterPosition /= (float)m_allAudioBoosterPositions.size();

		// that's the one we want on the new ship
		EveLocator2Ptr p;
		p.CreateInstance();
		Matrix m;
		D3DXMatrixTranslation( &m, newAudioBoosterPosition.x, newAudioBoosterPosition.y, newAudioBoosterPosition.z );
		p->SetTransform( m );
		p->SetName( "locator_audio_booster" );
		m_ship->m_locators.Insert( -1, p );
	}
}

void EveShip2Builder::CalculateBoundingSphere( const Vector3* offsets )
{
	CCP_STATS_ZONE( __FUNCTION__ );
	BoundingSphereInitialize( m_boundingSphere );

	for( int i = 0; i < MODULE_MAX_COUNT; ++i )
	{
		Vector4 moduleBoundingSphere( m_module[i]->GetBoundingSphereCenter() + offsets[i], m_module[i]->GetBoundingSphereRadius() );
		BoundingSphereUpdate( moduleBoundingSphere, m_boundingSphere );
	}

	m_ship->m_boundingSphereCenter = Vector3( m_boundingSphere.x, m_boundingSphere.y, m_boundingSphere.z );
	m_ship->m_boundingSphereRadius = m_boundingSphere.w;
}

void EveShip2Builder::FinalizeGrannyFile( const std::string& outputName )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	CcpAutoMutex guardGrannyWriteAccess( s_writeGrannyFileMutex );

	std::wstring outputNameW = (const wchar_t*)CA2W( outputName.c_str() );
	std::wstring fullName = BePaths->ResolvePathForWritingW( outputNameW );
	if( fullName.empty() )
	{
		CCP_LOGERR( "%s: '%s' is not a valid filename", __FUNCTION__, outputName.c_str() );
		return;
	}

	m_grannyTopology.GroupCount = (unsigned int)m_grannyGroups.size();
	m_grannyTopology.Groups = &m_grannyGroups[0];

	m_finalGrannyMesh.PrimaryTopology = &m_grannyTopology;
	m_finalGrannyMesh.PrimaryVertexData = &m_grannyVertexData;

	granny_mesh* meshes[] = { &m_finalGrannyMesh };
	granny_tri_topology* topologies[] = { m_finalGrannyMesh.PrimaryTopology };
	granny_vertex_data* vertexDatas[] = { m_finalGrannyMesh.PrimaryVertexData };

	m_grannyFileInfo.ModelCount = 0;
	m_grannyFileInfo.Models = 0;
	m_grannyFileInfo.MeshCount = 1;
	m_grannyFileInfo.Meshes = meshes;
	m_grannyFileInfo.VertexDataCount = 1;
	m_grannyFileInfo.VertexDatas = vertexDatas;
	m_grannyFileInfo.TriTopologyCount = 1;
	m_grannyFileInfo.TriTopologies = topologies;

	m_grannyFileInfo.MaterialCount = (unsigned int)m_grannyMaterials.size();
	m_grannyFileInfo.Materials = &m_grannyMaterials[0];

	m_finalGrannyMesh.MaterialBindingCount = m_grannyFileInfo.MaterialCount;
	m_finalGrannyMesh.MaterialBindings = (granny_material_binding*)CCP_MALLOC("myMesh.MaterialBindings", sizeof( granny_material_binding ) * m_grannyFileInfo.MaterialCount );

	for( int i = 0; i < m_grannyFileInfo.MaterialCount; ++i )
	{
		m_finalGrannyMesh.MaterialBindings[i].Material = m_grannyFileInfo.Materials[i];
	}


	CCP_LOG( "%s: GrannyBeginFileInMemory", __FUNCTION__ );

	granny_file_builder *Builder = GrannyBeginFileInMemory(
		1, 
		GrannyCurrentGRNStandardTag,
		GrannyGRNFileMV_32Bit_LittleEndian,
		8192
		);

	if( !Builder )
	{
		CCP_LOGERR( "%s: Could not create granny_file_builder", __FUNCTION__ );
		return;
	}

	CCP_LOG( "%s: GrannyBeginFileDataTreeWriting", __FUNCTION__ );

	granny_file_data_tree_writer *Writer =
		GrannyBeginFileDataTreeWriting(GrannyFileInfoType, &m_grannyFileInfo, 0, 0);
	if( !Writer )
	{
		CCP_LOGERR( "%s: Could not get granny_file_data_tree_writer", __FUNCTION__ );
		return;
	}

	CCP_LOG( "%s: GrannyWriteDataTreeToFileBuilder", __FUNCTION__ );

	if( !GrannyWriteDataTreeToFileBuilder(Writer, Builder) )
	{
		CCP_LOGERR( "%s: GrannyWriteDataTreeToFileBuilder failed", __FUNCTION__ );
		return;
	}

	CCP_LOG( "%s: GrannyEndFileDataTreeWriting", __FUNCTION__ );

	GrannyEndFileDataTreeWriting(Writer);


	CCP_LOG( "%s: GrannyEndFileToWriter", __FUNCTION__ );

	granny_file_writer* memWriter = GrannyCreateMemoryFileWriter( 8192 );

	GrannyEndFileToWriter( Builder, memWriter );

	CCP_FREE( m_finalGrannyMesh.MaterialBindings );
	CCP_FREE( m_grannyTopology.Indices );
	CCP_FREE( m_grannyVertexData.Vertices );

	IResFilePtr outputStream;

	BeClasses->CreateInstanceFromName( "ResFile", BlueInterfaceIID<IResFile>(), (void**)&outputStream );

	if( !outputStream->CreateW( fullName.c_str() ) )
	{
		CCP_LOGERR( "%s: Couldn't create file %S", __FUNCTION__, fullName.c_str() );
		return;
	}

	granny_uint8* buffer;
	granny_int32x bufferSize;
	GrannyStealMemoryWriterBuffer( memWriter, &buffer, &bufferSize );
	outputStream->Write( (void*)buffer, (size_t)bufferSize );
	outputStream->Close();

	GrannyFreeMemoryWriterBuffer( buffer );	

	CCP_LOG( "%s: Done", __FUNCTION__ );
}

void EveShip2Builder::StaticBuildAsync( void* context )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	EveShip2Builder* pThis = static_cast<EveShip2Builder*>( context );
	pThis->m_buildSucceeded = pThis->Build();
	pThis->m_buildQueueID = 0;
	BeResMan->AddToQueue( BRMQ_MAIN, StaticNotifyBuildDone, context, 0, &pThis->m_notifyBuildDoneId );
}

void EveShip2Builder::BuildAsync( const BlueScriptCallback& doneCallbackToPython )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	m_doneCallbackToPython = doneCallbackToPython;

	if( m_buildQueueID )
	{
		CCP_LOGERR( "EveShip2Builder::BuildAsync: Already building ship, don't call this twice on the same object!" );
		return;
	}
	s_backgroundWelder->Add( StaticBuildAsync, (void*)this, 0, &m_buildQueueID );
}

void EveShip2Builder::StaticNotifyBuildDone( void* context )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	EveShip2Builder* pThis = static_cast<EveShip2Builder*>( context );

	if( pThis->m_doneCallbackToPython )
	{
		pThis->m_doneCallbackToPython.CallVoid( pThis->m_buildSucceeded );
	}
}

