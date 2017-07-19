#pragma once
#include <QVector3D>
#include <QMatrix4x4>

class OpenGLCamera
{
public:
    OpenGLCamera() = default;
    OpenGLCamera(const OpenGLCamera &rhs);
    OpenGLCamera(float x, float y, float z, float xt = 0.0f, float yt = 0.0f, float zt = 0.0f)
        : position_{ x, y, z }, target_{ xt, yt, zt } {
        update();
    }
    OpenGLCamera(const QVector3D &pos, const QVector3D &tar = { 0.0f ,0.0f ,0.0f })
        : position_{ pos }, target_{ tar } {
        update();
    }
    ~OpenGLCamera();

    // Data Out
    QVector3D position()  const { return position_; }
    QVector3D target()    const { return target_; }
    QVector3D direction() const { return direction_; }
    QVector3D right()     const { return right_; }
    QVector3D up()        const { return up_; }

    // Data In
    void set_position(float x, float y, float z)
    {
        position_ = { x, y, z }; update();
    }
    void set_target(float x, float y, float z)
    {
        target_ = { x, y, z }; update();
    }

    // Data Update
    void update();

    // Moving
    void move_right(float dis);
    void move_up(float dis);
    void move_back(float dis);
    void move_right_target(float dis);
    void move_up_target(float dis);
    void move_back_target(float dis);
    void move_around_right(float angle);
    void move_around_up(float angle);
    void move_around_right_target(float angle);
    void move_around_up_target(float angle);

    // Generate View Matrix
    QMatrix4x4 view_mat() const;

private:
    QVector3D position_, target_, direction_, right_, up_;
};