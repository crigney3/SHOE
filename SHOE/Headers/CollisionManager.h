#pragma once

#include "Collider.h"
#include <vector>
#include <DirectXMath.h>

class CollisionManager
{
public:
	CollisionManager();

	static void Update();
	static std::vector<Collider>* GetTriggerboxes();
	static std::vector<Collider>* GetColliders();

private:
	static void CheckTriggerCollisions();
	static void CheckColliderCollisions();

	static std::vector<Collider> triggerboxes_;
	static std::vector<Collider> colliders_;
};
