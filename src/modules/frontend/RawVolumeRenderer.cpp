#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "frontend/MaterialColor.h"

namespace frontend {

const std::string MaxDepthBufferUniformName = "u_farplanes";

RawVolumeRenderer::RawVolumeRenderer(bool renderAABB) :
		_rawVolume(nullptr), _mesh(nullptr), _worldShader(shader::WorldShader::getInstance()), _renderAABB(renderAABB) {
}

bool RawVolumeRenderer::init(const glm::ivec2& dimension) {
	if (!_worldShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return false;
	}

	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	_vertexBufferIndex = _vertexBuffer.create();
	if (_vertexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object");
		return false;
	}

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, GL_ELEMENT_ARRAY_BUFFER);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return false;
	}

	const glm::vec3 sunDirection(glm::left.x, glm::down.y, 0.0f);
	_sunLight.init(sunDirection, dimension);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	if (!_depthBuffer.init(_sunLight.dimension(), video::DepthBufferMode::RGBA, maxDepthBuffers)) {
		return false;
	}

	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = _vertexBufferIndex;
	attributePos.index = _worldShader.getLocationPos();
	attributePos.stride = sizeof(voxel::Vertex);
	attributePos.size = _worldShader.getComponentsPos();
	attributePos.type = GL_UNSIGNED_BYTE;
	attributePos.typeIsInt = true;
	attributePos.offset = offsetof(voxel::Vertex, position);
	_vertexBuffer.addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeInfo;
	attributeInfo.bufferIndex = _vertexBufferIndex;
	attributeInfo.index = _worldShader.getLocationInfo();
	attributeInfo.stride = sizeof(voxel::Vertex);
	attributeInfo.size = _worldShader.getComponentsInfo();
	attributeInfo.type = GL_UNSIGNED_BYTE;
	attributeInfo.typeIsInt = true;
	attributeInfo.offset = offsetof(voxel::Vertex, ambientOcclusion);
	_vertexBuffer.addAttribute(attributeInfo);

	_whiteTexture = video::createWhiteTexture("**whitetexture**");

	_mesh = new voxel::Mesh(128, 128, true);

	return true;
}

bool RawVolumeRenderer::update(const std::vector<voxel::Vertex>& vertices, const std::vector<voxel::IndexType>& indices) {
	if (!_vertexBuffer.update(_vertexBufferIndex, vertices)) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!_vertexBuffer.update(_indexBufferIndex, indices)) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

bool RawVolumeRenderer::extract() {
	if (_rawVolume == nullptr) {
		return false;
	}

	if (_mesh == nullptr) {
		return false;
	}

	voxel::extractCubicMesh(_rawVolume, _rawVolume->getEnclosingRegion(), _mesh, voxel::IsQuadNeeded(false));
	const voxel::IndexType* meshIndices = _mesh->getRawIndexData();
	const voxel::Vertex* meshVertices = _mesh->getRawVertexData();
	const size_t meshNumberIndices = _mesh->getNoOfIndices();
	if (meshNumberIndices == 0) {
		_vertexBuffer.update(_vertexBufferIndex, nullptr, 0);
		_vertexBuffer.update(_indexBufferIndex, nullptr, 0);
	} else {
		const size_t meshNumberVertices = _mesh->getNoOfVertices();
		if (!_vertexBuffer.update(_vertexBufferIndex, meshVertices, sizeof(voxel::Vertex) * meshNumberVertices)) {
			Log::error("Failed to update the vertex buffer");
			return false;
		}
		if (!_vertexBuffer.update(_indexBufferIndex, meshIndices, sizeof(voxel::IndexType) * meshNumberIndices)) {
			Log::error("Failed to update the index buffer");
			return false;
		}
	}

	return true;
}

void RawVolumeRenderer::render(const video::Camera& camera) {
	if (_renderAABB) {
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}

	const GLuint nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(uint32_t));
	if (nIndices == 0) {
		return;
	}

	_sunLight.update(0.0f, camera);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LEQUAL);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	_whiteTexture->bind(0);

	video::ScopedShader scoped(_worldShader);
	const MaterialColorArray& materialColors = getMaterialColors();
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_model", glm::mat4());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_view", camera.viewMatrix());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_projection", camera.projectionMatrix());
	shaderSetUniformIf(_worldShader, setUniformVec4v, "u_materialcolor[0]", &materialColors[0], materialColors.size());
	shaderSetUniformIf(_worldShader, setUniformi, "u_texture", 0);
	shaderSetUniformIf(_worldShader, setUniformf, "u_fogrange", 250.0f);
	shaderSetUniformIf(_worldShader, setUniformf, "u_viewdistance", camera.farPlane());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_light_projection", _sunLight.projectionMatrix());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_light_view", _sunLight.viewMatrix());
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_lightdir", _sunLight.direction());
	shaderSetUniformIf(_worldShader, setUniformf, "u_depthsize", glm::vec2(_sunLight.dimension()));
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_light", _sunLight.viewProjectionMatrix(camera));
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_diffuse_color", _diffuseColor);
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_ambient_color", _ambientColor);
	shaderSetUniformIf(_worldShader, setUniformf, "u_debug_color", 1.0);
	shaderSetUniformIf(_worldShader, setUniformf, "u_screensize", glm::vec2(camera.dimension()));
	shaderSetUniformIf(_worldShader, setUniformf, "u_nearplane", camera.nearPlane());
	shaderSetUniformIf(_worldShader, setUniformf, "u_farplane", camera.farPlane());
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_campos", camera.position());
	const bool shadowMap = _worldShader.hasUniform("u_shadowmap1");
	if (shadowMap) {
		const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
		for (int i = 0; i < maxDepthBuffers; ++i) {
			glActiveTexture(GL_TEXTURE1 + i);
			glBindTexture(GL_TEXTURE_2D, _depthBuffer.getTexture(i));
			shaderSetUniformIf(_worldShader, setUniformi, core::string::format("u_shadowmap%i", 1 + i), 1 + i);
		}
	}
	core_assert_always(_vertexBuffer.bind());
	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glEnable(GL_POLYGON_OFFSET_LINE);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.0f);
	glPolygonOffset(-2, -2);
	shaderSetUniformIf(_worldShader, setUniformf, "u_debug_color", 0.0);
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);
	glLineWidth(1.0f);
	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_POLYGON_OFFSET_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL_checkError();

	_vertexBuffer.unbind();

	_whiteTexture->unbind();

	if (shadowMap) {
		const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
		for (int i = 0; i < maxDepthBuffers; ++i) {
			glActiveTexture(GL_TEXTURE1 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		glActiveTexture(GL_TEXTURE0);
	}

	GL_checkError();
}

voxel::RawVolume* RawVolumeRenderer::setVolume(voxel::RawVolume* volume) {
	voxel::RawVolume* old = _rawVolume;
	_rawVolume = volume;
	if (_rawVolume != nullptr) {
		const voxel::Region& region = _rawVolume->getEnclosingRegion();
		const core::AABB<float> aabb(region.getLowerCorner(), region.getUpperCorner());
		_shapeBuilder.clear();
		_shapeBuilder.aabb(aabb);
		if (_aabbMeshIndex == -1) {
			_aabbMeshIndex = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_aabbMeshIndex, _shapeBuilder);
		}
	} else {
		_shapeBuilder.clear();
	}
	return old;
}

voxel::RawVolume* RawVolumeRenderer::shutdown() {
	_vertexBuffer.shutdown();
	_worldShader.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	_aabbMeshIndex = -1;
	if (_mesh != nullptr) {
		delete _mesh;
	}
	_mesh = nullptr;
	voxel::RawVolume* old = _rawVolume;
	_whiteTexture->shutdown();
	_whiteTexture = video::TexturePtr();
	_rawVolume = nullptr;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_depthBuffer.shutdown();
	return old;
}

}
