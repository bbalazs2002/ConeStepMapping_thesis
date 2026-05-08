#include "CameraManipulator.h"

#include "Camera.h"

#include <SDL3/SDL.h>

CameraManipulator::CameraManipulator()
{
}

CameraManipulator::~CameraManipulator()
{
}

void CameraManipulator::SetCamera( Camera* _pCamera )
{
    m_pCamera = _pCamera;

    if ( !m_pCamera ) return;

    // Set the initial spherical coordinates.
    m_center = m_pCamera->GetAt();
    glm::vec3 ToAim = m_center - m_pCamera->GetEye();

    m_distance = glm::length( ToAim );

    m_u = atan2f( ToAim.z, ToAim.x );
    m_v = acosf( ToAim.y / m_distance );

}

void CameraManipulator::Update(float _deltaTime) {
    if (!m_pCamera) return;

    // 1. Calculate view direction from spherical coordinates
    glm::vec3 lookDirection(
        cosf(m_u) * sinf(m_v),
        cosf(m_v),
        sinf(m_u) * sinf(m_v)
    );

    // 2. Define the camera's local coordinate system
    // In UE5-style flight, 'forward' is the look direction itself
    glm::vec3 forward = lookDirection;
    glm::vec3 worldUp = m_pCamera->GetWorldUp();
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));

    // 3. Determine final movement speed
    // Combine base speed with the scroll-wheel multiplier and frame delta
    float finalSpeed = m_baseSpeed * m_speedMultiplier * _deltaTime;

    // Apply Shift boost if active
    if (m_isShiftDown) {
        finalSpeed *= 4.0f;
    }

    // 4. Calculate position offset
    // W/S: forward/backward, A/D: strafe, Q/E: vertical lift
    glm::vec3 deltaPosition = (
        static_cast<float>(m_goForward) * forward +
        static_cast<float>(m_goRight) * right +
        static_cast<float>(m_goUp) * worldUp
        ) * finalSpeed;

    // 5. Update positions
    // Move both the focal point (center) and the eye to maintain relative distance
    m_center += deltaPosition;
    glm::vec3 eye = m_center - m_distance * lookDirection;

    // 6. Finalize view matrix
    m_pCamera->SetView(eye, m_center, worldUp);
}

void CameraManipulator::KeyboardDown(const SDL_KeyboardEvent& key) {
    // SDL3-ban a key.key közvetlenül elérhető
    switch (key.key) {
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            if (!key.repeat) {
                m_isShiftDown = true;
                m_baseSpeed /= 4.0f;
            }
            break;
        case SDLK_W: m_goForward = 1;  break;
        case SDLK_S: m_goForward = -1; break;
        case SDLK_A: m_goRight = -1;   break;
        case SDLK_D: m_goRight = 1;    break;
        case SDLK_E: m_goUp = 1;       break;
        case SDLK_Q: m_goUp = -1;      break;
    }
}

void CameraManipulator::KeyboardUp(const SDL_KeyboardEvent& key) {
    switch (key.key) {
        case SDLK_LSHIFT:
        case SDLK_RSHIFT:
            m_isShiftDown = false;
            m_baseSpeed *= 4.0f;
            break;
        case SDLK_W: if (m_goForward > 0) m_goForward = 0; break;
        case SDLK_S: if (m_goForward < 0) m_goForward = 0; break;
        case SDLK_A: if (m_goRight < 0)   m_goRight = 0;   break;
        case SDLK_D: if (m_goRight > 0)   m_goRight = 0;   break;
        case SDLK_Q: if (m_goUp < 0)      m_goUp = 0;      break;
        case SDLK_E: if (m_goUp > 0)      m_goUp = 0;      break;
    }
}

void CameraManipulator::MouseMove(const SDL_MouseMotionEvent& mouse) {
    // UE5-ben a jobb gombbal forgatjuk a nézetet (FPS-szerűen)
    if (mouse.state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) {
        float sensitivity = 0.005f; // Érzékenység
        m_u += mouse.xrel * sensitivity;
        m_v = glm::clamp<float>(m_v + mouse.yrel * sensitivity, 0.1f, 3.1f);
    }

    // A bal gomb az Unrealben előre-hátra tol és forgat, 
    // de a legegyszerűbb UE5 élményhez a jobb gombos forgás az alap.
}

void CameraManipulator::MouseWheel(const SDL_MouseWheelEvent& wheel) {
    // Scale exponentially
    if (m_isShiftDown) {
        if (wheel.y > 0) m_speedMultiplier *= 1.1f;
        else if (wheel.y < 0) m_speedMultiplier /= 1.1f;
    }
    else {
        float dDistance = static_cast<float>(wheel.y) * m_baseSpeed * m_speedMultiplier / -100.0f;
        m_distance += dDistance;
    }

    // Set minimum and maximum value
    m_speedMultiplier = glm::clamp(m_speedMultiplier, 0.1f, 50.0f);
}