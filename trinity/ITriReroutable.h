#ifndef ITRIREROUTABLE_H
#define ITRIREROUTABLE_H

class TriValueBinding;

BLUE_INTERFACE( ITriReroutable ) : public IRoot
{
	virtual void SetDestination( void* dest, size_t size ) = 0;
	virtual void GetDestination( void*& dest, size_t& size ) = 0;
	virtual void RegisterBinding( TriValueBinding* vb ) = 0;
	virtual void UnregisterBinding( TriValueBinding* vb ) = 0;
	virtual bool IsRerouted() const = 0;
};

#endif
