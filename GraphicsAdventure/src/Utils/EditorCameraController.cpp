#include "EditorCameraController.h"

using namespace DirectX;
using namespace GDX11;
#include <iostream>

namespace GA::Utils
{
	EditorCameraController::EditorCameraController(Camera* camera, const DirectX::XMFLOAT3& focalPoint, float distance, float panSpeed, float rotationSpeed, float zoomSpeed)
	{
		Set(camera, focalPoint, distance, panSpeed, rotationSpeed, zoomSpeed);
	}

	void EditorCameraController::Set(Camera* camera, const DirectX::XMFLOAT3& focalPoint, float distance, float panSpeed, float rotationSpeed, float zoomSpeed)
	{
		m_camera = camera;
		m_focalPoint = focalPoint;
		m_distance = distance;
		m_panSpeed = panSpeed;
		m_rotationSpeed = rotationSpeed;
		m_zoomSpeed = zoomSpeed;
		m_mouseStartPos = { 0.0f, 0.0f };

		m_camera->SetPosition(CalculatePosition());
	}

	void EditorCameraController::SetCamera(Camera* camera)
	{
		m_camera = camera;
		m_camera->SetPosition(CalculatePosition());
	}

	void EditorCameraController::SetFocalPoint(const DirectX::XMFLOAT3& focalPoint)
	{
		m_focalPoint = focalPoint;
		m_camera->SetPosition(CalculatePosition());
	}

	void EditorCameraController::SetDistance(float distance)
	{
		m_distance = distance;
		m_camera->SetPosition(CalculatePosition());
	}

	void EditorCameraController::SetPanSpeed(float panSpeed)
	{
		m_panSpeed = panSpeed;
	}

	void EditorCameraController::SetRotationSpeed(float rotSpeed)
	{
		m_rotationSpeed = rotSpeed;
	}

	void EditorCameraController::SetZoomSpeed(float zoomSpeed)
	{
		m_zoomSpeed = zoomSpeed;
	}

	void EditorCameraController::OnEvent(GDX11::Event& event, float deltaTime)
	{
		if (event.GetEventType() == EventType::MouseScrolled)
		{
			MouseScrollEvent& e = static_cast<MouseScrollEvent&>(event);
			Zoom(e.GetAxisY() * deltaTime);
		}
	}

	void EditorCameraController::ProcessInput(const GDX11::Window* window, float deltaTime)
	{
		if (Input::GetKey(window, Key::LeftMenu))
		{
			XMVECTOR curMousPos = XMLoadFloat2(&Input::GetMousePos(window));
			XMVECTOR deltaXM = (curMousPos - XMLoadFloat2(&m_mouseStartPos)) * deltaTime;
			XMStoreFloat2(&m_mouseStartPos, curMousPos);
			XMFLOAT2 delta;
			XMStoreFloat2(&delta, deltaXM);
			if (Input::GetKey(window, Key::Control) && Input::GetMouseButton(window, Mouse::LeftButton))
				Pan(delta);
			else if (Input::GetMouseButton(window, Mouse::LeftButton))
				Rotate(delta);
		}
		else
		{
			m_mouseStartPos = Input::GetMousePos(window);
		}
	}

	void EditorCameraController::Pan(const DirectX::XMFLOAT2& delta)
	{
		XMVECTOR focalPointXM = XMLoadFloat3(&m_focalPoint);
		focalPointXM -= m_camera->GetRightDirection() * delta.x * m_panSpeed;
		focalPointXM += m_camera->GetUpDirection() * delta.y * m_panSpeed;

		XMStoreFloat3(&m_focalPoint, focalPointXM);
		m_camera->SetPosition(CalculatePosition());
	}

	void EditorCameraController::Rotate(const DirectX::XMFLOAT2& delta)
	{
		XMFLOAT3 rot = m_camera->GetDesc().rotation;

		rot.x = std::clamp(rot.x + delta.y * m_rotationSpeed, -90.0f, 90.0f);
		rot.y += delta.x * m_rotationSpeed;

		m_camera->SetRotation(rot);
		m_camera->SetPosition(CalculatePosition());
	}

	void EditorCameraController::Zoom(float delta)
	{
		m_distance -= delta * m_zoomSpeed;
		if (m_distance < 1.0f)
		{
			XMVECTOR focalPointXM = XMLoadFloat3(&m_focalPoint);
			focalPointXM += m_camera->GetForwardDirection();
			XMStoreFloat3(&m_focalPoint, focalPointXM);
		}

		m_camera->SetPosition(CalculatePosition());
	}

	DirectX::XMFLOAT3 EditorCameraController::CalculatePosition()
	{
		XMVECTOR posXM = XMLoadFloat3(&m_focalPoint) - m_camera->GetForwardDirection() * m_distance;
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, posXM);
		return pos;
	}
}

