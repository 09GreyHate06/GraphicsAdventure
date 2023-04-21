#include "Camera.h"

using namespace DirectX;

namespace GA
{
	Camera::Camera(const CameraDesc& desc)
	{
		Set(desc);
	}

	Camera::Camera()
		: m_desc()
	{
		XMStoreFloat4x4(&m_view, XMMatrixIdentity());
		XMStoreFloat4x4(&m_projection, XMMatrixIdentity());
	}

	void Camera::Set(const CameraDesc& desc)
	{
		m_desc = desc;
		UpdateViewMatrix();
		UpdateProjectionMatrix();
	}

	void Camera::SetFov(float fov)
	{
		m_desc.fov = fov;
		UpdateProjectionMatrix();
	}

	void Camera::SetAspect(float aspect)
	{
		m_desc.aspect = aspect;
		UpdateProjectionMatrix();
	}

	void Camera::SetNearZ(float nearZ)
	{
		m_desc.nearZ = nearZ;
		UpdateProjectionMatrix();
	}

	void Camera::SetFarZ(float farZ)
	{
		m_desc.farZ = farZ;
		UpdateProjectionMatrix();
	}

	void Camera::SetPosition(const DirectX::XMFLOAT3& pos)
	{
		m_desc.position = pos;
		UpdateViewMatrix();
	}

	void Camera::SetRotation(const DirectX::XMFLOAT3& rot)
	{
		m_desc.rotation = rot;
		UpdateViewMatrix();
	}

	DirectX::XMMATRIX Camera::GetViewMatrix() const
	{
		return XMLoadFloat4x4(&m_view);
	}

	DirectX::XMMATRIX Camera::GetProjectionMatrix() const
	{
		return XMLoadFloat4x4(&m_projection);
	}

	DirectX::XMVECTOR Camera::GetUpDirection() const
	{
		return XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), GetOrientation());
	}

	DirectX::XMVECTOR Camera::GetRightDirection() const
	{
		return XMVector3Rotate(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), GetOrientation());
	}

	DirectX::XMVECTOR Camera::GetForwardDirection() const
	{
		return XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), GetOrientation());
	}

	DirectX::XMVECTOR Camera::GetOrientation() const
	{
		return XMQuaternionRotationRollPitchYaw(XMConvertToRadians(m_desc.rotation.x), XMConvertToRadians(m_desc.rotation.y), XMConvertToRadians(m_desc.rotation.z));
	}

	void Camera::UpdateViewMatrix()
	{
		XMMATRIX viewXM = XMMatrixRotationQuaternion(GetOrientation()) * XMMatrixTranslation(m_desc.position.x, m_desc.position.y, m_desc.position.z);
		XMStoreFloat4x4(&m_view, XMMatrixInverse(nullptr, viewXM));
	}

	void Camera::UpdateProjectionMatrix()
	{
		//XMMATRIX projectionXM = XMMatrixOrthographicLH(1.0f, 720.0f / 1280.0f, 0.1f, 100.0f);
		XMMATRIX projectionXM = XMMatrixPerspectiveFovLH(XMConvertToRadians(m_desc.fov), m_desc.aspect, m_desc.nearZ, m_desc.farZ);
		XMStoreFloat4x4(&m_projection, projectionXM);
	}
}

