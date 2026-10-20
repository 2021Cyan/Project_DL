# Project DL

Unreal Engine 5.6 기반 팀 프로젝트의 개인 기여 소스를 모은 코드 열람용 저장소입니다.

> 최영우(CYW / 2021Cyan)의 최초 작성 이력을 확인한 파일을 [Workspace/CYW](./Workspace/CYW)에 기능별로 정리했습니다. 포트폴리오의 주요 기능은 **포털과 집라인**입니다.

## 주요 기능

- **포털**: 입·출구 좌표 변환과 SceneCapture 렌더링, 캐릭터 이동 이후에도 유지되는 입구 시점, 카메라 복귀 및 포털을 통한 투사체 이동을 담당합니다.
- **집라인**: 탑승부터 이동·하차까지의 상태 흐름과 AbilityTask, 상호작용·탑승 표시·스플라인 시각화를 담당합니다.
- 그 외 코드는 전체 개인 기여를 확인할 수 있도록 보조 분류로 보관했습니다.

## 분류

| 분류 | 파일 수 | 내용 |
|---|---:|---|
| [포털](./Workspace/CYW/Portal) | 23 | 좌표·방향 변환, 포털 렌더링, 입구 시점 유지 및 카메라 복귀, 순간이동, 이동체 통과 |
| [집라인](./Workspace/CYW/Zipline) | 14 | 탑승·이동·하차 Ability/Task, 상호작용 판정, 이동 수학, 탑승 표시 및 스플라인 시각화 |
| [도어맨의 다른 스킬](./Workspace/CYW/Doorman) | 12 | CallBell, HotelGuest, LugguageCart 관련 Ability 및 Actor |
| [사격·무기](./Workspace/CYW/Weapon) | 10 | 투사체 발사·조준, 포털 조준 연계, 재장전, 무기 컴포넌트 |
| [근접전투](./Workspace/CYW/MeleeCombat) | 10 | 약·강공격, 패링, 돌진 AbilityTask 및 근접 판정 유틸 |
| [공통 유틸리티](./Workspace/CYW/Common) | 8 | Ability 보조, Actor 생성·풀링, 플레이어 시점 및 투사체 이동 지원 |
| [캐릭터 애니메이션](./Workspace/CYW/Animation) | 4 | 게임플레이 이벤트 AnimNotify 및 플레이어 AnimInstance |
| [애니메이션 에디터 도구](./Workspace/CYW/Editor/Animation) | 6 | 애니메이션 초기화, 시작 구간 트림·루프, 루트 위치 정리 Modifier |
| [서버 실행 도구](./Workspace/CYW/Server) | 1 | 로컬 백엔드·Dedicated Server 풀 실행 스크립트; 백엔드 본체는 미포함 |
| [빌드 설정](./Workspace/CYW/Build) | 3 | 게임·에디터 Target 및 모듈 의존성 설정; Unreal 생성 틀을 기반으로 하는 설정 파일 |

**총 91개**: 기능·도구 소스 88개 + 빌드 설정 3개입니다. 헤더와 구현 파일을 각각 집계합니다.

## 선정 기준과 기여 범위

- 원본 TeamProject2의 Git 이력을 기준으로 파일의 최초 추가자를 확인했습니다. 같은 이메일로 기록된 Youngwoo / youngwoo / Youngwoo Choi를 동일 작성자로 취급했습니다.
- 이름 변경·폴더 이동은 이전 경로까지 추적했습니다. 다른 클래스의 생성 틀을 복사해 새 클래스를 만든 경우에는 복사 원본의 작성자 대신 해당 새 파일을 도입한 커밋을 기준으로 삼았습니다. 삭제 후 동일 경로에 다시 추가된 파일은 이전 최초 추가 이력을 보존했습니다.
- 본인이 처음 만든 뒤 팀원이 수정한 파일은 현재 버전으로 포함합니다. **파일 선정 기준이며, 현재 파일의 모든 줄을 혼자 작성했다는 의미는 아닙니다.** 이후 팀 작업과 도구를 이용한 리팩토링이 반영되어 있습니다.
- 다른 사람이 처음 만들고 본인이 수정만 한 파일은 제외했습니다. 본인이 최초 커밋한 것으로 표시되어도 본인 코드가 아니라고 확인한 `CharacterMovementStateGuard.h/.cpp`는 제외했습니다.
- 외부 SDK·플러그인, node_modules, 에셋, 바이너리, 캐시, 로그, 프로젝트 환경 설정 및 Unreal 기본 모듈 등록 코드는 포함하지 않습니다. 빌드 설정 3개는 Unreal 생성 틀 기반의 설정 코드로 별도 분류합니다.
- 이 목록은 현재 소스 트리에 존재하는 파일을 대상으로 하며, 삭제된 과거 버전은 중복 수록하지 않습니다.

## 열람 안내

이 저장소는 **개인 기여 코드 열람용**입니다. 에셋·다른 팀원의 공통 클래스·일부 헤더·프로젝트 설정을 제외하고 기능별로 재배치했으므로, 이 저장소만으로 Unreal 실행 또는 패키징은 할 수 없습니다. 아래 표에서 원래 경로를 확인할 수 있습니다. 소스 본문과 include는 변경하지 않았습니다.

기준 원본 커밋: `db9a1720265f38f776d8c1ff003933a356ebeef3`

## 파일별 분류·원본 경로·최초 추가 이력

### 포털

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Portal/DoorwayActor.cpp](./Workspace/CYW/Portal/DoorwayActor.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayActor.cpp` | `8f15d293aa` |
| [Portal/DoorwayActor.h](./Workspace/CYW/Portal/DoorwayActor.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayActor.h` | `8f15d293aa` |
| [Portal/DoorwayActorCamera.cpp](./Workspace/CYW/Portal/DoorwayActorCamera.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayActorCamera.cpp` | `9390970c88` |
| [Portal/DoorwayActorPortal.cpp](./Workspace/CYW/Portal/DoorwayActorPortal.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayActorPortal.cpp` | `44b71d3b8d` |
| [Portal/DoorwayActorTeleport.cpp](./Workspace/CYW/Portal/DoorwayActorTeleport.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayActorTeleport.cpp` | `9390970c88` |
| [Portal/DoorwayCameraTransition.cpp](./Workspace/CYW/Portal/DoorwayCameraTransition.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayCameraTransition.cpp` | `e560d51176` |
| [Portal/DoorwayCameraTransition.h](./Workspace/CYW/Portal/DoorwayCameraTransition.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayCameraTransition.h` | `e560d51176` |
| [Portal/DoorwayCameraTransitionComponent.cpp](./Workspace/CYW/Portal/DoorwayCameraTransitionComponent.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayCameraTransitionComponent.cpp` | `9390970c88` |
| [Portal/DoorwayCameraTransitionComponent.h](./Workspace/CYW/Portal/DoorwayCameraTransitionComponent.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayCameraTransitionComponent.h` | `9390970c88` |
| [Portal/DoorwayPortalMath.cpp](./Workspace/CYW/Portal/DoorwayPortalMath.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayPortalMath.cpp` | `44b71d3b8d` |
| [Portal/DoorwayPortalMath.h](./Workspace/CYW/Portal/DoorwayPortalMath.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayPortalMath.h` | `44b71d3b8d` |
| [Portal/DoorwayPortalRenderComponent.cpp](./Workspace/CYW/Portal/DoorwayPortalRenderComponent.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayPortalRenderComponent.cpp` | `9390970c88` |
| [Portal/DoorwayPortalRenderComponent.h](./Workspace/CYW/Portal/DoorwayPortalRenderComponent.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayPortalRenderComponent.h` | `9390970c88` |
| [Portal/DoorwayPortalRenderer.cpp](./Workspace/CYW/Portal/DoorwayPortalRenderer.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayPortalRenderer.cpp` | `e560d51176` |
| [Portal/DoorwayPortalRenderer.h](./Workspace/CYW/Portal/DoorwayPortalRenderer.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayPortalRenderer.h` | `9390970c88` |
| [Portal/DoorwayTeleportComponent.cpp](./Workspace/CYW/Portal/DoorwayTeleportComponent.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayTeleportComponent.cpp` | `9390970c88` |
| [Portal/DoorwayTeleportComponent.h](./Workspace/CYW/Portal/DoorwayTeleportComponent.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayTeleportComponent.h` | `9390970c88` |
| [Portal/DoorwayTeleportHandler.cpp](./Workspace/CYW/Portal/DoorwayTeleportHandler.cpp) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayTeleportHandler.cpp` | `e560d51176` |
| [Portal/DoorwayTeleportHandler.h](./Workspace/CYW/Portal/DoorwayTeleportHandler.h) | `Source/TeamProject_DL/Doorman/Doorway/DoorwayTeleportHandler.h` | `9390970c88` |
| [Portal/GA_Doorway.cpp](./Workspace/CYW/Portal/GA_Doorway.cpp) | `Source/TeamProject_DL/Doorman/Doorway/GA_Doorway.cpp` | `ed3f2fd949` |
| [Portal/GA_Doorway.h](./Workspace/CYW/Portal/GA_Doorway.h) | `Source/TeamProject_DL/Doorman/Doorway/GA_Doorway.h` | `ed3f2fd949` |
| [Portal/PortalTraversable.cpp](./Workspace/CYW/Portal/PortalTraversable.cpp) | `Source/TeamProject_DL/Doorman/Doorway/PortalTraversable.cpp` | `858b2334a6` |
| [Portal/PortalTraversable.h](./Workspace/CYW/Portal/PortalTraversable.h) | `Source/TeamProject_DL/Doorman/Doorway/PortalTraversable.h` | `858b2334a6` |

### 집라인

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Zipline/Ability/AbilityTask_ZiplineMove.cpp](./Workspace/CYW/Zipline/Ability/AbilityTask_ZiplineMove.cpp) | `Source/TeamProject_DL/Zipline/Ability/AbilityTask_ZiplineMove.cpp` | `9a2b722ff8` |
| [Zipline/Ability/AbilityTask_ZiplineMove.h](./Workspace/CYW/Zipline/Ability/AbilityTask_ZiplineMove.h) | `Source/TeamProject_DL/Zipline/Ability/AbilityTask_ZiplineMove.h` | `9a2b722ff8` |
| [Zipline/Ability/GA_Zipline.cpp](./Workspace/CYW/Zipline/Ability/GA_Zipline.cpp) | `Source/TeamProject_DL/Zipline/Ability/GA_Zipline.cpp` | `9a2b722ff8` |
| [Zipline/Ability/GA_Zipline.h](./Workspace/CYW/Zipline/Ability/GA_Zipline.h) | `Source/TeamProject_DL/Zipline/Ability/GA_Zipline.h` | `9a2b722ff8` |
| [Zipline/ZiplineActor.cpp](./Workspace/CYW/Zipline/ZiplineActor.cpp) | `Source/TeamProject_DL/Zipline/ZiplineActor.cpp` | `7ea29a781a` |
| [Zipline/ZiplineActor.h](./Workspace/CYW/Zipline/ZiplineActor.h) | `Source/TeamProject_DL/Zipline/ZiplineActor.h` | `7ea29a781a` |
| [Zipline/ZiplineInteractionLibrary.cpp](./Workspace/CYW/Zipline/ZiplineInteractionLibrary.cpp) | `Source/TeamProject_DL/Zipline/ZiplineInteractionLibrary.cpp` | `d2b4064379` |
| [Zipline/ZiplineInteractionLibrary.h](./Workspace/CYW/Zipline/ZiplineInteractionLibrary.h) | `Source/TeamProject_DL/Zipline/ZiplineInteractionLibrary.h` | `d2b4064379` |
| [Zipline/ZiplineMountIndicatorComponent.cpp](./Workspace/CYW/Zipline/ZiplineMountIndicatorComponent.cpp) | `Source/TeamProject_DL/Zipline/ZiplineMountIndicatorComponent.cpp` | `d2b4064379` |
| [Zipline/ZiplineMountIndicatorComponent.h](./Workspace/CYW/Zipline/ZiplineMountIndicatorComponent.h) | `Source/TeamProject_DL/Zipline/ZiplineMountIndicatorComponent.h` | `d2b4064379` |
| [Zipline/ZiplineRideMath.h](./Workspace/CYW/Zipline/ZiplineRideMath.h) | `Source/TeamProject_DL/Zipline/ZiplineRideMath.h` | `85a30e40a7` |
| [Zipline/ZiplineSplineVisualComponent.cpp](./Workspace/CYW/Zipline/ZiplineSplineVisualComponent.cpp) | `Source/TeamProject_DL/Zipline/ZiplineSplineVisualComponent.cpp` | `d2b4064379` |
| [Zipline/ZiplineSplineVisualComponent.h](./Workspace/CYW/Zipline/ZiplineSplineVisualComponent.h) | `Source/TeamProject_DL/Zipline/ZiplineSplineVisualComponent.h` | `d2b4064379` |
| [Zipline/ZiplineTypes.h](./Workspace/CYW/Zipline/ZiplineTypes.h) | `Source/TeamProject_DL/Zipline/ZiplineTypes.h` | `d2b4064379` |

### 도어맨의 다른 스킬

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Doorman/CallBell/CallBellProjectile.cpp](./Workspace/CYW/Doorman/CallBell/CallBellProjectile.cpp) | `Source/TeamProject_DL/Doorman/CallBell/CallBellProjectile.cpp` | `ed3f2fd949` |
| [Doorman/CallBell/CallBellProjectile.h](./Workspace/CYW/Doorman/CallBell/CallBellProjectile.h) | `Source/TeamProject_DL/Doorman/CallBell/CallBellProjectile.h` | `ed3f2fd949` |
| [Doorman/CallBell/GA_CallBell.cpp](./Workspace/CYW/Doorman/CallBell/GA_CallBell.cpp) | `Source/TeamProject_DL/Doorman/CallBell/GA_CallBell.cpp` | `ed3f2fd949` |
| [Doorman/CallBell/GA_CallBell.h](./Workspace/CYW/Doorman/CallBell/GA_CallBell.h) | `Source/TeamProject_DL/Doorman/CallBell/GA_CallBell.h` | `ed3f2fd949` |
| [Doorman/HotelGuest/GA_HotelGuest.cpp](./Workspace/CYW/Doorman/HotelGuest/GA_HotelGuest.cpp) | `Source/TeamProject_DL/Doorman/HotelGuest/GA_HotelGuest.cpp` | `ed3f2fd949` |
| [Doorman/HotelGuest/GA_HotelGuest.h](./Workspace/CYW/Doorman/HotelGuest/GA_HotelGuest.h) | `Source/TeamProject_DL/Doorman/HotelGuest/GA_HotelGuest.h` | `ed3f2fd949` |
| [Doorman/HotelGuest/HotelGuestLocationActor.cpp](./Workspace/CYW/Doorman/HotelGuest/HotelGuestLocationActor.cpp) | `Source/TeamProject_DL/Doorman/HotelGuest/HotelGuestLocationActor.cpp` | `4f8b991456` |
| [Doorman/HotelGuest/HotelGuestLocationActor.h](./Workspace/CYW/Doorman/HotelGuest/HotelGuestLocationActor.h) | `Source/TeamProject_DL/Doorman/HotelGuest/HotelGuestLocationActor.h` | `4f8b991456` |
| [Doorman/LugguageCart/GA_LugguageCart.cpp](./Workspace/CYW/Doorman/LugguageCart/GA_LugguageCart.cpp) | `Source/TeamProject_DL/Doorman/LugguageCart/GA_LugguageCart.cpp` | `ed3f2fd949` |
| [Doorman/LugguageCart/GA_LugguageCart.h](./Workspace/CYW/Doorman/LugguageCart/GA_LugguageCart.h) | `Source/TeamProject_DL/Doorman/LugguageCart/GA_LugguageCart.h` | `ed3f2fd949` |
| [Doorman/LugguageCart/LugguageCartActor.cpp](./Workspace/CYW/Doorman/LugguageCart/LugguageCartActor.cpp) | `Source/TeamProject_DL/Doorman/LugguageCart/LugguageCartActor.cpp` | `506019c362` |
| [Doorman/LugguageCart/LugguageCartActor.h](./Workspace/CYW/Doorman/LugguageCart/LugguageCartActor.h) | `Source/TeamProject_DL/Doorman/LugguageCart/LugguageCartActor.h` | `506019c362` |

### 사격·무기

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Weapon/Ability/GA_FireProjectile.cpp](./Workspace/CYW/Weapon/Ability/GA_FireProjectile.cpp) | `Source/TeamProject_DL/Weapon/Ability/GA_FireProjectile.cpp` | `8f10466408` |
| [Weapon/Ability/GA_FireProjectile.h](./Workspace/CYW/Weapon/Ability/GA_FireProjectile.h) | `Source/TeamProject_DL/Weapon/Ability/GA_FireProjectile.h` | `8f10466408` |
| [Weapon/Ability/GA_Reload.cpp](./Workspace/CYW/Weapon/Ability/GA_Reload.cpp) | `Source/TeamProject_DL/Weapon/Ability/GA_Reload.cpp` | `8f10466408` |
| [Weapon/Ability/GA_Reload.h](./Workspace/CYW/Weapon/Ability/GA_Reload.h) | `Source/TeamProject_DL/Weapon/Ability/GA_Reload.h` | `8f10466408` |
| [Weapon/ProjectileAimLibrary.cpp](./Workspace/CYW/Weapon/ProjectileAimLibrary.cpp) | `Source/TeamProject_DL/Weapon/ProjectileAimLibrary.cpp` | `04a81db3fa` |
| [Weapon/ProjectileAimLibrary.h](./Workspace/CYW/Weapon/ProjectileAimLibrary.h) | `Source/TeamProject_DL/Weapon/ProjectileAimLibrary.h` | `04a81db3fa` |
| [Weapon/ProjectileBase.cpp](./Workspace/CYW/Weapon/ProjectileBase.cpp) | `Source/TeamProject_DL/Weapon/ProjectileBase.cpp` | `8f10466408` |
| [Weapon/ProjectileBase.h](./Workspace/CYW/Weapon/ProjectileBase.h) | `Source/TeamProject_DL/Weapon/ProjectileBase.h` | `8f10466408` |
| [Weapon/WeaponComponent.cpp](./Workspace/CYW/Weapon/WeaponComponent.cpp) | `Source/TeamProject_DL/Weapon/WeaponComponent.cpp` | `8f10466408` |
| [Weapon/WeaponComponent.h](./Workspace/CYW/Weapon/WeaponComponent.h) | `Source/TeamProject_DL/Weapon/WeaponComponent.h` | `8f10466408` |

### 근접전투

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [MeleeCombat/Ability/AbilityTask_HeavyMeleeLunge.cpp](./Workspace/CYW/MeleeCombat/Ability/AbilityTask_HeavyMeleeLunge.cpp) | `Source/TeamProject_DL/GameCore/Combat/Ability/AbilityTask_HeavyMeleeLunge.cpp` | `3b8e53a438` |
| [MeleeCombat/Ability/AbilityTask_HeavyMeleeLunge.h](./Workspace/CYW/MeleeCombat/Ability/AbilityTask_HeavyMeleeLunge.h) | `Source/TeamProject_DL/GameCore/Combat/Ability/AbilityTask_HeavyMeleeLunge.h` | `3b8e53a438` |
| [MeleeCombat/Ability/GA_HeavyMelee.cpp](./Workspace/CYW/MeleeCombat/Ability/GA_HeavyMelee.cpp) | `Source/TeamProject_DL/GameCore/Combat/Ability/GA_HeavyMelee.cpp` | `824f603752` |
| [MeleeCombat/Ability/GA_HeavyMelee.h](./Workspace/CYW/MeleeCombat/Ability/GA_HeavyMelee.h) | `Source/TeamProject_DL/GameCore/Combat/Ability/GA_HeavyMelee.h` | `824f603752` |
| [MeleeCombat/Ability/GA_LightMelee.cpp](./Workspace/CYW/MeleeCombat/Ability/GA_LightMelee.cpp) | `Source/TeamProject_DL/GameCore/Combat/Ability/GA_LightMelee.cpp` | `824f603752` |
| [MeleeCombat/Ability/GA_LightMelee.h](./Workspace/CYW/MeleeCombat/Ability/GA_LightMelee.h) | `Source/TeamProject_DL/GameCore/Combat/Ability/GA_LightMelee.h` | `824f603752` |
| [MeleeCombat/Ability/GA_Parry.cpp](./Workspace/CYW/MeleeCombat/Ability/GA_Parry.cpp) | `Source/TeamProject_DL/GameCore/Combat/Ability/GA_Parry.cpp` | `824f603752` |
| [MeleeCombat/Ability/GA_Parry.h](./Workspace/CYW/MeleeCombat/Ability/GA_Parry.h) | `Source/TeamProject_DL/GameCore/Combat/Ability/GA_Parry.h` | `824f603752` |
| [MeleeCombat/MeleeHitUtils.cpp](./Workspace/CYW/MeleeCombat/MeleeHitUtils.cpp) | `Source/TeamProject_DL/GameCore/Combat/MeleeHitUtils.cpp` | `04a81db3fa` |
| [MeleeCombat/MeleeHitUtils.h](./Workspace/CYW/MeleeCombat/MeleeHitUtils.h) | `Source/TeamProject_DL/GameCore/Combat/MeleeHitUtils.h` | `04a81db3fa` |

### 공통 유틸리티

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Common/AbilitySupport.h](./Workspace/CYW/Common/AbilitySupport.h) | `Source/TeamProject_DL/Common/AbilitySupport.h` | `85a30e40a7` |
| [Common/AbilityTags.h](./Workspace/CYW/Common/AbilityTags.h) | `Source/TeamProject_DL/Common/AbilityTags.h` | `85a30e40a7` |
| [Common/ActorSpawnSupport.h](./Workspace/CYW/Common/ActorSpawnSupport.h) | `Source/TeamProject_DL/Common/ActorSpawnSupport.h` | `85a30e40a7` |
| [Common/DLActorPoolSubsystem.cpp](./Workspace/CYW/Common/DLActorPoolSubsystem.cpp) | `Source/TeamProject_DL/Common/DLActorPoolSubsystem.cpp` | `f2c274efee` |
| [Common/DLActorPoolSubsystem.h](./Workspace/CYW/Common/DLActorPoolSubsystem.h) | `Source/TeamProject_DL/Common/DLActorPoolSubsystem.h` | `f2c274efee` |
| [Common/PlayerViewSupport.h](./Workspace/CYW/Common/PlayerViewSupport.h) | `Source/TeamProject_DL/Common/PlayerViewSupport.h` | `85a30e40a7` |
| [Common/PoolableActor.h](./Workspace/CYW/Common/PoolableActor.h) | `Source/TeamProject_DL/Common/PoolableActor.h` | `f2c274efee` |
| [Common/ProjectileMovementSupport.h](./Workspace/CYW/Common/ProjectileMovementSupport.h) | `Source/TeamProject_DL/Common/ProjectileMovementSupport.h` | `85a30e40a7` |

### 캐릭터 애니메이션

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Animation/AnimNotify/AnimNotify_SendGameplayEventToOwner.cpp](./Workspace/CYW/Animation/AnimNotify/AnimNotify_SendGameplayEventToOwner.cpp) | `Source/TeamProject_DL/AnimNotify/AnimNotify_SendGameplayEventToOwner.cpp` | `fec35c021d` |
| [Animation/AnimNotify/AnimNotify_SendGameplayEventToOwner.h](./Workspace/CYW/Animation/AnimNotify/AnimNotify_SendGameplayEventToOwner.h) | `Source/TeamProject_DL/AnimNotify/AnimNotify_SendGameplayEventToOwner.h` | `fec35c021d` |
| [Animation/Player/PlayerAnimInstance.cpp](./Workspace/CYW/Animation/Player/PlayerAnimInstance.cpp) | `Source/TeamProject_DL/Private/Animation/PlayerAnimInstance.cpp` | `f8d179d572` |
| [Animation/Player/PlayerAnimInstance.h](./Workspace/CYW/Animation/Player/PlayerAnimInstance.h) | `Source/TeamProject_DL/Private/Animation/PlayerAnimInstance.h` | `f8d179d572` |

### 애니메이션 에디터 도구

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Editor/Animation/AnimationResetModifier.cpp](./Workspace/CYW/Editor/Animation/AnimationResetModifier.cpp) | `Source/TeamProject_DLEditor/Private/Animation/AnimationResetModifier.cpp` | `4481d36727` |
| [Editor/Animation/AnimationResetModifier.h](./Workspace/CYW/Editor/Animation/AnimationResetModifier.h) | `Source/TeamProject_DLEditor/Private/Animation/AnimationResetModifier.h` | `4481d36727` |
| [Editor/Animation/TrimStartAndLoopModifier.cpp](./Workspace/CYW/Editor/Animation/TrimStartAndLoopModifier.cpp) | `Source/TeamProject_DLEditor/Private/Animation/TrimStartAndLoopModifier.cpp` | `25d307095c` |
| [Editor/Animation/TrimStartAndLoopModifier.h](./Workspace/CYW/Editor/Animation/TrimStartAndLoopModifier.h) | `Source/TeamProject_DLEditor/Private/Animation/TrimStartAndLoopModifier.h` | `25d307095c` |
| [Editor/Animation/ZeroRootMotionLocationModifier.cpp](./Workspace/CYW/Editor/Animation/ZeroRootMotionLocationModifier.cpp) | `Source/TeamProject_DLEditor/Private/Animation/ZeroRootMotionLocationModifier.cpp` | `dfc81883fb` |
| [Editor/Animation/ZeroRootMotionLocationModifier.h](./Workspace/CYW/Editor/Animation/ZeroRootMotionLocationModifier.h) | `Source/TeamProject_DLEditor/Private/Animation/ZeroRootMotionLocationModifier.h` | `dfc81883fb` |

### 서버 실행 도구

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Server/start-local.ps1](./Workspace/CYW/Server/start-local.ps1) | `Server/start-local.ps1` | `d12e82ff01` |

### 빌드 설정

| 파일 | 원본 경로 | 최초 추가 커밋 |
|---|---|---|
| [Build/TeamProject_DL.Target.cs](./Workspace/CYW/Build/TeamProject_DL.Target.cs) | `Source/TeamProject_DL.Target.cs` | `ddf70f058a` |
| [Build/TeamProject_DL.Build.cs](./Workspace/CYW/Build/TeamProject_DL.Build.cs) | `Source/TeamProject_DL/TeamProject_DL.Build.cs` | `ddf70f058a` |
| [Build/TeamProject_DLEditor.Target.cs](./Workspace/CYW/Build/TeamProject_DLEditor.Target.cs) | `Source/TeamProject_DLEditor.Target.cs` | `ddf70f058a` |
