#include "..\Headers\Collider.h"

#include "..\Headers\GameEntity.h"
#include "../Headers/CollisionManager.h"

using namespace DirectX;

Collider::Collider()
{

}

void Collider::Start()
{
    isEnabled_ = true;
    isTrigger_ = false;
    isVisible_ = false;

    owner_ = GetGameEntity();
    transform_ = owner_->GetTransform();

    //Need to make the OBB with a quaternion as XMFLOAT4
    XMFLOAT3 pyr = transform_->GetPitchYawRoll();
    XMMATRIX mtrx = DirectX::XMMatrixRotationRollPitchYaw(pyr.x, pyr.y, pyr.z);
    XMVECTOR quat = XMQuaternionRotationMatrix(mtrx);
    XMFLOAT4 quatF;
    XMStoreFloat4(&quatF, quat);

    // Remember Extents are a radius but a scale is like a diameter
    XMFLOAT3 halfWidth = transform_->GetScale();
    halfWidth = XMFLOAT3(halfWidth.x / 2, halfWidth.y / 2, halfWidth.z / 2);

    // Create an Oriented Bounding Box with info from the Transform
    obb_ = BoundingOrientedBox(transform_->GetPosition(), halfWidth, quatF);

    // Track it
    CollisionManager::AddColliderToManager(*this);
}


#pragma region Getters/Setters
std::shared_ptr<GameEntity> Collider::GetOwner()                { return owner_; }
void Collider::SetOwner(std::shared_ptr<GameEntity> _newOwner)  { owner_ = _newOwner; }

//TODO: if we want colliders to be independent, should we store a transform_ member within the class?
std::shared_ptr<Transform> Collider::GetTransform() { return transform_; }

void Collider::SetPosition(DirectX::XMFLOAT3 _newPos) { transform_->SetPosition(_newPos); }
BoundingOrientedBox Collider::GetOrientedBoundingBox() { return obb_; }

XMFLOAT3 Collider::GetExtents() { return transform_->GetScale(); }
void Collider::SetExtents(DirectX::XMFLOAT3 _newExtents) { transform_->SetScale(_newExtents); }
void Collider::SetXExtent(float _x)
{
    XMFLOAT3 old = transform_->GetScale();
    XMFLOAT3 newE = XMFLOAT3(_x, old.y, old.z);

    //transform_->SetScale(newE);
	obb_.Extents.x = _x;
}
void Collider::SetYExtent(float _y)
{
    XMFLOAT3 old = transform_->GetScale();
    XMFLOAT3 newE = XMFLOAT3(old.x, _y, old.z);

    //transform_->SetScale(newE);
    obb_.Extents.y = _y;
}
void Collider::SetZExtent(float _z)
{
    XMFLOAT3 old = transform_->GetScale();
    XMFLOAT3 newE = XMFLOAT3(old.x, old.y, _z);

    //transform_->SetScale(newE);
    obb_.Extents.z = _z;
}

//TODO: fix these to mess with the Transform not OBB
void Collider::SetWidth(float _width)   {} //{ obb_.Extents.x = _width / 2.0f; }
void Collider::SetHeight(float _height) {} //{ obb_.Extents.y = _height / 2.0f; }
void Collider::SetDepth(float _depth)   {} //{ obb_.Extents.x = _depth / 2.0f; }

bool Collider::GetTriggerStatus()       { return isTrigger_; }
bool Collider::GetVisibilityStatus()    { return isVisible_; }
bool Collider::GetEnabledStatus()       { return isEnabled_; }
void Collider::SetTriggerStatus(bool _isTrigger)    { isTrigger_ = _isTrigger; }
void Collider::SetVisibilityStatus(bool _isVisible) { isVisible_ = _isVisible; }
void Collider::SetEnabledStatus(bool _isEnabled)    { isEnabled_ = _isEnabled; }
void Collider::OnCollisionEnter(std::shared_ptr<GameEntity> other)
{
}
void Collider::OnTriggerEnter(std::shared_ptr<GameEntity> other)
{
}
#pragma endregion

void Collider::Update()
{
    XMFLOAT4X4 wrld = transform_->GetWorldMatrix();
    
    // Make sure OBB sticks to Transform
    obb_.Center = transform_->GetPosition();
    // Remember Extents are a radius but a scale is like a diameter
    XMFLOAT3 halfWidth = transform_->GetScale();
    halfWidth = XMFLOAT3(halfWidth.x / 2, halfWidth.y / 2, halfWidth.z / 2);
    obb_.Extents = halfWidth;
    //Need to make the OBB with a quaternion as XMFLOAT4
    XMFLOAT3 pyr = transform_->GetPitchYawRoll();
    XMMATRIX mtrx = DirectX::XMMatrixRotationRollPitchYaw(pyr.x, pyr.y, pyr.z);
    XMVECTOR quat = XMQuaternionRotationMatrix(mtrx);
    XMFLOAT4 quatF;
    XMStoreFloat4(&quatF, quat);
    obb_.Orientation = quatF;
}
