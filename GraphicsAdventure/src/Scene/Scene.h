#pragma once
#include <entt/entt.hpp>

namespace GA
{
	class Entity;

	class Scene
	{
		friend class Entity;
		friend class System;

	public:
		Scene() = default;
		virtual ~Scene() = default;

		Scene(const Scene&) = delete;
		const Scene& operator=(const Scene&) = delete;

		Entity CreateEntity();
		void DestroyEntity(Entity entity);
		bool EntityExists(Entity entity);

	private:
		entt::registry m_registry;
	};
}