#include "AudioManager.h"

#include "LogManager.h"
#include "ResourceManager.h"

bool AudioManager::initialize() {
    if (m_initialized) {
        return true;
    }

    auto &resources = ResourceManager::getInstance();
    const sf::SoundBuffer *templateSound = resources.getSfxTemplate();
    if (!templateSound) {
        LogManager::getInstance().error("AudioManager", "효과음 채널을 초기화할 SoundBuffer가 없습니다.");
        return false;
    }

    for (SfxVoice &voice : m_sfxVoices) {
        voice.sound = std::make_unique<sf::Sound>(*templateSound);
        voice.ownerId = 0;
    }

    for (const std::string &musicName : resources.getMusicNames()) {
        const std::string *path = resources.getMusicPath(musicName);
        auto bgm = std::make_unique<sf::Music>();
        if (!path || !bgm->openFromFile(*path)) {
            LogManager::getInstance().error("AudioManager", "BGM preload failed: " + musicName);
            return false;
        }
        bgm->setLooping(true);
        m_bgms.emplace(musicName, std::move(bgm));
    }

    m_initialized = true;
    return true;
}

void AudioManager::playSfx(EntityId ownerId, const std::string &soundName) {
    if (!m_initialized)
        return;

    const std::vector<sf::SoundBuffer> *sounds = ResourceManager::getInstance().getSoundBuffers(soundName);
    if (!sounds || sounds->empty())
        return;

    SfxVoice &voice = acquireSfxVoice();
    const std::size_t variantIndex = m_nextSoundVariants[soundName]++ % sounds->size();
    voice.sound->setBuffer((*sounds)[variantIndex]);
    voice.ownerId = ownerId;
    voice.sound->play();
}

void AudioManager::stopActorSounds(EntityId ownerId) {
    if (!m_initialized || ownerId == 0) {
        return;
    }

    for (SfxVoice &voice : m_sfxVoices) {
        if (voice.ownerId == ownerId) {
            voice.sound->stop();
            voice.ownerId = 0;
        }
    }
}

void AudioManager::playBgm(const std::string &musicName) {
    if (!m_initialized || (m_currentMusic && *m_currentMusic == musicName))
        return;

    const auto bgmIt = m_bgms.find(musicName);
    if (bgmIt == m_bgms.end()) {
        LogManager::getInstance().warning("AudioManager", "BGM was not preloaded: " + musicName);
        return;
    }
    if (m_currentMusic) {
        const auto currentBgmIt = m_bgms.find(*m_currentMusic);
        if (currentBgmIt != m_bgms.end())
            currentBgmIt->second->stop();
    }

    bgmIt->second->play();
    m_currentMusic = musicName;
}

SfxVoice &AudioManager::acquireSfxVoice() {
    for (std::size_t offset = 0; offset < m_sfxVoices.size(); ++offset) {
        const std::size_t index = (m_nextSfxVoice + offset) % m_sfxVoices.size();
        if (m_sfxVoices[index].sound->getStatus() != sf::SoundSource::Status::Playing) {
            m_nextSfxVoice = (index + 1) % m_sfxVoices.size();
            return m_sfxVoices[index];
        }
    }

    SfxVoice &voice = m_sfxVoices[m_nextSfxVoice];
    m_nextSfxVoice = (m_nextSfxVoice + 1) % m_sfxVoices.size();
    voice.sound->stop();
    return voice;
}
