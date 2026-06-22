#pragma once

// NOTE: Parent-child hierarchy is partially implemented.
// SetParent() sets the parent pointer and GetWorldMatrix() recursively
// computes the full chain, but the following is NOT yet handled:
//   - No child registry: parents don't know their children
//   - No dangling pointer protection: if a parent Transform is destroyed,
//     children will hold a dangling pointer
//   - SceneManager is unaware of parent-child relationships and may
//     delete a parent without notifying its children
// Full implementation requires a bidirectional hierarchy with
// AddChild() / RemoveChild() and lifecycle management.

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
public:
    Transform() = default;
    explicit Transform(const glm::vec3& location,
        const glm::quat& rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f),
        const glm::vec3& scale = glm::vec3(1.0f));

    // ── Getters / Setters (inline) ────────────────────────────────────────────

    inline const glm::vec3& GetLocation() const { return m_location; }
    inline void SetLocation(const glm::vec3& location) {
        m_location = location;
        m_dirty = true;
    }

    inline const glm::quat& GetRotation() const { return m_rotation; }
    inline void SetRotation(const glm::quat& rotation) {
        m_rotation = rotation;
        m_dirty = true;
    }

    inline const glm::vec3& GetScale() const { return m_scale; }
    inline void SetScale(const glm::vec3& scale) {
        m_scale = scale;
        m_dirty = true;
    }

    inline Transform* GetParent() const { return m_parent; }

    // WARNING: Parent-child hierarchy is not fully implemented.
    // Setting a parent does not register this transform as a child,
    // and there is no protection against dangling pointers if the
    // parent is destroyed. Use with caution.
    inline void SetParent(Transform* parent) {
#pragma message("WARNING: Transform::SetParent() - parent-child hierarchy is not fully implemented. See Transform.h for details.")
        m_parent = parent;
        m_dirty = true;
    }

    inline bool IsDirty() const { return m_dirty; }

    // ── Matrix computation ────────────────────────────────────────────────────

    // Returns the local transformation matrix.
    // Recomputes and caches only when dirty, otherwise returns the cached value.
    const glm::mat4& GetMatrix() const;

    // Returns the world transformation matrix (parent chain applied recursively).
    // NOTE: The world matrix is not cached – recomputed on every call if a
    // parent is set. For deeply nested hierarchies this may be a bottleneck.
    glm::mat4 GetWorldMatrix() const;

private:
    glm::vec3  m_location = glm::vec3(0.0f);
    glm::quat  m_rotation = glm::identity<glm::quat>();
    glm::vec3  m_scale    = glm::vec3(1.0f);
    Transform* m_parent   = nullptr;

    mutable bool      m_dirty        = true;
    mutable glm::mat4 m_cachedMatrix = glm::identity<glm::mat4>();
};