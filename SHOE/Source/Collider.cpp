#include "..\Headers\Collider.h"

#include "..\Headers\GameEntity.h"

using namespace DirectX;

Collider::Collider(std::shared_ptr<GameEntity> _owner)
    :
    isEnabled_(true),
    isTrigger_(false),
    isVisible_(false)
{
    owner_ = _owner;
    transform_ = owner_->GetTransform();

    //Need to make the OOB with a quaternion as XMFLOAT4
    XMFLOAT3 pyr = transform_->GetPitchYawRoll();
    XMMATRIX mtrx = DirectX::XMMatrixRotationRollPitchYaw(pyr.x, pyr.y, pyr.z);
    XMVECTOR quat = XMQuaternionRotationMatrix(mtrx);
    XMFLOAT4 quatF;
    XMStoreFloat4(&quatF, quat);
    obb_ = BoundingOrientedBox(transform_->GetPosition(), transform_->GetScale(), quatF);
}

#pragma region Getters/Setters
std::shared_ptr<GameEntity> Collider::GetOwner()                { return owner_; }
void Collider::SetOwner(std::shared_ptr<GameEntity> _newOwner)  { owner_ = _newOwner; }

//TODO: if we want colliders to be independent, should we store a transform_ member within the class?
std::shared_ptr<Transform> Collider::GetTransform() { return transform_; }

void Collider::SetPosition(DirectX::XMFLOAT3 _newPos) { obb_.Center = _newPos; }

BoundingOrientedBox Collider::GetOrientedBoundingBox() { return obb_; }

XMFLOAT3 Collider::GetExtents() { return obb_.Extents; }
void Collider::SetExtents(DirectX::XMFLOAT3 _newExtents) { obb_.Extents = _newExtents; }
void Collider::SetXExtent(float _x)     { obb_.Extents.x = _x; }
void Collider::SetYExtent(float _y)     { obb_.Extents.x = _y; }
void Collider::SetZExtent(float _z)     { obb_.Extents.x = _z; }

void Collider::SetWidth(float _width)   { obb_.Extents.x = _width / 2.0f; }
void Collider::SetHeight(float _height) { obb_.Extents.y = _height / 2.0f; }
void Collider::SetDepth(float _depth)   { obb_.Extents.x = _depth / 2.0f; }

bool Collider::GetTriggerStatus()       { return isTrigger_; }
bool Collider::GetVisibilityStatus()    { return isVisible_; }
bool Collider::GetEnabledStatus()       { return isEnabled_; }
void Collider::SetTriggerStatus(bool _isTrigger)    { isTrigger_ = _isTrigger; }
void Collider::SetVisibilityStatus(bool _isVisible) { isVisible_ = _isVisible; }
void Collider::SetEnabledStatus(bool _isEnabled)    { isEnabled_ = _isEnabled; }
#pragma endregion

void Collider::OnTriggerEnter(GameEntity& _other)
{

}

void Collider::OnCollisionEnter(GameEntity& _other)
{
	
}


