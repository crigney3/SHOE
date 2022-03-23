#include "..\Headers\CollisionManager.h"

#include "../Headers/GameEntity.h"
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

CollisionManager::~CollisionManager()
{
	for (auto c : markedAsColliders_) {
		c.reset();
	}
}

std::vector<std::shared_ptr<Collider>> CollisionManager::GetMarkedAsTriggerboxes() { return markedAsTriggerboxes_; }

std::vector<std::shared_ptr<Collider>> CollisionManager::GetMarkedAsColliders() { return markedAsColliders_; }

void CollisionManager::Update()
{
	for (auto& c : ComponentManager::GetAllEnabled<Collider>())
	{
		c->Update();
	}
	//CheckTriggerCollisions();
	//CheckColliderCollisions();

	// ROUGH implementation of collision. Seems to work and no dupes occur (I think)
	std::vector<std::shared_ptr<Collider>> c = ComponentManager::GetAllEnabled<Collider>();
	for (int i = 0; i < c.size(); i++)
	{
		std::shared_ptr<Collider> a = c[i];
		for (int j = 0; j < c.size(); j++)
		{
			std::shared_ptr<Collider> b = c[j];
	
			std::string aName = a->GetOwner()->GetName();
			std::string bName = b->GetOwner()->GetName();

			// Skip if about to check against self
			if (aName == bName)
				continue;
	
			// TODO: Somehow need to check for collisions while avoiding dupes 
			if ((!a->GetTriggerStatus() && b->GetTriggerStatus())
				&& a->GetOrientedBoundingBox().Intersects(b->GetOrientedBoundingBox()))
			{
#if defined(DEBUG) || defined(_DEBUG)
				printf("\n%s Colliding with %s", aName.c_str(), bName.c_str());
#endif
			}
		}
	}
}

/**
 * \brief Adds a collider shared_ptr to appropriate subset (collider or triggerbox) list
 * \param _c collider to add
 */
void CollisionManager::AddColliderToManager(std::shared_ptr<Collider> _c)
{
	// add it to the appropriate subset
	if (_c->GetTriggerStatus() == true)
		markedAsTriggerboxes_.push_back(_c);
	else
		markedAsColliders_.push_back(_c);
}

void CollisionManager::CheckTriggerCollisions()
{
	for (auto& t : markedAsTriggerboxes_)
	{
		DirectX::BoundingOrientedBox tOBB = t.get()->GetOrientedBoundingBox();
		for (auto& c : markedAsColliders_)
		{
			BoundingOrientedBox cOBB = c->GetOrientedBoundingBox();
			if (tOBB.Intersects(cOBB))
			{
#if defined(DEBUG) || defined(_DEBUG)
				printf("Colliding\n");
#endif
			}
			if (cOBB.Intersects(tOBB))
			{
#if defined(DEBUG) || defined(_DEBUG)
				printf("Colliding2\n");
#endif
			}
		}
	}
}

void CollisionManager::CheckColliderCollisions()
{
}
