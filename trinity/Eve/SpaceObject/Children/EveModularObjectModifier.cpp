// Copyright © 2026 CCP ehf.

#include "StdAfx.h"
#include "EveModularObjectModifier.h"
#include "IEveEffectChildrenOwner.h"
#include "../EveStation2.h"
#include "../Attachments/EveImpactOverlay.h"
#include "EveChildInstancedMeshes.h"
#include "EveChildContainer.h"
#include <cmf/transforms.h>


void EveModularObjectModifier::Create( SpaceObjectType* object, EveSOF* sof )
{
	m_object = object;
	m_sof = sof;
	for( auto& child : object->GetEffectChildren() )
	{
		if( EveChildPartDataPtr partData = BlueCastPtr( child ) )
		{
			m_data = partData;
			break;
		}
	}
	if( !m_data )
	{
		m_data.CreateInstance();
		object->AddToEffectChildrenList( m_data );
	}
	for( auto& child : object->GetEffectChildren() )
	{
		if( EveChildInstancedMeshesPtr instancedMeshes = BlueCastPtr( child ) )
		{
			m_instancedMeshes = instancedMeshes;
			break;
		}
	}
}

EveModularObjectModifier::~EveModularObjectModifier()
{
	ApplyBounds();
}

void EveModularObjectModifier::ApplyBounds()
{
	if( m_object )
	{
		std::vector<const EveChildPartData::PartData*> orderedParts;
		orderedParts.reserve( m_data->m_parts.size() );
		for( const auto& part : m_data->m_parts )
		{
			orderedParts.push_back( &part );
		}
		std::sort( orderedParts.begin(), orderedParts.end(), []( const auto* a, const auto* b ) {
			return a->boundingSphere.radius > b->boundingSphere.radius;
		} );

		CcpMath::Sphere bounds;
		CcpMath::AxisAlignedBox box;
		for( const auto* part : orderedParts )
		{
			bounds.Include( part->boundingSphere );
			box.IncludeSphere( part->boundingSphere );
		}
		m_object->SetBoundingSphereInformation( bounds );
		m_object->SetShapeEllipsoid( m_data->m_parts.empty() ? CcpMath::AxisAlignedEllipsoid{} : CcpMath::AxisAlignedEllipsoid{ box, true } );
	}
}

void EveModularObjectModifier::UpdateImpactOverlayLocatorCount() const
{
	if( EveImpactOverlayPtr overlay = m_object->GetImpactOverlay() )
	{
		auto locators = m_object->GetLocatorsForSet( DAMAGE_LOCATOR_SET_NAME );
		overlay->SetDamageLocatorCount( locators ? uint32_t( locators->size() ) : 0 );
	}
}

EveSpaceObjectChild::PartTag EveModularObjectModifier::AllocatePartId() const
{
	auto id = m_data->GetUnusedPartID();
	for( auto& child : m_object->GetEffectChildren() )
	{
		if( child->GetPartTag() != EveSpaceObjectChild::NO_PART_TAG )
		{
			id = std::max( id, child->GetPartTag() + 1 );
		}
	}
	return id;
}

EveSpaceObjectChild::PartTag EveModularObjectModifier::AddHull( const char* hullName, const char* factionName, const char* raceName, const Vector3& position, const Quaternion& rotation, const Vector3& scale )
{
	auto id = AllocatePartId();
	auto size = m_object->GetEffectChildren().size();
	auto dna = std::string( hullName ) + ":" + ( factionName[0] ? factionName : m_data->m_faction.c_str() ) + ":" + ( raceName[0] ? raceName : m_data->m_race.c_str() );
	if( !m_sof->BuildChild( m_object, dna.c_str(), id, TransformationMatrix( scale, rotation, position ), m_armorDamageEffectCache ) )
	{
		return INVALID_PART_TAG;
	}

	if( !m_instancedMeshes )
	{
		for( size_t i = size; i < m_object->GetEffectChildren().size(); ++i )
		{
			if( EveChildInstancedMeshesPtr instancedMesh = BlueCastPtr( m_object->GetEffectChildren()[i] ) )
			{
				m_instancedMeshes = instancedMesh;
				break;
			}
		}
	}

	// SOF will reset the bounding sphere of the object to the one of the part
	// Store the part bounding sphere and recalculate the bounding sphere of the modular object after adding all the parts
	CcpMath::Sphere sphere{ m_object->GetBoundingSphereCenter(), m_object->GetBoundingSphereRadius() };

	auto part = EveChildPartData::PartData{ id, position, rotation, scale, sphere };
	m_data->m_parts.emplace_back( part );
	m_object->InvalidateMergedLocators( LocatorInvalidationReason::StructureChanged );
	UpdateImpactOverlayLocatorCount();
	return id;
}

EveSpaceObjectChild::PartTag EveModularObjectModifier::AddChild( const char* resPath, const Vector3& position, const Quaternion& rotation, const Vector3& scale )
{
	if( auto child = BeResMan->LoadObject<EveSpaceObjectChild>( resPath ) )
	{
		child->Setup( &scale, &rotation, &position, Tr2Lod::TR2_LOD_LOW );
		m_object->AddToEffectChildrenList( child );
		auto id = AllocatePartId();
		child->SetPartTag( id );
		m_data->m_parts.emplace_back( EveChildPartData::PartData{ id, position, rotation, scale } );
		m_object->InvalidateMergedLocators( LocatorInvalidationReason::StructureChanged );
		return id;
	}
	return INVALID_PART_TAG;
}

BlueStdResult EveModularObjectModifier::Remove( EveSpaceObjectChild::PartTag partId )
{
	auto found = std::find_if( m_data->m_parts.begin(), m_data->m_parts.end(), [partId]( const EveChildPartData::PartData& part ) {
		return part.partId == partId;
	} );
	if( found == m_data->m_parts.end() )
	{
		return BlueStdResultType::BLUE_STD_RESULT_KEY_ERROR;
	}

	for( size_t i = 0; i < m_object->GetEffectChildren().size(); )
	{
		auto child = m_object->GetEffectChildren()[i];
		if( child->GetPartTag() == partId )
		{
			m_object->RemoveFromEffectChildrenList( child );
			continue;
		}
		++i;
	}

	for( auto& set : m_object->GetLocatorSets() )
	{
		auto& locators = *set->GetLocators();
		auto removed = std::remove_if( locators.begin(), locators.end(), [partId]( const auto& locator ) {
			return locator.partTag == partId;
		} );
		locators.Resize( std::distance( locators.begin(), removed ) );
	}

	if( m_instancedMeshes )
	{
		m_instancedMeshes->RemoveInstancesByPartTag( partId );
	}
	m_data->m_parts.erase( found );
	m_object->InvalidateMergedLocators( LocatorInvalidationReason::StructureChanged );
	m_object->ClearImpactDamage();
	UpdateImpactOverlayLocatorCount();
	return BlueStdResultType::BLUE_STD_RESULT_OK;
}

BlueStdResult EveModularObjectModifier::SetTransform( EveSpaceObjectChild::PartTag partId, const Vector3& position, const Quaternion& rotation, Vector3 scale )
{
	auto found = std::find_if( m_data->m_parts.begin(), m_data->m_parts.end(), [partId]( const EveChildPartData::PartData& part ) {
		return part.partId == partId;
	} );
	if( found == m_data->m_parts.end() )
	{
		return BlueStdResultType::BLUE_STD_RESULT_KEY_ERROR;
	}

	cmf::Transform oldTransform{ found->position, found->rotation, found->scale };
	cmf::Transform newTransform{ position, rotation, scale };
	auto invOldTransform = cmf::Inverse( oldTransform );

	for( auto& set : m_object->GetLocatorSets() )
	{
		auto& locators = *set->GetLocators();
		for( auto& locator : locators )
		{
			if( locator.partTag == partId )
			{
				locator.scale.x = scale.x / found->scale.x;
				locator.scale.y = scale.y / found->scale.y;
				locator.scale.z = scale.z / found->scale.z;
				locator.direction = invOldTransform.rotation * rotation;
				locator.position = cmf::TransformPoint( cmf::TransformPoint( locator.position, invOldTransform ), newTransform );
			}
		}
	}

	found->boundingSphere.center = cmf::TransformPoint( cmf::TransformPoint( found->boundingSphere.center, invOldTransform ), newTransform );
	found->boundingSphere.radius *= std::max( { scale.x, scale.y, scale.z } ) / std::max( { found->scale.x, found->scale.y, found->scale.z } );

	found->position = position;
	found->rotation = rotation;
	found->scale = scale;

	for( auto& child : m_object->GetEffectChildren() )
	{
		if( child->GetPartTag() == partId )
		{
			child->Setup( &scale, &rotation, &position, Tr2Lod::TR2_LOD_LOW );
		}
		if( EveChildInstancedMeshesPtr instancedMeshes = BlueCastPtr( child ) )
		{
			instancedMeshes->SetInstanceTransformByPartTag( partId, position, rotation, scale );
		}
	}
	m_object->InvalidateMergedLocators( LocatorInvalidationReason::PartMoved );
	return BlueStdResultType::BLUE_STD_RESULT_OK;
}

BlueStdResult EveModularObjectModifier::GetPosition( EveSpaceObjectChild::PartTag partId, Vector3& position ) const
{
	auto found = std::find_if( m_data->m_parts.begin(), m_data->m_parts.end(), [partId]( const EveChildPartData::PartData& part ) {
		return part.partId == partId;
	} );
	if( found == m_data->m_parts.end() )
	{
		return BlueStdResultType::BLUE_STD_RESULT_KEY_ERROR;
	}
	position = found->position;
	return BlueStdResultType::BLUE_STD_RESULT_OK;
}

BlueStdResult EveModularObjectModifier::GetRotation( EveSpaceObjectChild::PartTag partId, Quaternion& rotation ) const
{
	auto found = std::find_if( m_data->m_parts.begin(), m_data->m_parts.end(), [partId]( const EveChildPartData::PartData& part ) {
		return part.partId == partId;
	} );
	if( found == m_data->m_parts.end() )
	{
		return BlueStdResultType::BLUE_STD_RESULT_KEY_ERROR;
	}
	rotation = found->rotation;
	return BlueStdResultType::BLUE_STD_RESULT_OK;
}

BlueStdResult EveModularObjectModifier::GetScale( EveSpaceObjectChild::PartTag partId, Vector3& scale ) const
{
	auto found = std::find_if( m_data->m_parts.begin(), m_data->m_parts.end(), [partId]( const EveChildPartData::PartData& part ) {
		return part.partId == partId;
	} );
	if( found == m_data->m_parts.end() )
	{
		return BlueStdResultType::BLUE_STD_RESULT_KEY_ERROR;
	}
	scale = found->scale;
	return BlueStdResultType::BLUE_STD_RESULT_OK;
}


std::pair<IEveSpaceObject2Ptr, EveModularObjectModifierPtr> CreateModularObject( EveSOF* sof, const char* factionName, const char* raceName )
{
	EveStation2Ptr object;
	object.CreateInstance();
	object->Initialize();

	EveChildPartDataPtr partData;
	partData.CreateInstance();
	partData->m_faction = factionName;
	partData->m_race = raceName;

	object->AddToEffectChildrenList( partData );

	EveModularObjectModifierPtr modifier;
	modifier.CreateInstance();
	modifier->Create( object, sof );
	return { IEveSpaceObject2Ptr( object ), modifier };
}

EveModularObjectModifierPtr ModifyModularObject( EveModularObjectModifier::SpaceObjectType* object, EveSOF* sof )
{
	EveModularObjectModifierPtr modifier;
	modifier.CreateInstance();
	modifier->Create( object, sof );
	return modifier;
}
EveSpaceObjectChild::PartTag GetInvalidPartTag()
{
	return EveModularObjectModifier::INVALID_PART_TAG;
}