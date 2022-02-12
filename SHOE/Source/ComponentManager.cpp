#include "../Headers/ComponentManager.h"

void ComponentManager::Initialize()
{
	poolDeallocateCalls = std::vector<std::function<void()>>();
}

void ComponentManager::DumpAll()
{
	for (auto& deallocate : poolDeallocateCalls) {
		deallocate();
	}
}
