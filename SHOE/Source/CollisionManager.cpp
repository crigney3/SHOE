#include "..\Headers\CollisionManager.h"

#include "../Headers/GameEntity.h"
#include "..\Headers\ComponentManager.h"

using namespace DirectX;

// Singleton requirement
CollisionManager* CollisionManager::instance;

CollisionManager::CollisionManager()
{
	activeCollisions = std::vector<Collision>();
	activeTriggers = std::vector<Collision>();
	lastFrameCollisions = std::vector<Collision>();
	lastFrameTriggers = std::vector<Collision>();
}

CollisionManager::~CollisionManager()
{
	activeCollisions.clear();
	activeTriggers.clear();
	lastFrameCollisions.clear();
	lastFrameTriggers.clear();
}

void CollisionManager::Update()
{
	std::vector<std::shared_ptr<Collider>> c = ComponentManager::GetAll<Collider>();
	for (int i = 0; i < c.size() - 1; i++)
	{
		if (!c[i]->IsEnabled()) continue;
		std::shared_ptr<Collider> a = c[i];
		for (int j = i + 1; j < c.size(); j++)
		{
			if (!c[j]->IsEnabled() || (c[i]->IsTrigger() && c[j]->IsTrigger())) continue;
			std::shared_ptr<Collider> b = c[j];
	
			if (a->GetOrientedBoundingBox().Intersects(b->GetOrientedBoundingBox())) {
				//Collision
				if (!a->IsTrigger() && !b->IsTrigger())
				{
					RegisterColliderCollision(a, b);
				}
				//Triggers
				else 
				{
					RegisterTriggerCollision(a, b);
				}
			}
		}
	}

	//Signals the end of previously registered collisions that weren't triggered this frame
	for (int i = 0; i < lastFrameCollisions.size(); i++) {
		lastFrameCollisions[i].a->GetGameEntity()->PropagateEvent(EntityEventType::OnCollisionExit, lastFrameCollisions[i].b->GetGameEntity());
		lastFrameCollisions[i].b->GetGameEntity()->PropagateEvent(EntityEventType::OnCollisionExit, lastFrameCollisions[i].a->GetGameEntity());
	}
	for (int i = 0; i < lastFrameTriggers.size(); i++) {
		lastFrameTriggers[i].a->GetGameEntity()->PropagateEvent(EntityEventType::OnTriggerExit, lastFrameTriggers[i].b->GetGameEntity());
		lastFrameTriggers[i].b->GetGameEntity()->PropagateEvent(EntityEventType::OnTriggerExit, lastFrameTriggers[i].a->GetGameEntity());
	}

	lastFrameCollisions.clear();
	lastFrameTriggers.clear();
	lastFrameCollisions.swap(activeCollisions);
	lastFrameTriggers.swap(activeTriggers);
}

void CollisionManager::RegisterColliderCollision(std::shared_ptr<Collider> a, std::shared_ptr<Collider> b)
{
	Collision newCollision{ a, b };
	auto& collisionPos = std::find(lastFrameCollisions.begin(), lastFrameCollisions.end(), newCollision);
	//If this collision has already been logged
	if (collisionPos != lastFrameCollisions.end()) {
		a->GetGameEntity()->PropagateEvent(EntityEventType::InCollision, b->GetGameEntity());
		b->GetGameEntity()->PropagateEvent(EntityEventType::InCollision, a->GetGameEntity());
		lastFrameCollisions.erase(collisionPos);
	}
	else {
		//It's a new collision
		a->GetGameEntity()->PropagateEvent(EntityEventType::OnCollisionEnter, b->GetGameEntity());
		b->GetGameEntity()->PropagateEvent(EntityEventType::OnCollisionEnter, a->GetGameEntity());
	}
	activeCollisions.push_back(newCollision);
}

void CollisionManager::RegisterTriggerCollision(std::shared_ptr<Collider> a, std::shared_ptr<Collider> b)
{
	Collision newTrigger{ a, b };
	auto& triggerPos = std::find(lastFrameTriggers.begin(), lastFrameTriggers.end(), newTrigger);
	//If this collision has already been logged
	if (triggerPos != lastFrameTriggers.end()) {
		a->GetGameEntity()->PropagateEvent(EntityEventType::InTrigger, b->GetGameEntity());
		b->GetGameEntity()->PropagateEvent(EntityEventType::InTrigger, a->GetGameEntity());
		lastFrameTriggers.erase(triggerPos);
	}
	else {
		//It's a new collision
		a->GetGameEntity()->PropagateEvent(EntityEventType::OnTriggerEnter, b->GetGameEntity());
		b->GetGameEntity()->PropagateEvent(EntityEventType::OnTriggerEnter, a->GetGameEntity());
	}
	activeTriggers.push_back(newTrigger);
}
