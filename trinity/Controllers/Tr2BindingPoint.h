////////////////////////////////////////////////////////////
//
//    Created:   March 2018
//    Copyright: CCP 2018
//

#pragma once


class Tr2BindingPoint
{
public:
	Tr2BindingPoint();

	void Link( const std::vector<std::pair<std::string, IRoot*>>& roots );
	void Unlink();

	bool IsValid() const;

	void SetValue( float value ) const;
	bool GetValue( float& value ) const;

	IRoot* GetBoundObject() const;

	std::string m_path;
	IRootPtr m_object;
	std::string m_attribute;
protected:
	bool SetDestination( IRoot* object, const std::string& attribute );
private:
	BlueWeakRef<IRoot> m_resolvedObject;
	BlueWeakRef<INotify> m_notifyPtr;
	const Be::VarEntry* m_entry;
	Be::Var* m_destination;
	int32_t m_entryOffset;
	int32_t m_arraySize;
};
