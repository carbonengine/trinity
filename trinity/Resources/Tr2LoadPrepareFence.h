////////////////////////////////////////////////////////////
//
//    Created:   January 2021
//    Copyright: CCP 2021
//

#pragma once

// Helper object that acts like a fence in load/prepare ResMan queues.
// It will put a fence into the background queue first and then when it's
// reached, into the main thread queue. This way we can track if all the
// resources that started loading before this fence is put, have finished
// loading and preparing.
class Tr2LoadPrepareFence
{
public:
	Tr2LoadPrepareFence();
	~Tr2LoadPrepareFence();

	//void Put();
	void Put( const std::function<void()>& onPrepared = std::function<void()>() );
	bool IsReached() const;
	void Cancel();

private:
	static void StaticResourceLoadFinished( void* pContext );
	static void StaticResourcePrepFinished( void* pContext );

	CcpAtomic<uint32_t> m_resourceLoadCbId;
	CcpAtomic<uint32_t> m_resourcePrepCbId;
	std::function<void()> m_onPrepared;

	bool m_reached;
};
