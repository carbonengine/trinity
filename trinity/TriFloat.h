#pragma once
#ifndef TriFloat_h
#define TriFloat_h



// This class exists to help break circular references.
// A particular case is when the length of an EveStretch needs to
// be bound to something via TriValueBinding. If the length is a
// regular attribute on the EveStretch the binding would create
// a circular reference (stretch->curve set->binding->stretch).
// Separating the length into its own object breaks the circle.

BLUE_CLASS( TriFloat ):
     public IRoot
{
public:
    EXPOSE_TO_BLUE();
    TriFloat( IRoot* lockobj = NULL );

	float m_value;
};

TYPEDEF_BLUECLASS( TriFloat );
#endif //TriFloat_h
