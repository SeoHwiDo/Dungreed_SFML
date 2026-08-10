# 비규격 애니메이션 장식 타일

방의 충돌 격자와 별개로 장식을 추가하려면 `room_data.json`의 `roomReferences[].layout` 항목에
`decorations` 배열을 작성한다. 장식은 `TileMap`의 충돌 목록에 추가되지 않으므로 플레이어,
몬스터, 투사체의 이동이나 판정에 영향을 주지 않는다.

```json
"decorations": [
  {
    "atlasKey": "Effect",
    "animation": "Torch",
    "position": [8.5, 4.0],
    "offset": [0, 0],
    "scale": [1.0, 1.0],
    "frameDuration": 0.12,
    "isLoop": true,
    "drawAboveTiles": false
  },
  {
    "atlasKey": "TileMap",
    "frame": "Statue.png",
    "position": [15.25, 12.0],
    "drawAboveTiles": true
  }
]
```

- `frame`은 정지 장식, `animation`은 `이름-00.png`, `이름-01.png`처럼 등록된 프레임 묶음이다.
- `position`은 타일 단위 좌표이므로 소수점으로 격자 밖 위치를 지정할 수 있다. 스프라이트의
  JSON 피벗이 이 좌표에 맞춰진다.
- `offset`은 픽셀 보정값, `scale`은 원본 크기 배율이다.
- `drawAboveTiles`가 `false`이면 배경 타일 위·벽과 플랫폼 아래, `true`이면 벽·플랫폼 위에
  그린다. 두 경우 모두 액터보다 먼저 렌더링된다.
- `atlasKey`는 `main.cpp`에서 `ResourceManager::loadAtlas`로 먼저 로드한 아틀라스 키여야 한다.
