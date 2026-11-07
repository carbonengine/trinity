// Copyright © 2026 CCP ehf.

#ifndef CARBON_PARTICLEANNOTATIONS_FXH
#define CARBON_PARTICLEANNOTATIONS_FXH

// This file contains definitions for particle system shader annotations.
// These annotations are used to provide metadata and hints to the editor.
// These annotations should be specified for the particle system vertex shader, and
// they describe the particle attributes used by the shader.
// For example:
// ParticleVtx ParticleVS( ParticleAppVtx inVtx )
// <
// 	DESC_PARTICLE_POSITION;
// 	DESC_PARTICLE_LIFETIME;
// 	DESC_PARTICLE_CUSTOM( 0, "size", 2 );
// >
// {
// ...
// }

#define DESC_PARTICLE_CONCAT( x, y ) x##y

#define DESC_PARTICLE_ELEMENT( usage, index, name, dimension ) \
	string DESC_PARTICLE_CONCAT( ParticleElement_Name_, __LINE__ ) = name; \
	int DESC_PARTICLE_CONCAT( ParticleElement_Dimension_, __LINE__ ) = dimension; \
	int DESC_PARTICLE_CONCAT( ParticleElement_Usage_, __LINE__ ) = usage; \
	int DESC_PARTICLE_CONCAT( ParticleElement_Index_, __LINE__ ) = index

#define DESC_PARTICLE_POSITION DESC_PARTICLE_ELEMENT( 1, 0, "POSITION", 3 )
#define DESC_PARTICLE_VELOCITY DESC_PARTICLE_ELEMENT( 2, 0, "VELOCITY", 3 )
#define DESC_PARTICLE_LIFETIME DESC_PARTICLE_ELEMENT( 0, 0, "LIFETIME", 2 )
#define DESC_PARTICLE_MASS DESC_PARTICLE_ELEMENT( 3, 0, "MASS", 1 )

#define DESC_PARTICLE_CUSTOM( index, name, dimension ) DESC_PARTICLE_ELEMENT( 4, index, name, dimension )


#endif