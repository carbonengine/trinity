// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

#include "Tr2SkinnedObject.h"
#include "Resources/TriGeometryRes.h"
#include "TriFrustum.h"
#include "TriSettingsRegistrar.h"
#include "include/ITr2DebugRenderer.h"
#include "Utilities/Obb.h"
#include "Curves/TriCurveSet.h"
#include "TriLineSet.h"
#include "Include/ITr2AnimationUpdater.h"
#include "ITr2WorldTransformUpdater.h"

Tr2SkinnedObject::Tr2SkinnedObject( IRoot* lockobj ) :
	PARENTLOCK( m_transform ),
	PARENTLOCK( m_curveSets ),
	m_skinningMatrixFrameDelay( 0 ),
	m_skinningMatrixQueueIndex( 0 ),
	m_boneList( NULL ),
	m_skinningMatrixCount( 0 ),
	m_skinningMatrixQueue( "Tr2SkinnedObject/m_skinningMatrixQueue" ),
	m_worldTransformsQueue( "Tr2SkinnedObject/m_worldTransformsQueue" ),
	m_accumulatedTransformsQueue( "Tr2SkinnedObject/m_accumulatedTransformQueue" ),
	m_skinningMatrixQueueNeedsPriming( true ),
	m_display( true ),
	m_animRigToRenderRigMapping( NULL ),
	m_renderRigToAnimRigMapping( NULL ),
	m_animationUpdater( (ITr2AnimationUpdater*)NULL ),
	m_worldTransformUpdater( (ITr2WorldTransformUpdater*)NULL ),
	m_renderRigBoneList( "Tr2SkinnedObject/m_renderRigBoneList" ),
	m_numRenderRigBones( 0 ),
	m_lastUpdateTime( 0 ),
	m_lastTranslation( 0.0f, 0.0f, 0.0f ),
	m_estimatedPixelDiameter( 0.0f ),
	m_skeletonTag( 0 ),
	m_useDynamicBounds( true ),
	m_useExplicitBounds( false ),
	m_hasDynamicBounds( false ),
	m_minBounds( 0.0f, 0.0f, 0.0f ),
	m_maxBounds( 0.0f, 0.0f, 0.0f ),
	m_minDynamicBounds( 0, 0, 0 ),
	m_maxDynamicBounds( 0, 0, 0 ),
	m_updatePeriod( 0.0f )
{
}

Tr2SkinnedObject::~Tr2SkinnedObject()
{
	CCP_DELETE[] m_animRigToRenderRigMapping;
	;
	CCP_DELETE[] m_renderRigToAnimRigMapping;
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
		( *it )->Update( TimeAsDouble( time ) );
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

	m_hasDynamicBounds = false;

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
			m_renderRigBoneList.resize( numRenderRigBones );
			for( unsigned int i = 0; i < numRenderRigBones; ++i )
			{
				m_renderRigBoneList[i] = skel->m_joints[i].m_name;
			}
			rebuildMapping = true;
		}

		if( m_numRenderRigBones == 0 )
		{
			// Guard against an empty skeleton - shouldn't happen but can
			// if assets are bad.
			CCP_LOGERR(
				"Tr2SkinnedObject '%s' has an empty skeleton in visual model from '%s'",
				m_name.c_str(),
				m_visualModel->GetGeometryResPath() );
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

		CCP_DELETE[] m_animRigToRenderRigMapping;
		CCP_DELETE[] m_renderRigToAnimRigMapping;
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

	if( skel != NULL )
	{
		CCP_STATS_ZONE( "UpdateBones_MatrixUpdate" );

		++m_skinningMatrixQueueIndex;
		m_skinningMatrixQueueIndex %= m_skinningMatrixQueue.size();

		const Matrix* accumulatedTransforms = NULL;

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

			if( accumulatedTransforms )
			{
				if( m_visualModel->GetDynamicBoundingBox( m_animationUpdater->GetAnimationTransforms(), m_minDynamicBounds, m_maxDynamicBounds ) )
				{
					m_hasDynamicBounds = true;
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
				if( accumulatedTransforms )
				{
					const Matrix& invBind = skel->m_joints[renderRigIx].m_inverseWorldTransform;

					final = invBind * accumulatedTransforms[transformIx];
				}
				else
				{
					final = IdentityMatrix();
				}

				float* p = &dst[transformIx * 3 * 4];
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
				unsigned int queueIx = ( m_skinningMatrixQueueIndex + ix ) % m_skinningMatrixQueue.size();
				float* copyDst = m_skinningMatrixQueue[queueIx];
				memcpy( copyDst, dst, m_skinningMatrixCount * sizeof( float ) * 3 * 4 );

				m_worldTransformsQueue[queueIx] = m_transform;
			}

			m_skinningMatrixQueueNeedsPriming = false;
		}
	}
}

unsigned Tr2SkinnedObject::GetBoneIndex( const std::string& boneName ) const
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
		for( unsigned int i = 0; i < skel->m_joints.size(); ++i )
		{
			CCP_LOG( "%d - %s", i, skel->m_joints[i].m_name.c_str() );
		}
	}
}

unsigned Tr2SkinnedObject::GetSkeletonTag() const
{
	return m_skeletonTag;
}

const Matrix* Tr2SkinnedObject::GetBoneTransform( unsigned joint ) const
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

	if( const Matrix* m = GetBoneTransform( joint ) )
	{
		return Vector3( m->_41, m->_42, m->_43 );
	}
	return Vector3( 0, 0, 0 );
}

void Tr2SkinnedObject::SetPosition( const Vector3& pos )
{
	m_transform._41 = pos.x;
	m_transform._42 = pos.y;
	m_transform._43 = pos.z;
}

const Quaternion Tr2SkinnedObject::GetRotation() const
{
	Vector3 tmpScale;
	Quaternion tmpRotation;
	Vector3 tmpTranslation;

	Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );

	return tmpRotation;
}

void Tr2SkinnedObject::SetRotation( const Quaternion& rotQuat )
{
	Vector3 tmpScale;
	Quaternion tmpRotation;
	Vector3 tmpTranslation;

	Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );
	static_cast<Matrix&>( m_transform ) = TransformationMatrix( tmpScale, rotQuat, tmpTranslation );

	return;
}

const Vector3 Tr2SkinnedObject::GetScaling() const
{
	Vector3 tmpScale;
	Quaternion tmpRotation;
	Vector3 tmpTranslation;

	Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );

	return tmpScale;
}

void Tr2SkinnedObject::SetScaling( const Vector3& scaleVec )
{
	Vector3 tmpScale;
	Quaternion tmpRotation;
	Vector3 tmpTranslation;

	Decompose( tmpScale, tmpRotation, tmpTranslation, m_transform );
	static_cast<Matrix&>( m_transform ) = TransformationMatrix( scaleVec, tmpRotation, tmpTranslation );
}

void Tr2SkinnedObject::ResetAnimationBindings()
{
	if( m_visualModel )
	{
		m_visualModel->ResetBindings();
	}
}

void Tr2SkinnedObject::GetDebugOptions( Tr2DebugRendererOptions& options )
{
	options.insert( "Names" );
	options.insert( "Transforms" );
	options.insert( "Bounds" );
	if( m_visualModel && m_visualModel->GetSkeleton() )
	{
		options.insert( "Bones" );
		options.insert( "Bones Names" );
	}
}

void Tr2SkinnedObject::RenderDebugInfo( ITr2DebugRenderer2& renderer )
{
	if( !DoDisplay() )
	{
		return;
	}

	if( renderer.HasOption( GetRawRoot(), "Names" ) )
	{
		Vector3 pos = GetPosition();
		Vector3 boxMin, boxMax;
		if( GetLocalBoundingBox( boxMin, boxMax ) )
		{
			pos = Vector3( 0, boxMax.y, 0 );
		}
		renderer.DrawText( TRI_DBG_FONT_MEDIUM, TransformCoord( pos, GetTransform() ), 0xffff00ff, "%s", m_name.c_str() );
	}
	if( renderer.HasOption( GetRawRoot(), "Transforms" ) )
	{
		renderer.DrawAxis( this, m_transform, Tr2DebugRenderer::Lit );
	}
	if( renderer.HasOption( GetRawRoot(), "Bounds" ) )
	{
		Vector3 boxMin, boxMax;
		if( GetLocalBoundingBox( boxMin, boxMax ) )
		{
			if( m_useExplicitBounds )
			{
				renderer.DrawBox( GetRawRoot(), m_transform, boxMin, boxMax, Tr2DebugRenderer::Wireframe, 0xffff8800 );
			}
			else
			{
				renderer.DrawBox( GetRawRoot(), m_transform, boxMin, boxMax, Tr2DebugRenderer::Wireframe, 0xffffff00 );
			}
		}
	}
	if( renderer.HasOption( GetRawRoot(), "Bones" ) && m_visualModel && m_visualModel->GetSkeleton() )
	{
		TriGeometryResSkeletonData* skel = m_visualModel->GetSkeleton();
		unsigned int n = (unsigned int)m_accumulatedTransformsQueue.size();
		for( unsigned int frameIx = 0; frameIx < n; ++frameIx )
		{
			unsigned int ix = ( m_skinningMatrixQueueIndex + n - frameIx ) % n;
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
							from = XMVector3TransformCoord( from, mat );
							to = XMVector3TransformCoord( to, mat );

							renderer.DrawLine( GetRawRoot(), from, to, Tr2DebugColor( 0xffff0000, 0x44ff0000 ) );
						}
					}
				}
			}
		}
	}
	if( renderer.HasOption( GetRawRoot(), "Bones Names" ) && m_visualModel && m_visualModel->GetSkeleton() )
	{
		TriGeometryResSkeletonData* skel = m_visualModel->GetSkeleton();
		unsigned int n = (unsigned int)m_accumulatedTransformsQueue.size();
		for( unsigned int frameIx = 0; frameIx < n; ++frameIx )
		{
			unsigned int ix = ( m_skinningMatrixQueueIndex + n - frameIx ) % n;
			const Matrix& worldTransform = m_worldTransformsQueue[ix];

			for( unsigned int animRigIx = 0; animRigIx < m_skinningMatrixCount; ++animRigIx )
			{
				unsigned int renderRigIx = m_animRigToRenderRigMapping[animRigIx];
				if( renderRigIx != -1 )
				{
					if( renderer.HasOption( GetRawRoot(), "Bones Names" ) )
					{
						Vector3 from = TransformCoord( m_accumulatedTransformsQueue[ix][animRigIx].GetTranslation(), GetTransform() );
						renderer.DrawText( TRI_DBG_FONT_SMALL, from, 0x99ffffff, "%s", skel->m_joints[renderRigIx].m_name.c_str() );
					}
				}
			}
		}
	}
}

void Tr2SkinnedObject::GetBatches( ITriRenderBatchAccumulator* batches,
								   TriBatchType batchType,
								   const Tr2PerObjectData* perObjectData,
								   Tr2RenderReason reason )
{
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
}

bool Tr2SkinnedObject::GetLocalBoundingBox( Vector3& min, Vector3& max ) const
{
	if( m_useExplicitBounds )
	{
		min = m_minBounds;
		max = m_maxBounds;
		return true;
	}

	if( m_useDynamicBounds && m_hasDynamicBounds )
	{
		min = m_minDynamicBounds;
		max = m_maxDynamicBounds;
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
bool Tr2SkinnedObject::GetClippedWorldBoundingObb( const Matrix& localToWorld, Vector3& x, Vector3& y, Vector3& z, Vector3& center, Vector3& sizes, TriFrustum* cameraFrustum )
{
	Vector3 min, max;
	if( !GetLocalBoundingBox( min, max ) )
	{
		return false;
	}

	Obb obb;
	obb.CreateClippedWorldBoundingObb( min, max, localToWorld, cameraFrustum );
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

	unsigned int numToAllocate = m_skinningMatrixFrameDelay + 1;

	float* p = CCP_NEW( "Tr2SkinnedObject/m_skinningMatrices" ) float[numBones * 3 * 4];
	m_skinningMatrixQueue.push_back( p );
	m_worldTransformsQueue.push_back( IdentityMatrix() );

	Matrix* m = CCP_NEW( "Tr2SkinnedObject/m_accumulatedTransformsQueue" ) Matrix[numBones];
	m_accumulatedTransformsQueue.push_back( m );
	std::fill_n( m, m_skinningMatrixCount, IdentityMatrix() );

	m_skinningMatrixQueueNeedsPriming = true;
}

void Tr2SkinnedObject::FreeSkinningMatrices()
{
	for( SkinningMatrixQueue_t::iterator it = m_skinningMatrixQueue.begin(); it != m_skinningMatrixQueue.end(); ++it )
	{
		CCP_DELETE[] * it;
	}
	m_skinningMatrixQueue.clear();
	m_worldTransformsQueue.clear();

	for( AccumulatedTransforms_t::iterator it = m_accumulatedTransformsQueue.begin(); it != m_accumulatedTransformsQueue.end(); ++it )
	{
		CCP_DELETE[] * it;
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

	unsigned int ix = ( m_skinningMatrixQueueIndex - m_skinningMatrixFrameDelay + (unsigned int)m_skinningMatrixQueue.size() ) % m_skinningMatrixQueue.size();
	return m_skinningMatrixQueue[ix];
}

const Matrix& Tr2SkinnedObject::GetSkinningTransform() const
{
	if( m_worldTransformsQueue.empty() )
	{
		return m_transform;
	}

	unsigned int ix = ( m_skinningMatrixQueueIndex - m_skinningMatrixFrameDelay + (unsigned int)m_worldTransformsQueue.size() ) % m_worldTransformsQueue.size();
	return m_worldTransformsQueue[ix];
}

void Tr2SkinnedObject::OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* theList )
{
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

void Tr2SkinnedObject::SetLowDetailModel( Tr2SkinnedModel* model )
{
	m_lod.SetLowDetailModel( model );
}

void Tr2SkinnedObject::SetLOD( const TriFrustum* frustum )
{
	if( !frustum )
	{
		return;
	}

	Vector4 boundingSphere;
	if( GetBoundingSphere( boundingSphere ) && frustum->IsSphereVisible( &boundingSphere, true ) )
	{
		const float estimate = frustum->GetPixelSizeAccross( &boundingSphere );
		if( estimate >= 0.0f && estimate < 1000000.0f ) // block off any remaining silliness
		{
			m_estimatedPixelDiameter = estimate;
		}
	}

	Tr2SkinnedModel* model = m_lod.SetLOD( frustum, m_estimatedPixelDiameter );

	// lod change?
	if( model && model != m_visualModel ) // don't just compare lod numbers, actual model of a given lod might change as well (dynamic builders)
	{
		m_visualModel = model;

		// this invalidates the rig-bindings, cause we switched to a new skeleton and therefore have to rebuild
		m_skinningMatrixCount = 0;

		// the skeleton is only posed during Update; make sure we don't render a frame with a model in the
		// restpose, by posing it right now.
		UpdateBones( m_lastUpdateTime, NULL );
	}
}

AxisAlignedBoundingBox Tr2SkinnedObject::GetBoundingBoxInLocalSpace() const
{
	AxisAlignedBoundingBox result( Vector3( 0.0f, 0.0f, 0.0f ), Vector3( 0.0f, 0.0f, 0.0f ) );
	GetLocalBoundingBox( result.m_min, result.m_max );
	return result;
}
