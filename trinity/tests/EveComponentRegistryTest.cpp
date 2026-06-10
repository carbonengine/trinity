#include "StdAfx.h"
#include "gtest/gtest.h"

#include "Eve/EveEntity.h"
#include "Eve/EveComponentRegistry.h"

int g_eveReflectionMode = 0;

struct ITestComponent
{
	virtual ~ITestComponent() {}
};

REGISTER_COMPONENT_TYPE( "ITestComponent", ITestComponent )

#define TEST_STUB_IROOT( BaseType ) \
	bool QueryInterface( const Be::IID&, void**, BLUEQIOPT ) override { return false; } \
	void Lock() override {} \
	void Unlock() override {} \
	long GetFlags() override { return 0; } \
	int GetRefCount() const override { return 1; } \
	IRoot* GetRootObject() const override { return const_cast<BaseType*>( static_cast<const BaseType*>( this ) ); } \
	void FinalDelete() override {}

namespace
{
	class TestComponentRegistry : public EveComponentRegistry
	{
	public:
		TestComponentRegistry() : EveComponentRegistry( nullptr ) {}
		TEST_STUB_IROOT( EveComponentRegistry )
	};

	class TestComponentEntity : public EveEntity, public ITestComponent
	{
	public:
		TestComponentEntity() : EveEntity( nullptr ) {}
		TEST_STUB_IROOT( EveEntity )

	protected:
		void RegisterComponents() override
		{
			if( EveComponentRegistry* registry = GetComponentRegistry() )
			{
				registry->RegisterComponent<ITestComponent>( this );
			}
		}

		void UnRegisterComponents() override { }
	};
}

TEST( EveComponentRegistry, RegisterAddsComponent )
{
	TestComponentRegistry registry;
	TestComponentEntity entity;

	entity.Register( &registry );
	EXPECT_EQ( static_cast<size_t>( 1 ), registry.ComponentCount<ITestComponent>() );

	registry.Clear();
}

TEST( EveComponentRegistry, ClearEmptiesCollections )
{
	TestComponentRegistry registry;
	TestComponentEntity entity;

	entity.Register( &registry );
	registry.Clear();

	EXPECT_EQ( static_cast<size_t>( 0 ), registry.ComponentCount<ITestComponent>() );
}

TEST( EveComponentRegistry, ReRegisterAfterClearReAddsComponent )
{
	TestComponentRegistry registry;
	TestComponentEntity entity;

	entity.Register( &registry );
	ASSERT_EQ( static_cast<size_t>( 1 ), registry.ComponentCount<ITestComponent>() );

	registry.Clear();
	ASSERT_EQ( static_cast<size_t>( 0 ), registry.ComponentCount<ITestComponent>() );

	entity.Register( &registry );
	EXPECT_EQ( static_cast<size_t>( 1 ), registry.ComponentCount<ITestComponent>() );

	ASSERT_NO_THROW( entity.UnRegister( &registry ) );

	registry.Clear();
}
