// Copyright © 2024 CCP ehf.

#include "StdAfx.h"
#include "PriorityBlend.h"
#include <vector>


using namespace PriorityBlend;


Vector2 PriorityBlend::Zero( Vector2 )
{
	return Vector2( 0, 0 );
}

Vector3 PriorityBlend::Zero( Vector3 )
{
	return Vector3( 0, 0, 0 );
}

Vector4 PriorityBlend::Zero( Vector4 )
{
	return Vector4( 0, 0, 0, 0 );
}

Color PriorityBlend::Zero( Color )
{
	return Color( 0, 0, 0, 0 );
}

bool PriorityBlend::Zero( bool )
{
	return false;
}