#include "StdAfx.h"
#include "Tr2GrannyAnimationLayer.h"
#include "Resources/TriGrannyRes.h"
#include "Tr2GrannyAnimation.h"
#include "Tr2Renderer.h"


Tr2GrannyAnimationLayer::Tr2GrannyAnimationLayer() :
	m_modelInstance( nullptr ),
	m_trackMask( nullptr ),
	m_trackMaskName( nullptr ),
	m_defaultBoneWeight( 0.f ),
	m_boneCount( 0 ),
	m_layerWeight( 1.f ),
	m_controlParam( 0.f ),
	m_controlParamTarget( 0.f ),
	m_lastControlUpdateTime( 0.f ),
	m_controlParamEnabled( false ),
	m_basePose( nullptr ),
	m_skewRate( 0.f ),
	m_pauseTime( 0.f ),
	m_paused( false ),
	m_totalPauseOffset( 0.f )
{
}

Tr2GrannyAnimationLayer::Tr2GrannyAnimationLayer( float defaultBoneWeight ) :
	m_modelInstance( nullptr ),
	m_trackMask( nullptr ),
	m_trackMaskName( nullptr ),
	m_defaultBoneWeight( defaultBoneWeight ),
	m_boneCount( 0 ),
	m_layerWeight( 1.f ),
	m_controlParam( 0.f ),
	m_controlParamTarget( 0.f ),
	m_lastControlUpdateTime( 0.f ),
	m_controlParamEnabled( false ),
	m_basePose( nullptr ),
	m_skewRate( 0.f ),
	m_pauseTime( 0.f ),
	m_paused( false ),
	m_totalPauseOffset( 0.f )
{
}

Tr2GrannyAnimationLayer::Tr2GrannyAnimationLayer( float defaultBoneWeight, float layerWeight ) :
	m_modelInstance( nullptr ),
	m_trackMask( nullptr ),
	m_trackMaskName( nullptr ),
	m_defaultBoneWeight( defaultBoneWeight ),
	m_boneCount( 0 ),
	m_layerWeight( layerWeight ),
	m_controlParam( 0.f ),
	m_controlParamTarget( 0.f ),
	m_lastControlUpdateTime( 0.f ),
	m_controlParamEnabled( false ),
	m_basePose( nullptr ),
	m_skewRate( 0.f ),
	m_pauseTime( 0.f ),
	m_paused( false ),
	m_totalPauseOffset( 0.f )
{
}

void Tr2GrannyAnimationLayer::InitializeAnimationLayer( const Tr2GrannyAnimation* grannyAnimation )
{
	granny_model* model = grannyAnimation->GetGrannyModel();
	if( !model )
	{
		return;
	}

	m_modelInstance = GrannyInstantiateModel( model );
	granny_skeleton* skeleton = GrannyGetSourceSkeleton( m_modelInstance );
	m_boneCount = skeleton->BoneCount;
	m_trackMask = GrannyNewTrackMask( m_defaultBoneWeight, skeleton->BoneCount );

	if( m_trackMaskName )
	{
		ExtractTrackMask( grannyAnimation, m_trackMaskName );
	}
	for( auto it = m_bones.begin(); it != m_bones.end(); it++ )
	{
		unsigned int boneIndex;
		if( grannyAnimation->FindBoneByName( it->c_str(), boneIndex ) )
		{
			GrannySetTrackMaskBoneWeight( m_trackMask, boneIndex, m_defaultBoneWeight );
		}
	}
}

void Tr2GrannyAnimationLayer::ConsumeAnimationQueue( const Tr2GrannyAnimation* animationController )
{
	// PlayAnimation will clear the animation queue if the replace flag is set.
	// We really should be searching backwards through the queue here to find the
	// last entry with replace set, then go forward from there. In reality we
	// rarely have more than one entry here, and this approach is much simpler:
	// Make a copy of the queue, iterate through that and be done with it...
	std::vector<AnimationRequest> queueSnapshot = m_animationQueue;
	for( auto it = queueSnapshot.cbegin(); it != queueSnapshot.cend(); ++it )
	{
		PlayAnimation( animationController, it->m_animationName.c_str(), it->m_replace, it->m_loopCount, it->m_start, it->m_speed, it->m_clearWhenDone );
	}
	m_animationQueue.clear();
}

void Tr2GrannyAnimationLayer::QueueAnimation( const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone )
{
	AnimationRequest ar;
	ar.m_animationName = animName;
	ar.m_replace = replace;
	ar.m_clearWhenDone = clearWhenDone;
	ar.m_loopCount = loopCount;
	ar.m_start = delay;
	ar.m_speed = speed;

	m_animationQueue.push_back( ar );
}


float Tr2GrannyAnimationLayer::GetLayerAnimationTime()
{
	if ( m_paused )
	{
		return m_pauseTime;
	}
	return Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
}


void Tr2GrannyAnimationLayer::TogglePauseAnimation( bool pause )
{
	if ( m_paused && !pause )
	{
		m_paused = false;
		m_totalPauseOffset = Tr2Renderer::GetAnimationTime() - m_pauseTime;
		m_pauseTime = 0.0;
	}
	else if ( !m_paused && pause )
	{
		m_paused = true;
		m_pauseTime = Tr2Renderer::GetAnimationTime() - m_totalPauseOffset;
	}
}


bool Tr2GrannyAnimationLayer::PlayAnimation( const Tr2GrannyAnimation* grannyAnimation, const char* animName, bool replace, int loopCount, float delay, float speed, bool clearWhenDone )
{
	if( !m_modelInstance )
	{
		return false;
	}

	const granny_animation* animation = grannyAnimation->FindAnimationByName( animName );
	if( !animation )
	{
		return false;
	}

	if( replace )
	{
		ClearAnimations();
	}

	float startTime = GetLayerAnimationTime();
	if( !replace )
	{
		float maxRemaining = 0.0f;
		for(	granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); 
				binding != GrannyModelControlsEnd( m_modelInstance ); 
				binding = GrannyModelControlsNext( binding ) )
		{
			granny_control *control = GrannyGetControlFromBinding( binding );
			if( !control )
			{
				CCP_LOGERR( "Failed to play animation %s: could not create granny control instance. Possible reason is the skeleton mismatch", animName );
				return false;
			}

			// Force control to stop at the end of its current loop iteration
			int loopCount = max(0, GrannyGetControlLoopIndex( control ));
			int newLoopCount = loopCount + 1;
		
			GrannySetControlLoopCount( control, newLoopCount );
			float remaining = GrannyGetControlDurationLeft( control );
			GrannyCompleteControlAt( control, GetLayerAnimationTime() + remaining );
			if( remaining > maxRemaining )
			{
				maxRemaining = remaining;
			}
		}

		delay += maxRemaining;
	}
	startTime += delay;

	granny_control* control = GrannyPlayControlledAnimation( startTime, animation, m_modelInstance );  
	if( !control )
	{
		CCP_LOGERR( "Failed to play animation %s: could not create granny control instance. Possible reason is the skeleton mismatch", animName );
		return false;
	}
	GrannyEaseControlIn( control, 0.0f, false );
	GrannySetControlLoopCount( control, loopCount );
	GrannySetControlSpeed( control, speed );

	if( loopCount > 0 && clearWhenDone )
	{
		GrannyCompleteControlAt( control, GetLayerAnimationTime() + GrannyGetControlDurationLeft( control ) + delay );
	}

	GrannySetControlClock( control, GetLayerAnimationTime());
	RegisterTextTracks( control, animation );
	RegisterMorphTracks( control, animation );

	
	if (m_controlParamEnabled)
	{
		auto duration = GrannyGetControlLocalDuration( control );
		GrannySetControlRawLocalClock( control, m_controlParam * duration );
	}

	return true;
}

void Tr2GrannyAnimationLayer::RegisterTextTracks( granny_control* control, const granny_animation* anim )
{
	for( int groupIdx = 0; groupIdx < anim->TrackGroupCount; groupIdx++ )
	{
		for( int trackIdx = 0; trackIdx < anim->TrackGroups[groupIdx]->TextTrackCount; trackIdx++ )
		{
			m_controlTextTracks[control].push_back( TextEventTrack( &anim->TrackGroups[groupIdx]->TextTracks[trackIdx] ) );
		}
	}
}

void Tr2GrannyAnimationLayer::RegisterMorphTracks( granny_control* control, const granny_animation* anim )
{
	for( int groupIdx = 0; groupIdx < anim->TrackGroupCount; groupIdx++ )
	{
		granny_track_group* trackGroup = anim->TrackGroups[groupIdx];
		if( strcmp( trackGroup->Name, "root" ) == 0 )
		{
			for( int trackIdx = 0; trackIdx < trackGroup->VectorTrackCount; trackIdx++ )
			{
				m_controlMorphTracks[control].push_back( MorphTrack( &trackGroup->VectorTracks[trackIdx] ) );
			}
			break;
		}
	}
}

void Tr2GrannyAnimationLayer::EndAnimation()
{
	if( !m_modelInstance )
	{
		m_animationQueue.clear();
		return;
	}

	for(	granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); 
			binding != GrannyModelControlsEnd( m_modelInstance ); 
			binding = GrannyModelControlsNext( binding ) )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );
		// Force control to stop at the end of its current loop iteration
		int newLoopCount;
		newLoopCount = max(0, GrannyGetControlLoopIndex( control )) + 1;
		GrannySetControlLoopCount( control, newLoopCount );
	}
	FreeCompletedControls();
}

void Tr2GrannyAnimationLayer::ClearTextTracks( granny_control* control )
{
	m_controlTextTracks.erase( control );
}

void Tr2GrannyAnimationLayer::ClearMorphTracks( granny_control* control )
{
	m_controlMorphTracks.erase( control );
}

void Tr2GrannyAnimationLayer::ClearAnimations()
{	
	m_animationQueue.clear();

	if( !m_modelInstance )
	{
		return;
	}

	for( granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); binding != GrannyModelControlsEnd( m_modelInstance ); )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );
		binding = GrannyModelControlsNext( binding );
		ClearTextTracks( control );
		ClearMorphTracks( control );
		GrannyFreeControl( control );
	}
}

const char* TextEventTrack::SampleTrack( float time, int loop )
{
	int entryIndex = -1;
	for( int entryIdx = 0; entryIdx < m_grannyTrack->EntryCount; entryIdx++ )
	{
		if( m_grannyTrack->Entries[entryIdx].TimeStamp > time )
		{
			break;
		}
		entryIndex = entryIdx;
	}
	// Currently only the last event will be triggered
	if( entryIndex >= 0 && ( entryIndex > m_lastIndex || loop > m_lastLoop )  )
	{
		m_lastIndex = entryIndex;
		m_lastLoop = loop;
		return m_grannyTrack->Entries[entryIndex].Text;
	}
	return nullptr;
}

const float MorphTrack::SampleTrack( float time, float duration )
{
	// TODO: intern, what about loop?

	float defaultValue = 0.f;
	float value = 0.f;
	GrannyEvaluateCurveAtT( 1, false, false, &m_grannyTrack->ValueCurve, false, duration, (float)time, (float*)&value, &defaultValue );
	return value;
}

void Tr2GrannyAnimationLayer::SampleTextTracks( IBlueEventListener* listener )
{
	if( !listener )
	{
		return;
	}
	for( auto it = m_controlTextTracks.begin(); it != m_controlTextTracks.end(); it++ )
	{
		granny_control* control = it->first;
		int currentLoop = GrannyGetControlLoopIndex( control );
		granny_real32 t = GrannyGetControlRawLocalClock( control );
		if( t < 0 )
		{
			continue; // Animation hasn't started yet
		}

		for( auto trackIt = it->second.begin(); trackIt != it->second.end(); trackIt++ )
		{
			const char* evt = trackIt->SampleTrack( t, currentLoop );
			if( evt )
			{
				listener->HandleEvent( CA2W( evt ) );
			}
		}
	}
}

void Tr2GrannyAnimationLayer::SampleMorphTracks( float animationTime, std::unordered_map<std::string, float>& morphAnimations, bool additive )
{
	for( auto it = m_controlMorphTracks.begin(); it != m_controlMorphTracks.end(); it++ )
	{
		granny_control* control = it->first;
		granny_real32 t = GrannyGetControlRawLocalClock( control );

		auto duration = GrannyGetControlDuration( control );

		if( t < 0 || t >= duration )
		{
			continue;
		}

		for( auto trackIt = it->second.begin(); trackIt != it->second.end(); trackIt++ )
		{
			float trackValue = trackIt->SampleTrack( t, duration ) * m_layerWeight;
			auto entry = morphAnimations.find( trackIt->m_grannyTrack->Name );
			if ( entry != morphAnimations.end() )
			{
				entry->second = additive ? entry->second + trackValue : trackValue;
			}
			else
			{
				morphAnimations[trackIt->m_grannyTrack->Name] = trackValue;
			}
		}
	}
}

void Tr2GrannyAnimationLayer::SampleAnimation( float animationTime, granny_local_pose* resultPose, IBlueEventListener* listener, 
	std::unordered_map<std::string, float>& morphAnimations )
{
	GrannySetModelClock( m_modelInstance, animationTime );
	SampleTextTracks( listener );
	SampleMorphTracks( animationTime, morphAnimations );
	FreeCompletedControls();
	if ( m_controlParamEnabled )
	{
		UpdateControlParam( animationTime );
	}
	GrannySampleModelAnimations( m_modelInstance, 0, m_boneCount, resultPose );
}

void Tr2GrannyAnimationLayer::SampleAnimation( float animationTime, granny_local_pose* compositePose, granny_local_pose* resultPose, 
	IBlueEventListener* listener, std::unordered_map<std::string, float>& morphAnimations, bool additive )
{
	GrannySetModelClock( m_modelInstance, animationTime );
	SampleTextTracks( listener );
	SampleMorphTracks( animationTime, morphAnimations, additive );
	FreeCompletedControls();
	if ( m_controlParamEnabled )
	{
		UpdateControlParam( animationTime );
	}
	if ( additive )
	{
		if ( !m_basePose )
		{
			m_basePose = GrannyNewLocalPose( m_boneCount );
		}

		granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance );

		if ( binding == GrannyModelControlsEnd( m_modelInstance ) )
		{
			// no bindings
			return;
		}
		
		// Construct the m_basePose from raw local clock 0 for the first control.
		granny_control *control = GrannyGetControlFromBinding( binding );
		
		if ( !control )
		{
			return;
		}

		const float raw_local_time = GrannyGetControlRawLocalClock( control );
		GrannySetControlRawLocalClock( control, 0 );
		GrannySampleModelAnimations( m_modelInstance, 0, m_boneCount, m_basePose );
		GrannySetControlRawLocalClock( control, raw_local_time );

		GrannySampleModelAnimations( m_modelInstance, 0, m_boneCount, compositePose );
		GrannyMaskedAdditiveBlend( resultPose, compositePose, m_basePose, 0, m_boneCount, m_trackMask, m_layerWeight );
	}
	else
	{
		GrannySampleModelAnimations( m_modelInstance, 0, m_boneCount, compositePose );
		GrannyModulationCompositeLocalPose( resultPose, 0, m_layerWeight, m_trackMask, compositePose );
	}
}


void Tr2GrannyAnimationLayer::FreeCompletedControls()
{
	for( granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); binding != GrannyModelControlsEnd( m_modelInstance ); )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );
		binding = GrannyModelControlsNext( binding );
		if( GrannyFreeControlIfComplete( control ) )
		{
			ClearTextTracks( control );
			ClearMorphTracks( control );
		}
	}
}

float Tr2GrannyAnimationLayer::GetAnimationChainCompleteTime()
{
	float startTime = GetLayerAnimationTime();
	return startTime + GetAnimationRemainingTime();
}

float Tr2GrannyAnimationLayer::GetAnimationRemainingTime()
{
	if( !m_modelInstance )
	{
		return 0.f;
	}

	float maxRemaining = 0.0f;
	for(	granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); 
			binding != GrannyModelControlsEnd( m_modelInstance ); 
			binding = GrannyModelControlsNext( binding ) )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );

		// Force control to stop at the end of its current loop iteration
		int loopCount = max(0, GrannyGetControlLoopIndex( control ));
		int newLoopCount = loopCount + 1;
		int loopsTotal = GrannyGetControlLoopCount( control );
		
		GrannySetControlLoopCount( control, newLoopCount );
		float remaining = GrannyGetControlDurationLeft( control );
		GrannySetControlLoopCount( control, loopsTotal );
		if( remaining > maxRemaining )
		{
			maxRemaining = remaining;
		}
	}

	return maxRemaining;
}

void Tr2GrannyAnimationLayer::Cleanup()
{
	if( m_modelInstance )
	{
		ClearAnimations();
		GrannyFreeModelInstance( m_modelInstance );
		m_modelInstance = nullptr;
	}

	if( m_trackMask )
	{
		GrannyFreeTrackMask( m_trackMask );
		m_trackMask = nullptr;
	}

	if ( m_basePose )
	{
		GrannyFreeLocalPose( m_basePose );
		m_basePose = nullptr;
	}
}

void Tr2GrannyAnimationLayer::AddBone( const Tr2GrannyAnimation* grannyAnimation, const char* name )
{
	m_bones.push_back( name );

	if( !m_trackMask )
	{
		return;
	}
	
	unsigned int boneIndex = 0;
	if( !grannyAnimation->FindBoneByName( name, boneIndex ) )
	{
		return;
	}

	GrannySetTrackMaskBoneWeight( m_trackMask, boneIndex, 1.0 );
}


void Tr2GrannyAnimationLayer::AddAllBones( const Tr2GrannyAnimation* grannyAnimation )
{
	unsigned int bone_count;
	const std::string *boneList = grannyAnimation->GetAnimationBoneList( bone_count );
	for (unsigned int i=0; i < bone_count; i++)
	{
		AddBone( grannyAnimation, boneList[i].c_str() );
	}
}



void Tr2GrannyAnimationLayer::ExtractTrackMask( const Tr2GrannyAnimation* grannyAnimation, const char* name )
{
	m_trackMaskName = name;
	if( !m_trackMask )
	{
		return;
	}
	
	granny_extract_track_mask_result etmr = GrannyExtractTrackMask(
		m_trackMask,
		m_boneCount,
		GrannyGetSourceSkeleton( m_modelInstance ),
		name,
		0.0f,
		false );
	if ( etmr == GrannyExtractTrackMaskResult_NoDataPresent )
	{
		CCP_LOGNOTICE( "Tr2GrannyAnimationLayer: Track mask not found." );
	}

	return;
}

void Tr2GrannyAnimationLayer::RemoveBone( const Tr2GrannyAnimation* grannyAnimation, const char* name )
{
	m_bones.erase( std::remove(m_bones.begin(), m_bones.end(), name), m_bones.end() );

	if( !m_trackMask )
	{
		return;
	}
	
	unsigned int boneIndex = 0;
	if( !grannyAnimation->FindBoneByName( name, boneIndex ) )
	{
		return;
	}

	GrannySetTrackMaskBoneWeight( m_trackMask, boneIndex, 0.0 );
}

float Tr2GrannyAnimationLayer::GetLayerWeight() const
{
	return m_layerWeight;
}

void Tr2GrannyAnimationLayer::SetLayerWeight( float layerWeight )
{
	m_layerWeight = layerWeight;
}

void Tr2GrannyAnimationLayer::SetControlParam( float controlParam )
{
	m_controlParamTarget = controlParam;
	m_controlParamEnabled = true;
}

void Tr2GrannyAnimationLayer::SetControlParamSkewRate( float skewRate )
{
	m_skewRate = skewRate;
}

void Tr2GrannyAnimationLayer::UpdateControlParam( float animation_time )
{
	if ( m_controlParam == m_controlParamTarget )
	{
		return;
	}

	if (animation_time == m_lastControlUpdateTime)
	{
		return;
	}

	float timeIncrement = animation_time - m_lastControlUpdateTime;
	float controlIncrement = timeIncrement * m_skewRate;

	if ( m_skewRate == 0.f || std::abs( m_controlParamTarget - m_controlParam ) <= controlIncrement )
	{
		m_controlParam = m_controlParamTarget;
	}
	else
	{
		if ( m_controlParamTarget - m_controlParam < 0 )
		{
			m_controlParam -= controlIncrement;
		}
		else
		{
			m_controlParam += controlIncrement;
		}
	}
	for( granny_model_control_binding *binding = GrannyModelControlsBegin( m_modelInstance ); binding != GrannyModelControlsEnd( m_modelInstance ); binding = GrannyModelControlsNext( binding ) )
	{
		granny_control *control = GrannyGetControlFromBinding( binding );
		auto duration = GrannyGetControlLocalDuration( control );
		GrannySetControlRawLocalClock( control, m_controlParam * duration );
	}
	m_lastControlUpdateTime = animation_time;
}