# 🎮 Dungreed_SFML (던그리드 모작)

## 📖 프로젝트 소개 (Overview)
SFML을 활용한 C++ 기반 2D 액션 로그라이크 '던그리드(Dungreed)' 게임 모작 프로젝트입니다.
객체지향 설계를 바탕으로 절차적 맵 생성(Procedural Map Generation), 물리 및 상태 머신(FSM) 기반의 액터 구조를 체계화하였으며, 텍스처 아틀라스와 JSON 데이터를 로드하여 애니메이션과 리소스를 효율적으로 관리합니다.

## 💻 개발 환경 및 기술 스택 (Environment & Tech Stack)
* **OS**: Windows 11
* **IDE**: Visual Studio 2022
* **Language**: C++ (C++17 이상)
* **Library/Framework**: 
  * SFML 3.1.0 (Simple and Fast Multimedia Library)
  * Nlohmann JSON (JSON for Modern C++)

## 🏗️ 시스템 아키텍처 (Class Diagram)
![Class Diagram](class.png)

위 클래스 다이어그램과 코드는 게임의 전반적인 핵심 구조를 나타냅니다:

* **MapManager & Room (절차적 맵 생성 시스템)**
  * `MapManager` (싱글톤)는 BFS(너비 우선 탐색) 알고리즘을 활용하여 방(`Room`)들을 절차적으로 생성하고 무작위로 연결(`linkRoom`)합니다. 가장 먼 방을 계산하여 시작(Start) 방과 보스(Boss) 방을 자동 배치합니다.
  * 각 `Room`은 `RoomInfo`와 `MonsterSpawnConfig`를 기반으로 방의 넓이에 비례하여 몬스터와 보물상자를 동적으로 스폰합니다.
* **Actor & Player (물리 및 비트마스크 상태 머신)**
  * `Actor`: `std::optional<sf::Sprite>`를 활용해 안전하게 스프라이트를 관리하며, 중력(`gravity`) 및 속도(`velocity`)를 계산하는 기초 물리 연산(`updatePhysics`)을 수행합니다.
  * `Player`: 비트마스크(Bitmask) 기법을 활용한 상태 머신(`PlayerState` Enum)을 구현하여 복합적인 상태(예: 점프 중 공격 등)를 효율적으로 제어합니다.
* **Controller (입력 및 수학적 방향 계산)**
  * 키보드 및 마우스 입력을 전담하며, 아크탄젠트(`std::atan2`) 함수를 활용해 플레이어와 마우스 포인터 간의 라디안(`aimRadian`) 및 단위 벡터(`aimDir`)를 계산하여 무기 조준 및 스프라이트 반전을 처리합니다.
* **ResourceManager (아틀라스 & 스마트 포인터 관리)**
  * 폴더 경로가 포함된 프레임 이름(예: `Walk/Player_Walk-00`)에서 슬래시(`/`)와 하이픈(`-`)을 자동으로 파싱하여 애니메이션 클립 이름(`Player_Walk`)을 정확히 추출합니다. 
  * `std::unique_ptr`를 통해 리소스의 소유권을 독점하고, `nullptr` 반환을 통한 안전한 예외 처리를 보장합니다.
* **Animator (비동기 애니메이션 갱신)**
  * 프레임 전환 시간(`dt`)을 누적하여 스프라이트를 갱신하며, 루프/단발성 재생을 지원합니다. 테스트를 위해 `main.cpp`에서 모든 애니메이션 클립을 그리드(Grid) 형태로 렌더링하는 기능을 포함하고 있습니다.

## 🚀 주요 기능 (Features)
- **절차적 맵 생성 (Procedural Generation)**: 무작위 노드 연결 및 BFS 탐색을 통한 로그라이크식 맵 동적 생성
- **비트마스크 기반 FSM**: 메모리 효율적이고 동시 상태 처리가 가능한 플레이어 상태 제어 로직
- **수학적 마우스 에임 구현**: `atan2`를 활용한 360도 전방위 조준 및 캐릭터 좌우 반전 로직 적용
- **물리 연산 및 플랫폼 충돌 기초**: 중력(Gravity) 적용 및 점프(Velocity) 제어
- **안전한 메모리/예외 관리**: `std::unique_ptr`와 `std::optional`을 적극 활용한 모던 C++ 기반 객체 관리

## 🛠️ 외부 라이브러리 및 출처 (Dependencies & Acknowledgments)
* **[SFML (Simple and Fast Multimedia Library)](https://www.sfml-dev.org/)**
  * 용도: 그래픽 렌더링, 윈도우/입력 관리
  * 라이센스: [zlib/png License](https://opensource.org/licenses/Zlib)
* **[JSON for Modern C++ (nlohmann/json)](https://github.com/nlohmann/json)**
  * 용도: 아틀라스 구성 정보 파싱
  * 라이센스: [MIT License](https://opensource.org/licenses/MIT)

## 📄 라이센스 (License)
이 프로젝트의 코드는 **MIT License**에 따라 배포됩니다. 자세한 내용은 `LICENSE` 파일을 참고해주세요.