////////////////////////////////////////////////////////////
//
//    Created:   October 2018
//    Copyright: CCP 2018
//

#pragma once


BLUE_DECLARE_INTERFACE( ITr2ControllerAction );
BLUE_DECLARE_IVECTOR( ITr2ControllerAction );
BLUE_DECLARE( Tr2Controller );


BLUE_CLASS( Tr2ControllerEventHandler ): public IListNotify
{
public:
	Tr2ControllerEventHandler( IRoot* lockobj = nullptr );

	EXPOSE_TO_BLUE();

	void OnListModified( long event, ssize_t key, ssize_t key2, IRoot* value, const IList* list ) override;

	void Link( Tr2Controller& controller );
	void Unlink();

	const char* GetName() const;
	void Execute( Tr2Controller& controller );
private:
	std::string m_name;
	PITr2ControllerActionVector m_actions;

	Tr2Controller* m_controller;
};

TYPEDEF_BLUECLASS( Tr2ControllerEventHandler );