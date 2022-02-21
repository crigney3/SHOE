#include "..\Headers\Collider.h"

#include "..\Headers\GameEntity.h"

Collider::Collider(GameEntity& _owner)
    :
    extentsXYZ_(DirectX::XMFLOAT3(1, 1, 1)),
    isEnabled_(true),
    isTrigger_(false),
    isVisible_(false)
{
    owner_ = std::make_shared<GameEntity>(_owner);
}

#pragma region Getters/Setters
std::shared_ptr<GameEntity> Collider::GetOwner()
{
    return owner_;
}

void Collider::SetOwner(std::shared_ptr<GameEntity> _newOwner)
{
    owner_ = _newOwner;
}

std::shared_ptr<Transform> Collider::GetTransform()
{
    return transform_;
}

void Collider::SetTransform(std::shared_ptr<Transform> _newTransform)
{
    transform_ = _newTransform;
}

DirectX::XMFLOAT3 Collider::GetExtents()
{
    return extentsXYZ_;
}

void Collider::SetExtents(DirectX::XMFLOAT3 _newExtents)
{
    extentsXYZ_ = _newExtents;
}

void Collider::SetXExtent(float _x)
{
    extentsXYZ_ = DirectX::XMFLOAT3(_x, extentsXYZ_.y, extentsXYZ_.z);
}

void Collider::SetYExtent(float _y)
{
    extentsXYZ_ = DirectX::XMFLOAT3(extentsXYZ_.x, _y, extentsXYZ_.z);
}

void Collider::SetZExtent(float _z)
{
    extentsXYZ_ = DirectX::XMFLOAT3(extentsXYZ_.x, extentsXYZ_.y, _z);
}

void Collider::SetWidth(float _width)
{
    extentsXYZ_ = DirectX::XMFLOAT3(_width/2.0f, extentsXYZ_.y, extentsXYZ_.z);
}

void Collider::SetHeight(float _height)
{
    extentsXYZ_ = DirectX::XMFLOAT3(extentsXYZ_.x, _height/2, extentsXYZ_.z);
}

void Collider::SetDepth(float _depth)
{
    extentsXYZ_ = DirectX::XMFLOAT3(extentsXYZ_.x, extentsXYZ_.y, _depth/2);
}

void Collider::SetTriggerStatus(bool _isTrigger)
{
    isTrigger_ = _isTrigger;
}

void Collider::SetVisibilityStatus(bool _isVisible)
{
    isVisible_ = _isVisible;
}

void Collider::SetEnabledStatus(bool _isEnabled)
{
    isEnabled_ = _isEnabled;
}
#pragma endregion

void Collider::OnTriggerEnter(GameEntity& _other)
{

}

void Collider::OnCollisionEnter(GameEntity& _other)
{
	
}


