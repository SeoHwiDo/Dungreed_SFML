# Dungreed 구조와 호출 흐름 안내서

이 문서는 코드에 남긴 짧은 한글 주석을 보완하기 위한 설계 문서입니다.  
새 기능을 추가할 때 **어느 클래스의 어떤 함수를 호출해야 하는지**, 그리고 각 함수가 다른 시스템에 어떤 영향을 주는지를 설명합니다.

## 1. 전체 프레임 흐름

프로그램은 `main.cpp`에서 리소스와 방을 한 번 준비한 뒤, 창이 열려 있는 동안 아래 순서로 반복합니다.

```text
리소스 로드
  ↓
방 레이아웃 생성 → TileMap 생성 → 플레이어/몬스터 스폰
  ↓
매 프레임
  입력 수집 → 상태/AI 판단 → 물리·피격 갱신 → 공격 판정 → 맵 충돌 보정 → 렌더링
```

`Player::update` 내부는 다음 순서를 유지합니다.

1. `Controller::getInput`으로 키보드·마우스 입력을 `InputData`로 바꿉니다.
2. `Player::handleState`가 입력과 현재 상태를 해석합니다.
3. `Actor::updatePhysics`가 중력, 일반 이동 속도, 넉백 속도를 합쳐 이동합니다.
4. `Actor::updateHitFeedback`이 피격 색상과 넉백 시간을 갱신합니다.
5. `Actor::updateAnimation`이 현재 애니메이션 프레임과 피격 상자를 갱신합니다.

맵 타일과의 최종 충돌 보정은 `main.cpp`의 `Collision::resolveMapCollision`에서 수행합니다. 이 분리는 입력/AI가 이동 의도를 정하고, 충돌 시스템이 실제 가능한 위치를 확정하게 하기 위한 것입니다.

## 2. 리소스와 애니메이션

### ResourceManager

`ResourceManager`는 싱글턴입니다. 게임 시작 시 다음처럼 아틀라스를 한 번 등록합니다.

```cpp
auto& resources = ResourceManager::getInstance();
resources.loadAtlas("Player", PLAYER_JSON, PLAYER_ATLAS);
```

`loadAtlas`는 TexturePacker JSON의 각 프레임에서 다음 정보를 `AtlasData`에 보관합니다.

| 정보 | 조회 함수 | 주 사용처 |
| --- | --- | --- |
| 아틀라스 텍스처 | `getAtlasTexture` | `sf::Sprite`, `TileMap` |
| 잘린 텍스처 영역 | `getFrameRect` | 단일 스프라이트, 타일 |
| 원본 피벗 | `getFramePivot` | 무기 원점 |
| 자르기 전 원본 크기 | `getFrameSourceSize` | 타일 셀 크기 |
| 이름별 프레임 배열 | `getAnimationFrames` | `Animator` |

`getFramePivot`과 `getFrameSourceSize`는 데이터가 없을 수 있으므로 `std::optional`을 반환합니다. 호출자는 `if (pivot)` 또는 `value_or(...)`로 대체값을 명시해야 합니다. 무기는 피벗이 없으면 프레임 중앙을 원점으로 사용하고, 타일맵은 `sourceSize`가 없으면 생성에 실패합니다. 타일의 크기가 임의로 달라지면 충돌 격자가 깨질 수 있기 때문입니다.

### Animator

`Animator`는 스프라이트를 직접 소유하지 않습니다. 대신 애니메이션 이름과 프레임 배열을 등록하고, 매 프레임 대상 스프라이트의 `TextureRect`만 변경합니다.

```cpp
animator.addAnimation("Player_Run", AnimationClip(frames, 0.05f, true));
animator.play("Player_Run");
animator.update(dt, sprite);
```

- `addAnimation`: 아틀라스에서 얻은 프레임 배열을 이름으로 등록합니다.
- `play`: 시간을 0으로 초기화하고 첫 프레임부터 재생합니다.
- `update`: 누적 시간으로 프레임을 전진시킵니다. 비루프 클립은 마지막 프레임에 멈춥니다.
- `isFinished`: 공격이나 사망처럼 비루프 애니메이션이 끝났는지 확인할 때 사용합니다.

`Actor::playAnimation`은 이미 같은 이름의 애니메이션을 재생 중이면 `Animator::play`를 다시 호출하지 않습니다. 같은 상태가 매 프레임 반복되어도 첫 프레임으로 되감기지 않게 하는 장치입니다.

## 3. Actor 공통 물리와 피격 처리

### 좌표와 충돌 기준

`Actor`의 스프라이트 원점은 `setBottomCenterOrigin`으로 **발밑 중앙**에 맞춥니다. 따라서 `move(x, y)`는 발밑을 기준으로 이동시키며, 플레이어/몬스터 스폰 좌표도 타일의 발밑 중앙 기준으로 계산해야 합니다.

`getCenterPosition`은 조준과 거리 계산용 스프라이트 중심을, `getBodyCenterPosition`은 무기 배치용 몸통 중심을 반환합니다. 무기를 그릴 때 스프라이트 전체 중앙을 쓰면 발밑 여백이나 애니메이션 프레임 차이 때문에 위치가 어색해질 수 있습니다.

### 피격과 넉백

피해를 줄 때는 가능한 한 공격자 좌표를 함께 전달합니다.

```cpp
target.takeDamage(damage, attacker.getCenterPosition());
```

`Actor::takeDamage(damage, attackerPosition)`은 다음을 처리합니다.

1. 이미 사망했다면 추가 처리를 막습니다.
2. 체력을 감소시킵니다.
3. 공격자 반대 방향의 `m_knockbackVelocity`를 만듭니다.
4. 넉백과 피격 색상 타이머를 시작합니다.
5. 콘솔에 피해량, 남은 체력, 공격자 위치, 넉백 속도를 출력합니다.

이후 `updatePhysics`는 일반 `movement.velocity`와 넉백 속도를 더해 이동합니다. `updateHitFeedback`은 시간이 끝나면 넉백을 제거하고 스프라이트 색을 흰색으로 복구합니다.

공격자 위치를 모르는 기존 호출은 `takeDamage(damage)`를 사용해도 됩니다. 이 경우 대상의 왼쪽에서 맞은 것으로 간주하여 기본 넉백 방향을 제공합니다.

### 맵 충돌

`Collision::resolveMapCollision(actor, map)`은 `TileMap::getCollisionTiles()`의 각 타일과 액터 경계를 비교합니다.

- 가로 겹침이 더 작으면 좌우 벽 충돌로 판단하고 X축 위치·속도를 보정합니다.
- 세로 겹침이 더 작으면 바닥/천장 충돌로 판단합니다.
- 아래로 이동 중인 액터가 타일 위와 닿으면 Y 속도를 0으로 만들고 `isGrounded`를 `true`로 설정합니다.
- `OneWay` 타일은 아래에서 부딪힐 때 통과하지만 위에서 떨어질 때는 착지합니다.

물리 이동 뒤에 이 함수를 호출해야 합니다. 충돌 전에 호출하면 아직 이전 프레임의 위치를 보정하게 됩니다.

## 4. 플레이어 입력과 상태

`Controller::getInput(window, actorCenter)`은 다음 정보를 `InputData`에 채웁니다.

- 좌우 이동: 방향키 또는 `A`/`D`
- 점프: `Space`
- 대시 입력: `Left Shift` 또는 마우스 오른쪽 버튼
- 공격: 마우스 왼쪽 버튼을 뗀 순간의 단발 입력
- 조준: 액터 중심에서 마우스 월드 좌표까지의 단위 벡터와 `atan2` 라디안

`Player::handleState`는 이 입력을 실제 행동으로 번역합니다. 좌우 입력은 `setHorizontalInput`으로 전달되고, 점프는 착지 상태일 때만 `jump`를 호출합니다. 조준 방향 X값은 플레이어 스프라이트의 좌우 반전에 사용하며, 조준 라디안은 장비에 전달합니다.

### 사망 상태

체력이 0 이하가 되면 `Player::update`는 입력 수집을 건너뛰고 `Dead` 상태로 전환합니다. 이 상태에서는 다음을 보장합니다.

- 모든 이동 입력과 공격 입력이 무효입니다.
- 일반 수평 속도, 점프 속도, 넉백 속도가 제거됩니다.
- 중력만 계속 적용됩니다.
- `Player_Die` 애니메이션은 한 번 시작되어 마지막 프레임에 멈춥니다.

따라서 사망 상태 추가 시에는 `handleState`에 새 입력 처리를 넣지 말고, `changeState(PlayerState::Dead)`에서 물리값 초기화와 애니메이션 시작을 처리해야 합니다.

## 5. 몬스터 FSM과 공격

몬스터는 `Idle`, `Patrol`, `Chase`, `Attack`, `Dead` 상태를 가집니다. `Monster::update`는 매 프레임 아래 순서로 실행됩니다.

1. `handleFSM`이 플레이어 거리와 상태 타이머를 바탕으로 다음 행동을 결정합니다.
2. `Actor::update`가 몬스터의 이동, 중력, 피격, 애니메이션을 처리합니다.
3. `Chase` 상태에서만 플레이어 **본체 충돌 상자**와 겹쳤는지 검사합니다.
4. 겹쳤다면 둘을 분리하고 플레이어에게 피해/넉백을 적용한 뒤 `Attack` 상태로 바꿉니다.

몬스터의 공격은 플레이어 장비의 스프라이트나 공격 판정을 사용하지 않습니다. `player.getCollision().checkHit(getGlobalBounds())`처럼 플레이어 본체와 몬스터 본체의 충돌만 사용해야, 무기를 향해 달려간 몬스터가 부당하게 피해를 주는 문제를 막을 수 있습니다.

사망 애니메이션은 몬스터 종류별 이름이 아니라 공통 `Monster_Die`를 등록·재생합니다. 새 몬스터 타입을 추가할 때도 해당 타입의 Idle/Run/Attack 프레임과 `Monster_Die` 프레임을 같은 아틀라스에 포함하면 됩니다.

## 6. 장비 배치와 스윙

장비는 생성 후 `init(atlasKey, frameName)`으로 스프라이트를 준비하고, `Actor::setEquipment`로 소유자와 연결합니다.

```cpp
auto sword = std::make_shared<Equip>("ShortSword", stats);
sword->init("Equip", "ShortSword");
player.setEquipment(sword);
```

`Equip::init`은 JSON 피벗을 스프라이트 원점으로 사용합니다. 그러므로 회전은 이미지 중앙이 아니라 실제 손잡이/피벗을 기준으로 이루어집니다.

`Equip::update(dt, ownerBodyCenter, aimRadian)`은 다음을 계산합니다.

1. 조준 라디안의 절대값으로 좌우 방향을 판별합니다.
2. 몸통 중심에서 전방으로 이동한 위치에 무기를 둡니다.
3. 무기 원본이 향한 방향을 보정한 기본 회전을 만듭니다.
4. 공격 중이면 시간 비율로 시작 각도에서 끝 각도까지 한 번만 보간합니다.
5. 좌측일 때 Y축 스케일을 반전하여 대칭 자세를 만듭니다.

`attack`은 새 스윙을 시작하고, `consumeHit`은 한 번의 스윙에서 첫 피해만 허용합니다. 공격 판정은 다음처럼 사용합니다.

```cpp
if (weapon->isAttacking()) {
    if (const auto attackBox = player.getAttackHitbox();
        attackBox && monster.getCollision().checkHit(*attackBox)
        && weapon->consumeHit(monster.getId())) {
        monster.takeDamage(weapon->getStat().damage, player.getCenterPosition());
    }
}
```

`consumeHit`은 충돌이 실제로 확인된 뒤 호출해야 합니다. 먼저 호출하면 빗나간 스윙도 이미 적중 처리되어 이후 프레임의 실제 충돌을 놓치게 됩니다.

## 7. 타일맵

`TileMap::load`는 `TileConfig` 배열을 받아 두 종류의 버텍스 배열을 만듭니다.

- `isBackground == false`: 일반 벽·바닥·플랫폼. 화면 앞에 그리고, `TileType`에 따라 충돌 타일을 생성합니다.
- `isBackground == true`: 백타일. 어둡게 렌더링하지만 충돌은 만들지 않습니다.

타일 크기는 호출자가 넣지 않습니다. 첫 유효 프레임의 `sourceSize`를 읽어 `m_tileSize`로 정합니다. 모든 방 타일이 같은 원본 크기를 사용한다는 전제이므로, 새 타일은 같은 sourceSize를 유지해야 합니다.

`rotationQuarterTurns`는 텍스처 좌표의 순서만 바꿔 90도 단위 회전을 구현합니다. 물리 충돌 사각형은 회전과 무관하게 한 셀 전체를 사용합니다.

렌더링 순서는 다음과 같습니다.

```text
단일 배경 이미지 → 어두운 백타일 → 일반 타일 → 액터/장비
```

## 8. 방 레퍼런스와 문

### RoomCell의 의미

`RoomLayout`은 숫자 대신 `RoomCell` 열거형으로 셀의 역할을 보관합니다.

| 셀 | 역할 | 충돌 |
| --- | --- | --- |
| `Empty` | 방 바깥의 빈 공간 | 없음 |
| `Ceiling`, `Ground`, `LeftWall`, `RightWall` | 방 외곽 벽 | Solid |
| `Platform` | 공중 발판 | OneWay |
| `BackTile`, `SpawnPoint`, `Door` | 시각 전용 내부/문 공간 | 없음 |
| `*Corner` | 벽의 모서리 | Solid |

`Room::getReferenceLayout`은 방 종류별 기본 형태를 만들고, 플랫폼은 길이 3~5타일로 배치합니다. 보스방은 일반 방보다 가로 폭이 두 배입니다.

### DoorPositions와 문 확장

`DoorPositions`는 `{ Up, Down, Left, Right }` 순서의 `std::array<bool, 4>`입니다.

```cpp
DoorPositions doors{ true, false, true, false };
Room room(RoomType::Monster, doors);
```

`applyDoorways`는 `true`인 방향에만 3타일 크기의 통로를 만듭니다. 상하 문은 방 중앙, 좌우 문은 바닥 바로 위에 놓습니다. 통로 주위의 외벽·바닥·코너 셀도 함께 채우므로, 문 바깥 확장 영역으로 빠져나갈 수 없습니다.

### 벽과 백타일 선택

`Room::buildTileMap`은 `RoomTileSet`에 전달된 프레임 이름으로 논리 셀을 실제 타일로 바꿉니다.

- 외벽: `Wall_Top`, `Wall_Ground`, `Wall_Left`, `Wall_Right`
- 일반 코너: `Wall_TopLCorner`, `Wall_TopRCorner`, `Wall_BotLCorner`, `Wall_BotRCorner`
- 문 입구 경계 코너: 하단 `Wall_H0/H2`, 상단 `Wall_H6/H8`, 좌우는 방향별 H 코너 조합
- 백타일: `Back_Inner`, `Back_Top`, `Back_Ground`, `Back_Left`, `Back_Right`
- 문 그림자: `Back_DoorTopL`, `Back_DoorTopR`, `Back_DoorBotL`, `Back_DoorBotR`

특히 문은 `Door` 셀만 바꾸지 않습니다. 방 내부와 문 통로가 처음 맞닿는 백타일도 `getBackFrame`이 검사하여 방향별 그림자 타일이나 모서리 그림자 타일로 교체합니다. 이 처리가 없으면 벽 그림자가 문 안쪽에서 끊기거나, 문 주변이 평면적으로 보입니다.

### 최근 문 내부 타일 규칙

상하 문은 문 방향에 맞춰 방 내부 첫 백타일 줄을 처리합니다.

- 아래 문: 바깥 경계는 `Back_Ground`, 입구 양끝은 `Back_DoorBotL/R`, 중앙은 `Back_Inner`
- 위 문: 바깥 경계는 `Back_Top`, 입구 양끝은 `Back_DoorTopL/R`, 중앙은 `Back_Inner`
- 왼쪽 문: 위쪽 경계는 `Back_Left`, 입구 그림자는 `Back_DoorTopRight`, 중앙은 `Back_Inner`, 아래쪽은 `Back_Ground`
- 오른쪽 문: 위쪽 경계는 `Back_Right`, 입구 그림자는 `Back_DoorTopLeft`, 중앙은 `Back_Inner`, 아래쪽은 `Back_Ground`

좌우 문의 하단 내부 타일을 `Back_Ground`로 유지하는 것은 바닥 그림자가 이어져야 한다는 예외 규칙입니다. H 모서리 벽은 일반 `Ground`·`Ceiling`·코너 셀을 유지하면서 `buildTileMap`이 방향별 `Wall_H0/H2/H6/H8` 프레임으로 덮어씁니다.

### 스폰 좌표

`getPlayerSpawnPosition`은 `SpawnPoint`를 타일의 발밑 중앙 좌표로 바꿉니다.  
`getMonsterSpawnPosition`은 백타일 중에서 바로 아래가 바닥 또는 플랫폼인 셀만 후보로 선택합니다. 현재는 후보 배열의 가운데를 고르므로 결과가 재현 가능하며, 나중에 난수 선택으로 확장할 수 있습니다.

## 9. 방 전체 디버그 보기

`MapManager`는 실제 플레이용 `TileMap`을 바꾸지 않습니다. 디버그 전용 TileMap을 별도로 만들어 모든 `RoomType`을 축소 배치합니다.

```cpp
if (SHOW_ALL_ROOMS_DEBUG) {
    mapManager.buildAllRoomsDebug("TileMap", roomTiles, window.getSize());
    mapManager.renderAllRoomsDebug(window);
}
```

`buildAllRoomsDebug`은 창 너비를 넘으면 다음 줄로 이동해 방이 겹치지 않도록 배치합니다. 방 타일이 하나라도 생성되지 않으면 이전 미리보기를 비우고 `false`를 반환합니다. 따라서 실패 시 부분적으로 잘못된 디버그 화면이 남지 않습니다.

## 10. 기능 추가 시 확인 순서

새 기능을 넣을 때는 아래 순서로 확인하면 시스템 간 누락을 줄일 수 있습니다.

1. 새 스프라이트가 필요하면 JSON과 이미지 경로를 `ResourceManager::loadAtlas`에 등록합니다.
2. 애니메이션이면 `init`에서 프레임을 `Animator::addAnimation`으로 등록합니다.
3. 새 상태라면 상태 열거형, `changeState`, 상태 처리 함수, 애니메이션 이름을 함께 추가합니다.
4. 이동이나 피격이 바뀌면 `Actor::updatePhysics`와 `Collision::resolveMapCollision`의 호출 순서를 유지합니다.
5. 새 방 타일이면 `RoomTileSet`, `RoomCell` 변환, 필요 시 `getBackFrame`의 그림자 규칙을 함께 갱신합니다.
6. 타일 원본 크기가 기존과 다른지 확인합니다. `TileMap`은 첫 타일의 `sourceSize`를 전체 셀 크기로 사용합니다.
7. 마지막으로 Debug x64 빌드와 방 전체 디버그 보기를 확인합니다.

## 11. 다중 몬스터·원거리 장비·오브젝트 풀

### 책임 분리

- `ObjectPoolingManager`가 `Monster`와 `Projectile`의 생성, 소유, 재사용, 비활성화를 담당합니다. 매니저가 객체를 `delete`하거나 벡터에서 제거하지 않으므로 포인터가 프레임 사이에 안정적으로 유지됩니다.
- `MonsterManager`는 풀에 있는 활성 몬스터를 순회해 AI와 물리를 실행하고, 각 몬스터가 이동한 직후 `Collision::resolveMapCollision`으로 벽을 먼저 해결합니다.
- `CombatManager`는 객체를 소유하지 않고 공격 상호작용만 계산합니다. 따라서 몬스터 수가 늘어나도 생성 정책과 충돌 정책이 섞이지 않습니다.

### 장비 기반 원거리 공격

`EquipStat::type`이 `WeaponType::Ranged`이고 `projectile` 설정이 있으면 `Equip::attack`은 스윙 대신 요청을 예약합니다. `consumeProjectileRequests`가 다음 값을 요청마다 복사합니다.

1. 투사체 종류와 대상 그룹
2. 소유자 몸통 중심에서 시작하는 위치
3. 마우스/타깃 방향의 라디안과 산탄 간격
4. 속도, 피해량, 수량, 수명

`CombatManager`는 이 요청을 `ObjectPoolingManager::acquireProjectile`에 전달할 뿐, 발사자를 저장하지 않습니다. `ProjectileTarget::Player` 또는 `ProjectileTarget::Monster`가 충돌 대상을 결정하므로 발사자 ID가 없어도 팀별 판정이 가능합니다.

### 동일 프레임 우선순위

메인 루프는 다음 순서를 고정합니다.

1. 몬스터와 플레이어를 이동시킨 뒤 벽 충돌을 해결합니다. 투사체도 `Collision::resolveProjectileMapCollision`에서 이전 위치와 현재 위치 사이를 샘플링해 벽 통과를 막습니다.
2. 플레이어의 근접 공격을 검사하고 원거리 투사체를 생성합니다.
3. 몬스터 대상 투사체를 갱신합니다. 이 단계에서 실제로 맞은 몬스터 ID를 `playerHitMonsters`에 추가합니다.
4. 몬스터 공격을 처리할 때 해당 집합에 포함된 몬스터만 이번 프레임 공격을 건너뜁니다. 다른 몬스터의 공격과 이미 발사되어 이동 중인 투사체는 무효화하지 않습니다.
5. 마지막으로 플레이어 대상 투사체를 갱신합니다. 따라서 원거리 몬스터의 투사체는 발사자와 무관하게 계속 유효합니다.

`Equip::m_hitTargets`는 한 번의 근접 스윙에서 같은 몬스터가 여러 프레임/여러 검사로 중복 피격되는 것을 막고, `EntityId`는 풀에서 재사용되는 객체도 프레임 내 대상 집합에서 구분할 수 있게 합니다.

### 풀 선생성과 비활성 우선순위 큐

`ObjectPoolingManager::prewarmMonsters`와 `prewarmProjectiles`는 게임 시작이나 방 진입 시 필요한 수만큼 객체를 먼저 생성하고, 렌더링·업데이트 대상이 아닌 비활성 슬롯으로 보관합니다.

```cpp
objectPool.prewarmMonsters(4, "SkelDog", { 100.f, 100.f, 10.f, 1.f }, "Monster");
objectPool.prewarmProjectiles(32);
```

풀은 비활성 슬롯의 벡터를 매 요청마다 선형 탐색하지 않습니다. 슬롯 번호와 비활성화 순번을 `std::priority_queue` 최소 힙에 넣고, `acquire` 시 가장 오래 비활성 상태였던 슬롯을 먼저 꺼냅니다. 선생성된 객체도 생성 순서대로 큐에 들어가므로 첫 요청부터 순서대로 재사용됩니다. 큐가 비면 그때만 새 슬롯을 만들고 활성화합니다. `release`는 중복 반환을 막은 뒤 슬롯을 다시 큐에 넣습니다.
