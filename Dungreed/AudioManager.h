#pragma once

#include <SFML/Audio.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include "EntityId.h"

struct SfxVoice {
    std::unique_ptr<sf::Sound> sound;
    EntityId ownerId = 0;
};

// 액터 행동 SFX와 씬 BGM을 미리 준비해 재생합니다.
class AudioManager {
  public:
    static AudioManager &getInstance() {
        static AudioManager instance;
        return instance;
    }

    AudioManager(const AudioManager &) = delete;
    AudioManager &operator=(const AudioManager &) = delete;

    // ResourceManager가 오디오 파일을 로드한 뒤 한 번 호출합니다.
    bool initialize();

    void playSfx(EntityId ownerId, const std::string &soundName);
    void stopActorSounds(EntityId ownerId);
    void playBgm(const std::string &musicName);

  private:
    AudioManager() = default;
    ~AudioManager() = default;

    static constexpr std::size_t kSfxVoiceCount = 32;

    SfxVoice &acquireSfxVoice();

    std::array<SfxVoice, kSfxVoiceCount> m_sfxVoices;
    std::unordered_map<std::string, std::unique_ptr<sf::Music>> m_bgms;
    std::unordered_map<std::string, std::size_t> m_nextSoundVariants;
    std::optional<std::string> m_currentMusic;
    std::size_t m_nextSfxVoice = 0;
    bool m_initialized = false;
};
