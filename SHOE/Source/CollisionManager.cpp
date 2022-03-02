#include "..\Headers\CollisionManager.h"

using namespace DirectX;

// forward declaration for static
std::vector<std::shared_ptr<Collider>> CollisionManager::allColliders_;
std::vector<std::shared_ptr<Collider>> CollisionManager::markedAsTriggerboxes_;
std::vector<std::shared_ptr<Collider>> CollisionManager::markedAsColliders_;

CollisionManager::CollisionManager()
{
	markedAsTriggerboxes_ = std::vector<std::shared_ptr<Collider>>();
	markedAsColliders_ = std::vector<std::shared_ptr<Collider>>();
}


std::vector<std::shared_ptr<Collider>> CollisionManager::GetAllColliders()
{
	return allColliders_;
}

std::vector<std::shared_ptr<Collider>> CollisionManager::GetMarkedAsTriggerboxes()
{
	return markedAsTriggerboxes_;
}

std::vector<std::shared_ptr<Collider>> CollisionManager::GetMarkedAsColliders()
{
	return markedAsColliders_;
}

void CollisionManager::Update()
{
	for (auto& c : allColliders_)
	{
		c->Update();
	}
	CheckTriggerCollisions();
	CheckColliderCollisions();
}

void CollisionManager::AddColliderToManager(Collider c_)
{
	std::shared_ptr<Collider> c = std::make_shared<Collider>(c_);
	allColliders_.push_back(c);
}

void CollisionManager::CheckTriggerCollisions()
{
	for (auto& t : markedAsTriggerboxes_)
	{
		DirectX::BoundingOrientedBox tOBB = t.get()->GetOrientedBoundingBox();
		for (auto& c : markedAsColliders_)
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
