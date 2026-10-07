// Copyright © 2026 CCP ehf.

#include "../../../../../include/Carbon/System.fxh"
#include "../../../../../include/Carbon/Math.fxh"
#include "../../../../../include/Carbon/RandomXOrShift.fxh"
#include "../../../../../include/Carbon/SpaceScene/GpuParticles.fxh"


float3 GetRandomDirection( float3 direction, float angle, float innerAngle, float3 rands )
{
    if( angle >= PI / 2 )
    {
        angle = PI;
    }
    float3 dir = normalize( direction );

    float phi = lerp( innerAngle, angle, rands.x );
    float theta = rands.y * 4 * PI;

    float4 q = QuaternionRotationYawPitchRoll( 0, phi, theta );
    float4 qp = float4( -q.xyz, q.w );
    float4 v = float4( 0, 0, 1, 0 );
    v = QuaternionMultiply( QuaternionMultiply( q, v ), qp );
	
    float3 x = abs( dir.x ) > abs( dir.y ) ? float3( 0, 1, 0 ) : float3( 1, 0, 0 );
    float3 y = normalize( cross( dir, x ) );
    x = cross( dir, y );

    return dir * v.z + x * v.x + y * v.y;
}


#define GROUP_X 16
#define GROUP_Y 16

struct EmitterData
{
    float3 position;
    uint count;

    float3 positionPrevious;
    float radius;

    float3 direction;
    float angle;

    float3 directionPrevious;
    uint emitterSeed; // LO: emitter, HI: seed

    float3 velocity;
    float minSpeed;

    float3 velocityPrevious;
    float maxSpeed;

    float innerAngle;
    float3 unused;
};

struct PerObjectVSData
{
    uint count;
    uint3 padding;
	// should be EmitterData data[(PLATFORM_MAX_CONSTANT_BUFFER_SIZE - 16)/sizeof(EmitterData)], but ShaderCompiler can't handle that
#if PLATFORM_MAX_CONSTANT_BUFFER_SIZE == 4096
    EmitterData data[36];
#elif PLATFORM_MAX_CONSTANT_BUFFER_SIZE == 65536
	EmitterData data[585];
#else
#error "Need to define proper length for the emitter CB"
#endif

} PerObjectVS;

RWStructuredBuffer<GpuParticleData> ParticleBuffer;
RWBuffer<int> ParticleCounters;
RWStructuredBuffer<uint> DeadBuffer;

groupshared EmitterData g_emitter;

[numthreads( GROUP_X, GROUP_Y, 1 )]
void Emit( uint3 grid : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 threadID : SV_GroupThreadID )
{
    if( groupIndex == 0 )
    {
        uint emitIndex = grid.x;
        g_emitter = PerObjectVS.data[emitIndex];
    }

    GroupMemoryBarrierWithGroupSync();

    uint rng_state = RandInit( groupIndex + ( g_emitter.emitterSeed >> 16 ) );

    uint start = threadID.x + threadID.y * GROUP_X;
    for( uint i = start; i < g_emitter.count; i += GROUP_X * GROUP_Y )
    {
        int deadIndex;
        InterlockedAdd( ParticleCounters[0], -1, deadIndex );
        --deadIndex;

        if( deadIndex >= 0 )
        {
            float birthTime = RandFloat( rng_state );
            float3 velocityDirection = GetRandomDirection(
				normalize( lerp( g_emitter.directionPrevious, g_emitter.direction, birthTime ) ),
				g_emitter.angle,
				g_emitter.innerAngle,
				float3( RandFloat( rng_state ), RandFloat( rng_state ), RandFloat( rng_state ) ) );

            uint index = DeadBuffer[deadIndex];
            GpuParticleData particle;
            particle.position = lerp( g_emitter.positionPrevious, g_emitter.position, birthTime );
            particle.position += velocityDirection * g_emitter.radius;
            particle.velocity = velocityDirection *
				lerp( g_emitter.minSpeed, g_emitter.maxSpeed, RandFloat( rng_state ) ) + lerp( g_emitter.velocityPrevious, g_emitter.velocity, birthTime );
            particle.age = 0;
            particle.emitterSeed = ( g_emitter.emitterSeed & 0xffff ) | ( Rand( rng_state ) << 16 );


            ParticleBuffer[index] = particle;
        }
        else
        {
            break;
        }
    }
}



technique Main
{
    pass p0
    {
        ComputeShader = compile cs_5_0 Emit();
    }
}
