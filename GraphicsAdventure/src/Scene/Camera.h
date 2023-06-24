#pragma once
#include <DirectXMath.h>

namespace GA
{
	struct CameraDesc
	{
		float fov;
		float aspect;
		float nearZ;
		float farZ;

		DirectX::XMFLOAT3 rotation;
		DirectX::XMFLOAT3 position;
	};

	class Camera
	{
	public:
		Camera(const CameraDesc& desc);
		Camera();
		virtual ~Camera() = default;

		void Set(const CameraDesc& desc);

		void SetFov(float fov);
		void SetAspect(float aspect);
		void SetNearZ(float nearZ);
		void SetFarZ(float farZ);
		void SetPosition(const DirectX::XMFLOAT3& pos);
		void SetRotation(const DirectX::XMFLOAT3& rot);

		const CameraDesc& GetDesc() const { return m_desc; }

		DirectX::XMMATRIX GetViewMatrix() const;
		DirectX::XMMATRIX GetProjectionMatrix() const;
		DirectX::XMVECTOR GetUpDirection() const;
		DirectX::XMVECTOR GetRightDirection() const;
		DirectX::XMVECTOR GetForwardDirection() const;
		DirectX::XMVECTOR GetOrientation() const;

	protected:
		void UpdateViewMatrix();
		void UpdateProjectionMatrix();

		CameraDesc m_desc;
		DirectX::XMFLOAT4X4 m_view;
		DirectX::XMFLOAT4X4 m_projection;
	};
}