#include "Transform.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

Transform::Transform()
    : m_location(0.0f)
    , m_rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
    , m_scale(1.0f)
    , m_parent(nullptr)
    , m_dirty(true)
{}

Transform::Transform(const glm::vec3& location,
    const glm::quat& rotation,
    const glm::vec3& scale)
    : m_location(location)
    , m_rotation(rotation)
    , m_scale(scale)
    , m_parent(nullptr)
    , m_dirty(true)
{}

glm::mat4 Transform::GetMatrix() const
{
    // Build local matrix in T * R * S order
    glm::mat4 T = glm::translate(glm::mat4(1.0f), m_location);
    glm::mat4 R = glm::mat4_cast(m_rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), m_scale);

    m_dirty = false;
    return T * R * S;
}

glm::mat4 Transform::GetWorldMatrix() const
{
    // NOTE: Parent-child hierarchy is partially implemented.
    // If a parent is set, the world matrix is computed recursively.
    // However, there is no protection against dangling pointers.
    if (m_parent != nullptr)
        return m_parent->GetWorldMatrix() * GetMatrix();

    return GetMatrix();
}