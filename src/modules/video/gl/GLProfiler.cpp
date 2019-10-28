/**
 * @file
 */

#include "video/Renderer.h"
#include "flextGL.h"
#include <algorithm>

namespace video {

bool ProfilerGPU::init() {
	GLuint lid;
	glGenQueries(1, &lid);
	_id = (Id)lid;
	return _id != InvalidId;
}

void ProfilerGPU::shutdown() {
	if (_id == InvalidId) {
		return;
	}
	const GLuint lid = (GLuint)_id;
	glDeleteQueries(1, &lid);
	_id = InvalidId;
}

void ProfilerGPU::enter() {
	if (_id == InvalidId) {
		return;
	}
	core_assert(_state == 0 || _state == 2);

	if (_state == 0) {
		const GLuint lid = (GLuint)_id;
		glBeginQuery(GL_TIME_ELAPSED, lid);
		_state = 1;
	}
}

void ProfilerGPU::leave() {
	if (_id == InvalidId) {
		return;
	}
	core_assert(_state == 1 || _state == 2);

	if (_state == 1) {
		glEndQuery(GL_TIME_ELAPSED);
		_state = 2;
	} else if (_state == 2) {
		GLint availableResults = 0;
		const GLuint lid = (GLuint)_id;
		glGetQueryObjectiv(lid, GL_QUERY_RESULT_AVAILABLE, &availableResults);
		if (availableResults > 0) {
			_state = 0;
			GLuint64 time = 0;
			glGetQueryObjectui64v(_id, GL_QUERY_RESULT, &time);
			const double timed = double(time);
			_samples[_sampleCount & (_maxSampleCount - 1)] = timed;
			++_sampleCount;
			_max = core_max(_max, timed);
			_min = core_min(_min, timed);
			_avg = _avg * 0.5 + timed / 1e9 * 0.5;
		}
	}
}

}