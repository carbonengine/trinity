#ifndef CARBON_BONETRANSFORMS_FXH_
#define CARBON_BONETRANSFORMS_FXH_

// Contains bone transforms for skeletal animation for all objects in the render pass. Each bone transform is a 
// 4x3 matrix (3 rows of 4 floats) that represents the transformation of a bone in the skeleton.
StructuredBuffer<float4x3> BoneTransforms <bool AutoRegister = true;>;

float4x4 GetBoneTransform( int boneIndex )
{
	float4x3 bone = BoneTransforms[boneIndex];

	float4x4 boneMatrix = float4x4( float4( bone[0], 0.0 ),
									float4( bone[1], 0.0 ),
									float4( bone[2], 0.0 ), 
									float4( bone[3], 1.0 ) );
	return boneMatrix;
}

#endif