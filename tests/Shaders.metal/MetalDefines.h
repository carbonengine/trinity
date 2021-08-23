#pragma once

// These values must be synchronized with defines in TrinityAL/metal/MetalWorkQueue.h

// buffers
#define CBUFFER(i) buffer(4 + i)
#define SRV(i) buffer(4 + i)
#define UAV(i) buffer(24 + i)

// textures
#define UAVT(i) texture(24 + i)
