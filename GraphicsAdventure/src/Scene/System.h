#pragma once
#include "Scene.h"

namespace GA
{
	class System
	{
	public:
		System(Scene* scene)
			: m_scene(scene) { }
		virtual ~System() = default;

		System(const System&) = delete;
		System& operator=(const System&) = delete;

	protected:
		entt::registry& GetRegistry() { return m_scene->m_registry; }

		Scene* m_scene = nullptr;
	};
}