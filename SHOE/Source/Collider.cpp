#include "..\Headers\Collider.h"
#include "..\Headers\GameEntity.h"
#include "../Headers/CollisionManager.h"

using namespace DirectX;

void Collider::Start()
{
    isTrigger_ = false;
    isVisible_ = true;

    offset = ComponentManager::Instantiate<Transform>(nullptr);
    offset->SetParentNoReciprocate(GetTransform());

    obb_ = BoundingOrientedBox();
    RegenerateBoundingBox();
}

void Collider::OnDestroy() {
    offset->OnDestroy();
    ComponentManager::Free<Transform>(offset);
}

void Collider::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s entered collision with %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

void Collider::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s entered trigger area %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

void Collider::InCollision(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s is in collision with %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

void Collider::InTrigger(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s is in trigger area %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

void Collider::OnCollisionExit(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s exited collision with %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

void Collider::OnTriggerExit(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s exited trigger area %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

void Collider::OnTransform()
{
    offset->MarkMatricesDirty();
    RegenerateBoundingBox();
}

void Collider::OnParentTransform(std::shared_ptr<GameEntity> parent)
{
    offset->MarkMatricesDirty();
    RegenerateBoundingBox();
}

#pragma region Getters/Setters

BoundingOrientedBox Collider::GetOrientedBoundingBox() { return obb_; }

DirectX::XMFLOAT3 Collider::GetPositionOffset()
{
    return offset->GetLocalPosition();
}

void Collider::SetPositionOffset(DirectX::XMFLOAT3 posOffset)
{
    offset->SetPosition(posOffset);
    obb_.Center = offset->GetGlobalPosition();
}

DirectX::XMFLOAT3 Collider::GetRotationOffset()
{
    return offset->GetLocalPitchYawRoll();
}

void Collider::SetRotationOffset(DirectX::XMFLOAT3 rotOffset)
{
    offset->SetRotation(rotOffset);
    RegenerateBoundingBox();
}

DirectX::XMFLOAT3 Collider::GetScale()
{
    return offset->GetLocalScale();
}

void Collider::SetScale(DirectX::XMFLOAT3 scale)
{
    offset->SetScale(scale);
    obb_.Orientation = offset->GetGlobalRotation();
}

DirectX::XMFLOAT4X4 Collider::GetWorldMatrix()
{
    return offset->GetWorldMatrix();
}

bool Collider::IsTrigger()       { return isTrigger_; }
bool Collider::IsVisible()    { return isVisible_; }

/// <summary>
/// Sets whether this is a collider or a trigger box.
/// </summary>
/// <param name="_isTrigger">0/false is Collider, 1/true is TriggerBox</param>
void Collider::SetIsTrigger(bool _isTrigger)
{
	isTrigger_ = _isTrigger;
}

void Collider::SetVisible(bool _isVisible) { isVisible_ = _isVisible; }

void Collider::RegenerateBoundingBox()
{
    obb_.Center = offset->GetGlobalPosition();
    // Remember Extents are a radius but a scale is like a diameter
    XMFLOAT3 halfWidth = offset->GetGlobalScale();
    obb_.Extents = XMFLOAT3(halfWidth.x / 2, halfWidth.y / 2, halfWidth.z / 2);
    obb_.Orientation = offset->GetGlobalRotation();
}

#pragma endregion