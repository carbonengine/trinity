#include "StdAfx.h"

#include "Tr2SkinnedObject.h"
#include "Tr2CpuSkinnedModel.h"
#include "Resources/TriGeometryRes.h"
#include "TriFrustum.h"
#include "TriSettingsRegistrar.h"
#include "include/ITr2DebugRenderer.h"
#include "Utilities/Obb.h"
#include "Curves/TriCurveSet.h"
#include "TriLineSet.h"
#include "Include/ITr2AnimationUpdater.h"
#include "ITr2WorldTransformUpdater.h"

#if APEX_ENABLED
#include "Apex/Apex.h"
#include "Apex/Tr2ClothingActor.h"

int g_maxClothLod = 0;
float g_clothTranslationResetThreshold = 0.5f;
float g_clothTimeDeltaResetThreshold = 0.5f;

TRI_REGISTER_SETTING( "maxClothLod", g_maxClothLod );
TRI_REGISTER_SETTING( "clothTranslationResetThreshold", g_clothTranslationResetThreshold );
TRI_REGISTER_SETTING( "clothTimeDeltaResetThreshold", g_clothTimeDeltaResetThreshold );
#endif

Tr2SkinnedObject::Tr2SkinnedObject(IRoot* lockobj) :
    PARENTLOCK( m_transform ),
#if APEX_ENABLED
	PARENTLOCK( m_clothMeshes ),
#endif
	PARENTLOCK( m_curveSets ),
	m_skinningMatrixFrameDelay( 0 ),
	m_skinningMatrixQueueIndex( 0 ),
	m_boneList( NULL ),
	m_skinningMatrixCount( 0 ),
	m_skinningMatrixQueue( "Tr2SkinnedObject/m_skinningMatrixQueue" ),
	m_worldTransformsQueue( "Tr2SkinnedObject/m_worldTransformsQueue" ),
	m_accumulatedTransformsQueue( "Tr2SkinnedObject/m_accumulatedTransformQueue"),
	m_skinningMatrixQueueNeedsPriming( true ),
	m_display( true ),
	m_displayMarker( false ),
	m_displayName( false ),
	m_animRigToRenderRigMapping( NULL ),
	m_renderRigToAnimRigMapping( NULL ),
	m_animationUpdater( (ITr2AnimationUpdater*)NULL ),
	m_worldTransformUpdater( (ITr2WorldTransformUpdater*)NULL ),
	m_renderRigBoneList( "Tr2SkinnedObject/m_renderRigBoneList" ),
	m_numRenderRigBones( 0 ),
	m_debugRenderSkeletonTrail( false ),
	m_debugRenderSkeletonTrailIx( false ),
	m_debugFreezeSkeletonTrail( false ),
	m_debugSkeletonTrailLength( 0 ),
	m_debugRenderSkeletonJointIndices( false ),
#if APEX_ENABLED
	m_parallelPhysXMeshSkinning( false ),
	m_parallelMeshMeshSkinning( false ),
	m_clothFadeEndTime( 0 ),
#endif
	m_lastUpdateTime( 0 ),
	m_lastTranslation( 0.0f, 0.0f, 0.0f ),
	m_estimatedPixelDiameter( 0.0f ),
	m_skeletonTag( 0 ),
	m_useExplicitBounds( false ),
	m_minBounds( 0.0f, 0.0f, 0.0f ),
	m_maxBounds( 0.0f, 0.0f, 0.0f ),
	m_updatePeriod( 0.0f )
{
	m_lineSet.CreateInstance();

#if APEX_ENABLED
	m_clothMeshes.SetNotify( this );

	m_isClothSimParallel = g_Tr2Apex != NULL && g_Tr2Apex->ApexSceneIsParallel();
#endif
}

Tr2SkinnedObject::~Tr2SkinnedObject()
{
	CCP_DELETE [] m_animRigToRenderRigMapping;;
	CCP_DELETE [] m_renderRigToAnimRigMapping;
	m_animRigToRenderRigMapping = NULL;
	m_renderRigToAnimRigMapping = NULL;
	
	FreeSkinningMatrices();
}

void Tr2SkinnedObject::PrePhysicsUpdate( Be::Time time )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const Be::Time deltaTime = time - m_lastUpdateTime;	
	if( TimeAsFloat( deltaTime ) < m_updatePeriod )
	{
		return;
	}
	
	m_lineSet->Clear();

	if( m_visualModel == NULL || m_visualModel->GetSkeleton() == NULL )
    {
        return;
    }

    if( m_animationUpdater != NULL )
    {
		m_animationUpdater->PrePhysicsAnimation( time, m_transform );
	}
	
	return;
}

void Tr2SkinnedObject::PostPhysicsUpdate( Be::Time time, Tr2ApexScene* apexScene )
{
	CCP_STATS_ZONE( __FUNCTION__ );

	const Be::Time deltaTime = time - m_lastUpdateTime;	
	if( TimeAsFloat( deltaTime ) < m_updatePeriod )
	{
		return;
	}

	m_lastUpdateTime = time;

	for( TriCurveSetVector::const_iterator it = m_curveSets.begin(); it != m_curveSets.end(); ++it )
	{
		(*it)->Update( TimeAsDouble( time ) );
	}

    if( m_visualModel == NULL || m_visualModel->GetSkeleton() == NULL )
    {
        return;
    }

	if( m_animationUpdater != NULL )
    {
		m_animationUpdater->PostPhysicsAnimation( time, m_transform );
	}

	UpdateBones( time, apexScene );
	

	// Update the translation if we have a worldtranslation updater
	if( m_worldTransformUpdater != NULL )
	{
		m_worldTransformUpdater->UpdateTransform( time, &m_transform );
	}

	// Check LOD
	

	if( m_lod.UnloadLodIfNeeded( time, deltaTime ) )
	{
		m_visualModel = NULL;
	}

	return;
}

void Tr2SkinnedObject::UpdateBones( Be::Time time, Tr2ApexScene* apexScene )
{
	CCP_STATS_ZONE( __FUNCTION__ );

    if( m_visualModel == NULL || m_visualModel->GetSkeleton() == NULL )
    {
        return;
    }

	unsigned numBones = 0;
	bool isAnimRig = false;
	const std::string* boneList = NULL;
    bool rebuildMapping = false;
    TriGeometryResSkeletonData* skel = m_visualModel->GetSkeleton();

	if( m_animationUpdater != NULL )
    {
        boneList = m_animationUpdater->GetAnimationBoneList( numBones );

		if( boneList )
        {
            rebuildMapping = ( numBones != m_skinningMatrixCount );
			isAnimRig = numBones > 0;
        }
    }

    if( boneList == NULL && skel != NULL )
    {
		CCP_STATS_ZONE( "UpdateBones_BuildRenderRigBoneList" );

		// Either the animationUpdater is NULL or it hasn't been set up fully yet,
		// so create a new boneList based off the render rig. This allows rendering
		// of a skinned model without an animation playing - it'll be static in
		// the bind pose.
		unsigned int numRenderRigBones = (unsigned int)skel->m_joints.size();
        if( m_numRenderRigBones != numRenderRigBones )
        {
			// Recreate mapping if the number of renderRigBones has changed, this is definitely hacky
            m_numRenderRigBones = numRenderRigBones;
            m_renderRigBoneList.resize(numRenderRigBones);
            for(unsigned int i = 0; i < numRenderRigBones; ++i)
            {
                m_renderRigBoneList[i] = skel->m_joints[i].m_name;
            }
            rebuildMapping = true;
        }

		if( m_numRenderRigBones == 0 )
		{
			// Guard against an empty skeleton - shouldn't happen but can
			// if assets are bad.
			CCP_LOGERR
			( 
				"Tr2SkinnedObject '%s' has an empty skeleton in visual model from '%s'",
				m_name.c_str(),
				m_visualModel->GetGeometryResPath()
			);
			m_renderRigBoneList.push_back( "Render_rig_missing" );
			m_numRenderRigBones = 1;
		}

        boneList = &m_renderRigBoneList[0];
        numBones = m_numRenderRigBones;
    }

	if( boneList != m_boneList )
	{
		rebuildMapping = true;
	} 

	// Cache the bone list pointer so we can determine when to rebuild mappings.
	m_boneList = boneList;

	m_visualModel->BindToRig( boneList, numBones, rebuildMapping );
	
	if( rebuildMapping && skel != NULL )
	{
		CCP_STATS_ZONE( "UpdateBones_RebuildMapping" );

		++m_skeletonTag;

		FreeSkinningMatrices();
		AllocateSkinningMatrices( numBones );
		m_visualModel->ResetBindings();

		CCP_DELETE [] m_animRigToRenderRigMapping;
		CCP_DELETE [] m_renderRigToAnimRigMapping;
		m_animRigToRenderRigMapping = CCP_NEW( "Tr2SkinnedObject/m_animRigToRenderRigMapping" ) unsigned int[numBones];
		m_renderRigToAnimRigMapping = CCP_NEW( "Tr2SkinnedObject/m_renderRigToAnimRigMapping" ) unsigned int[skel->m_joints.size()];

		for( unsigned int jointIx = 0; jointIx < skel->m_joints.size(); ++jointIx )
		{
			m_renderRigToAnimRigMapping[jointIx] = -1;
		}

		for( unsigned int jointIx = 0; jointIx < numBones; ++jointIx )
		{
			unsigned int ix = skel->FindJoint( boneList[jointIx].c_str() );
			m_animRigToRenderRigMapping[jointIx] = ix;
			if( ix != -1 )
			{
				m_renderRigToAnimRigMapping[ix] = jointIx;
			}
		}
	}

#if APEX_ENABLED
	SetSkinningMatrixFrameDelay( g_Tr2Apex ? g_Tr2Apex->ApexGetFrameDelay() : 0 );
#endif

	if( !m_debugFreezeSkeletonTrail && skel != NULL )
	{
		CCP_STATS_ZONE( "UpdateBones_MatrixUpdate" );

		++m_skinningMatrixQueueIndex;
		m_skinningMatrixQueueIndex %= m_skinningMatrixQueue.size();

		const Matrix *accumulatedTransforms = NULL;

		if( m_animationUpdater != NULL )
		{
			CCP_STATS_ZONE( "UpdateBones_AnimationUpdate" );

			accumulatedTransforms = m_animationUpdater->GetAnimationTransforms();

			// We may have animation transforms, but no animation rig; in that case, numBones is
			// based on the renderRig, so make sure we don't ask for more skinning matrices than
			// the animation track has available.
			if( accumulatedTransforms && isAnimRig && numBones == m_skinningMatrixCount )
			{
				Matrix* dest = m_accumulatedTransformsQueue[m_skinningMatrixQueueIndex];
				if( dest )
				{
					memcpy( dest, accumulatedTransforms, sizeof( Matrix ) * m_skinningMatrixCount );
				}
			}
		}

		float* dst = m_skinningMatrixQueue[m_skinningMatrixQueueIndex];

		for( unsigned int transformIx = 0; transformIx < m_skinningMatrixCount; ++transformIx )
		{
			unsigned int renderRigIx = m_animRigToRenderRigMapping[transformIx];
			if( renderRigIx != 0xffffffff )
			{
				// If the joint is not found in the render rig there'll be a gap in
				// this array but it won't matter since the rendered geometry won't
				// refer to that joint.

				Matrix final;
				if(accumulatedTransforms)
				{
					const Matrix& invBind = skel->m_joints[renderRigIx].m_inverseWorldTransform;

					D3DXMatrixMultiply( &final, &invBind, &accumulatedTransforms[transformIx] );
				}
				else
				{
					D3DXMatrixScaling(&final, 1.0f, 1.0f, 1.0f);
				}

				float* p = &dst[transformIx * 3*4];
				*p++ = final._11;
				*p++ = final._21;
				*p++ = final._31;
				*p++ = final._41;

				*p++ = final._12;
				*p++ = final._22;
				*p++ = final._32;
				*p++ = final._42;

				*p++ = final._13;
				*p++ = final._23;
				*p++ = final._33;
				*p++ = final._43;
			}
		}

		m_worldTransformsQueue[m_skinningMatrixQueueIndex] = m_transform;

		if( m_skinningMatrixQueueNeedsPriming )
		{
			CCP_STATS_ZONE( "UpdateBones_Priming" );

			// Starting at 0 as m_skinningMatrixQueueIndex has already been incremented
			unsigned int n = (unsigned int)m_skinningMatrixQueue.size() - 1;
			for( unsigned int ix = 0; ix < n; ++ix )
			{
				unsigned int queueIx = (m_skinningMatrixQueueIndex + ix) % m_skinningMatrixQueue.size();
				float* copyDst = m_skinningMatrixQueue[queueIx];
				memcpy( copyDst, dst, m_skinningMatrixCount * sizeof(float)*3*4 );

				m_worldTransformsQueue[queueIx] = m_transform;
			}

			m_skinningMatrixQueueNeedsPriming = false;
		}

#if APEX_ENABLED
		if( (rebuildMapping || m_lod.IsSimulatingCloth( g_maxClothLod )) && !m_clothMeshes.empty() )
		{
			CCP_STATS_ZONE( "UpdateBones_ClothUpdate" );

			Tr2ClothingActor::UpdateStateFlag continuousFlag = Tr2ClothingActor::IS_CONTINUOUS;

			float td = TimeAsFloat( time - m_lastUpdateTime );

			const Vector3& t = m_transform.GetTranslation();
			Vector3 d = t - m_lastTranslation;
			float len = D3DXVec3Length( &d );

			m_lastTranslation = t;

			if( td > g_clothTimeDeltaResetThreshold )
			{
				CCP_LOGWARN( "Tr2ClothingActor::UpdateState: Resetting cloth as time delta is above threshold" );
				continuousFlag = Tr2ClothingActor::IS_NOT_CONTINUOUS;
			}
			else
			{
				if( len > g_clothTranslationResetThreshold )
				{
					CCP_LOGWARN( "Tr2ClothingActor::UpdateState: Resetting cloth as translation delta is above threshold" );
					continuousFlag = Tr2ClothingActor::IS_NOT_CONTINUOUS;
				}
			}


			for( Tr2ClothingActorVector::iterator it = m_clothMeshes.begin(); it != m_clothMeshes.end(); ++it )
			{
				if( Tr2ClothingActor* ca = *it )
				{
					ca->BindToRig( boneList, numBones, rebuildMapping );
					ca->UpdateState( *m_transform.GetMatrix(), dst, m_skinningMatrixCount, continuousFlag, apexScene );
				}
			}
		}
#endif
	}

	// Render debug info
	extern ITr2DebugRendererPtr g_debugRenderer;
	if( g_debugRenderer )
	{
		if( m_debugRenderSkeletonTrail && skel != NULL )
		{
			CCP_STATS_ZONE( "UpdateBones_DebugRender" );

			unsigned int n = (unsigned int)m_accumulatedTransformsQueue.size();
			for( unsigned int frameIx = 0; frameIx < n; ++frameIx )
			{
				unsigned int ix = (m_skinningMatrixQueueIndex + n - frameIx) % n;
				const Matrix& worldTransform = m_worldTransformsQueue[ix];

				for( unsigned int animRigIx = 0; animRigIx < m_skinningMatrixCount; ++animRigIx )
				{
					unsigned int renderRigIx = m_animRigToRenderRigMapping[animRigIx];
					if( renderRigIx != -1 )
					{
						unsigned int renderParentIx = skel->m_joints[renderRigIx].m_parentJoint;
						if( renderParentIx != -1 )
						{
							unsigned int parentIx = m_renderRigToAnimRigMapping[renderParentIx];
							if( parentIx != -1 )
							{
								Matrix fromTransform = m_accumulatedTransformsQueue[ix][animRigIx];
								Matrix toTransform = m_accumulatedTransformsQueue[ix][parentIx];
								Vector3 from = fromTransform.GetTranslation();
								Vector3 to = toTransform.GetTranslation();

								XMMATRIX mat = GetTransform();
								from = XMVector3TransformCoord(from, mat);
								to = XMVector3TransformCoord(to, mat);

								g_debugRenderer->DrawLine( from, to, 0xffff0000 );

								if( m_debugRenderSkeletonJointIndices )
								{
									g_debugRenderer->Printf( from, 0xffffffff, "%d (%d)", animRigIx, renderRigIx );
								}
							}
						}
						else if( m_debugRenderSkeletonTrailIx )
						{
							Matrix transform = m_accumulatedTransformsQueue[ix][animRigIx] * worldTransform;
							const Vector3& pos = transform.GetTranslation();

							g_debugRenderer->Printf( pos, 0xffffffff, "%d", frameIx );
						}
					}
				}
			}
		}
	}
}

unsigned Tr2SkinnedObject::GetBoneIndex( const std::string & boneName ) const
{
	if( !m_animationUpdater )
	{
		return ~0u;
	}

	unsigned numBones = 0;
	const std::string* const boneList = m_animationUpdater->GetAnimationBoneList( numBones );
	if( !boneList || !numBones || numBones != m_skinningMatrixCount )
	{
		return ~0u;
	}

	for( unsigned int jointIx = 0; jointIx < numBones; ++jointIx )
	{
		if( boneList[jointIx] == boneName )
		{
			return jointIx;
		}
	}

	return ~0u;
}

void Tr2SkinnedObject::PrintAllBones()
{
	if( !m_animationUpdater || !m_visualModel )
	{
		return;
	}

	unsigned numBones = 0;
	const std::string* const boneList = m_animationUpdater->GetAnimationBoneList( numBones );

	if( numBones && boneList )
	{
		CCP_LOG( "Animation bones:" );
		for( unsigned i = 0; i != numBones; ++i )
		{
			CCP_LOG( "%d - %s", i, boneList[i].c_str() );
		}
	}

	TriGeometryResSkeletonData* skel = m_visualModel->GetSkeleton();
	if( skel && !skel->m_joints.empty() )
	{
		CCP_LOG( "RenderRig bones:" );
		for(unsigned int i = 0; i < skel->m_joints.size(); ++i)
		{
			CCP_LOG( "%d - %s", i, skel->m_joints[i].m_name.c_str() );
		}
	}
}

unsigned Tr2SkinnedObject::GetSkeletonTag() const
{
	return m_skeletonTag;
}

const Matrix * Tr2SkinnedObject::GetBoneTransform( unsigned joint ) const
{
	if( joint >= m_skinningMatrixCount )
	{
		return NULL;
	}
    
	return &m_accumulatedTransformsQueue[m_skinningMatrixQueueIndex][joint];
}

Vector3 Tr2SkinnedObject::GetBonePosition( unsigned joint ) const
{
	CCP_ASSERT( joint < m_skinningMatrixCount );

	if( joint >= m_skinningMatrixCount )
	{
		// I know we just asserted for this, but Python programmers can't be trusted.
		// Check for this and fail gracefully.
		CCP_LOGWARN( "Tr2SkinnedObject::GetBonePosition called with an invalid joint index (%d)", joint );
		return Vector3( 0, 0, 0 );
	}

	if( const Matrix* m = GetBoneTransform( joint ))
	{
		return Vector3( m->_41, m->_42, m->_43 );
	}
	return Vector3( 0, 0, 0 );
}

void Tr2SkinnedObject::SetPosition(const Vector3 &pos)
{
	m_transform._41 = pos.x;
	m_transform._42 = pos.y;
	m_transform._43 = pos.z;
}

const Quaternion Tr2SkinnedObject::GetRotation() const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpRotation;
}

void Tr2SkinnedObject::SetRotation( const Quaternion& rotQuat )
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	
	
	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &tmpScale, NULL, &rotQuat, &tmpTranslation );

	return;
}

const Vector3 Tr2SkinnedObject::GetScaling() const
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	
	
	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );

	return tmpScale;
}

void Tr2SkinnedObject::SetScaling( const Vector3& scaleVec )
{
	Vector3		tmpScale;		
	Quaternion	tmpRotation;	
	Vector3		tmpTranslation;	

	D3DXMatrixDecompose( &tmpScale, &tmpRotation, &tmpTranslation, &m_transform );
	D3DXMatrixTransformation( &m_transform, NULL, NULL, &scaleVec, NULL, &tmpRotation, &tmpTranslation );
}

void Tr2SkinnedObject::ResetAnimationBindings()
{
	if( m_visualModel )
	{
		m_visualModel->ResetBindings();
	}
}

void Tr2SkinnedObject::RenderDebugInfo( Tr2RenderContext& renderContext )
{
	if( !DoDisplay() )
	{
		return;
	}

	const Vector3& position = GetPosition();

	if( m_displayMarker && m_lineSet != NULL )
	{
		const uint32_t colorRed  ( 0xffff0000 );
		const uint32_t colorGreen( 0xff00ff00 );
		const uint32_t colorBlue ( 0xff0000ff );
		const uint32_t colorWhite( 0xffffffff );

		m_lineSet->Add( position, colorRed  , position + Vector3( 1.0f, 0.0f, 0.0f ), colorRed   );
		m_lineSet->Add( position, colorGreen, position + Vector3( 0.0f, 1.0f, 0.0f ), colorGreen );
		m_lineSet->Add( position, colorBlue , position + Vector3( 0.0f, 0.0f, 1.0f ), colorBlue  );

		Vector3 vDirectionPlus(0.5f,1.4f,0.0f);
		Vector3 vDirectionNeg(-0.5f,1.4f,0.0f);

		Vector3 vPlus;
		D3DXVec3TransformCoord( &vPlus, &vDirectionPlus, &m_transform );
		Vector3 vNeg;
		D3DXVec3TransformCoord( &vNeg, &vDirectionNeg, &m_transform );

		m_lineSet->Add( vNeg , colorWhite, vPlus, colorWhite );
		m_lineSet->Add( position + Vector3(0.0f,1.0f,0.0f)  , colorWhite, position + Vector3(0.0f,1.8f,0.0f) , colorWhite );

		Vector3 vTravel(0.0f,0.0f,-1.0f);
		D3DXVec3TransformCoord( &vTravel, &vTravel, &m_transform );
		m_lineSet->Add( position , colorWhite, vTravel, colorWhite );
	}

	if( m_displayName )
	{
		Vector3 pos = position;
		pos.y += 1.6f;
		Tr2Renderer::Printf( TRI_DBG_FONT_MEDIUM, pos, 0xffff00ff, m_name.c_str() );
	}

	m_lineSet->Render( renderContext );
}

void Tr2SkinnedObject::GetBatches( ITriRenderBatchAccumulator* batches,
								   TriBatchType batchType,
								   const Tr2PerObjectData* perObjectData )
{
	#if APEX_ENABLED
	// Warning: Tr2IntSkinnedObject overrides GetBatches to handle
	// transparent batches a bit differently.
	if( DoDisplay()									&& 
		m_lod.IsSimulatingCloth( g_maxClothLod )	&& 
		g_Tr2Apex != NULL							&&
			(batchType == TRIBATCHTYPE_DECAL		||
			 batchType == TRIBATCHTYPE_TRANSPARENT	||
			 batchType == TRIBATCHTYPE_DEPTHNORMAL	||
			 batchType == TRIBATCHTYPE_DEPTH )
		)
	{
		g_Tr2Apex->ApexGatherBatches( m_clothMeshes, batches, batchType, perObjectData );
	}
	#endif

	if( m_visualModel && perObjectData && DoDisplay() )
	{
		m_visualModel->GetBatches( batches, batchType, GetSkinningTransform(), perObjectData );
	}
}

bool Tr2SkinnedObject::HasTransparentBatches()
{
	if( m_visualModel )
	{
		return m_visualModel->HasTransparency();
	}
	return false;
}

void Tr2SkinnedObject::UpdatePerObjectData()
{
	CCP_STATS_ZONE( __FUNCTION__ );

	if( !m_visualModel )
	{
		// Nothing to render
		return;
	}

	if( m_skinningMatrixCount )
	{
		// if the model is of type cpuskinned, then we can call ::deform
		Tr2CpuSkinnedModel* cpuSkinnedModel = dynamic_cast<Tr2CpuSkinnedModel*>( m_visualModel.p );
		if( cpuSkinnedModel )
		{
			cpuSkinnedModel->deform( GetSkinningMatrices(), m_skinningMatrixCount );
		}
	}
}

bool Tr2SkinnedObject::GetLocalBoundingBox( Vector3& min, Vector3& max ) const
{
	if( m_useExplicitBounds )
	{
		min = m_minBounds;
		max = m_maxBounds;
		return true;
	}

	// pass down to 
	if( m_visualModel )
	{
		return m_visualModel->GetBoundingBox( min, max );
	}
	return false;
}

// -------------------------------------------------------------
// Description:
//   Computes an Oriented Bounding Box in world space, which has been shrunk
//   to fit the current viewing frustum as tightly as possible without visibly clipping
//   into it.  The frustum is derived from Tr2Renderer GetViewTransform, GetProjectionTransform, and so on.
//   The calculation starts out with the AABB from GetLocalBoundingBox, so the explicit bounds
//   are supported.
// Arguments:
//   localToWorld - [in]  transform that takes this skinned object from local to world coordinates
//   x, y, z      - [out] normalized vectors defining the OBB's orientation
//   center       - [out] the center, in world coordinates, of the OBB
//   sizes        - [out] half the size of the OBB along every axis x, y or z. Ie you get to a corner point with center + sizes[0] * x. Full width/height/depth is sizes*2.
// Return Value:
//   false if for some reason getting the object's bounding box in local coordinates didn't work.
// SeeAlso:
//   GetLocalBoundingBox
// Summary:
//   Compute a world-space OBB, tightened for the current frustum.
// -------------------------------------------------------------
bool Tr2SkinnedObject::GetClippedWorldBoundingObb( const Matrix& localToWorld, Vector3& x, Vector3& y, Vector3& z, Vector3& center, Vector3& sizes )
{
	Vector3 min, max;
	if( !GetLocalBoundingBox( min, max ) )
	{
		return false;
	}

	TriFrustum frustum;
	
	frustum.DeriveFrustum( 
			&Tr2Renderer::GetViewTransform(), 
			&Tr2Renderer::GetViewPosition(), 
			&Tr2Renderer::GetProjectionTransform(), 
			Tr2Renderer::GetViewport()
			);
	
	Obb obb;
	obb.CreateClippedWorldBoundingObb( min, max, localToWorld, &frustum );
	x = obb.x;
	y = obb.y;
	z = obb.z;
	center = obb.center;
	sizes = obb.sizes;
	return true;
}

void Tr2SkinnedObject::AllocateSkinningMatrices( unsigned int numBones )
{
	CCP_ASSERT( numBones > 0 );

	m_skinningMatrixCount = numBones;

	unsigned int numToAllocate = m_debugSkeletonTrailLength;
	if( numToAllocate < m_skinningMatrixFrameDelay + 1 )
	{
		numToAllocate = m_skinningMatrixFrameDelay + 1;
	}

	for( unsigned int i = 0; i < numToAllocate; ++i )
	{
		float* p = CCP_NEW( "Tr2SkinnedObject/m_skinningMatrices" ) float[numBones * 3*4];
		m_skinningMatrixQueue.push_back( p );
		m_worldTransformsQueue.push_back( Tr2Renderer::GetIdentityTransform() );

		Matrix* m = CCP_NEW( "Tr2SkinnedObject/m_accumulatedTransformsQueue" ) Matrix[numBones];
		m_accumulatedTransformsQueue.push_back( m );

		if( m_debugRenderSkeletonTrail )
		{
			for( unsigned int idIx = 0; idIx < m_skinningMatrixCount; ++idIx )
			{
				D3DXMatrixIdentity( &m[idIx] );
			}
		}
	}

	m_skinningMatrixQueueNeedsPriming = true;
}

void Tr2SkinnedObject::FreeSkinningMatrices()
{
	for( SkinningMatrixQueue_t::iterator it = m_skinningMatrixQueue.begin(); it != m_skinningMatrixQueue.end(); ++it )
	{
		CCP_DELETE [] *it;
	}
	m_skinningMatrixQueue.clear();
	m_worldTransformsQueue.clear();

	for( AccumulatedTransforms_t::iterator it = m_accumulatedTransformsQueue.begin(); it != m_accumulatedTransformsQueue.end(); ++it )
	{
		CCP_DELETE [] *it;
	}
	m_accumulatedTransformsQueue.clear();
	m_skinningMatrixQueueIndex = 0;
	m_skinningMatrixCount = 0;
}

void Tr2SkinnedObject::ResetSkinningMatrices()
{
	unsigned int numBones = m_skinningMatrixCount;
	FreeSkinningMatrices();
	AllocateSkinningMatrices( numBones );
}

float* Tr2SkinnedObject::GetSkinningMatrices()
{
	if( m_skinningMatrixQueue.empty() )
	{
		return NULL;
	}

	unsigned int ix = (m_skinningMatrixQueueIndex - m_skinningMatrixFrameDelay + (unsigned int)m_skinningMatrixQueue.size()) % m_skinningMatrixQueue.size();
	return m_skinningMatrixQueue[ix];
}

const Matrix& Tr2SkinnedObject::GetSkinningTransform() const
{
	if( m_worldTransformsQueue.empty() )
	{
		return m_transform;
	}

	unsigned int ix = (m_skinningMatrixQueueIndex - m_skinningMatrixFrameDelay + (unsigned int)m_worldTransformsQueue.size()) % m_worldTransformsQueue.size();
	return m_worldTransformsQueue[ix];
}

unsigned int Tr2SkinnedObject::GetSkinningMatrixFrameDelay() const
{
	return m_skinningMatrixFrameDelay;
}

void Tr2SkinnedObject::SetSkinningMatrixFrameDelay( unsigned int val )
{
	if( val != m_skinningMatrixFrameDelay )
	{
		m_skinningMatrixFrameDelay = val;
		ResetSkinningMatrices();
	}
}

void Tr2SkinnedObject::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
#if APEX_ENABLED
	switch( event )
	{
		case BELIST_REMOVED:
			{
				Tr2ClothingActor* ca = dynamic_cast<Tr2ClothingActor*>( value );
				if( ca )
				{
					ca->RemoveFromApexScene();
				}
			}
			break;

		case BELIST_UNLOADSTART:
			{
				for( Tr2ClothingActorVector::iterator it = m_clothMeshes.begin(); it != m_clothMeshes.end(); ++it )
				{
					if( Tr2ClothingActor* ca = *it )
					{
						ca->RemoveFromApexScene();
					}
				}
			}
			break;
	}
#endif
}

bool Tr2SkinnedObject::OnModified( Be::Var* value )
{
	if( IsMatch( value, m_visualModel ) )
	{
		// paperdoll is changing the visual model.  Make sure that the LOD system knows about, else when we go to another lod and come
		// back, we'll get proxy->GetObject and the changes will be lost.
		m_lod.OnModelChanged( m_visualModel );
		return true;
	}
	else if( IsMatch( value, m_useExplicitBounds ) || IsMatch( value, m_minBounds ) || IsMatch( value, m_maxBounds ) )
	{
		ExplicitBoundsChanged();
		return true;
	}

	return false;
}

void Tr2SkinnedObject::SetHighDetailModel( Tr2SkinnedModel* model )
{
	m_lod.SetHighDetailModel( model );	
}

void Tr2SkinnedObject::SetMediumDetailModel( Tr2SkinnedModel* model )
{
	m_lod.SetMediumDetailModel( model );
}

void Tr2SkinnedObject::SetLowDetailModel( Tr2SkinnedModel * model )
{
	m_lod.SetLowDetailModel( model );
}

void Tr2SkinnedObject::SetLOD( const TriFrustum* frustum )
{
	if( !frustum )
	{
		return;
	}

#if APEX_ENABLED
	// are we waiting for a cloth transition to finish?  If so, stick with the current lod until that's done.
	bool clothFadeFinishedThisFrame = false;
	if( m_clothFadeEndTime )
	{
		if( BeOS->GetActualTime() < m_clothFadeEndTime )
		{
			return;
		}

		m_clothFadeEndTime = 0;
		clothFadeFinishedThisFrame = true;	// allow a switch to the next lod, ie. don't accidentally retrigger a new blend in this update
	}
#endif

	Vector4 boundingSphere;
	if( GetBoundingSphere( boundingSphere ) 
		&& frustum->IsSphereVisible( &boundingSphere, true ) )
	{
		const float estimate = frustum->GetPixelSizeAccross( &boundingSphere );
		if( estimate >= 0.0f && estimate < 1000000.0f )	// block off any remaining silliness
		{
			m_estimatedPixelDiameter = estimate;
		}
	}

	Tr2SkinnedModel* model = m_lod.SetLOD( frustum, m_estimatedPixelDiameter );

#if APEX_ENABLED
	const int oldLod = m_lod.GetCurrentLod();

	// trying to move to a lod without simulated cloth?  If so, delay that if there is a fade-out period.
	if( !clothFadeFinishedThisFrame && oldLod <= g_maxClothLod && m_lod.GetCurrentLod() > g_maxClothLod )
	{
		float biggestBlendTime = 0.0f;
		for( size_t i = 0; i != m_clothMeshes.size(); ++i)
		{
			CCP_ASSERT( m_clothMeshes[i] != NULL );
			if( m_clothMeshes[i] != NULL )
			{
				biggestBlendTime = std::max( biggestBlendTime, m_clothMeshes[i]->GetMaxDistanceBlendTime() );
				m_clothMeshes[i]->SetForcedLod( 0.0f );	// request skinned
			}
		}

		// any piece of cloth wants to blend to skinned?
		if( biggestBlendTime )
		{
			m_clothFadeEndTime = BeOS->GetActualTime() + TimeFromDouble( biggestBlendTime );
			m_lod.SetCurrentLod( oldLod );	// undo the move away from old lod
			return;
		}
	}
	else
	// check the opposite: we go from a no-(simulated)-cloth-lod to one that has (simulated) cloth.
	// In this case, make sure we re-enable nvidia simulation.
	// The reason we do this here, and not just nicely at the end of the fade out at the top of this function, 
	// is that keeping it in a skinned state is cheaper, and also gives us the right fade-in again from skinned to simulated.
	if( oldLod > g_maxClothLod && m_lod.GetCurrentLod() <= g_maxClothLod )
	{
		// end the fade
		for( size_t i = 0; i != m_clothMeshes.size(); ++i)
		{
			if( m_clothMeshes[i] != NULL )
			{
				m_clothMeshes[i]->SetForcedLod( -1.0f );	// nvidia magic number for 'simulate'
			}
		}
	}
#endif

	// lod change?
	if( model && model != m_visualModel )	// don't just compare lod numbers, actual model of a given lod might change as well (dynamic builders)
	{
		m_visualModel = model;
			
		// this invalidates the rig-bindings, cause we switched to a new skeleton and therefore have to rebuild
		m_skinningMatrixCount = 0;

		// the skeleton is only posed during Update; make sure we don't render a frame with a model in the
		// restpose, by posing it right now.
		UpdateBones( m_lastUpdateTime, NULL );
	}
}

bool Tr2SkinnedObject::GetDebugRenderSkeletonTrail() const
{
	return m_debugRenderSkeletonTrail;
}

void Tr2SkinnedObject::SetDebugRenderSkeletonTrail( bool val )
{
	m_debugRenderSkeletonTrail = val;
	ResetSkinningMatrices();
}

unsigned int Tr2SkinnedObject::GetDebugSkeletonTrailLength() const
{
	return m_debugSkeletonTrailLength;
}

void Tr2SkinnedObject::SetDebugSkeletonTrailLength( unsigned int val )
{
	m_debugSkeletonTrailLength = val;
	ResetSkinningMatrices();
}

AxisAlignedBoundingBox Tr2SkinnedObject::GetBoundingBoxInLocalSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.0f, 0.0f, 0.0f ), Vector3( 0.0f, 0.0f, 0.0f ) );
	GetLocalBoundingBox( result.m_min, result.m_max );
	return result;
}
