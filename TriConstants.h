/* 
	*************************************************************************************

	TriConstants.h

	Author:    Hilmar Veigar Pétursson
	Created:   November 2000
	OS:        Win32
	Project:   Trinity

	Description:   

		Constants for Trinity, mostly from D3D, mapped over to Blue


	Dependencies:

		DirectX 9.0, Blue

	(c) CCP 2000

	*************************************************************************************
*/

#ifndef _TRICONSTANTS_H_
#define _TRICONSTANTS_H_

const char* KeyFromVal(const Be::VarChooser* i, long val);
#if BLUE_WITH_PYTHON
void AddTriConstants(PyObject *d);
#endif

// the rot group ids
extern const long TRIGRPID_TEXTURE;
extern const long TRIGRPID_VERTEXBUFFER;
extern const long TRIGRPID_INDEXBUFFER;
extern const long TRIGRPID_SURFACE;
extern const long TRIGRPID_GR2;
extern const long TRIGRPID_MORPHEME;

extern const Be::VarChooser TriTextureChooser[];
extern const Be::VarChooser TriModelChooser[];
extern const Be::VarChooser TriGR2Chooser[];
extern const Be::VarChooser TriMorphemeBundleChooser[];
extern const Be::VarChooser TriParticleBirth[];
extern const Be::VarChooser TriParticleDeath[];
extern const Be::VarChooser TriParticleAnimation[];
extern const Be::VarChooser TriParticleCycle[];
extern const Be::VarChooser TriParticleType[];
extern const Be::VarChooser TriBoidSwarmType[];
extern const Be::VarChooser TriExtrapolation[];
extern const Be::VarChooser TriInterpolation[];
extern const Be::VarChooser TriOperator[];
extern const Be::VarChooser TriTextureSource[];
extern const Be::VarChooser TriMaterialSource[];
extern const Be::VarChooser TriStageSelection[];
extern const Be::VarChooser TriTransformBase[];
extern const Be::VarChooser TriCloudType[];
extern const Be::VarChooser TriTransformBaseFlags[];
extern const Be::VarChooser TriLodBy[];
extern const Be::VarChooser TriCull[];
extern const Be::VarChooser TriFillMode[];
extern const Be::VarChooser TriCmpFunc[];
extern const Be::VarChooser TriBlendOp[];
extern const Be::VarChooser TriBlend[];
extern const Be::VarChooser TriTextureAddress[];
extern const Be::VarChooser TriImageFileFormat[];
extern const Be::VarChooser TriMultiSampleType[];
extern const Be::VarChooser TriClearFlags[];
extern const Be::VarChooser TriFormat[];
extern const Be::VarChooser TriFormatZStencil[];
extern const Be::VarChooser TriResourceTypeChooser[];
extern const Be::VarChooser TriHACKFORTESTING[];
extern const Be::VarChooser TriScissorMode[];
extern const Be::VarChooser TriPoseClipTime[];
extern const Be::VarChooser TriBipedMovementState[];
extern const Be::VarChooser TriSkeletonType[];
extern const Be::VarChooser TriD3DRenderState[];
extern const Be::VarChooser TriD3DFormat[];
extern const Be::VarChooser TriFilterMode[];

#endif

