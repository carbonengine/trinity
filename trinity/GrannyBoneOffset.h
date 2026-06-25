// Copyright © 2023 CCP ehf.

#pragma once
#ifndef GrannyBoneOffset_h_
#define GrannyBoneOffset_h_

// Description:
//  Helper class to store and apply an extra translation and/or rotation for every bone in a granny
//  animation rig.
//  Used by Tr2GrannyAnimation to let us patch in small corrections without the need to create a single-frame
//  morpheme animation for overlaying.
//  Branched off from MorphemeBoneOffset, not much code worth sharing once the individual matrix transforms have
//  been changed to work with the native types of the SDKs (NMP::Matrix34 vs granny_local_transform).
// See Also: MorphemeBoneOffset
BLUE_CLASS( GrannyBoneOffset ) :
	public IInitialize
{
public:
	EXPOSE_TO_BLUE();

	using IInitialize::Lock;
	using IInitialize::Unlock;

	/////////////////////////////////////////////////////////////////////////////////////
	// IInitialize
	bool Initialize();

	GrannyBoneOffset( IRoot* lockobj = 0 );
	virtual ~GrannyBoneOffset();

	//	Set or replace the additional translation for this bone.
	void SetOffset( const std::string& bone, float x, float y, float z );

	//	Set or replace the additional rotation for this bone.
	void SetRotation( const std::string& bone, float r, float i, float j, float k );


	//	Map bone strings to joint indices using this skeleton
	void BindToRig( const std::string* bones, size_t numBones );

	//	Apply stored offset and/or rotation to specific bone whose rig index is joint.
	bool Apply( float* targetMatrix4x4, unsigned joint, const float* boneMatrix4x4, const float* parentMatrix4x4 );

	// Return Value:
	//	true if there are any transforms at all
	bool HaveTransforms() const;

	//  check if the number of bones has changed since the last rebind.
	bool NeedRebind( unsigned numBones ) const;

	// Undo the link between the offsets and the skeleton; use when the skeleton has changed
	void ClearRigBindings();
	// Clear the offsets alltogether, ie forget all settings
	void ClearTransforms();

	bool ApplyToLocal( unsigned joint, Quaternion& rotation, Vector3& position ) const;

private:
	struct TMatrix
	{
		float m[16];
	};
	typedef TrackableStdMap<std::string, TMatrix> TransformsMap;

	// bone name -> full transform
	TransformsMap m_transforms;

	// For every bone in the rig, an optional pointer into m_transforms specifying the transform to apply on this bone. Exactly numbones of these, in order.
	TrackableStdVector<const float*> m_riggedTransforms;
};

TYPEDEF_BLUECLASS( GrannyBoneOffset );

#endif // GrannyBoneOffset_h_
