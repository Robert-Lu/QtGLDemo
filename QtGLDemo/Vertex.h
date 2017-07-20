#pragma once
/// From: http://www.trentreed.net/blog/qt5-opengl-part-1-basic-rendering/ *Altered*

#include <QVector3D>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

class Vertex3D
{
public:
    // Constructors
    Q_DECL_CONSTEXPR Vertex3D();
    Q_DECL_CONSTEXPR explicit Vertex3D(const QVector3D &position);
    Q_DECL_CONSTEXPR Vertex3D(const QVector3D &position, const QVector3D &color);
    Q_DECL_CONSTEXPR Vertex3D(const QVector3D &position, const QVector3D &color, const QVector3D &normal);
    Q_DECL_CONSTEXPR Vertex3D(const OpenMesh::Vec3f &position, 
        const OpenMesh::Vec3f &color,
        const OpenMesh::Vec3f &normal);

    // Accessors / Mutators
    Q_DECL_CONSTEXPR const QVector3D& position() const;
    Q_DECL_CONSTEXPR const QVector3D& color() const;
    Q_DECL_CONSTEXPR const QVector3D& normal() const;
    void setPosition(const QVector3D& position);
    void setColor(const QVector3D& color);
    void setNormal(const QVector3D& normal);

    // OpenGL Helpers
    static const int PositionTupleSize = 3;
    static const int ColorTupleSize = 3;
    static const int NormalTupleSize = 3;
    static Q_DECL_CONSTEXPR int positionOffset();
    static Q_DECL_CONSTEXPR int colorOffset();
    static Q_DECL_CONSTEXPR int normalOffset();
    static Q_DECL_CONSTEXPR int stride();

private:
    QVector3D m_position;
    QVector3D m_color;
    QVector3D m_normal;
};

/*******************************************************************************
* Inline Implementation
******************************************************************************/

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(Vertex3D, Q_MOVABLE_TYPE);

// Constructors
Q_DECL_CONSTEXPR inline Vertex3D::Vertex3D() {}
Q_DECL_CONSTEXPR inline Vertex3D::Vertex3D(const QVector3D &position) : m_position(position), m_color{ 1.0f, 1.0f, 1.0f } {}
Q_DECL_CONSTEXPR inline Vertex3D::Vertex3D(const QVector3D &position, const QVector3D &color) : m_position(position), m_color(color) {}
Q_DECL_CONSTEXPR inline Vertex3D::Vertex3D(const QVector3D &position, const QVector3D &color, const QVector3D &normal) : m_position(position), m_color(color), m_normal(normal) {}
Q_DECL_CONSTEXPR inline Vertex3D::Vertex3D(const OpenMesh::Vec3f& position, const OpenMesh::Vec3f& color, const OpenMesh::Vec3f& normal) 
    : m_position{ position[0], position[1], position[2] },
    m_color{ color[0], color[1], color[2] },
    m_normal{ normal[0], normal[1], normal[2] } {}

// Accessors / Mutators
Q_DECL_CONSTEXPR inline const QVector3D& Vertex3D::position() const { return m_position; }
Q_DECL_CONSTEXPR inline const QVector3D& Vertex3D::color() const { return m_color; }
Q_DECL_CONSTEXPR inline const QVector3D& Vertex3D::normal() const { return m_normal; }
void inline Vertex3D::setPosition(const QVector3D& position) { m_position = position; }
void inline Vertex3D::setColor(const QVector3D& color) { m_color = color; }
void inline Vertex3D::setNormal(const QVector3D& normal) { m_normal = normal; }

// OpenGL Helpers
Q_DECL_CONSTEXPR inline int Vertex3D::positionOffset() { return offsetof(Vertex3D, m_position); }
Q_DECL_CONSTEXPR inline int Vertex3D::colorOffset() { return offsetof(Vertex3D, m_color); }
Q_DECL_CONSTEXPR inline int Vertex3D::normalOffset() { return offsetof(Vertex3D, m_normal); }
Q_DECL_CONSTEXPR inline int Vertex3D::stride() { return sizeof(Vertex3D); }


class Vertex2D
{
public:
    // Constructors
    Q_DECL_CONSTEXPR Vertex2D();
    Q_DECL_CONSTEXPR explicit Vertex2D(const QVector3D &position);
    Q_DECL_CONSTEXPR Vertex2D(const QVector3D &position, const QVector3D &color);
    Q_DECL_CONSTEXPR Vertex2D(const OpenMesh::Vec3f &position, const OpenMesh::Vec3f &color);

    // Accessors / Mutators
    Q_DECL_CONSTEXPR const QVector3D& position() const;
    Q_DECL_CONSTEXPR const QVector3D& color() const;
    void setPosition(const QVector3D& position);
    void setColor(const QVector3D& color);

    // OpenGL Helpers
    static const int PositionTupleSize = 3;
    static const int ColorTupleSize = 3;
    static Q_DECL_CONSTEXPR int positionOffset();
    static Q_DECL_CONSTEXPR int colorOffset();
    static Q_DECL_CONSTEXPR int stride();

private:
    QVector3D m_position;
    QVector3D m_color;
};

/*******************************************************************************
* Inline Implementation
******************************************************************************/

// Note: Q_MOVABLE_TYPE means it can be memcpy'd.
Q_DECLARE_TYPEINFO(Vertex2D, Q_MOVABLE_TYPE);

// Constructors
Q_DECL_CONSTEXPR inline Vertex2D::Vertex2D() {}
Q_DECL_CONSTEXPR inline Vertex2D::Vertex2D(const QVector3D &position) : m_position(position), m_color{ 1.0f, 1.0f, 1.0f } {}
Q_DECL_CONSTEXPR inline Vertex2D::Vertex2D(const QVector3D &position, const QVector3D &color) : m_position(position), m_color(color) {}
Q_DECL_CONSTEXPR inline Vertex2D::Vertex2D(const OpenMesh::Vec3f& position, const OpenMesh::Vec3f& color)
    : m_position{ position[0], position[1], position[2] },
    m_color{ color[0], color[1], color[2] } {}

// Accessors / Mutators
Q_DECL_CONSTEXPR inline const QVector3D& Vertex2D::position() const { return m_position; }
Q_DECL_CONSTEXPR inline const QVector3D& Vertex2D::color() const { return m_color; }
void inline Vertex2D::setPosition(const QVector3D& position) { m_position = position; }
void inline Vertex2D::setColor(const QVector3D& color) { m_color = color; }

// OpenGL Helpers
Q_DECL_CONSTEXPR inline int Vertex2D::positionOffset() { return offsetof(Vertex2D, m_position); }
Q_DECL_CONSTEXPR inline int Vertex2D::colorOffset() { return offsetof(Vertex2D, m_color); }
Q_DECL_CONSTEXPR inline int Vertex2D::stride() { return sizeof(Vertex2D); }