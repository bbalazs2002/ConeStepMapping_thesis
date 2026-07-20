#pragma once

#include <glm/glm.hpp>
#include "ICamera.h"

class Camera : public ICamera
{
public:
	Camera();

	~Camera();

	inline glm::vec3 GetEye()     const override { return m_eye; }
	inline glm::vec3 GetAt()      const override { return m_at; }
	inline glm::vec3 GetWorldUp() const { return m_worldUp; }

	inline glm::mat4 GetViewMatrix() const { return m_viewMatrix; }
	inline glm::mat4 GetInvView()    const { return m_invViewMatrix; }
	inline glm::mat4 GetProj()       const { return m_projMatrix; }
	inline glm::mat4 GetInvProj()    const { return m_invProjMatrix; }
	inline glm::mat4 GetViewProj()   const override { return m_projMatrix * m_viewMatrix; }

	void SetView(glm::vec3 _eye, glm::vec3 _at, glm::vec3 _up);

	inline float GetAngle()  const { return m_angle; }
	void SetAngle( const float _angle ) noexcept;
	inline float GetAspect() const { return m_aspect; }
	void SetAspect( const float _aspect ) noexcept override;
	inline float GetZNear()  const { return m_zNear; }
	void SetZNear( const float _zn ) noexcept;
	inline float GetZFar()   const { return m_zFar; }
	void SetZFar( const float _zf ) noexcept;

	void SetProj(float _angle, float _aspect, float _zn, float _zf);

private:

	glm::vec3	m_eye;
	glm::vec3	m_worldUp;
	glm::vec3	m_at;

	glm::mat4	m_viewMatrix;
	glm::mat4	m_invViewMatrix;

	float m_zNear =    0.01f;
	float m_zFar  = 1000.0f;

	float m_angle  = glm::radians( 27.0f );
	float m_aspect = 1.0f;

	glm::mat4	m_projMatrix;
	glm::mat4	m_invProjMatrix;
};
