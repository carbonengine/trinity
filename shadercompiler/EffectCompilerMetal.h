// Copyright © 2023 CCP ehf.

#pragma once

#include "EffectCompilerBase.h"

struct MetalRegister
{
	enum Enum : char
	{
		Invalid = '\0',
		StageIn = 'i', // [[stage_in]]
		Attribute = 'a', // [[attribute(#)]]
		CBuffer = 'c', // [[CBUFFER(#)]]
		SRV = 'b', // [[SRV(#)]]
		Texture = 't', // [[texture(#)]]
		Sampler = 's', // [[sampler(#)]]
		UAV = 'u', // [[UAV(#)]], [[UAVT(#)]]
		ThreadGroup = 'g', // [[threadgroup(#)]]
		User = 'x', // [[user(...)]]
		System = 'v', // [[position]], etc. see MetalSystemSemanticsType
	};
};

struct MetalSystemSemanticsType
{
	// Note: If you add value to this enum or rearrange values then don't forget
	// to update string value(s) in GetString function.
	enum Enum : int
	{
		position = 0,
		position_invariant,
		front_facing,
		vertex_id,
		instance_id,
		primitive_id,
		clip_distance,
		point_size,
		color_0,
		color_1,
		color_2,
		color_3,
		color_4,
		color_5,
		color_6,
		color_7,
		depth,
		stencil,
		thread_position_in_grid,
		thread_position_in_threadgroup,
		thread_index_in_threadgroup,
		threadgroup_position_in_grid,
		sample_id,
		threads_per_grid,

		payload,
		barycentric_coord,
		origin,
		direction,
		min_distance,
		distance,
		instance_intersection_function_table_offset,
		object_to_world_transform,
		world_to_object_transform,

		// these two are only available since shader model 6.8 in hlsl, which we don't use (yet).
		// but we need them to emulate the behaviour of SV_VertexID and SV_InstanceID
		base_vertex,
		base_instance,
	};

	static const char* GetString( int type );
};

class EffectCompilerMetal : public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result, class IWorkQueue* workQueue ) override;

private:
	struct SyncData
	{
		bool compiled = false;
		std::condition_variable conditionVariable;
		std::mutex mutex;
		std::vector<uint8_t> compiledCode;
	};

	std::unordered_map<std::string, std::shared_ptr<SyncData>> m_compiled;
	std::mutex m_compiledCS;
};
