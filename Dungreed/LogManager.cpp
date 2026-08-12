#include "LogManager.h"

#include <iostream>

void LogManager::info(std::string_view source, std::string_view message) const { write(Level::Info, source, message); }

void LogManager::warning(std::string_view source, std::string_view message) const { write(Level::Warning, source, message); }

void LogManager::error(std::string_view source, std::string_view message) const { write(Level::Error, source, message); }

void LogManager::write(Level level, std::string_view source, std::string_view message) const {
    std::string_view levelText = "UNKNOWN";
    switch (level) {
    case Level::Info:
        levelText = "INFO";
        break;
    case Level::Warning:
        levelText = "WARN";
        break;
    case Level::Error:
        levelText = "ERROR";
        break;
    }

    const std::lock_guard<std::mutex> lock(m_writeMutex);
    std::cerr << '[' << levelText << "][" << source << "] " << message << '\n';
}
