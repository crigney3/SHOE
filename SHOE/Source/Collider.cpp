#include "..\Headers\Collider.h"
#include "..\Headers\GameEntity.h"
#include "../Headers/CollisionManager.h"

using namespace DirectX;

void Collider::OnDestroy() {
}

void Collider::Start()
{
    isTrigger_ = false;
    isVisible_ = true;
    isTransformVisible_ = true;

    //Need to make the OBB with a quaternion as XMFLOAT4
    XMFLOAT3 pyr = GetTransform()->GetLocalPitchYawRoll();
    XMMATRIX mtrx = DirectX::XMMatrixRotationRollPitchYaw(pyr.x, pyr.y, pyr.z);
    XMVECTOR quat = XMQuaternionRotationMatrix(mtrx);
    XMFLOAT4 quatF;
    XMStoreFloat4(&quatF, quat);

    // Remember Extents are a radius but a scale is like a diameter
    XMFLOAT3 halfWidth = GetTransform()->GetLocalScale();
    halfWidth = XMFLOAT3(halfWidth.x / 2, halfWidth.y / 2, halfWidth.z / 2);

    // Create an Oriented Bounding Box with info from the Transform
    obb_ = BoundingOrientedBox(GetTransform()->GetGlobalPosition(), halfWidth, quatF);

    // Track it
    CollisionManager::AddColliderToManager(shared_from_this());
}


#pragma region Getters/Setters

BoundingOrientedBox Collider::GetOrientedBoundingBox() { return obb_; }

XMFLOAT3 Collider::GetExtents() { return GetTransform()->GetLocalScale(); }
void Collider::SetExtents(DirectX::XMFLOAT3 _newExtents) { obb_.Extents = _newExtents; }
void Collider::SetXExtent(float _x) { obb_.Extents.x = _x;}
void Collider::SetYExtent(float _y) { obb_.Extents.y = _y; }
void Collider::SetZExtent(float _z) { obb_.Extents.z = _z; }

//TODO: fix these to mess with the Transform not OBB
void Collider::SetWidth(float _width)   { obb_.Extents.x = _width / 2.0f; }
void Collider::SetHeight(float _height) { obb_.Extents.y = _height / 2.0f; }
void Collider::SetDepth(float _depth)   { obb_.Extents.x = _depth / 2.0f; }

/// <summary>
/// Gets whether this is a collider or a trigger box.
/// </summary>
/// <returns>0/false is Collider, 1/true is TriggerBox</returns>
bool Collider::GetTriggerStatus()       { return isTrigger_; }
bool Collider::GetVisibilityStatus()    { return isVisible_; }
bool Collider::GetTransformVisibilityStatus()    { return isTransformVisible_; }

/// <summary>
/// Sets whether this is a collider or a trigger box.
/// </summary>
/// <param name="_isTrigger">0/false is Collider, 1/true is TriggerBox</param>
void Collider::SetTriggerStatus(bool _isTrigger)
{
	isTrigger_ = _isTrigger;

    for (int i = 0; i < ComponentManager::GetAll<Collider>().size(); i++)
    {
	    //CollisionManager::GetMarkedAsColliders()
    }
}
void Collider::SetVisibilityStatus(bool _isVisible) { isVisible_ = _isVisible; }
void Collider::SetTransformVisibilityStatus(bool _isTransformVisible) { isTransformVisible_ = _isTransformVisible; }

void Collider::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
}

void Collider::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
#if defined(DEBUG) || defined(_DEBUG)
    printf("\n%s Colliding with %s", GetGameEntity()->GetName().c_str(), other->GetName().c_str());
#endif
}

#pragma endregion

void Collider::Update()
{
	// Make sure OBB sticks to Transform
    obb_.Center = GetTransform()->GetGlobalPosition();

	obb_.Orientation = GetTransform()->GetGlobalRotation();
    return;
    // DON'T DO THIS. DO NOT DO THIS or the object will scale. NO
    // ----------------------------------------------------------------------
    // Remember Extents are a radius but a scale is like a diameter
    //XMFLOAT3 halfWidth = transform_->GetScale();
    //halfWidth = XMFLOAT3(halfWidth.x / 2, halfWidth.y / 2, halfWidth.z / 2);
    //obb_.Extents = halfWidth;
    //------------------------------------------------------------------------
}
