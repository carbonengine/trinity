// Copyright © 2023 CCP ehf.

#include "StdAfx.h"

TEST( VertexDefinition, CanCompareVertexDefinitionItems )
{
	Tr2VertexDefinition::Item item1;
	item1.m_usage = Tr2VertexDefinition::POSITION;
	item1.m_usageIndex = 0;
	item1.m_dataType = Tr2VertexDefinition::FLOAT16_2;
	item1.m_offset = 0;
	item1.m_stream = 0;
	item1.m_instanceStepRate = 0;
	EXPECT_TRUE( item1 == item1 );

	Tr2VertexDefinition::Item item2;
	item2.m_usage = Tr2VertexDefinition::TEXCOORD;
	item2.m_usageIndex = 0;
	item2.m_dataType = Tr2VertexDefinition::FLOAT16_2;
	item2.m_offset = 16;
	item2.m_stream = 0;
	item2.m_instanceStepRate = 0;
	EXPECT_TRUE( item2 == item2 );

	EXPECT_FALSE( item1 == item2 );
	EXPECT_TRUE( item1 < item2 );
}

TEST( VertexDefinition, CanAddItemsToVertexDefinition )
{
	Tr2VertexDefinition def;
	EXPECT_TRUE( def.empty() );

	def.Add( Tr2VertexDefinition::FLOAT16_2, Tr2VertexDefinition::TEXCOORD );
	EXPECT_FALSE( def.empty() );
	EXPECT_EQ( Tr2VertexDefinition::TEXCOORD, def.m_items[0].m_usage );
	EXPECT_EQ( 0, def.m_items[0].m_usageIndex );
	EXPECT_EQ( Tr2VertexDefinition::FLOAT16_2, def.m_items[0].m_dataType );
	EXPECT_EQ( 0, def.m_items[0].m_offset );
	EXPECT_EQ( 0, def.m_items[0].m_stream );
	EXPECT_EQ( 0, def.m_items[0].m_instanceStepRate );

	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION, 2, 1, 1 );
	EXPECT_FALSE( def.empty() );
	EXPECT_EQ( Tr2VertexDefinition::POSITION, def.m_items[1].m_usage );
	EXPECT_EQ( 2, def.m_items[1].m_usageIndex );
	EXPECT_EQ( Tr2VertexDefinition::FLOAT32_3, def.m_items[1].m_dataType );
	EXPECT_EQ( 0, def.m_items[1].m_offset );
	EXPECT_EQ( 1, def.m_items[1].m_stream );
	EXPECT_EQ( 1, def.m_items[1].m_instanceStepRate );

	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::POSITION, 0, 1, 0 );
	EXPECT_FALSE( def.empty() );
	EXPECT_EQ( Tr2VertexDefinition::POSITION, def.m_items[2].m_usage );
	EXPECT_EQ( 0, def.m_items[2].m_usageIndex );
	EXPECT_EQ( Tr2VertexDefinition::FLOAT32_3, def.m_items[2].m_dataType );
	EXPECT_EQ( 12, def.m_items[2].m_offset );
	EXPECT_EQ( 1, def.m_items[2].m_stream );
	EXPECT_EQ( 0, def.m_items[2].m_instanceStepRate );

	def.Add( Tr2VertexDefinition::FLOAT32_1, Tr2VertexDefinition::NORMAL, 0, 0, 1 );
	EXPECT_FALSE( def.empty() );
	EXPECT_EQ( Tr2VertexDefinition::NORMAL, def.m_items[3].m_usage );
	EXPECT_EQ( 0, def.m_items[3].m_usageIndex );
	EXPECT_EQ( Tr2VertexDefinition::FLOAT32_1, def.m_items[3].m_dataType );
	EXPECT_EQ( 4, def.m_items[3].m_offset );
	EXPECT_EQ( 0, def.m_items[3].m_stream );
	EXPECT_EQ( 1, def.m_items[3].m_instanceStepRate );
}

TEST( VertexDefinition, CanSearchForVertexDefinitionItems )
{
	Tr2VertexDefinition def;
	def.Add( Tr2VertexDefinition::FLOAT32_1, Tr2VertexDefinition::POSITION );
	def.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION, 1 );
	def.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::NORMAL, 0, 1 );

	Tr2VertexDefinition::Item* item = def.Find( Tr2VertexDefinition::POSITION );
	ASSERT_NE( nullptr, item );
	EXPECT_EQ( Tr2VertexDefinition::POSITION, item->m_usage );
	EXPECT_EQ( 0, item->m_usageIndex );

	item = def.Find( Tr2VertexDefinition::POSITION, 1 );
	ASSERT_NE( nullptr, item );
	EXPECT_EQ( Tr2VertexDefinition::POSITION, item->m_usage );
	EXPECT_EQ( 1, item->m_usageIndex );

	item = def.Find( Tr2VertexDefinition::TEXCOORD );
	ASSERT_EQ( nullptr, item );
}

TEST( VertexDefinition, CanQueryVertexDefinitionItemSizeInMembers )
{
	EXPECT_EQ( 1, Tr2VertexDefinition::GetDataTypeSizeInMembers( Tr2VertexDefinition::FLOAT32_1 ) );
	EXPECT_EQ( 2, Tr2VertexDefinition::GetDataTypeSizeInMembers( Tr2VertexDefinition::FLOAT16_2 ) );
	EXPECT_EQ( 3, Tr2VertexDefinition::GetDataTypeSizeInMembers( Tr2VertexDefinition::UINT32_3 ) );
	EXPECT_EQ( 4, Tr2VertexDefinition::GetDataTypeSizeInMembers( Tr2VertexDefinition::FLOAT32_4 ) );
}

TEST( VertexDefinition, CanQueryVertexDefinitionItemSizeInBytes )
{
	EXPECT_EQ( 4, Tr2VertexDefinition::GetDataTypeSizeInBytes( Tr2VertexDefinition::FLOAT32_1 ) );
	EXPECT_EQ( 4, Tr2VertexDefinition::GetDataTypeSizeInBytes( Tr2VertexDefinition::FLOAT16_2 ) );
	EXPECT_EQ( 12, Tr2VertexDefinition::GetDataTypeSizeInBytes( Tr2VertexDefinition::UINT32_3 ) );
	EXPECT_EQ( 4, Tr2VertexDefinition::GetDataTypeSizeInBytes( Tr2VertexDefinition::BYTE_4 ) );
}

TEST( VertexDefinition, CanCompareVertexDefinitions )
{
	Tr2VertexDefinition def1;
	def1.Add( Tr2VertexDefinition::FLOAT32_1, Tr2VertexDefinition::POSITION );
	def1.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION, 1 );
	def1.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::NORMAL, 0, 1 );

	Tr2VertexDefinition def2;
	def2.Add( Tr2VertexDefinition::FLOAT32_1, Tr2VertexDefinition::POSITION );
	def2.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION, 1 );
	def2.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::NORMAL, 0, 1 );

	EXPECT_TRUE( def1 == def2 );

	def2.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::TEXCOORD, 0, 2 );

	EXPECT_FALSE( def1 == def2 );
}

TEST( VertexDefinition, CanMoveVertexDefinition )
{
	Tr2VertexDefinition def1;
	def1.Add( Tr2VertexDefinition::FLOAT32_1, Tr2VertexDefinition::POSITION );
	def1.Add( Tr2VertexDefinition::FLOAT32_2, Tr2VertexDefinition::POSITION, 1 );
	def1.Add( Tr2VertexDefinition::FLOAT32_3, Tr2VertexDefinition::NORMAL, 0, 1 );

	Tr2VertexDefinition def2 = def1;

	Tr2VertexDefinition def3;
	def3 = std::move( def1 );

	EXPECT_TRUE( def2 == def3 );
}
