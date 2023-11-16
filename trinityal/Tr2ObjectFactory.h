#pragma once

class Tr2PrimaryRenderContextAL;

namespace TrinityALImpl
{

	template <typename ObjectType, typename CreateArgument>
	class Tr2ObjectFactory
	{
	public:
		ALResult Get( std::shared_ptr<ObjectType>& object, const CreateArgument& argument, Tr2PrimaryRenderContextAL &renderContext )
		{
			auto found = m_objects.find( argument );
			if( found != m_objects.end() )
			{
				if( !found->second->IsValid() )
				{
					m_objects.erase( found );
				}
				else
				{
					object = found->second;
					return S_OK;
				}
			}
			auto newObject = std::make_shared<ObjectType>();
			CR_RETURN_HR( newObject->Create( argument, renderContext ) );
			m_objects[argument] = newObject;
			object = newObject;
			return S_OK;
		}

		void Clear()
		{
			m_objects.clear();
		}
	private:
		std::unordered_map<CreateArgument, std::shared_ptr<ObjectType>> m_objects;
	};
}