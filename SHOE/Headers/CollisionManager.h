#pragma once

#include "Collider.h"
#include <vector>

class CollisionManager
{
public:
	CollisionManager();
	~CollisionManager();

	static std::vector<std::shared_ptr<Collider>> GetMarkedAsTriggerboxes();
	static std::vector<std::shared_ptr<Collider>> GetMarkedAsColliders();

	static void Update();

	static void AddColliderToManager(std::shared_ptr<Collider> _c);
private:
	static void CheckTriggerCollisions();
	static void CheckColliderCollisions();

	static std::vector<std::shared_ptr<Collider>> markedAsTriggerboxes_;
	static std::vector<std::shared_ptr<Collider>> markedAsColliders_;
};
