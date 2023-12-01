////////////////////////////////////////////////////////////////////////////////
//
// Created:		June 2021
// Copyright:	CCP 2021
//
#pragma once

BLUE_DECLARE_INTERFACE( ITr2Renderable );
BLUE_DECLARE_INTERFACE( IEveSpaceObjectChild );
BLUE_DECLARE_INTERFACE( ITr2LightOwner );
BLUE_DECLARE_INTERFACE( ITr2VolumetricRenderable );
BLUE_DECLARE( EveEntity );

struct RegistrationState
{
	bool reflectionRenderable;
	bool volumetricRenderable;
	bool lightOwner;
};

enum ComponentType
{
	REFLECTION_RENDERABLE,
	VOLUMETRIC_RENDERABLE,
	COUNT
};


BLUE_CLASS( EveComponentRegistry ) :
	public IRoot
{
public:
	EXPOSE_TO_BLUE();

	
	EveComponentRegistry( IRoot* lockobj = NULL );
	~EveComponentRegistry();

	void Clear();

	void RegisterComponent( ComponentType type, EveEntity * entity, RegistrationState & state );
	void UnRegisterComponent( ComponentType type, EveEntity * entity, RegistrationState & state, bool silent = false );

	void ReRegister( EveEntity * entity );

	const std::vector<ITr2LightOwner*>& GetLightOwners() const
	{
		return m_lightOwners;
	};

	const std::vector<ITr2Renderable*>& GetReflectionRenderables() const
	{
		return m_reflectionRenderables;
	}

	const std::vector<ITr2VolumetricRenderable*>& GetVolumetricRenderables() const
	{
		return m_volumetricRenderables;
	}

	size_t GetLightOwnerCount() const;
	size_t GetReflectionRenderableCount() const;

	bool HasComponent( ComponentType type, RegistrationState & state );

private:
	std::vector<ITr2Renderable*> m_reflectionRenderables;
	std::vector<ITr2VolumetricRenderable*> m_volumetricRenderables;
	std::vector<ITr2LightOwner*> m_lightOwners;

	CcpMutex* m_entityReregisterMutex;
};

TYPEDEF_BLUECLASS( EveComponentRegistry );