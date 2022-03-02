#pragma once

#include "Collider.h"
#include <vector>

class CollisionManager
{
public:
	CollisionManager();

	static std::vector<std::shared_ptr<Collider>> GetAllColliders();
	static std::vector<std::shared_ptr<Collider>> GetMarkedAsTriggerboxes();
	static std::vector<std::shared_ptr<Collider>> GetMarkedAsColliders();

	static void Update();

	static void AddColliderToManager(Collider c_);
	//TODO: this is going to get so bad with memory leaks so we need a clean removal process somehow

private:
	static void CheckTriggerCollisions();
	static void CheckColliderCollisions();

	static std::vector<std::shared_ptr<Collider>> allColliders_;
	static std::vector<std::shared_ptr<Collider>> markedAsTriggerboxes_;
	static std::vector<std::shared_ptr<Collider>> markedAsColliders_;
};
