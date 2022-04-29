#pragma once

#include "Collider.h"
#include <vector>

struct Collision {
	std::shared_ptr<Collider> a;
	std::shared_ptr<Collider> b;
	friend bool operator==(const Collision& lhs, const Collision& rhs) { 
		return lhs.a == rhs.a && lhs.b == lhs.b || lhs.a == rhs.b && lhs.b == lhs.a;
	}
};

class CollisionManager
{
#pragma region Singleton
public:
	// Gets the one and only instance of this class
	static CollisionManager& GetInstance()
	{
		if (!instance)
		{
			instance = new CollisionManager();
		}

		return *instance;
	}

	// Remove these functions (C++ 11 version)
	CollisionManager(CollisionManager const&) = delete;
	void operator=(CollisionManager const&) = delete;

private:
	static CollisionManager* instance;
	CollisionManager();
#pragma endregion
public:
	~CollisionManager();

	void Update();
private:
	void RegisterColliderCollision(std::shared_ptr<Collider> a, std::shared_ptr<Collider> b);
	void RegisterTriggerCollision(std::shared_ptr<Collider> collider, std::shared_ptr<Collider> trigger);

	std::vector<Collision> activeCollisions;
	std::vector<Collision> activeTriggers;
	std::vector<Collision> lastFrameCollisions;
	std::vector<Collision> lastFrameTriggers;
};
