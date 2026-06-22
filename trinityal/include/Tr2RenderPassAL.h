// Copyright © 2023 CCP ehf.

#pragma once


namespace Tr2LoadAction
{
enum Type
{
	DONT_CARE,
	LOAD,
	CLEAR,
};
}

namespace Tr2StoreAction
{
enum Type
{
	DONT_CARE,
	STORE,
};
}

struct Tr2DepthAttachment
{
	Tr2LoadAction::Type load = Tr2LoadAction::DONT_CARE;
	Tr2StoreAction::Type store = Tr2StoreAction::DONT_CARE;
	float clearValue = 0;
};

struct Tr2ColorAttachment
{
	Tr2LoadAction::Type load = Tr2LoadAction::DONT_CARE;
	Tr2StoreAction::Type store = Tr2StoreAction::DONT_CARE;
	uint32_t clearColor = 0;
};
