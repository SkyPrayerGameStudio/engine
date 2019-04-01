/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "io/Filesystem.h"
#include <unordered_map>

struct Mix_Chunk;
struct _Mix_Music;

#define MAX_CHANNELS 16

namespace audio {

enum class SoundState {
	CLOSED = 1 << 0, INITIALIZED = 1 << 1, PAUSE = 1 << 2
};
CORE_ENUM_BIT_OPERATIONS(SoundState)

/**
 * @note There should only be one sound manager instance
 */
class SoundManager: public core::IComponent {
private:
	static constexpr auto logid = Log::logid("SoundManager");
	io::FilesystemPtr _filesystem;
	typedef std::unordered_map<std::string, Mix_Chunk*> ChunkMap;
	typedef ChunkMap::iterator ChunkMapIter;
	ChunkMap _map;
	core::VarPtr _volume;
	core::VarPtr _musicVolume;
	glm::ivec2 _listenerPosition;
	_Mix_Music *_music = nullptr;
	std::string _musicPlaying;
	int _currentChannel = 0;
	SoundState _state = SoundState::CLOSED;
	uint32_t _time = 0u;

	Mix_Chunk* getChunk(const std::string& filename);

	struct Channel {
		int channel;
		Mix_Chunk *chunk;
		glm::ivec2 pos;
	};

	static Channel _channels[MAX_CHANNELS];
	static void channelFinished(int channel);

	inline bool isActive() const;

public:
	SoundManager(const io::FilesystemPtr& filesystem);

	bool init();
	void construct();
	void shutdown();

	bool exists(const std::string& sound) const;
	int playMusic(const std::string& music, bool loop);
	void haltMusic(int music);
	void halt(int sound);
	void haltAll();
	void pause();
	void resume();
	int play(const std::string& filename, const glm::ivec2& position, bool loop);
	void update(uint32_t deltaTime);
	void setListenerPosition(const glm::ivec2& position, const glm::vec2& velocity);
	int volume(int newVolume);
	int musicVolume(int newVolume);
};

}
