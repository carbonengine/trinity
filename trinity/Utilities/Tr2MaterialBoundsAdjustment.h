////////////////////////////////////////////////////////////////////////////////
//
// Created:		January 2022
// Copyright:	CCP 2022
//

#pragma once

struct Tr2MaterialBoundsAdjustment
{
	float maxLocalScale = 1;
	float maxLocalDisplacement = 0;
	bool rotatesVertices = false;

	CcpMath::AxisAlignedBox AdjustBounds( const CcpMath::AxisAlignedBox& box ) const;
};

bool BlueExtractArgumentImpl( BlueScriptArguments argument, Tr2MaterialBoundsAdjustment& result, unsigned int argID, std::false_type isBlueType );
BlueScriptValue BlueWrapReturnValueImpl( BlueScriptArguments args, const Tr2MaterialBoundsAdjustment& val );
