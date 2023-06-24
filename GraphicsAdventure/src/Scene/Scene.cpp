#include "Scene.h"
#include "Entity.h"

namespace GA
{
	Entity Scene::CreateEntity()
	{
		return Entity(m_registry.create(), this);
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_registry.destroy(entity);
	}

	bool Scene::EntityExists(Entity entity)
	{
		return m_registry.valid(entity);
	}
}