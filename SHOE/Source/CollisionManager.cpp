#include "..\Headers\CollisionManager.h"

using namespace DirectX;

CollisionManager::CollisionManager()
{
}

void CollisionManager::CheckTriggerCollisions()
{
}

void CollisionManager::CheckColliderCollisions()
{
}

// check if there's a separating plane in between the selected axes
XMVECTOR CollisionManager::GetSeparatingPlane(const XMVECTOR& _rPos, const XMVECTOR& _plane, Collider& _a, Collider& _b)
{
	const XMFLOAT3 ax_f = _a.GetTransform()->GetXAxis();
	const XMVECTOR ax = XMLoadFloat3(&ax_f);
	const XMFLOAT3 ay_f = _a.GetTransform()->GetYAxis();
	const XMVECTOR ay = XMLoadFloat3(&ay_f);
	const XMFLOAT3 az_f = _a.GetTransform()->GetZAxis();
	const XMVECTOR az = XMLoadFloat3(&az_f);

	const XMFLOAT3 bx_f = _b.GetTransform()->GetXAxis();
	const XMVECTOR bx = XMLoadFloat3(&bx_f);
	const XMFLOAT3 by_f = _b.GetTransform()->GetYAxis();
	const XMVECTOR by = XMLoadFloat3(&by_f);
	const XMFLOAT3 bz_f = _b.GetTransform()->GetZAxis();
	const XMVECTOR bz = XMLoadFloat3(&bz_f);

    return XMVectorGreater(XMVectorAbs(XMVectorMultiply(_rPos, _plane)),
						(XMVectorAbs((ax * _a.GetExtents().x) * _plane) +
						XMVectorAbs((ay * _a.GetExtents().y) * _plane) +
						XMVectorAbs((az * _a.GetExtents().z) * _plane) +
						XMVectorAbs((bx * _b.GetExtents().x) * _plane) +
						XMVectorAbs((by * _b.GetExtents().y) * _plane) +
						XMVectorAbs((bz * _b.GetExtents().z) * _plane)));
}

bool CollisionManager::SAT(Collider& _a, Collider& _b)
{
	// Oriented Bounding Box collision detection summary:
	//		Two OBBs are separated if for some axis L the sum of their projected radii is less
	//		than the distance between their projected centers
	//		(from http://www.r-5.org/files/books/computers/algo-list/realtime-3d/Christer_Ericson-Real-Time_Collision_Detection-EN.pdf)

	float ra, rb;
	XMFLOAT3X3 r{}, absR{};

	Collider a = _a;
	XMFLOAT3 ac_f = a.GetTransform()->GetPosition();
	XMVECTOR ac = XMLoadFloat3(&ac_f);
	XMFLOAT3 ae_f = a.GetExtents();
	XMVECTOR ae = XMLoadFloat3(&ae_f);

	Collider b = _b;
	XMFLOAT3 bc_f = b.GetTransform()->GetPosition();
	XMVECTOR bc = XMLoadFloat3(&bc_f);
	XMFLOAT3 be_f = b.GetExtents();
	XMVECTOR be = XMLoadFloat3(&be_f);



	////R = B * A(inverse)
	////R is the matrix to get from B space to A space
	////compute rotation matrix expressing b in a's coordinate frame
	//for (int i = 0; i < 3; i++)
	//{
	//	for (int j = 0; j < 3; j++)
	//	{
	//		R[i][j] = glm::dot(m_m4ToWorld[i], b->m_m4ToWorld[j]);
	//	}
	//}

	//translation vector
	XMVECTOR t = bc - ac;
	XMFLOAT3 t_f{};
	XMStoreFloat3(&t_f, t);
	//bring t into a's coordinate frame
	//t = XMVECTOR(glm::dot(t, static_cast<vector3>(m_m4ToWorld[0])), glm::dot(t, static_cast<vector3>(m_m4ToWorld[1])), glm::dot(t, static_cast<vector3>(m_m4ToWorld[2])));

	//compute common subexpressions.
	//Add in Epsilon term to counteract math errors.
	//AKA to account for super super close numbers, treat them as equal
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			absR.m[i][j] = std::abs(r.m[i][j]) + FLT_EPSILON;
		}
	}

	//test axes L = A0, L = A1, L = A2
	for (int i = 0; i < 3; i++)
	{
		ra = XMVectorGetByIndex(ae, i);
		rb = be_f.x * absR.m[i][0] + be_f.y * absR.m[i][1] + be_f.z * absR.m[i][2];

		if (std::abs(XMVectorGetByIndex(t, i)) > ra + rb)
			return true; //SAT_AX + i;
	}

	//test axes L = B0, L = B1, L = B2
	for (int i = 0; i < 3; i++)
	{
		ra = ae_f.x * absR.m[0][i] + ae_f.y * absR.m[1][i] + ae_f.z * absR.m[2][i];
		rb = XMVectorGetByIndex(be, i);

		if (std::abs(t_f.x * r.m[0][i] + t_f.y * r.m[1][i] + t_f.z * r.m[2][i]) > ra + rb)
			return true; //SAT_BX + i;
	}

	//test axis L = A0 x B0
	ra = ae_f.y * absR.m[2][0] + ae_f.z * absR.m[1][0];
	rb = be_f.y * absR.m[0][2] + be_f.z * absR.m[0][1];
	
	if (std::abs(t_f.z * r.m[1][0] - t_f.y * r.m[2][0]) > ra + rb) 
		return true; //SAT_AXxBX;

	//test axis L = A0 x B1
	ra = ae_f.y * absR.m[2][1] + ae_f.z * absR.m[1][1];
	rb = be_f.x * absR.m[0][2] + be_f.z * absR.m[0][0];
	
	if (std::abs(t_f.z * r.m[1][1] - t_f.y * r.m[2][1]) > ra + rb)
		return true; //SAT_AXxBY;

	//test axis L = A0 x B2
	ra = ae_f.y * absR.m[2][2] + ae_f.z * absR.m[1][2];
	rb = be_f.x * absR.m[0][1] + be_f.y * absR.m[0][0];
	
	if (std::abs(t_f.z * r.m[1][2] - t_f.y * r.m[2][2]) > ra + rb)
		return true; //SAT_AXxBZ;

	//test axis L = A1 x B0
	ra = ae_f.x * absR.m[2][0] + ae_f.z * absR.m[0][0];
	rb = be_f.y * absR.m[1][2] + be_f.z * absR.m[1][1];
	
	if (std::abs(t_f.x * r.m[2][0] - t_f.z * r.m[0][0]) > ra + rb)
		return true; //SAT_AYxBX;

	//test axis L = A1 x B1
	ra = ae_f.x * absR.m[2][1] + ae_f.z * absR.m[0][1];
	rb = be_f.x * absR.m[1][2] + be_f.z * absR.m[1][0];
	
	if (std::abs(t_f.x * r.m[2][1] - t_f.z * r.m[0][1]) > ra + rb)
		return true; //SAT_AYxBY;

	//test axis L = A1 x B2
	ra = ae_f.x * absR.m[2][2] + ae_f.z * absR.m[0][2];
	rb = be_f.x * absR.m[1][1] + be_f.y * absR.m[1][0];
	
	if (std::abs(t_f.x * r.m[2][2] - t_f.z * r.m[0][2]) > ra + rb)
		return true; //SAT_AYxBZ;

	//test axis L = A2 x B0
	ra = ae_f.x * absR.m[1][0] + ae_f.y * absR.m[0][0];
	rb = be_f.y * absR.m[2][2] + be_f.z * absR.m[2][0];
	
	if (std::abs(t_f.y * r.m[0][0] - t_f.x * r.m[1][1]) > ra + rb)
		return true; //SAT_AZxBX;

	//test axis L = A2 x B1
	ra = ae_f.x * absR.m[1][1] + ae_f.y * absR.m[0][1];
	rb = be_f.x * absR.m[2][2] + be_f.z * absR.m[2][0];
	
	if (std::abs(t_f.y * r.m[0][1] - t_f.x * r.m[1][1]) > ra + rb)
		return true; //SAT_AZxBY;

	//test axis L = A2 x B2
	ra = ae_f.x * absR.m[1][2] + ae_f.y * absR.m[0][2];
	rb = be_f.x * absR.m[2][1] + be_f.y * absR.m[2][0];
	
	if (std::abs(t_f.y * r.m[0][2] - t_f.x * r.m[1][2]) > ra + rb)
		return true; //SAT_AZxBZ;


	//there is no axis test that separates this two objects
	return false; // eSATResults::SAT_NONE;
}
