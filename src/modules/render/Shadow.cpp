/**
 * @file
 */

#include "Shadow.h"
#include "video/Camera.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "video/ScopedViewPort.h"
#include "video/VertexBuffer.h"
#include "video/Renderer.h"

namespace render {

Shadow::Shadow() :
		_shadowMapShader(shader::ShadowmapShader::getInstance()),
		_shadowMapRenderShader(shader::ShadowmapRenderShader::getInstance()),
		_shadowMapInstancedShader(shader::ShadowmapInstancedShader::getInstance()) {
}

bool Shadow::init(int maxDepthBuffers) {
	_maxDepthBuffers = maxDepthBuffers;
	core_assert(_maxDepthBuffers >= 1);
	const float length = 50.0f;
	const glm::vec3 sunPos(length, length, -length);
	const glm::vec3 center(0.0f);
	_lightView = glm::lookAt(sunPos, center, glm::up);
	//_sunDirection = normalize(center - sunPos);
	_sunDirection = glm::vec3(glm::column(glm::inverse(_lightView), 2));
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	if (!_shadowMapRenderShader.setup()) {
		Log::error("Failed to init shadowmap debug shader");
		return false;
	}
	if (!_shadowMapInstancedShader.setup()) {
		Log::error("Failed to init shadowmap instanced debug shader");
		return false;
	}
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, _maxDepthBuffers)) {
		Log::error("Failed to init the depthbuffer");
		return false;
	}

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	return true;
}

void Shadow::shutdown() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_depthBuffer.shutdown();
	_shadowMapShader.shutdown();
	_shadowMapInstancedShader.shutdown();
}

void Shadow::calculateShadowData(const video::Camera& camera, bool active, float sliceWeight) {
	core_trace_scoped(ShadowCalculate);
	_cascades.resize(_maxDepthBuffers);
	_distances.resize(_maxDepthBuffers);
	const glm::ivec2& dim = dimension();

	if (!active) {
		for (int i = 0; i < _maxDepthBuffers; ++i) {
			_cascades[i] = glm::mat4(1.0f);
			_distances[i] = camera.farPlane();
		}
		return;
	}

	std::vector<float> planes(_maxDepthBuffers * 2);
	camera.sliceFrustum(&planes.front(), planes.size(), _maxDepthBuffers, sliceWeight);
	const glm::mat4& inverseView = camera.inverseViewMatrix();
	const float shadowRangeZ = camera.farPlane() * 3.0f;

	for (int i = 0; i < _maxDepthBuffers; ++i) {
		const float near = planes[i * 2 + 0];
		const float far = planes[i * 2 + 1];
		const glm::vec4& sphere = camera.splitFrustumSphereBoundingBox(near, far);
		const glm::vec3 lightCenter(_lightView * inverseView * glm::vec4(sphere.x, sphere.y, sphere.z, 1.0f));
		const float lightRadius = sphere.w;

		// round to prevent movement
		const float xRound = lightRadius * 2.0f / dim.x;
		const float yRound = lightRadius * 2.0f / dim.y;
		const float zRound = 1.0f;
		const glm::vec3 round(xRound, yRound, zRound);
		const glm::vec3 lightCenterRounded = glm::round(lightCenter / round) * round;
		const glm::mat4& lightProjection = glm::ortho(
				 lightCenterRounded.x - lightRadius,
				 lightCenterRounded.x + lightRadius,
				 lightCenterRounded.y - lightRadius,
				 lightCenterRounded.y + lightRadius,
				-lightCenterRounded.z - (shadowRangeZ - lightRadius),
				-lightCenterRounded.z + lightRadius);
		_cascades[i] = lightProjection * _lightView;
		_distances[i] = far;
	}
}

bool Shadow::bind(video::TextureUnit unit) {
	const bool state = video::bindTexture(unit, _depthBuffer);
	core_assert(state);
	return state;
}

void Shadow::renderShadowMap(const video::Camera& camera) {
	core_trace_scoped(TestMeshAppDoShowShadowMap);
	const int width = camera.width();
	const int height = camera.height();

	// activate shader
	video::ScopedShader scopedShader(_shadowMapRenderShader);
	_shadowMapRenderShader.recordUsedUniforms(true);
	_shadowMapRenderShader.clearUsedUniforms();
	_shadowMapRenderShader.setShadowmap(video::TextureUnit::Zero);
	_shadowMapRenderShader.setFar(camera.farPlane());
	_shadowMapRenderShader.setNear(camera.nearPlane());

	// bind buffers
	video::ScopedVertexBuffer scopedBuf(_shadowMapDebugBuffer);

	// configure shadow map texture
	video::bindTexture(video::TextureUnit::Zero, _depthBuffer);
	video::setupDepthCompareTexture(_depthBuffer.textureType(), video::CompareFunc::Less, video::TextureCompareMode::None);

	// render shadow maps
	for (int i = 0; i < _maxDepthBuffers; ++i) {
		const int halfWidth = (int) (width / 4.0f);
		const int halfHeight = (int) (height / 4.0f);
		video::ScopedViewPort scopedViewport(i * halfWidth, 0, halfWidth, halfHeight);
		_shadowMapRenderShader.setCascade(i);
		video::drawArrays(video::Primitive::Triangles, _shadowMapDebugBuffer.elements(0));
	}

	// restore texture
	video::setupDepthCompareTexture(_depthBuffer.textureType(), video::CompareFunc::Less, video::TextureCompareMode::RefToTexture);
}

glm::ivec2 Shadow::dimension() const {
	return _depthBuffer.dimension();
}


}
