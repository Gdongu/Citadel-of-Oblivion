# 망각의 성채 · Citadel of Oblivion

**적의 패턴을 읽고, 회피와 반격으로 돌파하는 소울라이크 액션 게임**

[프로젝트 홈페이지](https://kdw1102.imweb.me/CitadelofOblivion) · [플레이 영상](https://www.youtube.com/watch?v=qGrMPiaB-7g)

[![성채에서 보스와 맞서는 플레이어](https://cdn-optimized.imweb.me/v2/brand/S20260802eaf9fc95f5402/5ea4c0ed-ecc0-4988-a462-b1c217202742)](https://www.youtube.com/watch?v=qGrMPiaB-7g)

## 프로젝트 개요

| 항목 | 내용 |
| --- | --- |
| 개발자 | 김동우 |
| 제작 기간 | 2026.03 ~ 2026.05 |
| 개발 형태 | 개인 프로젝트 · 1명 |
| 장르 / 플랫폼 | 소울라이크 액션 / PC |
| 엔진 / 언어 | Unreal Engine 5.6 / C++, Blueprint |

팀 프로젝트 **DAWN**의 소울라이크 제작 경험을 바탕으로 전투·AI·UI·저장 시스템을 C++ 중심으로 재구성한 개인 프로젝트입니다. 공격과 피격, 사망과 휴식처럼 여러 기능이 맞물리는 상황에서 애니메이션·타격 판정·게임 상태를 연결하는 데 집중했습니다.

홈페이지에서 플레이 결과와 회고를 확인할 수 있으며, 이 저장소에서는 주요 구현의 소스 코드를 확인할 수 있습니다.

## 담당 역할

- **플레이어:** 이동, 3단 콤보, 회피, 락온, 스태미너, 백어택, 스킬
- **적과 보스:** 일반 몬스터 행동, 보스 공격 패턴, 그로기, 최종 보스 페이즈 전환
- **성장과 아이템:** 인벤토리, 장비, 상점, 능력치·포션 강화
- **진행 시스템:** 사망 패널티, 재화 드랍·회수, 화톳불 휴식·이동, 저장·불러오기
- **UI와 연출:** HUD, 인벤토리·메뉴 UI, 보스 및 엔딩 연출 연동

## 사용 기술

| 분야 | 기술 |
| --- | --- |
| 입력·캐릭터 | C++, Blueprint, Enhanced Input |
| 애니메이션·전투 | Animation Blueprint, Animation Montage, Anim Notify, Socket, Sphere Trace |
| AI | AIController, Behavior Tree, Blackboard, AI Perception, Navigation |
| 데이터·저장 | DataTable, Actor Component, GameInstance, SaveGame, Delegate |
| UI·연출 | UMG, Niagara, Level Sequence |

## 핵심 구현과 코드 안내

### 1. 입력 예약 기반 3단 콤보와 공격 판정

공격 중 들어온 입력을 예약하고, 공격 종료 시 예약 여부와 스태미너를 확인해 다음 몽타주 섹션으로 연결합니다. 공격이 끝난 뒤에도 콤보 유지 시간 안에 입력하면 후속 공격을 이어갈 수 있습니다.

무기의 두 소켓 사이에 Sphere Trace를 수행하고, 이미 타격한 액터를 기록해 한 공격 구간에서 피해가 중복 적용되지 않도록 처리합니다. 벽과 몬스터의 검출 거리를 비교하는 무기 튕김 처리도 연결했습니다.

| 확인할 내용 | 주요 함수 | 코드 |
| --- | --- | --- |
| 입력 예약·콤보 연결 | `Attack`, `PlayAttackSection`, `OnAttackEnded` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L589-L799) |
| 타격 검사·중복 피해 방지 | `StartAttackTrace`, `PerformAttackTrace` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L1214-L1344) |

### 2. 피격 시 행동 중단과 상태 정리

피격 처리에서 무적, 슈퍼아머, 보스 강공격을 구분합니다. 일반 피격으로 행동을 중단할 때는 공격 플래그뿐 아니라 예약 입력, 공격 판정, 타이머, 이동과 몽타주를 함께 정리하는 처리를 두었습니다.

몬스터 역시 피격으로 공격을 취소할 때 공격 판정과 Blackboard 상태를 함께 갱신합니다. **공격 모션이 중단된 뒤에도 타격 판정이 남는 상황**을 다루면서, 애니메이션과 판정의 종료 처리를 함께 관리했습니다.

| 확인할 내용 | 주요 함수 | 코드 |
| --- | --- | --- |
| 피해 종류·상태별 분기 | `TakeDamage` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L912-L1048) |
| 플레이어 행동 중단 | `InterruptCurrentAction` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L1491-L1617) |
| 몬스터 공격 취소 | `CancelAttackForPlayerHit` | [BaseMonster.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/BaseMonster.cpp#L673-L709) |

### 3. 보스 공격 예약과 텔레포트 우선순위

최종 보스 2페이즈는 예약된 공격과 플레이어와의 거리에 따라 행동합니다. 근접 공격이 예약된 경우 텔레포트보다 접근을 우선하고, 공격 범위에 들어오면 예약된 패턴을 실행합니다.

공격·그로기·다운 중에는 텔레포트를 제한합니다. 그로기 진입 시에는 공격 루프와 지속 피해 타이머 등을 정리하고, 기상 과정이 끝난 뒤 전투를 재개하도록 상태를 나누었습니다.

| 확인할 내용 | 주요 함수 | 코드 |
| --- | --- | --- |
| 예약된 근접 공격 실행 순서 | `Tick` | [FinalBossPhase2.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/FinalBossPhase2.cpp#L53-L90) |
| 텔레포트 조건·목적지·실행 | `CanPhase2Teleport`, `FindTeleportLocation`, `DoTeleportTo` | [FinalBossPhase2.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/FinalBossPhase2.cpp#L208-L431) |
| 그로기 진입·기상 상태 | `EnterGroggy`, `ExitGroggy` | [TutorialBossMonster.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/TutorialBossMonster.cpp#L215-L336) |
| 최종 보스 페이즈 전환 | `StartPhaseTransition`, `TravelToPhase2` | [FinalBossPhase1.cpp](Private/FinalBossPhase1.cpp) |

### 4. 진행 상태 저장과 복원

GameInstance에서 플레이 중 진행 데이터를 관리하고, SaveGame 객체로 복사해 디스크에 저장합니다. 체크포인트, 활성화된 화톳불, 재화, 인벤토리, 장비, 강화 정보를 저장하며 문·상자·보스·튜토리얼 등의 진행 여부는 ID 목록으로 관리합니다.

인벤토리 복원 후에는 장비 능력치와 무기 외형을 다시 적용해 데이터와 캐릭터 상태를 연결합니다.

| 확인할 내용 | 주요 함수 / 자료형 | 코드 |
| --- | --- | --- |
| 디스크 저장·불러오기 | `SaveProgressToDisk`, `LoadProgressFromDisk` | [FortressGameInstance.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/FortressGameInstance.cpp#L480-L688) |
| 저장 데이터 정의 | `UFortressSaveGame` | [FortressSaveGame.h](Public/FortressSaveGame.h) |
| 장비 데이터·능력치·외형 복원 | `LoadInventoryFromGameInstance`, `ReapplyEquippedItemBonusesFromInventory` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L3490-L3607) |

### 5. 아이템 데이터와 인벤토리·장비 연동

DataTable 행 구조인 `FItemData`에 아이템 종류, 장착 부위, 능력치, 무기 외형과 스킬 설정을 정의했습니다. `UInventoryComponent`는 획득·소비·장착·해제를 담당합니다.

장비 교체 시 기존 장비의 능력치 보너스를 제거하고 새 장비의 보너스를 적용합니다. 변경 시 `OnInventoryUpdated` 델리게이트를 발생시키고, 플레이어에서 무기 외형 갱신과 인벤토리 저장으로 연결합니다.

| 확인할 내용 | 주요 함수 / 자료형 | 코드 |
| --- | --- | --- |
| 아이템·장비·무기 설정 | `FItemData`, `FInventoryItem` | [ItemTypes.h](Public/ItemTypes.h) |
| 장비 교체·해제 | `EquipItem`, `UnequipItem` | [InventoryComponent.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/InventoryComponent.cpp#L139-L217) |
| 변경 알림과 저장 연결 | `HandleInventoryUpdated` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L3603-L3607) |

### 6. 사망 재화 드랍·회수와 위치 예외 처리

사망 시 재화 수량과 드랍 레벨·위치를 기록하고, 해당 레벨에 진입하면 드랍 액터를 생성합니다. 회수하면 플레이어에게 재화를 돌려주고 저장된 드랍 정보를 제거합니다.

KillZone에 의한 사망에서는 최근 지상 위치 기록을 활용합니다. 최종 보스 2페이즈에서 사망한 경우에는 1페이즈 레벨의 지정 지점에 재화를 배치하도록 별도 처리했습니다.

| 확인할 내용 | 주요 함수 | 코드 |
| --- | --- | --- |
| 사망 드랍 기록·레벨 진입 후 생성 | `DropMemoryShardsOnDeath`, `SpawnPendingMemoryShardDropFromGI` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L3278-L3382) |
| 최근 지상 위치 기록·선택 | `UpdateRecentSafeTransforms`, `GetSafeMemoryShardDropTransform` | [Knight.cpp](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/Knight.cpp#L3983-L4035) |
| 드랍 재화 회수 | `Interact_Implementation` | [MemoryShardDrop.cpp](Private/MemoryShardDrop.cpp) |

## 주요 클래스 구조

소스는 `Public/`의 헤더와 `Private/`의 구현 파일로 구성되어 있습니다.

| 클래스 / 파일 | 역할 |
| --- | --- |
| [Knight](Public/Knight.h) | 플레이어 입력·전투·상호작용 및 UI·진행 데이터 연동 |
| [BaseMonster](Public/BaseMonster.h) | 몬스터 공통 전투, 타깃 획득, 귀환, 드랍 |
| [MonsterAIController](Public/MonsterAIController.h) | AI Perception과 Blackboard 상태 연결 |
| [TutorialBossMonster](Public/TutorialBossMonster.h) | 보스 콤보·강공격·잡기·광역·원거리 패턴과 그로기 |
| [FinalBossPhase1](Public/FinalBossPhase1.h) / [FinalBossPhase2](Public/FinalBossPhase2.h) | 최종 보스 페이즈 전환과 2페이즈 행동 |
| [InventoryComponent](Public/InventoryComponent.h) / [ItemTypes](Public/ItemTypes.h) | 인벤토리 기능과 아이템 데이터 |
| [FortressGameInstance](Public/FortressGameInstance.h) / [FortressSaveGame](Public/FortressSaveGame.h) | 진행 데이터 관리와 디스크 저장 |
| [InteractInterface](Public/InteractInterface.h) | 상호작용 가능 여부·표시 문구·실행 인터페이스 |
| [Bonfire](Public/Bonfire.h) / [MemoryShardDrop](Public/MemoryShardDrop.h) | 화톳불 상호작용과 사망 재화 회수 |

일반 몬스터의 복귀 흐름은 [`LoseTargetAndReturnHome` → `UpdateReturnHome` → `RestoreHomeIdleState`](https://github.com/Gdongu/Citadel-of-Oblivion/blob/7cef93c2933eee8d234b1fc5d8dd2f78cb2f914e/Private/BaseMonster.cpp#L267-L459)에서 확인할 수 있습니다.

## 회고와 개선 방향

전투·AI·UI·저장을 연결하면서, 기능 하나의 동작뿐 아니라 **상태 전환 전후에 남는 판정과 타이머, 데이터**를 함께 확인하는 중요성을 배웠습니다.

기능 확장 과정에서 플레이어 클래스에 여러 책임이 집중되어 수정 시 확인할 범위가 커졌습니다. 앞으로 전투·성장·저장의 책임과 데이터 흐름을 정리하고, 반복되는 처리를 공통화해 변경 범위를 줄이고자 합니다. 공격 중 피격, 사망 후 재화 회수, 휴식과 저장·불러오기를 주요 검증 항목으로 관리할 계획입니다.

## 저장소 공개 범위

이 저장소는 **C++ 소스 열람을 위한 포트폴리오 저장소**입니다.

- `Public/`과 `Private/`에 헤더 및 구현 파일이 포함되어 있습니다.
- 현재 `.uproject`, 빌드 설정 파일, `Config/`, `Content/`는 포함되어 있지 않습니다.
- 따라서 저장소를 복제하는 것만으로 Unreal Editor에서 빌드하거나 게임을 실행할 수는 없습니다.
- 블루프린트, Behavior Tree 에셋, UI 및 애니메이션 설정을 포함한 실행 결과는 [플레이 영상](https://www.youtube.com/watch?v=qGrMPiaB-7g)과 [프로젝트 홈페이지](https://kdw1102.imweb.me/CitadelofOblivion)에서 확인할 수 있습니다.

함수 단위 코드 링크는 설명과 코드 위치가 일치하도록 `7cef93c` 커밋을 기준으로 연결했습니다.
