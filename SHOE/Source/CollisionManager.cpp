#include "..\Headers\CollisionManager.h"
#include "..\Headers\ComponentManager.h"

using namespace DirectX;

// forward declaration for static
std::vector<std::shared_ptr<Collider>> CollisionManager::markedAsTriggerboxes_;
std::vector<std::shared_ptr<Collider>> CollisionManager::markedAsColliders_;

CollisionManager::CollisionManager()
{
	markedAsTriggerboxes_ = std::vector<std::shared_ptr<Collider>>();
	markedAsColliders_ = std::vector<std::shared_ptr<Collider>>();
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
	for (auto& c : ComponentManager::GetAllEnabled<Collider>())
	{
		c->Update();
	}
	CheckTriggerCollisions();
	CheckColliderCollisions();
	//for (int i = 0; i < allColliders_.size(); i++)
	//{
	//	std::shared_ptr<Collider> a = allColliders_[i];
	//	for (int j = 0; j < allColliders_.size(); j++)
	//	{
	//		std::shared_ptr<Collider> b = allColliders_[j];
	//
	//		// Skip if about to check against self
	//		if (b == a)
	//			continue;
	//
	//		// TODO: Somehow need to check for collisions while avoiding dupes 
	//		if (!a->GetTriggerStatus() && b->GetTriggerStatus())
	//		{
	//			
	//		}
	//	}
	//}
}

/**
 * \brief Adds a collider shared_ptr to main list and appropriate subset (collider or triggerbox) list
 * \param c_ collider to add
 */
void CollisionManager::AddColliderToManager(std::shared_ptr<Collider> c_)
{
	// add it to the appropriate subset
	if (c_->GetTriggerStatus() == true)
		markedAsTriggerboxes_.push_back(c_);
	else
		markedAsColliders_.push_back(c_);
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
