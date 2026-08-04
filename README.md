## 🛠️ 외부 라이브러리 및 출처 (Dependencies & Acknowledgments)
본 프로젝트는 다음의 오픈소스 라이브러리를 활용하여 작성되었습니다.

* **[SFML (Simple and Fast Multimedia Library)](https://www.sfml-dev.org/)**
  * **용도**: 그래픽 처리 및 렌더링
  * **라이센스**: [zlib/png License](https://opensource.org/licenses/Zlib) (상업적 이용 및 수정 자유, 출처 표기 권장)
* **[JSON for Modern C++ (nlohmann/json)](https://github.com/nlohmann/json)**
  * **용도**: 아틀라스 구성 정보가 담긴 JSON 파일 로드 및 데이터 파싱
  * **라이센스**: [MIT License](https://opensource.org/licenses/MIT) (상업적 이용 자유, 저작권 및 라이센스 고지 의무)

## 📄 라이센스 (License)
이 프로젝트의 코드는 **MIT License**에 따라 배포됩니다.
누구나 자유롭게 코드의 열람, 수정, 배포 및 상업적 이용이 가능합니다. (완전 자유 라이센스)
자세한 내용은 `LICENSE` 파일을 참고해주세요.

# Dungreed_SFML
sfml을 활용한 던그리드 게임 모작

## 🏗️ 시스템 아키텍처 (Class Diagram)
![Class Diagram](class.png)

위 클래스 다이어그램은 게임의 전반적인 구조를 나타냅니다:
* **ResourceLoader (ResourceManager)**: 싱글톤으로 구현되어 텍스처, 타일셋 등 게임 내 모든 그래픽 자원을 관리합니다.
* **Actor & 상속 구조**: `Status`(체력, 공격력 등)를 가진 `Actor` 기본 클래스를 정의하고, 이를 `player`, `Monster`, `Boss`가 상속받아 각각의 고유 상태와 동작(장비, 아이템 드롭 등)을 처리합니다.
* **Map & Room 시스템**: `MapManager`가 전체 맵의 `Room`을 생성하고 `door`를 통해 서로 연결(link)합니다. 각 `Room`은 `RoomInfo`를 바탕으로 몬스터와 보물상자를 배치합니다.
* **Collision 시스템**: 맵 충돌, 공격 판정, 피격 판정을 전담하여 처리합니다.