#pragma once
#ifndef Tr2LockGuard_H
#define Tr2LockGuard_H

class Tr2LockGuard
{
public:
	Tr2LockGuard();
	~Tr2LockGuard();

	void Lock( size_t size, void* originalMemory );
	void Unlock();
	void* GetMemory();
private:
	void* m_originalMemory;
	void* m_memory;
	size_t m_size;
};

#endif