#include "..\Headers\CollisionManager.h"

using namespace DirectX;

//
std::vector<Collider> CollisionManager::triggerboxes_;
std::vector<Collider> CollisionManager::colliders_;

CollisionManager::CollisionManager()
{
	triggerboxes_ = std::vector<Collider>();
	colliders_ = std::vector<Collider>();
}

void CollisionManager::Update()
{
	CheckTriggerCollisions();
	CheckColliderCollisions();
}

std::vector<Collider>* CollisionManager::GetTriggerboxes()
{
	return &triggerboxes_;
}

std::vector<Collider>* CollisionManager::GetColliders()
{
	return &colliders_;
}

void CollisionManager::CheckTriggerCollisions()
{
	for (auto& t : triggerboxes_)
	{
		DirectX::BoundingOrientedBox tOBB = t.GetOrientedBoundingBox();
		for (auto& c : colliders_)
		{
			BoundingOrientedBox cOBB = c.GetOrientedBoundingBox();
			if (tOBB.Intersects(cOBB))
			{
				printf("Colliding\n");
			}
			if (cOBB.Intersects(tOBB))
			{
				printf("Colliding2\n");
			}
		}
	}
}

void CollisionManager::CheckColliderCollisions()
{
}
