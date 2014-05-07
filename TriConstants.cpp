#include "StdAfx.h"


#include "TriConstants.h"

#include "include/ITriConstants.h"

using namespace Tr2RenderContextEnum;

#if BLUE_WITH_PYTHON
void AddTriConstants(PyObject* d)
{
	const Be::VarChooser* const triConstants [] = 
	{	
#if DEPRECATED_ENABLED
		TriTextureAddress,
		TriImageFileFormat,
		TriMultiSampleType,
		TriFormat,
		TriFormatZStencil,
		TriResourceTypeChooser,
		TriD3DFormat,
        TriFilterMode,
#endif
		
		TriBlendOp,
		TriBlend,
		TriFillMode,
        TriTextureChooser,
		TriModelChooser,
		TriParticleBirth,
		TriParticleDeath,
		TriParticleAnimation,
		TriParticleCycle,
		TriParticleType,
		TriBoidSwarmType,
		TriExtrapolation,
		TriInterpolation,
		TriOperator,
		TriTextureSource,
		TriMaterialSource,
		TriStageSelection,
		TriTransformBase,
		TriCloudType,
		TriTransformBaseFlags,
		TriLodBy,
		TriCull,
		TriCmpFunc,
		TriClearFlags,
		TriScissorMode,
		TriGR2Chooser,
		TriPoseClipTime,
		TriBipedMovementState,
		TriHACKFORTESTING,
		TriSkeletonType,
		TriD3DRenderState,
	};

	for (int i = 0; i < sizeof triConstants / sizeof triConstants[0]; i++)
	{
		for (const Be::VarChooser* j = triConstants[i]; j->mKey; j++)
		{
			// this obviously assumes that all values are LONG
			// later perhaps call BlueToPython to convert types
			// correctly
			PyObject *value = PyInt_FromLong( j->mValue.mLong );
			PyDict_SetItemString(d, const_cast<char*>( j->mKey ), value );
			Py_DECREF( value );
		}
	}
}
#endif


const char* KeyFromVal(const Be::VarChooser* i, long val)
{
	while (i->mKey)
	{
		if(i->mValue.mLong == val)
			return i->mKey;
		i++;
	}

	return "[-not found-]";
}



////////////////////////////////////////////////////////////////////////////////
// The actual definitions of these constants.
// the rot group ids
const long TRIGRPID_TEXTURE = 8001;
const long TRIGRPID_VERTEXBUFFER = 8002;
const long TRIGRPID_INDEXBUFFER = 8003;
const long TRIGRPID_SURFACE = 8004;
const long TRIGRPID_GR2 = 8005; // granny file
const long TRIGRPID_MORPHEME = 8006; // morpheme bundle

#define VAL(v) BeCast(v)
#define KV( name, doc ) \
{ \
#name, \
	VAL( name ), \
	doc \
}


#define KV_RC( name, doc ) \
{ \
	"D3D" #name, \
	VAL( Tr2RenderContextEnum::name ), \
	doc \
}

#define VAL_RC(v) BeCast(Tr2RenderContextEnum:: v)


const Be::VarChooser TriTextureChooser[] =
{
	{ 
		"SELECT_TEXTURE",     
		VAL(0),     
		"Texture (.dds, .tga, .bmp)|*.dds;*.tga;*.bmp;|All Files (*.*)|*.*" 
	},
	{0}
};

const Be::VarChooser TriModelChooser[] =
{
	{ 
		"SELECT_TRIMODEL",     
		VAL(0),     
		"Blue Object(.blue)|*.blue;|Maya Exported Model (.tri)|*.tri;|All Files (*.*)|*.*" 
	},
	{0}
};

const Be::VarChooser TriGR2Chooser[] =
{
	{ 
		"SELECT_GR2MODEL",     
		VAL(0),     
		"Granny file(.gr2)|*.GR2" 
	},
	{0}
};

const Be::VarChooser TriMorphemeBundleChooser[] =
{
	{ 
		"SELECT_BUNDLEMODEL",     
		VAL(0),     
		"Morpheme Bundle file|*.*" 
	},
	{0}
};

const Be::VarChooser TriParticleBirth[] =
{
	{ 
		"TRTPB_POINT",     
		VAL(TRTPB_POINT),     
		"no comment" 
	},
	{ 
		"TRTPB_FIELD", 
		VAL(TRTPB_FIELD), 
		"no comment" 
	},	
	{0}
};


const Be::VarChooser TriParticleDeath[] =
{
	{ 
		"TRTPD_TIME",     
		VAL(TRTPD_TIME),     
		"no comment" 
	},
	{ 
		"TRTPD_RANGE", 
		VAL(TRTPD_RANGE), 
		"no comment" 
	},
	{ 
		"TRTPD_CAMERADIST", 
		VAL(TRTPD_CAMERADIST), 
		"no comment" 
	},	
	{ 
		"TRTPD_NEEDED", 
		VAL(TRTPD_NEEDED), 
		"no comment" 
	},	
	{0}
};

const Be::VarChooser TriParticleAnimation[] =
{
	{ 
		"TRTPA_NONE",     
		VAL(TRTPA_NONE),     
		"no comment" 
	},
	{ 
		"TRTPA_4", 
		VAL(TRTPA_4), 
		"no comment" 
	},
	{ 
		"TRTPA_16", 
		VAL(TRTPA_16), 
		"no comment" 
	},
	{ 
		"TRTPA_64", 
		VAL(TRTPA_64), 
		"no comment" 
	},	
	{0}
};

const Be::VarChooser TriParticleCycle[] =
{
	{ 
		"TRTPC_NONE",     
		VAL(TRTPC_NONE),     
		"no uv cycling" 
	},
	{ 
		"TRTPC_RANDOM_HOLD", 
		VAL(TRTPC_RANDOM_HOLD), 
		"new particle starts somewhere in uv cycle and will always show that frame.\r\n" 
		"This has the same effect as TRTPC_RANDOM_LOOP with uvFPS == 0.0" 
	},
	{ 
		"TRTPC_RANDOM_LOOP", 
		VAL(TRTPC_RANDOM_LOOP), 
		"new particle starts somewhere in uv cycle, and loops, changes frames as per uvFPS" 
	},
	{ 
		"TRTPC_CONSTANT", 
		VAL(TRTPC_CONSTANT), 
		"new particle starts at frame 1, and plays to the end frame. Depends on uvFPS" 
	},
	{ 
		"TRTPC_LIFETIME", 
		VAL(TRTPC_LIFETIME), 
		"new particle starts at frame 1, and plays to the end frame over it's whole lifetime" 
	},	
	{0}
};

const Be::VarChooser TriParticleType[] =
{
	{ 
		"TRTPT_SIMPLE",     
		VAL(TRTPT_SIMPLE),     
		"no comment" 
	},
	{ 
		"TRTPT_MOVING", 
		VAL(TRTPT_MOVING), 
		"no comment" 
	},	
	{ 
		"TRTPT_RIBBON", 
		VAL(TRTPT_RIBBON), 
		"no comment" 
	},	
	{0}
};


const Be::VarChooser TriBoidSwarmType[] =
{
	{ 
		"TRTBST_WHIP",     
		VAL(TRTBST_WHIP),     
		"Makes the boids behave like a torch" 
	},
	{ 
		"TRTBST_TRAIL", 
		VAL(TRTBST_TRAIL), 
		"Makes the boids behave like smoke" 
	},	
	{0}
};


const Be::VarChooser TriExtrapolation[] =
{
	{ 
		"TRIEXT_NONE",     
		VAL(TRIEXT_NONE),     
		"no comment" 
	},
	{ 
		"TRIEXT_CONSTANT", 
		VAL(TRIEXT_CONSTANT), 
		"no comment" 
	},
	{ 
		"TRIEXT_GRADIENT", 
		VAL(TRIEXT_GRADIENT), 
		"no comment" 
	},
	{ 
		"TRIEXT_CYCLE",    
		VAL(TRIEXT_CYCLE),    
		"no comment" 
	},
	{0}
};


const Be::VarChooser TriInterpolation[] =
{
	{ 
		"TRIINT_NONE",     
		VAL(TRIINT_NONE),     
		"No Interpolation" 
	},
	{ 
		"TRIINT_CONSTANT", 
		VAL(TRIINT_CONSTANT), 
		"Performs a constant interpolation" 
	},
	{ 
		"TRIINT_LINEAR",   
		VAL(TRIINT_LINEAR),   
		"Performs a linear interpolation" 
	},
	{ 
		"TRIINT_HERMITE",  
		VAL(TRIINT_HERMITE),  
		"Performs a Hermite spline interpolation" 
	},
	{ 
		"TRIINT_CATMULLROM",  
		VAL(TRIINT_CATMULLROM),  
		"Performs a Catmull-Rom interpolation" 
	},
	{ 
		"TRIINT_SLERP",  
		VAL(TRIINT_SLERP),  
		"Interpolates between two quaternions, using spherical linear interpolation." 
	},
	{ 
		"TRIINT_SQUAD",  
		VAL(TRIINT_SQUAD),  
		"Interpolates between quaternions, using spherical quadrangle interpolation" 
	},			
	{ 
		"TRIINT_SIGMOID",  
		VAL(TRIINT_SIGMOID),  
		"Only used for scalar curves. Uses the first key's value and the right value" 
	},			
	{0}
};


const Be::VarChooser TriTextureSource[] =
{
	{
		"TRITEXSRC_NONE", 
		VAL(TRITEXSRC_NONE),   
		"The there is no texture to be used."
	},
	{
		"TRITEXSRC_SHADER", 
		VAL(TRITEXSRC_SHADER),   
		"The textures of the shader are used as texture inputs."
	},
	{
		"TRITEXSRC_AREA", 
		VAL(TRITEXSRC_AREA), 
		"The base textures of the model are used as texture inputs."
	},
	{
		"TRITEXSRC_SCENE", 
		VAL(TRITEXSRC_SCENE), 
		"The global textures of the scene are used as texture inputs."
	},

	{0}
};


const Be::VarChooser TriMaterialSource[] =
{
	{
		"TRIMATSRC_NONE", 
		VAL(TRITEXSRC_NONE),   
		"The there is no material to be used."
	},

	{
		"TRIMATSRC_SHADER", 
		VAL(TRIMATSRC_SHADER),   
		"The materials of the shader are used as material inputs."
	},
	{
		"TRIMATSRC_AREA", 
		VAL(TRIMATSRC_AREA), 
		"The base materials of the model are used as material inputs."
	},
	{
		"TRIMATSRC_SCENE", 
		VAL(TRIMATSRC_SCENE), 
		"The global materials of the scene are used as material inputs."
	},
	{
		"TRIMATSRC_VERTEX", 
		VAL(TRIMATSRC_VERTEX), 
		"Uses the color defined in the vertices of the geometry. THE GEOMETRY MUST HAVE COLOR FOR THIS TO WORK!!!!."
	},

	{0}
};


const Be::VarChooser TriStageSelection[] =
{
	{
		"TRISTS_USE2STAGEPASSES", 
		VAL(TRISTS_USE2STAGEPASSES),   
		"On Hardware that has 2 or more stages, use passes that have not more than "
		"2 stages (textureStage0 and textureStage1) enabled. This is the 'passes' "
		"list of passes"
	},

	{
		"TRISTS_USE3STAGEPASSES", 
		VAL(TRISTS_USE3STAGEPASSES),   
		"On Hardware that has 3 or more stages, use passes that have not more than "
		"3 stages (textureStage0, textureStage1 and textureStage2) enabled. This is "
		"the 'passes3Stage' list of passes"
	},
	{
		"TRISTS_USE4STAGEPASSES", 
		VAL(TRISTS_USE4STAGEPASSES), 
		"On Hardware that has 4 or more stages, use passes that have not more than "
		"4 stages (textureStage0, textureStage1, textureStage2, textureStage3) enabled. "
		"This is the 'passes4Stage' list of passes"
	},
	{0}
};


const Be::VarChooser TriTransformBase[] =
{
	{
		"TRITB_OBJECT", 
		VAL(TRITB_OBJECT),   
		"This makes TriTransforms build hierachies as expected, that is a child inherits "
		"its parent transforms.\r\n."
		"For TriTextures this is mosly usefull for projecting textures from objects"
	},
	{
		"TRITB_CAMERA_ROTATION", 
		VAL(TRITB_CAMERA_ROTATION),   
		"This sets the inverted rotation part of the camera matrix as the base for the"
		"transformations be they texture or transforms, good for environment maps and"
		"normal projected cubemaps."
	},
	{
		"TRITB_CAMERA_TRANSLATION", 
		VAL(TRITB_CAMERA_TRANSLATION),   
		"This sets the inverted translation part of the camera matrix as the base for the"
		"transformations be they texture or transforms, good for geometry that is locked,"
		"to the camera, like nebulas and starfields, etc."
	},
	{
		"TRITB_CAMERA", 
		VAL(TRITB_CAMERA),   
		"This sets the inverted camera matrix as the base for the transformations"
		"be they texture or geometry transforms, basically the same, as "
		"TRITB_CAMERA_ROTATION except the translation is taken also"
		""
	},
	{
		"TRITB_CAMERA_ROTATION_ALIGNED", 
		VAL(TRITB_CAMERA_ROTATION_ALIGNED),   
		"Makes the transform face the camera at all times, used for billboarding."
		"The up direction of the billboard is preserved."
	},
	{
		"TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY", 
		VAL(TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY),   
		"Makes the transform face the camera at all times, used for billboarding,"
		"the rotation around the axis to the camera is not corrected, hence this"
		"should only be used for rotationally symmetric billboards."
	},
	{
		"TRITB_CAMERA_ROTATION_FALLOFF", 
		VAL(TRITB_CAMERA_ROTATION_FALLOFF),   
		"Same as TRITB_CAMERA_ROTATION_ALIGNED, except the billboard is scaled"
		"by distance. This is useful for lights that are not rotationally symmetric."
	},
	{
		"TRITB_CAMERA_ROTATION_FALLOFF_SYMMETRY", 
		VAL(TRITB_CAMERA_ROTATION_FALLOFF_SYMMETRY),   
		"Same as TRITB_CAMERA_ROTATION_ALIGNED_SYMMETRY, except the billboard is scaled"
		"by distance. This is useful for lights that are rotationally symmetric."
	},
	{
		"TRITB_BOOSTER", 
		VAL(TRITB_BOOSTER),   
		"Makes the sprite behave as if it were an ellipsoid"
	},
	{
		"TRITB_SIMPLE_HALO", 
		VAL(TRITB_SIMPLE_HALO),   
		"Ask Torfi"
	},
	{
		"TRITB_SIMPLE_HALO_FALLOFF", 
		VAL(TRITB_SIMPLE_HALO_FALLOFF),   
		"Ask Torfi"
	},
	{
		"TRITB_SIMPLE_HALO_SYMMETRY", 
		VAL(TRITB_SIMPLE_HALO_SYMMETRY),   
		"Ask Torfi"
	},
	{
		"TRITB_SIMPLE_SPRITE", 
		VAL(TRITB_SIMPLE_SPRITE),   
		"Ask Torfi"
	},
	{
		"TRITB_SIMPLE_SPRITE_FALLOFF", 
		VAL(TRITB_SIMPLE_SPRITE_FALLOFF),   
		"Ask Torfi"
	},
	{
		"TRITB_SIMPLE_SPRITE_CONSTANT", 
		VAL(TRITB_SIMPLE_SPRITE_CONSTANT),   
		"Don't ask Torfi"
	},
	{
		"TRITB_FIXED", 
		VAL(TRITB_FIXED),   
		"This placed traditionally called world-space, the transform is considered to "
		"be in the world cordinate system (object hierarchies are ignored)"
		"This leves the texture space untouched (set to the identity matrix)."
	},
	{
		"TRITB_BOOSTER_FALLOFF",
		VAL(TRITB_BOOSTER_FALLOFF),
		"Makes the sprite behave as if it were an ellipsoid, includes falloff"
	},
	{
		"TRITB_WORLD", 
		VAL(TRITB_WORLD),   
		"Transforms the object relative to the world zero "		
	},
	{0}
};

const Be::VarChooser TriTransformBaseFlags[] =
{
	{
		"TRITBF_SCALING", 
		VAL(TRITBF_SCALING),   
		"Use Scaling"
	},
	{
		"TRITBF_TRANSLATION", 
		VAL(TRITBF_TRANSLATION),   
		"Use translation"
	},
	{
		"TRITBF_ROTATION_FWD", 
		VAL(TRITBF_ROTATION_FWD),   
		"Use rotation forward"
	},	
	{
		"TRITBF_ROTATION_UP", 
		VAL(TRITBF_ROTATION_UP),   
		"Use rotation up"
	},	
	{0}
};

const Be::VarChooser TriPoseClipTime[] =
{
	{
		"TRIPC_WORLD", 
		VAL(TRIPC_WORLD),   
		"World time"
	},
	{
		"TRIPC_LOCAL", 
		VAL(TRIPC_LOCAL),   
		"Local ( birth ) time"
	},
	{
		"TRIPC_OFFSETX", 
		VAL(TRIPC_OFFSETX),   
		"Use local X translation"
	},	
	{
		"TRIPC_OFFSETY", 
		VAL(TRIPC_OFFSETY),   
		"Use local Y translation"
	},	
	{
		"TRIPC_OFFSETZ", 
		VAL(TRIPC_OFFSETZ),   
		"Use local Z translation"
	},	
	{
		"TRIPC_YAW", 
		VAL(TRIPC_YAW),   
		"Use model yaw"
	},	
	{0}
};

const Be::VarChooser TriOperator[] =
{
	{
		"TRIOP_MULTIPLY", 
		VAL(TRIOP_MULTIPLY),   
		"multiply"
	},
	{
		"TRIOP_ADD", 
		VAL(TRIOP_ADD),   
		"add"
	},
	{0}
};

const Be::VarChooser TriCloudType[] =
{
	{
		"TRICT_SPRITE", 
		VAL(TRICT_SPRITE),   
		"Sprite"
	},
	{
		"TRICT_POINT", 
		VAL(TRICT_POINT),   
		"points"
	},
	{
		"TRICT_LINE", 
		VAL(TRICT_LINE),   
		"lines connecting points"
	},
	{0}
};

const Be::VarChooser TriLodBy[] =
{
	{
		"TRILB_NONE", 
		VAL(TRILB_NONE),   
		"No automated switching of LOD levels, use this if you want to control "
		"externaly what LOD level is displayed"
	},
	{
		"TRILB_CAMERA_DISTANCE", 
		VAL(TRILB_CAMERA_DISTANCE),   
		"Use the theshold values as distances from the camera"
	},
	{
		"TRITB_CAMERA_DISTANCE_FOV_HEIGHT", 
		VAL(TRITB_CAMERA_DISTANCE_FOV_HEIGHT),   
		"Use the theshold values as height of the view-volume at the current distance from the camera"
	},
	{0}
};

const Be::VarChooser TriCull[] =
{
	{
		"TRICULL_NONE", 
		VAL(CULLMODE_NONE),
		"Do not cull back faces."
	},
	{
		"TRICULL_CW",   
		VAL(CULLMODE_CW),
		"Cull back faces with clockwise vertices."
	},
	{
		"TRICULL_CCW",  
		VAL(CULLMODE_CCW),
		"Cull back faces with counterclockwise vertices."
	},
	{0}
};

const Be::VarChooser TriFillMode[] =
{
	{
		"TRIFILL_POINT",     
		VAL(Tr2RenderContextEnum::FM_POINT),     
		"Fill points."
	},
	{
		"TRIFILL_WIREFRAME", 
		VAL(Tr2RenderContextEnum::FM_WIREFRAME), 
		"Fill wireframes. This fill mode currently does not work for clipped "
		"primitives when you use the DrawPrimitive methods."
	},
	{
		"TRIFILL_SOLID",     
		VAL(Tr2RenderContextEnum::FM_SOLID),     
		"Fill solids."
	},
	{0}
};

const Be::VarChooser TriCmpFunc[] =
{
	{
		"TRICMP_NEVER",         
		VAL_RC(CMP_NEVER),         
		"Always fail the test\n" 		
	},
	{
		"TRICMP_LESS",         
		VAL_RC(CMP_LESS),         
		"Accept the new pixel if its value is less than the value of the current pixel.\n"
	},
	{
		"TRICMP_EQUAL",    
		VAL_RC(CMP_EQUAL),    
		"Accept the new pixel if its value equals the value of the current pixel.\n"
	},
	{
		"TRICMP_LESSEQUAL", 
		VAL_RC(CMP_LESSEQUAL ), 
		"Accept the new pixel if its value is less than or equal to the value of the current pixel.\n"
	},
	{
		"TRICMP_GREATER",         
		VAL_RC(CMP_GREATER),         
		"Accept the new pixel if its value is greater than the value of the current pixel.\n"
	},
	{
		"TRICMP_NOTEQUAL",         
		VAL_RC(CMP_NOTEQUAL),         
		"Accept the new pixel if its value does not equal the value of the current pixel.\n"
	},
	{
		"TRICMP_GREATEREQUAL",         
		VAL_RC(CMP_GREATEREQUAL),         
		"Accept the new pixel if its value is greater than or equal to the value of the current pixel.\n"
	},
	{
		"TRICMP_ALWAYS",         
		VAL_RC(CMP_ALWAYS ),         
		"Always pass the test.\n"
	},
	{0}
};

const Be::VarChooser TriBlendOp[] =
{
	{
		"TRIBLENDOP_DISABLE",         
		VAL(Tr2RenderContextEnum::BO_DISABLE),
		"No blending\n" 		
	},
	{
		"TRIBLENDOP_ADD",         
		VAL(Tr2RenderContextEnum::BO_ADD),         
		"The result is the destination added to the source.\n" 
		"Result = Source + Destination"
	},
	{
		"TRIBLENDOP_SUBTRACT",    
		VAL(Tr2RenderContextEnum::BO_SUBTRACT),    
		"The result is the destination subtracted from to the source.\n"
		"Result = Source - Destination"
	},
	{
		"TRIBLENDOP_REVSUBTRACT", 
		VAL(Tr2RenderContextEnum::BO_REVSUBTRACT), 
		"The result is the source subtracted from the destination.\n"
		"Result = Destination - Source"
	},
	{
		"TRIBLENDOP_MIN",         
		VAL(Tr2RenderContextEnum::BO_MIN),         
		"The result is the minimum of the source and destination.\n"
		"Result = MIN(Source, Destination)"
	},
	{
		"TRIBLENDOP_MAX",         
		VAL(Tr2RenderContextEnum::BO_MAX),         
		"The result is the maximum of the source and destination.\n"
		"Result = MAX(Source, Destination)"
	},
	{0}
};

const Be::VarChooser TriBlend[] =
{
	{
		"TRIBLEND_ZERO",            
		VAL(Tr2RenderContextEnum::BM_ZERO),
		"Blend factor is (0, 0, 0, 0)."
	},
	{
		"TRIBLEND_ONE",             
		VAL(Tr2RenderContextEnum::BM_ONE),             
		"Blend factor is (1, 1, 1, 1)."
	},
	{
		"TRIBLEND_SRCCOLOR",        
		VAL(Tr2RenderContextEnum::BM_SRCCOLOR),        
		"Blend factor is (Rs, Gs, Bs, As)."
	},
	{
		"TRIBLEND_INVSRCCOLOR",     
		VAL(Tr2RenderContextEnum::BM_INVSRCCOLOR),     
		"Blend factor is (1-Rs, 1-Gs, 1-Bs, 1-As)."
	},
	{
		"TRIBLEND_SRCALPHA",        
		VAL(Tr2RenderContextEnum::BM_SRCALPHA),        
		"Blend factor is (As, As, As, As)."
	},
	{
		"TRIBLEND_INVSRCALPHA",     
		VAL(Tr2RenderContextEnum::BM_INVSRCALPHA),     
		"Blend factor is (1-As, 1-As, 1-As, 1-As)."
	},
	{
		"TRIBLEND_DESTALPHA",       
		VAL(Tr2RenderContextEnum::BM_DESTALPHA),       
		"Blend factor is (Ad, Ad, Ad, Ad)."
	},
	{
		"TRIBLEND_INVDESTALPHA",    
		VAL(Tr2RenderContextEnum::BM_INVDESTALPHA),    
		"Blend factor is (1-Ad, 1-Ad, 1-Ad, 1-Ad)."
	},
	{
		"TRIBLEND_DESTCOLOR",    
		VAL(Tr2RenderContextEnum::BM_DESTCOLOR),    
		"Blend factor is (Rd, Gd, Bd, Ad)."
	},
	{
		"TRIBLEND_INVDESTCOLOR",    
		VAL(Tr2RenderContextEnum::BM_INVDESTCOLOR),    
		"Blend factor is (1-Rd, 1-Gd, 1-Bd, 1-Ad)."
	},
	{
		"TRIBLEND_SRCALPHASAT",     
		VAL(Tr2RenderContextEnum::BM_SRCALPHASAT),     
		"Blend factor is (f, f, f, 1); f = min(As, 1-Ad)."
	},
	{
		"TRIBLEND_BOTHINVSRCALPHA", 
		VAL(Tr2RenderContextEnum::BM_BOTHINVSRCALPHA), 
		"Source blend factor is (1-As, 1-As, 1-As, 1-As), and destination blend factor is "
		"(As, As, As, As); the destination blend selection is overridden. This blend mode is "
		"supported only for the D3DRS_SRCBLEND render state." 
	},
	{0}
};


#if DEPRECATED_ENABLED

const Be::VarChooser TriTextureAddress [] =
{
	{
		"TRITADDRESS_WRAP",            
		VAL(D3DTADDRESS_WRAP),            
		"Tile the texture at every integer junction. For example, "
		"for u values between 0 and 3, the texture is repeated three "
		"times; no mirroring is performed. "
	},
	{
		"TRITADDRESS_MIRROR",            
		VAL(D3DTADDRESS_MIRROR),            
		"Similar to D3DTADDRESS_WRAP, except that the texture is flipped at "
		"every integer junction. For u values between 0 and 1, for example, the "
		"texture is addressed normally; between 1 and 2, the texture is flipped "
		"(mirrored); between 2 and 3, the texture is normal again, and so on. "
	},
	{
		"TRITADDRESS_CLAMP",            
		VAL(D3DTADDRESS_CLAMP),            
		"Texture coordinates outside the range [0.0, 1.0] are set to "
		"the texture color at 0.0 or 1.0, respectively. "
	},
	{
		"TRITADDRESS_BORDER",            
		VAL(D3DTADDRESS_BORDER),            
		"Texture coordinates outside the range [0.0, 1.0] are set "
		"to the border color."
	},
	{
		"TRITADDRESS_MIRRORONCE",            
		VAL(D3DTADDRESS_MIRRORONCE),            
		"Similar to D3DTADDRESS_MIRROR and D3DTADDRESS_CLAMP. Takes the "
		"absolute value of the texture coordinate (thus, mirroring around 0), and "
		"then clamps to the maximum value. The most common usage is for volume textures, "
		"where support for the full D3DTADDRESS_MIRRORONCE texture-addressing mode is not "
		"necessary, but the data is symmetric around the one axis. "
	},
	{0}
};

const Be::VarChooser TriImageFileFormat  [] =
{
	{
		"TRIIFF_BMP",            
		VAL(D3DXIFF_BMP),            
		"Microsoft® Windows® bitmap file format."
	},
	{
		"TRIIFF_JPG",            
		VAL(D3DXIFF_JPG),            
		"Microsoft® Windows® bitmap file format."
	},
		{
		"TRIIFF_TGA",            
		VAL(D3DXIFF_TGA),            
		"Microsoft® Windows® bitmap file format."
	},
		{
		"TRIIFF_PNG",            
		VAL(D3DXIFF_PNG),            
		"Microsoft® Windows® bitmap file format."
	},
		{
		"TRIIFF_DDS",            
		VAL(D3DXIFF_DDS),            
		"Microsoft® Windows® bitmap file format."
	},
		{
		"TRIIFF_PPM",            
		VAL(D3DXIFF_PPM),            
		"Microsoft® Windows® bitmap file format."
	},
		{
		"TRIIFF_DIB",            
		VAL(D3DXIFF_DIB),            
		"Microsoft® Windows® bitmap file format."
	},
};


const Be::VarChooser TriMultiSampleType [] = 
{
	{
		"TRIMULTISAMPLE_NONE",            
		VAL(D3DMULTISAMPLE_NONE),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_NONMASKABLE",            
		VAL(D3DMULTISAMPLE_NONMASKABLE),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_2_SAMPLES",            
		VAL(D3DMULTISAMPLE_2_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_3_SAMPLES",            
		VAL(D3DMULTISAMPLE_3_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_4_SAMPLES",            
		VAL(D3DMULTISAMPLE_4_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_5_SAMPLES",            
		VAL(D3DMULTISAMPLE_5_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_6_SAMPLES",            
		VAL(D3DMULTISAMPLE_7_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_7_SAMPLES",            
		VAL(D3DMULTISAMPLE_7_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_8_SAMPLES",            
		VAL(D3DMULTISAMPLE_8_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_9_SAMPLES",            
		VAL(D3DMULTISAMPLE_9_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_10_SAMPLES",            
		VAL(D3DMULTISAMPLE_10_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_11_SAMPLES",            
		VAL(D3DMULTISAMPLE_11_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_12_SAMPLES",            
		VAL(D3DMULTISAMPLE_12_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_13_SAMPLES",            
		VAL(D3DMULTISAMPLE_13_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_14_SAMPLES",            
		VAL(D3DMULTISAMPLE_14_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_15_SAMPLES",            
		VAL(D3DMULTISAMPLE_15_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{
		"TRIMULTISAMPLE_16_SAMPLES",            
		VAL(D3DMULTISAMPLE_16_SAMPLES),            
		"See D3D Documentation.\r\n"
	},
	{0}
};

#endif
const Be::VarChooser TriClearFlags [] = 
{
	{
		"TRICLEAR_STENCIL",
		VAL(CLEARFLAGS_STENCIL),
		"Clear the stencil buffer to 0.\r\n"
	},
	{
		"TRICLEAR_TARGET",
		VAL(CLEARFLAGS_TARGET),
		"Clear the render target to the scenes bgColor parameter.\r\n"
	},
	{
		"TRICLEAR_ZBUFFER",
		VAL(CLEARFLAGS_ZBUFFER),
		"Clear the depth buffer to 1.0.\r\n"
	},
	{0}
};

#if DEPRECATED_ENABLED


const Be::VarChooser TriFormat [] = 
{
	{
		"TRIFMT_UNKNOWN",
		VAL(D3DFMT_UNKNOWN),
		"Surface format is unknown.\r\n"
	},

	//Unsigned Formats
	{
		"TRIFMT_R8G8B8",
		VAL(D3DFMT_R8G8B8),
		"24-bit RGB pixel format with 8 bits per channel.\r\n"
	},
	{
		"TRIFMT_A8R8G8B8",
		VAL(D3DFMT_A8R8G8B8),
		"32-bit ARGB pixel format with alpha, using 8 bits per channel.\r\n"
	},
	{
		"TRIFMT_X8R8G8B8",
		VAL(D3DFMT_X8R8G8B8),
		"32-bit RGB pixel format, where 8 bits are reserved for each color.\r\n"
	},
	{
		"TRIFMT_R5G6B5",
		VAL(D3DFMT_R5G6B5),
		"16-bit RGB pixel format with 5 bits for red, 6 bits for green, and 5 bits for blue.\r\n"
	},
	{
		"TRIFMT_X1R5G5B5",
		VAL(D3DFMT_X1R5G5B5),
		"16-bit pixel format where 5 bits are reserved for each color.\r\n"
	},
	{
		"TRIFMT_A1R5G5B5",
		VAL(D3DFMT_A1R5G5B5),
		"16-bit pixel format where 5 bits are reserved for each color and 1 bit is "
		"reserved for alpha.\r\n"
	},
	{
		"TRIFMT_A4R4G4B4",
		VAL(D3DFMT_A4R4G4B4),
		"16-bit ARGB pixel format with 4 bits for each channel.\r\n"
	},
	{
		"TRIFMT_R3G3B2",
		VAL(D3DFMT_R3G3B2),
		"8-bit RGB texture format using 3 bits for red, 3 bits for green, and 2 bits for blue.\r\n"
	},
	{
		"TRIFMT_A8",
		VAL(D3DFMT_A8),
		"8-bit alpha only.\r\n"
	},
	{
		"TRIFMT_A8R3G3B2",
		VAL(D3DFMT_A8R3G3B2),
		"16-bit ARGB texture format using 8 bits for alpha, 3 bits each for red and green, and 2 bits for blue.\r\n"
	},
	{
		"TRIFMT_X4R4G4B4",
		VAL(D3DFMT_X4R4G4B4),
		"16-bit RGB pixel format using 4 bits for each color.\r\n"
	},
	{
		"TRIFMT_A2B10G10R10",
		VAL(D3DFMT_A2B10G10R10),
		"32-bit pixel format using 10 bits for each color and 2 bits for alpha.\r\n"
	},
	{
		"TRIFMT_A8B8G8R8",
		VAL(D3DFMT_A8B8G8R8),
		"32-bit ARGB pixel format with alpha, using 8 bits per channel.\r\n"
	},
	{
		"TRIFMT_X8B8G8R8",
		VAL(D3DFMT_X8B8G8R8),
		"32-bit RGB pixel format, where 8 bits are reserved for each color.\r\n"
	},
	{
		"TRIFMT_G16R16",
		VAL(D3DFMT_G16R16),
		"32-bit pixel format using 16 bits each for green and red.\r\n"
	},
	{
		"TRIFMT_A2R10G10B10",
		VAL(D3DFMT_A2R10G10B10),
		"32-bit pixel format using 10 bits each for red, green, and blue, and 2 bits for alpha.\r\n"
	},
	{
		"TRIFMT_A16B16G16R16",
		VAL(D3DFMT_A16B16G16R16),
		"64-bit pixel format using 16 bits for each component.\r\n"
	},
	{
		"TRIFMT_A8P8",
		VAL(D3DFMT_A8P8),
		"8-bit color indexed with 8 bits of alpha.\r\n"
	},
	{
		"TRIFMT_P8",
		VAL(D3DFMT_P8),
		"color indexed.\r\n"
	},
	{
		"TRIFMT_L8",
		VAL(D3DFMT_L8),
		"8-bit luminance only.\r\n"
	},
	{
		"TRIFMT_A8L8",
		VAL(D3DFMT_A8L8),
		"using 8 bits each for alpha and luminance.\r\n"
	},
	{
		"TRIFMT_A4L4",
		VAL(D3DFMT_A4L4),
		"using 4 bits each for alpha and luminance.\r\n"
	},
	// Signed Formats
	{
		"TRIFMT_V8U8",
		VAL(D3DFMT_V8U8),
		"16-bit bump-map format using 8 bits each for u and v data.\r\n"
	},
	{
		"TRIFMT_Q8W8V8U8",
		VAL(D3DFMT_Q8W8V8U8),
		"32-bit bump-map format using 8 bits for each channel.\r\n"
	},
	{
		"TRIFMT_V16U16",
		VAL(D3DFMT_V16U16 ),
		"32-bit bump-map format using 16 bits for each channel.\r\n"
	},
	{
		"TRIFMT_Q16W16V16U16",
		VAL(D3DFMT_Q16W16V16U16 ),
		"64-bit bump-map format using 16 bits for each component.\r\n"
	},
	{
		"TRIFMT_CxV8U8",
		VAL(D3DFMT_CxV8U8 ),
		"16-bit normal compression format. The texture sampler computes the C channel from: C = sqrt(1 - U2 - V2).\r\n"
	},

	//Mixed Formats
	{
		"TRIFMT_L6V5U5",
		VAL(D3DFMT_L6V5U5 ),
		"16-bit bump-map format with luminance using 6 bits for luminance, and 5 bits each for v and u.\r\n"
	},

	{
		"TRIFMT_X8L8V8U8",
		VAL(D3DFMT_X8L8V8U8 ),
		"32-bit bump-map format with luminance using 8 bits for each channel.\r\n"
	},
	{
		"TRIFMT_A2W10V10U10",
		VAL(D3DFMT_A2W10V10U10 ),
		"32-bit bump-map format using 2 bits for alpha and 10 bits each for w, v, and u.\r\n"
	},
	// FOURCC Formats
	{
		"TRIFMT_MULTI2_ARGB8",
		VAL(D3DFMT_MULTI2_ARGB8 ),
		"MultiElement texture (not compressed).\r\n"
	},
	{
		"TRIFMT_G8R8_G8B8",
		VAL(D3DFMT_G8R8_G8B8 ),
		"A 16-bit packed RGB format analogous to YUY2 (Y0U0, Y1V0, Y2U2, etc.). It requires a pixel pair in order to properly represent the color value. The first pixel in the pair contains 8 bits of green (in the high 8 bits) and 8 bits of red (in the low 8 bits). The second pixel contains 8 bits of green (in the high 8 bits) and 8 bits of blue (in the low 8 bits). Together, the two pixels share the red and blue components, while each has a unique green component (G0R0, G1B0, G2R2, etc.). The texture sampler does not normalize the colors when looking up into a pixel shader; they remain in the range of 0.0f to 255.0f. This is true for all programmable pixel shader models. For the fixed function pixel shader, the hardware should normalize to the 0.f to 1.f range and essentially treat it as the YUY2 texture. Hardware that exposes this format must have the PixelShader1xMaxValue member of D3DCAPS9 set to a value capable of handling that range.\r\n"
	},
	{
		"TRIFMT_R8G8_B8G8",
		VAL(D3DFMT_R8G8_B8G8 ),
		"A 16-bit packed RGB format analogous to UYVY (U0Y0, V0Y1, U2Y2, etc.). It requires a pixel pair in order to properly represent the color value. The first pixel in the pair contains 8 bits of green (in the low 8 bits) and 8 bits of red (in the high 8 bits). The second pixel contains 8 bits of green (in the low 8 bits) and 8 bits of blue (in the high 8 bits). Together, the two pixels share the red and blue components, while each has a unique green component (R0G0, B0G1, R2G2, etc.). The texture sampler does not normalize the colors when looking up into a pixel shader; they remain in the range of 0.0f to 255.0f. This is true for all programmable pixel shader models. For the fixed function pixel shader, the hardware should normalize to the 0.f to 1.f range and essentially treat it as the YUY2 texture. Hardware that exposes this format must have PixelShader1xMaxValue member of D3DCAPS9 set to a value capable of handling that range.\r\n"
	},
	{
		"TRIFMT_DXT1",
		VAL(D3DFMT_DXT1 ),
		"DXT1 compression texture format.\r\n"
	},
	{
		"TRIFMT_DXT2",
		VAL(D3DFMT_DXT2 ),
		"DXT2 compression texture format.\r\n"
	},
	{
		"TRIFMT_DXT3",
		VAL(D3DFMT_DXT3 ),
		"DXT3 compression texture format.\r\n"
	},
	{
		"TRIFMT_DXT4",
		VAL(D3DFMT_DXT4 ),
		"DXT4 compression texture format.\r\n"
	},
	{
		"TRIFMT_DXT5",
		VAL(D3DFMT_DXT5 ),
		"DXT5 compression texture format.\r\n"
	},
	{
		"TRIFMT_UYVY",
		VAL(D3DFMT_UYVY ),
		"UYVY format (PC98 compliance).\r\n"
	},
	{
		"TRIFMT_YUY2",
		VAL(D3DFMT_YUY2 ),
		"YUY2 format (PC98 compliance).\r\n"
	},

	//Buffer Formats
	{
		"TRIFMT_D16_LOCKABLE",
		VAL(D3DFMT_D16_LOCKABLE ),
		"16-bit z-buffer bit depth. This is an application-lockable surface format.\r\n"
	},
	{
		"TRIFMT_D32",
		VAL(D3DFMT_D32 ),
		"32-bit z-buffer bit depth.\r\n"
	},
	{
		"TRIFMT_D15S1",
		VAL(D3DFMT_D15S1 ),
		"z-buffer bit depth where 15 bits are reserved for the depth channel and 1 bit is reserved for the stencil channel.\r\n"		
	},
	{
		"TRIFMT_D24S8",
		VAL(D3DFMT_D24S8 ),
		"32-bit z-buffer bit depth using 24 bits for the depth channel and 8 bits for the stencil channel."
	},
	{
		"TRIFMT_D24X8",
		VAL(D3DFMT_D24X8 ),
		"z-buffer bit depth using 24 bits for the depth channel."
	},
	{
		"TRIFMT_D24X4S4",
		VAL(D3DFMT_D24X4S4 ),
		"32-bit z-buffer bit depth using 24 bits for the depth channel and 4 bits for the stencil channel."
	},
	{
		"TRIFMT_D32F_LOCKABLE",
		VAL(D3DFMT_D32F_LOCKABLE ),
		"A lockable format where the depth value is represented as a standard IEEE floating-point number."
	},
	{
		"TRIFMT_D24FS8",
		VAL(D3DFMT_D24FS8 ),
		"A non-lockable format that contains 24 bits of depth (in a 24-bit floating point format - 20e4) and 8 bits of stencil."
	},
	{
		"TRIFMT_D16",
		VAL(D3DFMT_D16 ),
		"16-bit z-buffer bit depth.\r\n"
	},
	{
		"TRIFMT_VERTEXDATA",
		VAL(D3DFMT_VERTEXDATA ),
		"Describes a vertex buffer surface.\r\n"
	},
	{
		"TRIFMT_INDEX16",
		VAL(D3DFMT_INDEX16 ),
		"16-bit index buffer bit depth.\r\n"
	},
	{
		"TRIFMT_INDEX32",
		VAL(D3DFMT_INDEX32 ),
		"32-bit index buffer bit depth.\r\n"
	},

	// Floating-Point Formats
	{
		"TRIFMT_R16F",
		VAL(D3DFMT_R16F ),
		"16-bit float format using 16 bits for the red channel.\r\n"
	},
	{
		"TRIFMT_G16R16F",
		VAL(D3DFMT_G16R16F ),
		"32-bit float format using 16 bits for the red channel and 16 bits for the green channel.\r\n"
	},
	{
		"TRIFMT_A16B16G16R16F",
		VAL(D3DFMT_A16B16G16R16F ),
		"64-bit float format using 16 bits for the each channel (alpha, blue, green, red).\r\n"
	},
	// IEEE Formats
	{
		"TRIFMT_R32F",
		VAL(D3DFMT_R32F ),
		"32-bit float format using 32 bits for the red channel.\r\n"
	},
	{
		"TRIFMT_G32R32F",
		VAL(D3DFMT_G32R32F ),
		"64-bit float format using 32 bits for the red channel and 32 bits for the green channel.\r\n"
	},
	{
		"TRIFMT_A32B32G32R32F",
		VAL(D3DFMT_A32B32G32R32F ),
		"128-bit float format using 32 bits for the each channel (alpha, blue, green, red).\r\n"
	},
	{
		"TRIFMT_INTZ",
		VAL( MAKEFOURCC( 'I', 'N', 'T', 'Z' ) ),
		"32-bit format for depth texture (24-bit depth + 8-bit stencil).\r\n"
	},
	{0}
};


const Be::VarChooser TriFormatZStencil [] = 
{
	{
		"TRIFMT_D16_LOCKABLE",
		VAL(D3DFMT_D16_LOCKABLE ),
		"16-bit z-buffer bit depth. This is an application-lockable surface format.\r\n"
	},
	{
		"TRIFMT_D32",
		VAL(D3DFMT_D32 ),
		"32-bit z-buffer bit depth.\r\n"
	},
	{
		"TRIFMT_D15S1",
		VAL(D3DFMT_D15S1 ),
		"z-buffer bit depth where 15 bits are reserved for the depth channel and 1 bit is reserved for the stencil channel.\r\n"		
	},
	{
		"TRIFMT_D24S8",
		VAL(D3DFMT_D24S8 ),
		"32-bit z-buffer bit depth using 24 bits for the depth channel and 8 bits for the stencil channel."
	},
	{
		"TRIFMT_D24X8",
		VAL(D3DFMT_D24X8 ),
		"z-buffer bit depth using 24 bits for the depth channel."
	},
	{
		"TRIFMT_D24X4S4",
		VAL(D3DFMT_D24X4S4 ),
		"32-bit z-buffer bit depth using 24 bits for the depth channel and 4 bits for the stencil channel."
	},
	{
		"TRIFMT_D32F_LOCKABLE",
		VAL(D3DFMT_D32F_LOCKABLE ),
		"A lockable format where the depth value is represented as a standard IEEE floating-point number."
	},
	{
		"TRIFMT_D24FS8",
		VAL(D3DFMT_D24FS8 ),
		"A non-lockable format that contains 24 bits of depth (in a 24-bit floating point format - 20e4) and 8 bits of stencil."
	},
	{
		"TRIFMT_D16",
		VAL(D3DFMT_D16 ),
		"16-bit z-buffer bit depth.\r\n"
	},
	{
		"TRIFMT_VERTEXDATA",
		VAL(D3DFMT_VERTEXDATA ),
		"Describes a vertex buffer surface.\r\n"
	},
	{
		"TRIFMT_INDEX16",
		VAL(D3DFMT_INDEX16 ),
		"16-bit index buffer bit depth.\r\n"
	},
	{
		"TRIFMT_INDEX32",
		VAL(D3DFMT_INDEX32 ),
		"32-bit index buffer bit depth.\r\n"
	},
	{0}
};
     
const Be::VarChooser TriResourceTypeChooser [] = 
{
	{
		"TRIRTYPE_SURFACE",
		VAL(D3DRTYPE_SURFACE),
		"Surface resource.\r\n"
	},
	{
		"TRIRTYPE_VOLUME",
		VAL(D3DRTYPE_VOLUME),
		"Volume resource.\r\n"
	},
	{
		"TRIRTYPE_TEXTURE",
		VAL(D3DRTYPE_TEXTURE  ),
		"Texture resource.\r\n"
	},
	{
		"TRIRTYPE_VOLUMETEXTURE",
		VAL(D3DRTYPE_VOLUMETEXTURE  ),
		"Volume texture resource.\r\n"
	},
	{
		"TRIRTYPE_CUBETEXTURE",
		VAL(D3DRTYPE_CUBETEXTURE  ),
		"Cube texture resource.\r\n"
	},
	{
		"TRIRTYPE_VERTEXBUFFER",
		VAL(D3DRTYPE_VERTEXBUFFER  ),
		"Vertex buffer resource.\r\n"
	},
	{
		"TRIRTYPE_INDEXBUFFER",
		VAL(D3DRTYPE_INDEXBUFFER  ),
		"Index buffer resource.\r\n"
	},
	{0}
};

#endif

const Be::VarChooser TriHACKFORTESTING [] = 
{
	{
		"TRIVSDE_POSITION",
		VAL(0),
		"n/a.\r\n"
	},
	{
		"TRIVSDE_NORMAL",
		VAL(1),
		"n/a.\r\n"
	},
	{0}
};


const Be::VarChooser TriScissorMode[] =
{
	{
		"SCISSOR_NONE",
		VAL(SCISSOR_NONE),
		"disable scissoring"
	},
	{
		"SCISSOR_SCISSOR",
		VAL(SCISSOR_SCISSOR),
		"use d3d scissoring"
	},
	{
		"SCISSOR_EMULATE",
		VAL(SCISSOR_EMULATE),
		"emulate d3d scissoring"
	},
	{
		"SCISSOR_CHOOSE",
		VAL(SCISSOR_CHOOSE),
		"choose d3d scissoring"
	},
	{0}
};

const Be::VarChooser TriBipedMovementState[] =
{
	{
		"TRIBPS_IDLE",
		VAL(TRIBPS_IDLE),
		"Idle"
	},
	{
		"TRIBPS_WALKING",
		VAL(TRIBPS_WALKING),
		"Walking"
	},
	{
		"TRIBPS_TURNING",
		VAL(TRIBPS_TURNING),
		"Turning"
	},
	{
		"TRIBPS_RUNNING",
		VAL(TRIBPS_RUNNING),
		"Running"
	},
	{
		"TRIBPS_STRAFING",
		VAL(TRIBPS_STRAFING),
		"Running"
	},
	{0}
};

const Be::VarChooser TriSkeletonType[] =
{
	{
		"TRIST_MAIN",
		VAL(TRIST_MAIN),
		"Main skeleton"
	},
	{
		"TRIST_CLOTH_UPPER",
		VAL(TRIST_CLOTH_UPPER),
		"Upper cloth skeleton"
	},
	{
		"TRIST_CLOTH_LOWER",
		VAL(TRIST_CLOTH_LOWER),
		"Lower cloth skeleton"
	},
	{0}
};

const Be::VarChooser TriD3DRenderState[] =
{
	KV_RC( 
		RS_ZENABLE,
		"Depth-buffering state as one member of the ZBUFFERTYPE enumerated type.\n"
		"Set this state to ZB_TRUE to enable z-buffering, ZB_USEW to enable\n"
		"w-buffering, or ZB_FALSE to disable depth buffering. The default value\n"
		"for this render state is ZB_TRUE if a depth stencil was created along\n"
		"with the swap chain by setting the EnableAutoDepthStencil member of the\n"
		"PRESENT_PARAMETERS structure to TRUE, and ZB_FALSE otherwise."
	),
	KV_RC( 
		RS_FILLMODE, 
		"One or more members of the FILLMODE enumerated type.\n"
		"The default value is FILL_SOLID."
	),
	KV_RC( RS_SHADEMODE, "" ),
	KV_RC( RS_ZWRITEENABLE, "" ),
	KV_RC( RS_ALPHATESTENABLE, "" ),
	KV_RC( RS_LASTPIXEL, "" ),
	KV_RC( RS_SRCBLEND, "" ),
	KV_RC( RS_DESTBLEND, "" ),
	KV_RC( RS_CULLMODE, "" ),
	KV_RC( RS_ZFUNC, "" ),
	KV_RC( RS_ALPHAREF, "" ),
	KV_RC( RS_DITHERENABLE, "" ),
	KV_RC( RS_ALPHABLENDENABLE, "" ),
	KV_RC( RS_ALPHABLENDENABLE, "" ),
	KV_RC( RS_FOGENABLE, "" ),
	KV_RC( RS_SPECULARENABLE, "" ),
	KV_RC( RS_FOGCOLOR, "" ),
	KV_RC( RS_FOGTABLEMODE, "" ),
	KV_RC( RS_FOGSTART, "" ),
	KV_RC( RS_FOGEND, "" ),
	KV_RC( RS_FOGDENSITY, "" ),
	KV_RC( RS_RANGEFOGENABLE, "" ),
	KV_RC( RS_STENCILENABLE, "" ),
	KV_RC( RS_STENCILFAIL, "" ),
	KV_RC( RS_STENCILZFAIL, "" ),
	KV_RC( RS_STENCILPASS, "" ),
	KV_RC( RS_STENCILFUNC, "" ),
	KV_RC( RS_STENCILREF, "" ),
	KV_RC( RS_STENCILMASK, "" ),
	KV_RC( RS_STENCILWRITEMASK, "" ),
	KV_RC( RS_TEXTUREFACTOR, "" ),
	KV_RC( RS_WRAP0, "" ),
	KV_RC( RS_WRAP1, "" ),
	KV_RC( RS_WRAP2, "" ),
	KV_RC( RS_WRAP3, "" ),
	KV_RC( RS_WRAP4, "" ),
	KV_RC( RS_WRAP5, "" ),
	KV_RC( RS_WRAP6, "" ),
	KV_RC( RS_WRAP7, "" ),
	KV_RC( RS_CLIPPING, "" ),
	KV_RC( RS_LIGHTING, "" ),
	KV_RC( RS_AMBIENT, "" ),
	KV_RC( RS_FOGVERTEXMODE, "" ),
	KV_RC( RS_COLORVERTEX, "" ),
	KV_RC( RS_LOCALVIEWER, "" ),
	KV_RC( RS_NORMALIZENORMALS, "" ),
	KV_RC( RS_DIFFUSEMATERIALSOURCE, "" ),
	KV_RC( RS_SPECULARMATERIALSOURCE, "" ),
	KV_RC( RS_AMBIENTMATERIALSOURCE, "" ),
	KV_RC( RS_EMISSIVEMATERIALSOURCE, "" ),
	KV_RC( RS_VERTEXBLEND, "" ),
	KV_RC( RS_CLIPPLANEENABLE, "" ),
	KV_RC( RS_POINTSIZE, "" ),
	KV_RC( RS_POINTSIZE_MIN, "" ),
	KV_RC( RS_POINTSPRITEENABLE, "" ),
	KV_RC( RS_POINTSCALEENABLE, "" ),
	KV_RC( RS_POINTSCALE_A, "" ),
	KV_RC( RS_POINTSCALE_B, "" ),
	KV_RC( RS_POINTSCALE_C, "" ),
	KV_RC( RS_MULTISAMPLEANTIALIAS, "" ),
	KV_RC( RS_MULTISAMPLEMASK, "" ),
	KV_RC( RS_PATCHEDGESTYLE, "" ),
	KV_RC( RS_DEBUGMONITORTOKEN, "" ),
	KV_RC( RS_POINTSIZE_MAX, "" ),
	KV_RC( RS_INDEXEDVERTEXBLENDENABLE, "" ),
	KV_RC( RS_COLORWRITEENABLE, "" ),
	KV_RC( RS_TWEENFACTOR, "" ),
	KV_RC( RS_BLENDOP, "" ),
	KV_RC( RS_POSITIONDEGREE, "" ),
	KV_RC( RS_NORMALDEGREE, "" ),
	KV_RC( RS_SCISSORTESTENABLE, "" ),
	KV_RC( RS_SLOPESCALEDEPTHBIAS, "" ),
	KV_RC( RS_ANTIALIASEDLINEENABLE, "" ),
	KV_RC( RS_MINTESSELLATIONLEVEL, "" ),
	KV_RC( RS_MAXTESSELLATIONLEVEL, "" ),
	KV_RC( RS_ADAPTIVETESS_X, "" ),
	KV_RC( RS_ADAPTIVETESS_Y, "" ),
	KV_RC( RS_ADAPTIVETESS_Z, "" ),
	KV_RC( RS_ADAPTIVETESS_W, "" ),
	KV_RC( RS_ENABLEADAPTIVETESSELLATION, "" ),
	KV_RC( RS_TWOSIDEDSTENCILMODE, "" ),
	KV_RC( RS_CCW_STENCILFAIL, "" ),
	KV_RC( RS_CCW_STENCILZFAIL, "" ),
	KV_RC( RS_CCW_STENCILPASS, "" ),
	KV_RC( RS_CCW_STENCILFUNC, "" ),
	KV_RC( RS_COLORWRITEENABLE1, "" ),
	KV_RC( RS_COLORWRITEENABLE2, "" ),
	KV_RC( RS_COLORWRITEENABLE3, "" ),
	KV_RC( RS_BLENDFACTOR, "" ),
	KV_RC( RS_SRGBWRITEENABLE, "" ),
	KV_RC( RS_DEPTHBIAS, "" ),
	KV_RC( RS_WRAP8, "" ),
	KV_RC( RS_WRAP9, "" ),
	KV_RC( RS_WRAP10, "" ),
	KV_RC( RS_WRAP11, "" ),
	KV_RC( RS_WRAP12, "" ),
	KV_RC( RS_WRAP13, "" ),
	KV_RC( RS_WRAP14, "" ),
	KV_RC( RS_WRAP15, "" ),
	KV_RC( RS_SEPARATEALPHABLENDENABLE, "" ),
	KV_RC( RS_SRCBLENDALPHA, "" ),
	KV_RC( RS_DESTBLENDALPHA, "" ),
	KV_RC( RS_BLENDOPALPHA, "" ),

	{0}
};

#if DEPRECATED_ENABLED

const Be::VarChooser TriD3DFormat [] = 
{
	KV( D3DFMT_UNKNOWN, 	"Surface format is unknown.\r\n" ),

	//Unsigned Formats
	KV( 
		D3DFMT_R8G8B8,
		"24-bit RGB pixel format with 8 bits per channel.\r\n"
	),
	KV( 
		D3DFMT_A8R8G8B8,
		"32-bit ARGB pixel format with alpha, using 8 bits per channel.\r\n"
	),
	KV( 
		D3DFMT_X8R8G8B8,
		"32-bit RGB pixel format, where 8 bits are reserved for each color.\r\n"
	),
	KV( 
		D3DFMT_R5G6B5,
		"16-bit RGB pixel format with 5 bits for red, 6 bits for green, and 5 bits for blue.\r\n"
	),
	KV( 
		D3DFMT_X1R5G5B5,
		"16-bit pixel format where 5 bits are reserved for each color.\r\n"
	),
	KV( 
		D3DFMT_A1R5G5B5,
		"16-bit pixel format where 5 bits are reserved for each color and 1 bit is "
		"reserved for alpha.\r\n"
	),
	KV( 
		D3DFMT_A4R4G4B4,
		"16-bit ARGB pixel format with 4 bits for each channel.\r\n"
	),
	KV( 
		D3DFMT_R3G3B2,
		"8-bit RGB texture format using 3 bits for red, 3 bits for green, and 2 bits for blue.\r\n"
	),
	KV( 
		D3DFMT_A8,
		"8-bit alpha only.\r\n"
	),
	KV( 
		D3DFMT_A8R3G3B2,
		"16-bit ARGB texture format using 8 bits for alpha, 3 bits each for red and green, and 2 bits for blue.\r\n"
	),
	KV( 
		D3DFMT_X4R4G4B4,
		"16-bit RGB pixel format using 4 bits for each color.\r\n"
	),
	KV( 
		D3DFMT_A2B10G10R10,
		"32-bit pixel format using 10 bits for each color and 2 bits for alpha.\r\n"
	),
	KV( 
		D3DFMT_A8B8G8R8,
		"32-bit ARGB pixel format with alpha, using 8 bits per channel.\r\n"
	),
	KV( 
		D3DFMT_X8B8G8R8,
		"32-bit RGB pixel format, where 8 bits are reserved for each color.\r\n"
	),
	KV( 
		D3DFMT_G16R16,
		"32-bit pixel format using 16 bits each for green and red.\r\n"
	),
	KV( 
		D3DFMT_A2R10G10B10,
		"32-bit pixel format using 10 bits each for red, green, and blue, and 2 bits for alpha.\r\n"
	),
	KV( 
		D3DFMT_A16B16G16R16,
		"64-bit pixel format using 16 bits for each component.\r\n"
	),
	KV( 
		D3DFMT_A8P8,
		"8-bit color indexed with 8 bits of alpha.\r\n"
	),
	KV( 
		D3DFMT_P8,
		"color indexed.\r\n"
	),
	KV( 
		D3DFMT_L8,
		"8-bit luminance only.\r\n"
	),
	KV( 
		D3DFMT_A8L8,
		"using 8 bits each for alpha and luminance.\r\n"
	),
	KV( 
		D3DFMT_A4L4,
		"using 4 bits each for alpha and luminance.\r\n"
	),
	// Signed Formats
	KV( 
		D3DFMT_V8U8,
		"16-bit bump-map format using 8 bits each for u and v data.\r\n"
	),
	KV( 
		D3DFMT_Q8W8V8U8,
		"32-bit bump-map format using 8 bits for each channel.\r\n"
	),
	KV( 
		D3DFMT_V16U16 ,
		"32-bit bump-map format using 16 bits for each channel.\r\n"
	),
	KV( 
		D3DFMT_Q16W16V16U16 ,
		"64-bit bump-map format using 16 bits for each component.\r\n"
	),
	KV( 
		D3DFMT_CxV8U8 ,
		"16-bit normal compression format. The texture sampler computes the C channel from: C = sqrt(1 - U2 - V2).\r\n"
	),

	//Mixed Formats
	KV( 
		D3DFMT_L6V5U5 ,
		"16-bit bump-map format with luminance using 6 bits for luminance, and 5 bits each for v and u.\r\n"
	),

	KV( 
		D3DFMT_X8L8V8U8 ,
		"32-bit bump-map format with luminance using 8 bits for each channel.\r\n"
	),
	KV( 
		D3DFMT_A2W10V10U10 ,
		"32-bit bump-map format using 2 bits for alpha and 10 bits each for w, v, and u.\r\n"
	),
	// FOURCC Formats
	KV( 
		D3DFMT_MULTI2_ARGB8 ,
		"MultiElement texture (not compressed).\r\n"
	),
	KV( 
		D3DFMT_G8R8_G8B8 ,
		"A 16-bit packed RGB format analogous to YUY2 (Y0U0, Y1V0, Y2U2, etc.). It requires a pixel pair in order to properly represent the color value. The first pixel in the pair contains 8 bits of green (in the high 8 bits) and 8 bits of red (in the low 8 bits). The second pixel contains 8 bits of green (in the high 8 bits) and 8 bits of blue (in the low 8 bits). Together, the two pixels share the red and blue components, while each has a unique green component (G0R0, G1B0, G2R2, etc.). The texture sampler does not normalize the colors when looking up into a pixel shader; they remain in the range of 0.0f to 255.0f. This is true for all programmable pixel shader models. For the fixed function pixel shader, the hardware should normalize to the 0.f to 1.f range and essentially treat it as the YUY2 texture. Hardware that exposes this format must have the PixelShader1xMaxValue member of D3DCAPS9 set to a value capable of handling that range.\r\n"
	),
	KV( 
		D3DFMT_R8G8_B8G8 ,
		"A 16-bit packed RGB format analogous to UYVY (U0Y0, V0Y1, U2Y2, etc.). It requires a pixel pair in order to properly represent the color value. The first pixel in the pair contains 8 bits of green (in the low 8 bits) and 8 bits of red (in the high 8 bits). The second pixel contains 8 bits of green (in the low 8 bits) and 8 bits of blue (in the high 8 bits). Together, the two pixels share the red and blue components, while each has a unique green component (R0G0, B0G1, R2G2, etc.). The texture sampler does not normalize the colors when looking up into a pixel shader; they remain in the range of 0.0f to 255.0f. This is true for all programmable pixel shader models. For the fixed function pixel shader, the hardware should normalize to the 0.f to 1.f range and essentially treat it as the YUY2 texture. Hardware that exposes this format must have PixelShader1xMaxValue member of D3DCAPS9 set to a value capable of handling that range.\r\n"
	),
	KV( 
		D3DFMT_DXT1 ,
		"DXT1 compression texture format.\r\n"
	),
	KV( 
		D3DFMT_DXT2 ,
		"DXT2 compression texture format.\r\n"
	),
	KV( 
		D3DFMT_DXT3 ,
		"DXT3 compression texture format.\r\n"
	),
	KV( 
		D3DFMT_DXT4 ,
		"DXT4 compression texture format.\r\n"
	),
	KV( 
		D3DFMT_DXT5 ,
		"DXT5 compression texture format.\r\n"
	),
	KV( 
		D3DFMT_UYVY ,
		"UYVY format (PC98 compliance).\r\n"
	),
	KV( 
		D3DFMT_YUY2 ,
		"YUY2 format (PC98 compliance).\r\n"
	),

	//Buffer Formats
	KV( 
		D3DFMT_D16_LOCKABLE ,
		"16-bit z-buffer bit depth. This is an application-lockable surface format.\r\n"
	),
	KV( 
		D3DFMT_D32 ,
		"32-bit z-buffer bit depth.\r\n"
	),
	KV( 
		D3DFMT_D15S1 ,
		"z-buffer bit depth where 15 bits are reserved for the depth channel and 1 bit is reserved for the stencil channel.\r\n"		
	),
	KV( 
		D3DFMT_D24S8 ,
		"32-bit z-buffer bit depth using 24 bits for the depth channel and 8 bits for the stencil channel."
	),
	KV( 
		D3DFMT_D24X8 ,
		"z-buffer bit depth using 24 bits for the depth channel."
	),
	KV( 
		D3DFMT_D24X4S4 ,
		"32-bit z-buffer bit depth using 24 bits for the depth channel and 4 bits for the stencil channel."
	),
	KV( 
		D3DFMT_D32F_LOCKABLE ,
		"A lockable format where the depth value is represented as a standard IEEE floating-point number."
	),
	KV( 
		D3DFMT_D24FS8 ,
		"A non-lockable format that contains 24 bits of depth (in a 24-bit floating point format - 20e4) and 8 bits of stencil."
	),
	KV( 
		D3DFMT_D16 ,
		"16-bit z-buffer bit depth.\r\n"
	),
	KV( 
		D3DFMT_VERTEXDATA ,
		"Describes a vertex buffer surface.\r\n"
	),
	KV( 
		D3DFMT_INDEX16 ,
		"16-bit index buffer bit depth.\r\n"
	),
	KV( 
		D3DFMT_INDEX32 ,
		"32-bit index buffer bit depth.\r\n"
	),

	// Floating-Point Formats
	KV( 
		D3DFMT_R16F ,
		"16-bit float format using 16 bits for the red channel.\r\n"
	),
	KV( 
		D3DFMT_G16R16F ,
		"32-bit float format using 16 bits for the red channel and 16 bits for the green channel.\r\n"
	),
	KV( 
		D3DFMT_A16B16G16R16F ,
		"64-bit float format using 16 bits for the each channel (alpha, blue, green, red).\r\n"
	),
	// IEEE Formats
	KV( 
		D3DFMT_R32F ,
		"32-bit float format using 32 bits for the red channel.\r\n"
	),
	KV( 
		D3DFMT_G32R32F ,
		"64-bit float format using 32 bits for the red channel and 32 bits for the green channel.\r\n"
	),
	KV( 
		D3DFMT_A32B32G32R32F ,
		"128-bit float format using 32 bits for the each channel (alpha, blue, green, red).\r\n"
	),
	{ 0 }
};

const Be::VarChooser TriFilterMode[] =
{
    {
        "TRIFILTER_NONE",
        VAL(D3DX_FILTER_NONE),
        "No scaling or filtering will take place. Pixels outside the bounds of the source image are assumed to be transparent black."
    },
    {
        "TRIFILTER_POINT",
        VAL(D3DX_FILTER_POINT),
        "Each destination pixel is computed by sampling the nearest pixel from the source image."
    },
    {
        "TRIFILTER_LINEAR",
        VAL(D3DX_FILTER_LINEAR),
        "Each destination pixel is computed by sampling the four nearest pixels from the source image. This filter works best when the scale on both axes is less than two."
    },
    {
        "TRIFILTER_TRIANGLE",
        VAL(D3DX_FILTER_TRIANGLE),
        "Every pixel in the source image contributes equally to the destination image. This is the slowest of the filters."
    },
    {
        "TRIFILTER_BOX",
        VAL(D3DX_FILTER_BOX),
        "Each pixel is computed by averaging a 2x2(x2) box of pixels from the source image. This filter works only when the dimensions of the destination are half those of the source, as is the case with mipmaps."
    },
    {
        "TRIFILTER_MIRROR_U",
        VAL(D3DX_FILTER_MIRROR_U),
        "Pixels off the edge of the texture on the u-axis should be mirrored, not wrapped."
    },
    {
        "TRIFILTER_MIRROR_V",
        VAL(D3DX_FILTER_MIRROR_V),
        "Pixels off the edge of the texture on the v-axis should be mirrored, not wrapped."
    },
    {
        "TRIFILTER_MIRROR_W",
        VAL(D3DX_FILTER_MIRROR_W),
        "Pixels off the edge of the texture on the w-axis should be mirrored, not wrapped."
    },
    {
        "TRIFILTER_MIRROR",
        VAL(D3DX_FILTER_MIRROR),
        "Specifying this flag is the same as specifying the D3DX_FILTER_MIRROR_U, D3DX_FILTER_MIRROR_V, and D3DX_FILTER_MIRROR_W flags."
    },
    {
        "TRIFILTER_DITHER",
        VAL(D3DX_FILTER_DITHER),
        "The resulting image must be dithered using a 4x4 ordered dither algorithm."
    },
    {
        "TRIFILTER_SRGB_IN",
        VAL(D3DX_FILTER_SRGB_IN),
        "Input data is in sRGB (gamma 2.2) color space."
    },
    {
        "TRIFILTER_SRGB_OUT",
        VAL(D3DX_FILTER_SRGB_OUT),
        "The output data is in sRGB (gamma 2.2) color space."
    },
    {
        "TRIFILTER_SRGB",
        VAL(D3DX_FILTER_SRGB),
        "Same as specifying D3DX_FILTER_SRGB_IN | D3DX_FILTER_SRGB_OUT."
    },
    {0}
};

#endif
