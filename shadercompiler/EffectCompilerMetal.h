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
	};

	static const char* GetString( int type );
};

class EffectCompilerMetal: public EffectCompilerBase
{
public:
	bool Create() override;
	bool CompileEffect( const char* source, size_t sourceLength, const std::vector<Macro>& defines, EffectData& result ) override;

private:
	std::unordered_map<std::string, std::vector<uint8_t>> m_compiled;
	std::mutex m_compiledCS;
};
