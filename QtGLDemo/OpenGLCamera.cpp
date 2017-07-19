#include "stdafx.h"
#include "OpenGLCamera.h"

#define PI 3.14159f

OpenGLCamera::~OpenGLCamera()
{
}

OpenGLCamera::OpenGLCamera(const OpenGLCamera& rhs)
{
    position_ = rhs.position_;
    target_ = rhs.target_;
    direction_ = rhs.direction_;
    right_ = rhs.right_;
    up_ = rhs.up_;
}

void OpenGLCamera::update()
{
    direction_ = (position_ - target_).normalized();
    QVector3D up{ 0.0f, 0.0f, 1.0f };
    right_ = QVector3D::crossProduct(up, direction_);
    right_.normalize();
    up_ = QVector3D::crossProduct(direction_, right_);
    up_.normalize();
}

void OpenGLCamera::move_right(float dis)
{
    position_ += dis * right_;
    update();
}

void OpenGLCamera::move_up(float dis)
{
    position_ += dis * up_;
    update();
}

void OpenGLCamera::move_back(float dis)
{
    float radius = (position_ - target_).length();
    if (radius < -dis)
        return;
    position_ += dis * direction_;
    update();
}

void OpenGLCamera::move_right_target(float dis)
{
    target_ += dis * right_;
    update();
}

void OpenGLCamera::move_up_target(float dis)
{
    target_ += dis * up_;
    update();
}

void OpenGLCamera::move_back_target(float dis)
{
    target_ += dis * direction_;
    update();
}

void OpenGLCamera::move_around_right(float angle)
{
    float radius = (position_ - target_).length();
    float cos_theta = abs(QVector3D::dotProduct(direction_, { 0.0f, 0.0f, 1.0f })); // theta: up(absolute) and direction
    float sin_theta = sqrtf(1 - cos_theta * cos_theta);
    float dis = 2 * sin_theta * radius * sinf(angle * PI / 180.0f / 2.0f);          // d = 2 r cos(theta) sin(alpha * PI / 2)
    position_ += dis * cosf(angle * PI / 180.0f / 2.0f) * right_;                   // x += d cos(alpha * PI / 2)
    position_ -= dis * sinf(angle * PI / 180.0f / 2.0f) * direction_ * sin_theta;   // z -= d sin(alpha * PI / 2) sin(theta)
    position_ += dis * sinf(angle * PI / 180.0f / 2.0f) * up_ * cos_theta;          // y += d sin(alpha * PI / 2) cos(theta)
    update();
}

void OpenGLCamera::move_around_up(float angle)
{
    float radius = (position_ - target_).length();
    float cos_theta = abs(QVector3D::dotProduct(direction_, { 0.0f, 0.0f, 1.0f }));
    float cos_angle = cosf(angle * PI / 180.0f);
    if (cos_theta > cos_angle - 0.00001f && direction_.z() * angle > 0)
        return;
    float dis = 2 * radius * sinf(angle * PI / 180.0f / 2.0f);           // d = 2 r sin(alpha * PI / 2)
    position_ += dis * cosf(angle * PI / 180.0f / 2.0f) * up_;           // y += d cos(alpha * PI / 2)
    position_ -= dis * sinf(angle * PI / 180.0f / 2.0f) * direction_;    // z -= d sin(alpha * PI / 2)
    update();
}

void OpenGLCamera::move_around_right_target(float angle)
{
    float radius = (position_ - target_).length();
    float cos_theta = abs(QVector3D::dotProduct(direction_, { 0.0f, 0.0f, 1.0f })); // theta: up(absolute) and direction
    float sin_theta = sqrtf(1 - cos_theta * cos_theta);
    float dis = 2 * sin_theta * radius * sinf(angle * PI / 180.0f / 2.0f);          // d = 2 r cos(theta) sin(alpha * PI / 2)
    target_ += dis * cosf(angle * PI / 180.0f / 2.0f) * right_;                     // x += d cos(alpha * PI / 2)
    target_ -= dis * sinf(angle * PI / 180.0f / 2.0f) * direction_ * sin_theta;     // z -= d sin(alpha * PI / 2) sin(theta)
    target_ += dis * sinf(angle * PI / 180.0f / 2.0f) * up_ * cos_theta;            // y += d sin(alpha * PI / 2) cos(theta)
    update();
}

void OpenGLCamera::move_around_up_target(float angle)
{
    float radius = (position_ - target_).length();
    float dis = 2 * radius * sinf(angle * PI / 180.0f / 2.0f);           // d = 2 r sin(alpha * PI / 2)
    target_ += dis * cosf(angle * PI / 180.0f / 2.0f) * up_;             // y += d cos(alpha * PI / 2)
    target_ += dis * sinf(angle * PI / 180.0f / 2.0f) * direction_;      // z += d sin(alpha * PI / 2)
    update();
}

QMatrix4x4 OpenGLCamera::view_mat() const
{
    QMatrix4x4 view;
    view.lookAt(position_, target_, up_);
    return view;
}