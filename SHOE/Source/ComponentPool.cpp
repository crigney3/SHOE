#include "../Headers/ComponentPool.h"

template <>
std::shared_ptr<MeshRenderer> ComponentPool<MeshRenderer>::Instantiate(std::shared_ptr<GameEntity> gameEntity,
	bool hierarchyIsEnabled)
{
	//Allocates a new pool if there is no available components
	if (unallocated.size() == 0) {
		for (int i = 0; i < POOL_SIZE; i++) {
			unallocated.push(std::make_shared<MeshRenderer>());
		}
	}

	std::shared_ptr<MeshRenderer> component = unallocated.front();
	allocated.emplace_back(component);
	unallocated.pop();
	component->Bind(gameEntity, hierarchyIsEnabled);
	//Sorts the meshes
	std::sort(allocated.begin(), allocated.end(), [](std::shared_ptr<MeshRenderer> a, std::shared_ptr<MeshRenderer> b) {
		if (a->GetMaterial()->GetTransparent() != b->GetMaterial()->GetTransparent())
			return b->GetMaterial()->GetTransparent();
		return a->GetMaterial() < b->GetMaterial();
		});
	return component;
}