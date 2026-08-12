# 🎮 Dungreed_SFML (던그리드 모작)

## 📖 프로젝트 소개 (Overview)
SFML을 활용한 C++ 기반 2D 액션 로그라이크 '던그리드(Dungreed)' 게임 모작 프로젝트입니다.
객체지향 설계와 **데이터 주도 설계(Data-Driven Design)**를 바탕으로 제작되었습니다. JSON 데이터를 활용한 절차적 맵 생성 및 페이즈 기반 몬스터 스폰, 물리/상태 머신(FSM) 기반의 액터 제어, 그리고 **오브젝트 풀링(Object Pooling)**을 통한 메모리 최적화 등 완성도 높은 게임 코어 시스템을 구현하는 데 집중했습니다.

## 💻 개발 환경 및 기술 스택 (Environment & Tech Stack)
* **OS**: Windows 11
* **IDE**: Visual Studio 2022
* **Language**: C++ (C++17 이상)
* **Library/Framework**: 
  * SFML 3.1.0 (Simple and Fast Multimedia Library)
  * Nlohmann JSON (JSON for Modern C++)

## 🏗️ 시스템 아키텍처 (Core Architecture)
![Class Diagram](class.png)

게임의 핵심 구조는 각 매니저 클래스의 철저한 **책임 분리(Separation of Concerns)**를 원칙으로 설계되었습니다:

* **GameDataManager & ResourceManager (데이터 및 리소스 제어)**
  * `GameDataManager`: 무기, 몬스터, 방(Room)의 형태 및 스폰 정보를 JSON에서 파싱하여 런타임 데이터로 변환합니다.
  * `ResourceManager`: 텍스처 아틀라스의 프레임 정보와 피벗, 원본 크기 등을 `std::unique_ptr`와 `std::optional`을 통해 안전하게 독점 관리합니다.
* **MapManager & TileMap (맵 생성 및 방 전환 시스템)**
  * JSON 데이터를 기반으로 Start → 일반 방 → Boss 방으로 이어지는 경로를 무작위 생성하며, 겹치지 않는 문(Door) 방향을 자동 할당합니다.
  * 게임 진입 시 **해당 층의 모든 타일맵을 사전 생성(Preload)**하여, 문을 통한 방 전환 시 프레임 드랍 없이 즉각적으로 이동과 카메라 경계 갱신을 처리합니다.
* **ObjectPoolingManager (오브젝트 풀링 및 최적화)**
  * 게임 데이터(방 JSON)를 미리 분석하여 현재 층에서 필요한 몬스터와 투사체의 최대 개수를 계산하고(10% 여유값 포함) **사전 생성(Pre-warm)**합니다.
  * 런타임 중 동적 할당(`new`/`delete`)을 최소화하며, 우선순위 큐(Min-Heap)를 통해 오래 비활성화된 슬롯을 먼저 재사용합니다.
* **MonsterManager & CombatManager (전투 및 AI 독립성)**
  * `MonsterManager`: 페이즈(Phase) 기반 스폰 논리와 활성 몬스터의 AI/물리 업데이트만 전담하며, 객체의 메모리 생명주기는 풀링 매니저에 위임합니다.
  * `CombatManager`: 액터 간의 충돌, 공격 판정, 투사체 타격 등 '상호작용'만 처리합니다. (근접 공격 중복 타격 방지, 타겟팅 투사체 처리 등)
* **Actor & Player (물리 및 비트마스크 FSM)**
  * `Actor`: 중력 및 넉백 물리 연산을 처리하며, `OneWay` 플랫폼(위에서 떨어질 때만 착지) 충돌 로직을 구현했습니다.
  * `Player`: 대시(잔상, 쿨타임 포함), 점프, 에임(atan2 360도 전방위 조준) 등을 비트마스크(Bitmask) 기반 FSM으로 부드럽게 제어합니다.

## 🚀 주요 기능 (Key Features)
- **데이터 주도 설계 (Data-Driven)**: 몬스터 능력치, 무기별 발사체 속성, 방의 레이아웃과 소환될 몬스터 페이즈(Phase) 목록까지 모두 외부 JSON에서 제어
- **고도화된 타일맵 및 방 전환 로직**: 각 방을 연결하는 문(Door) 충돌 트리거 구현 및 부드러운 맵 전환 (내부/외곽 백타일 그림자 자동화 적용)
- **페이즈(Phase) 기반 소환 & 몬스터 AI**: 
  - 방 입장 시 안전 구역(공격 사거리 밖, 맵 내부)을 계산하여 몬스터를 순차 페이즈로 소환
  - 밴시(방사형 투사체 10발 발사), 미노타우로스(직선 돌진 후 공격), 꼬마유령(벽 통과 비행 궤적), 박쥐 등 다채로운 행동 패턴(FSM) 구현
- **정교한 물리 및 충돌 처리**: 대시 중 플랫폼 무시, 피격 시 방향 기반 넉백, 투사체 위치 보간(벽 통과 방지) 구현
- **안전한 메모리/예외 관리**: Modern C++ (C++17) 기능을 적극 활용한 메모리 릭(Leak) 방지 및 안전한 예외 처리 보장

## 🛠️ 외부 라이브러리 및 출처 (Dependencies & Acknowledgments)
* **[SFML (Simple and Fast Multimedia Library)](https://www.sfml-dev.org/)**
  * 용도: 그래픽 렌더링, 윈도우/입력 관리
  * 라이센스: [zlib/png License](https://opensource.org/licenses/Zlib)
* **[JSON for Modern C++ (nlohmann/json)](https://github.com/nlohmann/json)**
  * 용도: 게임 데이터(방, 무기, 몬스터) 및 아틀라스 정보 파싱
  * 라이센스: [MIT License](https://opensource.org/licenses/MIT)

## 📄 라이센스 (License)
이 프로젝트의 코드는 **MIT License**에 따라 배포됩니다. 자세한 내용은 `LICENSE` 파일을 참고해주세요.