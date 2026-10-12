// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <math.h>


Camera::Camera()
{
	SetView( glm::vec3(0.0f,0.0f,0.0f), glm::vec3(0.0f,0.0f,-1.0f), glm::vec3(0.0f,1.0f,0.0f));
}

Camera::~Camera()
{
}

void Camera::SetView(glm::vec3 _eye, glm::vec3 _at, glm::vec3 _worldUp)
{
	m_eye	   = _eye;
	m_at	   = _at;
	m_worldUp  = _worldUp;

	m_viewMatrix = glm::lookAt( m_eye, m_at, m_worldUp );
	m_invViewMatrix = glm::inverse(m_viewMatrix);
}

void Camera::SetProj(float _angle, float _aspect, float _zn, float _zf)
{
	m_angle  = _angle;
	m_aspect = _aspect;
	m_zNear  = _zn;
	m_zFar   = _zf;

	m_projMatrix = glm::perspective( m_angle, m_aspect, m_zNear, m_zFar );
	m_invProjMatrix = glm::inverse(m_projMatrix);
}

void Camera::SetAngle( const float _angle ) noexcept
{
	m_angle = _angle;
	m_projMatrix = glm::perspective( m_angle, m_aspect, m_zNear, m_zFar );
	m_invProjMatrix = glm::inverse(m_projMatrix);
}

void Camera::SetAspect( const float _aspect ) noexcept
{
	m_aspect = _aspect;
	m_projMatrix = glm::perspective( m_angle, m_aspect, m_zNear, m_zFar );
	m_invProjMatrix = glm::inverse(m_projMatrix);
}

void Camera::SetZNear( const float _zn ) noexcept
{
	m_zNear = _zn;
	m_projMatrix = glm::perspective( m_angle, m_aspect, m_zNear, m_zFar );
	m_invProjMatrix = glm::inverse(m_projMatrix);
}

void Camera::SetZFar( const float _zf ) noexcept
{
	m_zFar = _zf;
	m_projMatrix = glm::perspective( m_angle, m_aspect, m_zNear, m_zFar );
	m_invProjMatrix = glm::inverse(m_projMatrix);
}
