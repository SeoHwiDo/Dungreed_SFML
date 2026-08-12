#pragma once

#include <mutex>
#include <string_view>

/// 콘솔 로그 출력을 한 곳에서 직렬화하는 공용 로깅 관리자입니다.
/// 리소스·게임 데이터처럼 향후 비동기 로딩될 수 있는 시스템도 안전하게 같은 형식으로 기록합니다.
class LogManager {
  public:
    static LogManager &getInstance() {
        static LogManager instance;
        return instance;
    }

    LogManager(const LogManager &) = delete;
    LogManager &operator=(const LogManager &) = delete;

    void info(std::string_view source, std::string_view message) const;
    void warning(std::string_view source, std::string_view message) const;
    void error(std::string_view source, std::string_view message) const;

  private:
    enum class Level { Info, Warning, Error };

    LogManager() = default;
    ~LogManager() = default;
    void write(Level level, std::string_view source, std::string_view message) const;

    mutable std::mutex m_writeMutex;
};
