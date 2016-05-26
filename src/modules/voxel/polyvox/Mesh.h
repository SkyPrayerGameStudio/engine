/**
 * @file
 */

#pragma once

#include "Region.h"
#include "Vertex.h"
#include <algorithm>
#include <cstdlib>
#include <list>
#include <memory>
#include <set>
#include <vector>

namespace voxel {

// TODO: maybe reduce to uint16_t and use glDrawElementsBaseVertex
typedef uint32_t IndexType;

/**
 * A simple and general-purpose mesh class to represent the data returned by the surface extraction functions.
 * It supports different vertex types (which will vary depending on the surface extractor used and the contents
 * of the volume).
 */
class Mesh {
public:
	Mesh();
	Mesh(int vertices);
	~Mesh();

	size_t getNoOfVertices() const;
	const Vertex& getVertex(IndexType index) const;
	const Vertex* getRawVertexData() const;

	size_t getNoOfIndices() const;
	IndexType getIndex(IndexType index) const;
	const IndexType* getRawIndexData() const;

	const glm::ivec3& getOffset() const;
	void setOffset(const glm::ivec3& offset);

	IndexType addVertex(const Vertex& vertex);
	void addTriangle(IndexType index0, IndexType index1, IndexType index2);

	void clear();
	bool isEmpty() const;
	void removeUnusedVertices();

private:
	std::vector<IndexType> _vecIndices;
	std::vector<Vertex> _vecVertices;
	glm::ivec3 _offset;
};

inline Mesh::Mesh(int vertices) {
	_vecVertices.reserve(vertices);
}

inline Mesh::Mesh() {
}

inline Mesh::~Mesh() {
}

inline size_t Mesh::getNoOfVertices() const {
	return _vecVertices.size();
}

inline const Vertex& Mesh::getVertex(IndexType index) const {
	return _vecVertices[index];
}

inline const Vertex* Mesh::getRawVertexData() const {
	return _vecVertices.data();
}

inline size_t Mesh::getNoOfIndices() const {
	return _vecIndices.size();
}

inline IndexType Mesh::getIndex(IndexType index) const {
	return _vecIndices[index];
}

inline const IndexType* Mesh::getRawIndexData() const {
	return _vecIndices.data();
}

inline const glm::ivec3& Mesh::getOffset() const {
	return _offset;
}

inline void Mesh::setOffset(const glm::ivec3& offset) {
	_offset = offset;
}

inline void Mesh::addTriangle(IndexType index0, IndexType index1, IndexType index2) {
	//Make sure the specified indices correspond to valid vertices.
	core_assert_msg(index0 < _vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index1 < _vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index2 < _vecVertices.size(), "Index points at an invalid vertex.");

	_vecIndices.push_back(index0);
	_vecIndices.push_back(index1);
	_vecIndices.push_back(index2);
}

inline IndexType Mesh::addVertex(const Vertex& vertex) {
	// We should not add more vertices than our chosen index type will let us index.
	core_assert_msg(_vecVertices.size() < std::numeric_limits<IndexType>::max(), "Mesh has more vertices that the chosen index type allows.");

	_vecVertices.push_back(vertex);
	return _vecVertices.size() - 1;
}

inline void Mesh::clear() {
	_vecVertices.clear();
	_vecIndices.clear();
}

inline bool Mesh::isEmpty() const {
	return getNoOfVertices() == 0 || getNoOfIndices() == 0;
}

}
