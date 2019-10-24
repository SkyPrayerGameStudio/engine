/**
 * @file
 */

#pragma once

#include "Bone.h"

namespace animation {

inline constexpr Bone zero() {
	return Bone{glm::zero<glm::vec3>(), glm::zero<glm::vec3>(), glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f }};
}

inline Bone translate(float x, float y, float z) {
	return Bone{glm::one<glm::vec3>(), glm::vec3(x, y, z), glm::quat_identity<float, glm::defaultp>()};
}

inline glm::quat rotateX(float angle) {
	return glm::angleAxis(angle, glm::right);
}

inline glm::quat rotateZ(float angle) {
	return glm::angleAxis(angle, glm::backward);
}

inline glm::quat rotateY(float angle) {
	return glm::angleAxis(angle, glm::up);
}

inline glm::quat rotateXZ(float angleX, float angleZ) {
	return rotateX(angleX) * rotateZ(angleZ);
}

inline glm::quat rotateYZ(float angleY, float angleZ) {
	return rotateZ(angleZ) * rotateY(angleY);
}

inline glm::quat rotateXY(float angleX, float angleY) {
	return rotateY(angleY) * rotateX(angleX);
}

inline glm::quat rotateXYZ(float angleX, float angleY, float angleZ) {
	return rotateXY(angleX, angleY) * rotateZ(angleZ);
}

inline Bone mirrorX(const Bone& bone) {
	Bone mirrored = bone;
	mirrored.translation.x *= -1.0f;
	// the winding order is fixed by reverse index buffer filling
	mirrored.scale.x *= -1.0f;
	return mirrored;
}

inline Bone mirrorXYZ(const Bone& bone) {
	Bone mirrored = bone;
	mirrored.translation *= -1.0f;
	// the winding order is fixed by reverse index buffer filling
	mirrored.scale *= -1.0f;
	return mirrored;
}

inline Bone mirrorXZ(const Bone& bone) {
	Bone mirrored = bone;
	mirrored.translation.x *= -1.0f;
	mirrored.translation.z *= -1.0f;
	// the winding order is fixed by reverse index buffer filling
	mirrored.scale.x *= -1.0f;
	mirrored.scale.z *= -1.0f;
	return mirrored;
}

inline glm::vec3 mirrorXZ(glm::vec3 translation) {
	translation.x *= -1.0f;
	translation.z *= -1.0f;
	return translation;
}

}