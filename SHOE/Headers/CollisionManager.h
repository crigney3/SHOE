#pragma once

#include "Collider.h"
#include <vector>
#include <DirectXMath.h>

class CollisionManager
{
public:
	CollisionManager();

	static void CheckTriggerCollisions();
	static void CheckColliderCollisions();

	DirectX::XMVECTOR GetSeparatingPlane(const DirectX::XMVECTOR& _rPos, const DirectX::XMVECTOR& _plane, Collider& _a, Collider& _b);

	static bool SAT(Collider& _a, Collider& _b);

private:
	static std::vector<Collider> triggerboxes_;
	static std::vector<Collider> colliders_;
};
