#pragma once
#ifndef EveConstantBufferFormats_H
#define EveConstantBufferFormats_H

// keep some rather global per-object data structs here
//
// All PerFrame and PerObject structs must fill constant registers, which are 4 * float

struct EvePerObjectPSData
{
	Matrix WorldMat;
};

struct EvePerObjectVSData
{
	Matrix WorldMat;
};

#endif // EveConstantBufferFormats_H