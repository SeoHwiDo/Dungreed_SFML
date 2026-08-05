


# 🎮 Dungreed_SFML (던그리드 모작)

## 📖 프로젝트 소개 (Overview)
SFML을 활용한 C++ 기반 2D 액션 로그라이크 '던그리드(Dungreed)' 게임 모작 프로젝트입니다.
객체지향 설계를 바탕으로 액터(Actor)와 맵(Map)의 구조를 체계화하였으며, 텍스처 아틀라스(Texture Atlas) 이미지와 JSON 데이터를 로드하여 효율적으로 애니메이션과 리소스를 관리합니다.

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

* **ResourceManager**
  * **싱글톤(Singleton)** 패턴으로 구현되어 텍스처, 타일셋 등 게임 내 모든 그래픽 자원을 중앙에서 관리합니다.
  * `std::unique_ptr`를 활용하여 리소스 데이터(`AtlasData`, `sf::IntRect`)의 메모리 소유권을 매니저가 독점하게 함으로써 메모리 누수를 방지합니다.
* **Animator (애니메이션 시스템)**
  * 분리된 애니메이션 프레임 데이터(`AnimationClip`)를 기반으로 프레임 전환 시간(`dt`)을 계산하여 스프라이트를 갱신합니다. 루프(Loop) 및 단발성 애니메이션 재생을 지원합니다.
* **Actor & 상속 구조**
  * `Status` (체력, 공격력 등)를 가진 `Actor` 기본 클래스를 정의하고 공통 동작(이동, 점프, 공격)을 처리합니다.
  * 이를 `player`, `Monster`, `Boss`가 상속받아 각각의 고유 상태(State 머신)와 동작(장비 관리, 아이템/골드 드롭 등)을 세분화하여 처리합니다.
* **Map & Room 시스템**
  * `MapManager` (싱글톤)가 전체 맵의 `Room`을 생성하고 `door` 객체를 통해 서로 연결(Link)합니다. 
  * 각 `Room`은 `RoomInfo`를 바탕으로 몬스터와 보물상자(Chest)를 동적으로 배치합니다.
* **Collision 시스템**
  * 맵(타일)과의 충돌 판정, 공격 판정, 피격 판정을 독립적인 모듈로 분리하여 전담합니다.

## 🚀 주요 기능 (Features)
- **객체지향 설계**: 상속과 컴포지션을 활용한 유연한 엔티티(Entity) 및 맵 구조 설계
- **스마트 포인터 활용 메모리 관리**: `std::unique_ptr`를 사용해 불필요한 복사를 막고 안전한 메모리 해제 보장
- **JSON 파싱 기반 아틀라스 로딩**: 텍스처 아틀라스 이미지와 JSON 데이터를 읽어 개별 프레임 영역과 애니메이션 클립을 자동 추출 및 매핑
- **안전한 리소스 예외 처리**: 요청한 리소스가 없을 경우 프로그램 크래시 대신 `nullptr`을 반환하여 유연한 에러 헨들링 및 게임 속행 보장

## 🛠️ 외부 라이브러리 및 출처 (Dependencies & Acknowledgments)
본 프로젝트는 다음의 오픈소스 라이브러리를 활용하여 작성되었습니다.

* **[SFML (Simple and Fast Multimedia Library)](https://www.sfml-dev.org/)**
  * **용도**: 그래픽 처리, 렌더링 및 윈도우 관리
  * **라이센스**: [zlib/png License](https://opensource.org/licenses/Zlib) (상업적 이용 및 수정 자유, 출처 표기 권장)
* **[JSON for Modern C++ (nlohmann/json)](https://github.com/nlohmann/json)**
  * **용도**: 텍스처 아틀라스 구성 정보가 담긴 `.json` 파일 로드 및 데이터 파싱
  * **라이센스**: [MIT License](https://opensource.org/licenses/MIT) (상업적 이용 자유, 저작권 및 라이센스 고지 의무)

## 📄 라이센스 (License)
이 프로젝트의 코드는 **MIT License**에 따라 배포됩니다.
누구나 자유롭게 코드의 열람, 수정, 배포 및 상업적 이용이 가능합니다. (완전 자유 라이센스)
자세한 내용은 `LICENSE` 파일을 참고해주세요.
README.md
README.md 항목을 표시하는 중입니다.