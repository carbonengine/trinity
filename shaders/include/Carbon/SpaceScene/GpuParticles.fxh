// Copyright © 2026 CCP ehf.

#ifndef CARBON_GPUPARTICLES_FXH
#define CARBON_GPUPARTICLES_FXH

// Per particle data stored in the particle buffer. The particle buffer is a structured buffer of GpuParticleData elements.
struct GpuParticleData
{
    // Position of the particle in world space
    float3 position;
    // Age of the particle in seconds
    float age;
    // Particle velocity in world space
    float3 velocity;
    // 16 low bits - emitter index, 16 high bits - random seed for this particle
    uint emitterSeed;
};

// Per emitter data stored in the emitter buffer. The emitter buffer is a structured buffer of GpuParticleEmitterParams elements.
struct GpuParticleEmitterParams
{
    // Minimal particle lifetime in seconds
    float minLifeTime;
    // Maximal particle lifetime in seconds
    float maxLifeTime;
    // Index of the texture in the texture atlas
    float textureIndex;
    // Particle color over lifetime, defined by a piecewise linear curve of 4 colors
    float4 colors[4];
    // Particle sizes over lifetime, defined by a spline of 3 sizes
    float3 sizes;
    // Variance of the particle size
    float sizeVariance;
    // Drag force coefficient applied to the particles
    float drag;
    // Amplitude of the turbulence force applied to the particles
    float turbulenceAmplitude;
    // Frequency of the turbulence force applied to the particles
    float turbulenceFrequency;
    // Gravity force applied to the particles
    float gravity;
    // Attractor force applied to the particle: XYZ - attractor position, W - attractor strength
    float4 attractor;
    // Velocity stretch factor or rotation speed
    // If velocityStretchRotation is negative, it is used as a velocity stretch factor. If it is positive, it is used as a rotation speed in radians per second.
    float velocityStretchRotation;
};

float GetGpuParticleSize( GpuParticleEmitterParams params, float age, float seed )
{
    float v = 1.0 - age;
    float spline = params.sizes.x * v * v + params.sizes.y * 2.0 * age * v + params.sizes.z * age * age;
    return spline * ( 1 + params.sizeVariance * seed );
}

float4 GetGpuParticleColor( GpuParticleEmitterParams params, float age, float exponent )
{
    age = pow( abs( age ), log( exponent ) / log( 0.5 ) );
    float4 color;
    if( age <= 0.333 )
    {
        color = lerp( params.colors[0], params.colors[1], age / 0.333 );
    }
    else if( age <= 0.666 )
    {
        color = lerp( params.colors[1], params.colors[2], age / 0.333 - 1 );
    }
    else
    {
        color = lerp( params.colors[2], params.colors[3], age / ( 1 - 0.666 ) - 0.666 / ( 1 - 0.666 ) );
    }
    return color;
}


// Per particle data stored in the visible buffer. The visible buffer is a structured buffer of GpuParticleVisibleData elements.
struct GpuParticleVisibleData
{
    // Particle index in the particle buffer
    uint index;
    // Particle depth in projection space
    float depth;
};


#endif