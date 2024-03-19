#ifndef GLM_POSE_H
#define GLM_POSE_H

#include <openxr/openxr.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

template<typename T> static inline T clamp(T v, T mn, T mx)
{
	return (v < mn) ? mn : (v > mx) ? mx : v;
}

inline float sign(float val)
{
    return (val < 0.0f) ? -1.0f : 1.0f;
}

namespace BVR
{

const float ROOT_OF_HALF = 0.7071067690849304f;

const glm::fquat default_rotation(1.0f, 0.0f, 0.0f, 0.0f);
const glm::fquat rotate_90_CCW_by_x(0.7071067690849304f, 0.7071067690849304f, 0.0f, 0.0f);
const glm::fquat rotate_180_CCW_about_y(0.0f, 1.0f, 0.0f, 0.0f);
const glm::fquat rotate_CW_45_rotation_about_x(0.9238795f, -0.3826834f, 0.0f, 0.0f);

const glm::fquat CCW_180_rotation_about_y = glm::fquat(0, 1, 0, 0);
const glm::fquat CCW_180_rotation_about_x = glm::fquat(0, 0, 1, 0);
const glm::fquat CCW_180_rotation_about_z = glm::fquat(0, 0, 0, 1);

const glm::fquat CCW_45_rotation_about_y = glm::fquat(0, 0.3826834f, 0, 0.9238795f);
const glm::fquat CW_45_rotation_about_y = glm::fquat(0, -0.3826834f, 0, 0.9238795f);

const glm::fquat CCW_90_rotation_about_y = glm::fquat(0, ROOT_OF_HALF, 0, ROOT_OF_HALF);
const glm::fquat CW_90_rotation_about_y = glm::fquat(0, -ROOT_OF_HALF, 0, ROOT_OF_HALF);

const glm::fquat CW_90_rotation_about_x = glm::fquat(-ROOT_OF_HALF, 0, 0, ROOT_OF_HALF);
const glm::fquat CCW_90_rotation_about_x = glm::fquat(ROOT_OF_HALF, 0, 0, ROOT_OF_HALF);

const glm::fquat CW_30deg_rotation_about_x = glm::fquat(-0.258819f, 0, 0, 0.9659258f);
const glm::fquat CCW_30deg_rotation_about_x = glm::fquat(0.258819f, 0, 0, 0.9659258f);

const glm::fquat CW_30deg_rotation_about_Y = glm::fquat(0.0f, 0.258819f, 0.0f, 0.9659258f);
const glm::fquat CCW_30deg_rotation_about_Y = glm::fquat(0.0f, -0.258819f, 0.0f, 0.9659258f);

const glm::fquat front_rotation = default_rotation;
const glm::fquat back_rotation = CCW_180_rotation_about_y;

const glm::fquat left_rotation = CCW_90_rotation_about_y;
const glm::fquat right_rotation = CW_90_rotation_about_y;

const glm::fquat floor_rotation = CW_90_rotation_about_x;
const glm::fquat ceiling_rotation = CCW_90_rotation_about_x;

const glm::fquat down_rotation = CW_90_rotation_about_x;
const glm::fquat up_rotation = CCW_90_rotation_about_x;

struct GLMPose
{
    GLMPose()
    {
    }

    GLMPose(const glm::vec3 &translation, const glm::fquat &rotation) : translation_(
            translation), rotation_(rotation)
    {
    }

    glm::vec3 translation_ = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::fquat rotation_ = default_rotation;
    glm::vec3 scale_ = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 euler_angles_degrees_ = glm::vec3(0.0f, 0.0f, 0.0f);
    bool is_valid_ = true;

    uint64_t timestamp_ = 0;

    void clear();

    glm::mat4 to_matrix() const;

    void update_rotation_from_euler();

    void transform(const GLMPose &glm_pose);
};

XrVector3f convert_to_xr(const glm::vec3 &input);

glm::vec3 convert_to_glm(const XrVector3f &input);

XrQuaternionf convert_to_xr(const glm::fquat &input);

glm::fquat convert_to_glm(const XrQuaternionf &input);

glm::mat4 convert_to_rotation_matrix(const glm::fquat &rotation);

GLMPose convert_to_glm_pose(const XrVector3f &position, const XrQuaternionf &rotation,
                       const XrVector3f &scale);

GLMPose convert_to_glm_pose(const XrPosef &xr_pose);

XrPosef convert_to_xr_pose(const GLMPose &glm_pose);

} // BVR

#endif