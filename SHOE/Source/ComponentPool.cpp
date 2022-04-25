#include "../Headers/ComponentPool.h"
#include "../Headers/AssetManager.h"

/// <summary>
/// Binds an unallocated MeshRenderer from the pool to a GameEntity, then sorts the list by material
/// </summary>
/// <param name="gameEntity">The GameEntity the component is to be attached to</param>
/// <param name="hierarchyIsEnabled">Whether the GameEntity to be attached to is enabled</param>
/// <returns>A reference to the newly bound component</returns>
template<> std::shared_ptr<MeshRenderer> ComponentPool<MeshRenderer>::Instantiate(std::shared_ptr<GameEntity> gameEntity)
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
	component->Bind(gameEntity);
	//Sorts the meshes
	std::sort(allocated.begin(), allocated.end(), [](std::shared_ptr<MeshRenderer> a, std::shared_ptr<MeshRenderer> b) {
		if (a->GetMaterial()->GetTransparent() != b->GetMaterial()->GetTransparent())
			return b->GetMaterial()->GetTransparent();
		return a->GetMaterial() < b->GetMaterial();
		});
	AssetManager::materialSortDirty = false;
	return component;
}