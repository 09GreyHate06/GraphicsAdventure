#pragma once
#include <GDX11.h>
#include <DirectXMath.h>
#include "Scene/Camera.h"

namespace GA::Utils
{
	class EditorCameraController
	{
	public:
		EditorCameraController(Camera* camera, const DirectX::XMFLOAT3& focalPoint, float distance, float panSpeed, float rotationSpeed, float zoomSpeed);
		EditorCameraController() = default;

		void Set(Camera* camera, const DirectX::XMFLOAT3& focalPoint, float distance, float panSpeed, float rotationSpeed, float zoomSpeed);
		void SetCamera(Camera* camera);
		void SetFocalPoint(const DirectX::XMFLOAT3& focalPoint);
		void SetDistance(float distance);
		void SetPanSpeed(float panSpeed);
		void SetRotationSpeed(float rotSpeed);
		void SetZoomSpeed(float zoomSpeed);

		void OnEvent(GDX11::Event& event, float deltaTime);
		void ProcessInput(const GDX11::Window* window, float deltaTime);

	private:
		void Pan(const DirectX::XMFLOAT2& delta);
		void Rotate(const DirectX::XMFLOAT2& delta);
		void Zoom(float delta);

		DirectX::XMFLOAT3 CalculatePosition();

		Camera* m_camera;
		DirectX::XMFLOAT3 m_focalPoint;
		float m_distance;
		float m_panSpeed;
		float m_rotationSpeed;
		float m_zoomSpeed;

		DirectX::XMFLOAT2 m_mouseStartPos;
	};
}