# 애셋 에디터 설계 및 구현 계획

작성일: 2026-11-01

이 문서는 대화에서 정리한 설계 방향과 구현 순서를 기록한다. 체크리스트는 구현 계획이며 현재 코드의 완료 상태를 뜻하지 않는다. 구현에 착수할 때 기존 변경 사항과 생성 중인 클래스를 확인한 후 반영한다.

## 1. 목표와 범위

- Standalone 방식의 메시 편집기를 에디터 내부의 탭 기반 편집기로 통합한다.
- 레벨 에디터는 하나만 유지하고, 애셋 편집기는 애셋마다 하나씩 생성한다.
- 애셋 탭은 별도 창으로 분리하고 다시 도킹할 수 있다.
- 레벨 탭은 분리하지 않는 것을 설계 기준으로 한다. 테스트 중 변경된 `CanDetach()` 값이 있다면 구현 시 확인한다.
- 첫 목표는 애셋 열기, 프리뷰 표시, 탭 분리, 재활성화, 안전한 닫기까지 연결하는 것이다.
- 최초 지원 편집기는 `FStaticMeshEditor`, `FSkeletalMeshEditor`이다.
- SkeletalMesh 편집기는 기본 포즈 표시부터 구현하고 애니메이션 재생은 별도 단계로 확장한다.
- 레벨 에디터는 당장 `FEditorManager`의 관리 대상에 포함하지 않는다.

## 2. 구성 요소와 책임

| 구성 요소 | 소유자 | 책임 |
| --- | --- | --- |
| `FEditorManager` | `UEditorEngine` | 경로 기반 열기, 중복 편집기 검색, 활성화, 편집기 Tick·Render 및 종료 관리 |
| `FAssetEditor` | Manager의 스마트 포인터 배열 | 애셋 편집기의 공통 생명주기와 구성 요소 연결 |
| `FStaticMeshEditor` | Manager | StaticMesh 편집 상태, 프리뷰, 뷰포트, 탭 관리 |
| `FSkeletalMeshEditor` | Manager | SkeletalMesh 편집 상태, 프리뷰, 뷰포트, 탭 관리 |
| `FPreviewScene` | 각 AssetEditor | EditorPreview 월드, 프리뷰 액터·컴포넌트, 조명·바닥, 갱신 및 정리 |
| 전용 ViewportClient | 각 AssetEditor | 카메라, 입력, 뷰 설정, 렌더 타깃과 뷰포트 상태 관리 |
| 전용 EditorTab | 각 AssetEditor | 메뉴와 패널 배치, 뷰포트 크기·입력 상태 전달, 렌더 결과 표시 |
| `FEditorUI` / `FEditorTabContainer` | 기존 에디터 UI 구조 | 탭 비소유 참조, 도킹, 메뉴 위치, 선택·포커스, 등록·해제 관리 |

```text
UEditorEngine
└─ FEditorManager
   └─ TArray<TUniquePtr<FAssetEditor>>
      ├─ FStaticMeshEditor
      │  ├─ FPreviewScene
      │  ├─ FStaticMeshEditorViewportClient → PreviewScene 비소유 참조
      │  └─ FStaticMeshEditorTab → 편집기 비소유 참조
      └─ FSkeletalMeshEditor
         ├─ FPreviewScene
         ├─ FSkeletalMeshEditorViewportClient → PreviewScene 비소유 참조
         └─ FSkeletalMeshEditorTab → 편집기 비소유 참조

FEditorUI / FEditorTabContainer → 각 EditorTab 비소유 참조
```

### 소유 및 구현 규칙

- Manager는 편집기를 `TUniquePtr<FAssetEditor>`로 소유한다.
- 수명이 항상 함께하는 일반 C++ 구성 요소는 값 멤버여도 된다. 생성 시점 조절이나 다형성이 필요할 때 스마트 포인터를 사용한다.
- ViewportClient는 PreviewScene을 소유하지 않는다.
- PreviewScene보다 ViewportClient와 탭의 사용이 먼저 끝나야 한다.
- `UWorld`, 액터, 컴포넌트는 프로젝트의 UObject 생성·파괴 규칙을 따른다. 단순히 `TUniquePtr<UWorld>`로 소유하지 않는다.
- 원본 메시 애셋의 수명은 애셋 시스템의 규칙을 따른다. 프리뷰 컴포넌트를 정리하면서 공유 애셋까지 파괴하지 않는다.
- 원본 애셋 수정과 프리뷰의 카메라·조명·재생 상태 변경을 구분한다.
- Getter와 템플릿은 헤더에 유지하고, 나머지 구현은 `.cpp`로 분리한다.
- 기존 코딩 스타일을 유지하고 필요한 주석은 한국어로 작성한다.

## 3. 탭과 메뉴 구조

- `IEditorTab`은 탭 인터페이스를 제공한다.
- `FEditorTabContainer`는 별도의 `EditorTabContainer.h`와 `.cpp`에 둔다.
- 각 편집기는 탭을 소유하고 `FEditorUI`에 등록·해제한다.
- `DrawMenuBar()`는 메뉴 내용만 그린다. 메뉴 바의 Begin/End와 위치는 컨테이너가 결정한다.
- 메인 도킹 영역에서는 선택된 탭의 메뉴를 공통 상단에 표시한다.
- 분리된 창에서는 해당 창에서 선택된 탭의 메뉴를 표시한다.
- 메인 영역의 선택 탭과 입력 포커스를 가진 탭을 별도로 추적한다.
- 분리 여부는 실제 도킹 트리로 판별한다. `CanDetach()`나 viewport ID만으로 판단하지 않는다.
- 애셋 식별자를 ImGui ID에 포함하여 같은 파일명을 가진 서로 다른 애셋 간 충돌을 방지한다.
- `FEditorTabContainer`에서 제거하는 것은 프레임 Tick 책임이다. 등록·해제·활성화·메뉴 배치 기능은 유지한다.
- 기존 레벨 패널 등록은 `FLevelEditorTab::AddPanel<T>()`을 사용한다.

## 4. 공통 생명주기

`FAssetEditor`의 공통 계약은 다음을 기준으로 한다. 세부 인자와 반환형은 구현 시 프로젝트 스타일에 맞춘다.

| 함수 또는 상태 | 역할 |
| --- | --- |
| `Initialize()` | 애셋 확인, 프리뷰·뷰포트·탭 초기화, 마지막에 탭 등록 |
| `Tick(DeltaTime)` | 입력 및 프리뷰 갱신 |
| `Render()` | 프리뷰 렌더링 |
| `Activate()` | 탭 선택과 해당 창의 포커스 요청 |
| `RequestClose()` | 중복 없이 종료 요청 기록 |
| `Shutdown()` | 탭 등록 해제, 참조 및 프리뷰 자원 정리 |
| 정규화된 애셋 경로 | 중복 편집기 판단에 사용할 식별자 |

- 부분 초기화 실패도 안전하게 정리한다.
- `Shutdown()`은 중복 호출에 안전하게 만든다.
- 생성자·소멸자에 UI 등록을 숨기기보다 초기화·종료 함수에서 명시적으로 처리한다.

### OpenEditor 흐름

```text
OpenEditor(AssetPath)
→ 애셋 시스템과 동일한 기준으로 경로 정규화
→ 이미 열린 편집기 검색
   ├─ 존재: 해당 탭 선택 및 분리된 창 포커스
   └─ 없음: 로딩 및 실제 애셋 타입 확인
            → 지원 편집기 생성
            → 초기화
            → 성공 시 Manager 소유 배열에 등록
```

- 경로 표시 이름 대신 안정적인 식별자를 사용한다.
- 초기에는 두 타입에 대한 분기로 충분하며 팩토리 등록 시스템은 필요할 때 도입한다.
- `GetAssetByPath<T>()`는 등록된 애셋 조회이므로 미등록 경로의 로딩과 구분한다.
- 지원하지 않는 타입, 로딩 실패, 초기화 실패를 구분해 로그를 남긴다.
- 순회 중 배열 변경을 피하도록 필요한 경우 열기 요청도 대기열로 처리한다.

### 닫기 흐름

```text
탭 OnRender에서 닫기 감지
→ 닫기 요청 기록 및 해당 탭의 추가 처리 생략
→ UI 순회 종료
→ 탭 비소유 참조 제거, 선택·포커스·활성화 대기 참조 정리
→ Manager에 종료 요청 전달
→ ImGui 및 플랫폼 창 렌더링 완료
→ AssetEditor.Shutdown()
→ Manager 소유 배열에서 제거
```

- 배열 순회 도중에는 비소유 포인터도 직접 제거하지 않는다.
- UI 등록 해제와 객체 파괴를 구분한다.
- 렌더 타깃이 ImGui draw data에서 참조되는 동안 파괴하지 않는다.
- 비동기 렌더 작업이 있다면 해당 참조가 종료되는 시점까지 파괴를 늦춘다.
- 저장 확인 기능을 추가할 경우 닫기 승인 후 탭을 해제할 수 있도록 요청과 실행을 분리한다.

## 5. PreviewScene과 World 정책

### PreviewScene 역할

- `EWorldType::EditorPreview` 월드를 생성한다.
- 프리뷰 액터 및 컴포넌트를 생성·등록한다.
- 공통 조명·바닥 등 프리뷰 환경을 설정한다.
- 프리뷰에 허용된 갱신과 장면 변환 갱신을 수행한다.
- 컴포넌트 등록 해제와 월드 정리를 책임진다.

어떤 메시를 표시할지는 AssetEditor가 결정하고 PreviewScene에 요청한다. 공통 PreviewScene에 애셋 타입별 분기가 계속 누적되지 않도록 한다.

### World Tick 분리

| 월드 종류 | 정책 |
| --- | --- |
| Editor | 기존 레벨 편집용 갱신 유지 |
| Game / PIE | 기존 게임플레이 갱신 유지 |
| EditorPreview | 프리뷰에 명시적으로 허용한 갱신만 실행 |

- EditorPreview에서는 불필요한 게임 액터 Tick, 게임 스크립트, 전역 코루틴, 충돌 겹침 처리 등을 실행하지 않는다.
- SkeletalMesh의 포즈·본 변환 등 필요한 갱신은 별도 경로로 허용한다.
- 같은 컴포넌트를 World와 PreviewScene 양쪽에서 중복 갱신하지 않는다.
- StaticMesh는 변경 시 갱신하는 방식으로 확장할 수 있으나 최초 구현에서는 명확한 동작을 우선한다.
- 애니메이션 재생 시에만 필요한 시간 진행을 수행하도록 확장한다.

### 현 코드에서 확인할 사항

계획 작성 당시 확인한 코드 기준이며, 구현 전에 최신 상태를 다시 확인한다.

- `UWorld::Tick()`은 Editor가 아닌 월드에 게임 처리와 전역 스케줄러 호출을 수행하므로 EditorPreview 분기가 필요하다.
- `UWorld::~UWorld()`가 비어 있어 포인터 해제만으로 정리가 끝난다고 가정하면 안 된다.
- `CreateWorld()`의 `bInformEngineOfWorld`가 구현에서 사용되지 않으므로 WorldContext 등록 여부를 해당 인자에 의존하지 않는다.
- 우선 Manager가 프리뷰 Tick·Render·종료를 관리하고 프리뷰 월드는 레벨/PIE의 활성 월드 선택에서 제외한다.

## 6. ViewportClient 정책

- `FEditorViewportClient`의 기존 레벨용 생성 경로를 유지한다.
- 선택적인 비소유 `FPreviewScene` 참조와 프리뷰용 생성 경로를 추가한다.
- PreviewScene이 있으면 `GetWorld()`는 반드시 해당 월드를 반환한다.
- 월드를 직접 사용하는 피킹·입력 등 내부 코드도 `GetWorld()` 기준으로 점검한다.
- 프리뷰가 연결된 상태에서 `SetWorld()`로 별도 월드를 지정하지 못하도록 정책을 명확히 한다.
- 전용 클라이언트는 생성 시 `FPreviewScene&`을 받아 필수 의존성을 표현한다.

```cpp
FStaticMeshEditorViewportClient(FPreviewScene& InPreviewScene);
FSkeletalMeshEditorViewportClient(FPreviewScene& InPreviewScene);
```

ViewportClient는 카메라·입력·뷰 설정·렌더 타깃을 관리한다. 실제 GPU 렌더 경로는 공통 렌더러를 재사용할 수 있으며 ViewportClient에 모든 렌더링 코드를 넣을 필요는 없다.

## 7. 프레임 처리 순서

```text
UEditorEngine::Tick
├─ BeginFrame
├─ 기존 레벨 입력·월드 갱신
├─ LevelEditorTab.Tick → 기존 패널 Tick
├─ EditorManager.Tick → 각 AssetEditor.Tick → PreviewScene 갱신
├─ RenderViewports → 기존 레벨 렌더링
├─ EditorManager.Render → 각 AssetEditor 프리뷰 렌더링
├─ EndFrame
│  ├─ EditorUI.OnRender
│  ├─ 탭 순회 종료 후 등록 해제 및 종료 요청 전달
│  └─ ImGui 및 플랫폼 창 렌더링
└─ 안전한 시점에 종료 요청된 편집기 파괴
```

- 각 AssetEditor가 Tick을 담당한다. 탭은 UI 상태를 전달하며 프리뷰를 중복 Tick하지 않는다.
- 레벨 에디터는 Manager 밖에서 별도로 Tick한다.
- 애셋 렌더링은 기존 레벨용 뷰포트 배열과 분리한다.
- 공통 렌더러를 사용하되 월드·카메라·렌더 타깃·옵션은 편집기별로 전달한다.
- 기존 `RenderViewports()`의 EditorWorld/PlayWorld 선택을 애셋 프리뷰에 그대로 적용하지 않는다.
- UI에서 얻은 뷰포트 크기·표시 여부는 우선 다음 프레임의 렌더링에 반영한다.
- 최초 크기 미확정 또는 크기 0인 경우 Resize·Render를 생략한다.
- 렌더 여부는 메인 선택 상태가 아니라 해당 뷰포트의 표시 여부로 판단한다.
- 분리된 여러 창이 보이면 모두 렌더링한다. 숨겨진 탭은 우선 렌더링을 생략한다.
- 입력은 포커스·hover·마우스 캡처를 기준으로 전달한다.

## 8. 단계별 구현 계획

### 1단계: 공통 인터페이스와 파일 구성

- [ ] 기존 작업 트리와 이미 생성된 PreviewScene 파일을 확인한다.
- [ ] `FAssetEditor` 기반 클래스와 생명주기 계약을 작성한다.
- [ ] 편집기 경로·종료 요청 상태·초기화 실패 정리 규칙을 정한다.
- [ ] Manager 및 각 편집기 파일을 생성한다.

권장 구성은 아래와 같으며 기존 파일 위치를 우선 존중한다.

```text
Editor/
├─ AssetEditor/AssetEditor.h, .cpp
├─ AssetEditor/EditorManager.h, .cpp
├─ PreviewScene/PreviewScene.h, .cpp
├─ StaticMeshEditor/
│  ├─ StaticMeshEditor.h, .cpp
│  ├─ StaticMeshEditorViewportClient.h, .cpp
│  └─ StaticMeshEditorTab.h, .cpp
└─ SkeletalMeshEditor/
   ├─ SkeletalMeshEditor.h, .cpp
   ├─ SkeletalMeshEditorViewportClient.h, .cpp
   └─ SkeletalMeshEditorTab.h, .cpp
```

### 2단계: 탭 수명 관리

- [ ] 닫기 가능 여부 및 닫기 요청 전달 경로를 추가한다.
- [ ] UI와 컨테이너에 등록 해제·활성화 API를 추가한다.
- [ ] 선택·포커스·활성화 대기 참조 정리를 구현한다.
- [ ] 닫기 요청을 UI 순회 뒤에 반영한다.
- [ ] TabContainer의 Tick을 제거하고 레벨 패널 Tick 경로를 유지한다.
- [ ] 임시 탭으로 분리·활성화·닫기 반복을 검증한다.

완료 기준: 배열 순회 무효화와 무효 탭 참조가 발생하지 않는다.

### 3단계: EditorManager

- [ ] `UEditorEngine`에 Manager를 추가한다.
- [ ] 경로 정규화·중복 검색·기존 편집기 활성화를 구현한다.
- [ ] 애셋 로딩·타입 확인·편집기 생성 흐름을 연결한다.
- [ ] 초기화 성공 시 등록하고 실패 시 되돌린다.
- [ ] Tick·Render·종료 요청·전체 종료를 구현한다.
- [ ] 필요한 경우 열기와 종료 요청 대기열을 사용한다.

완료 기준: 같은 애셋은 하나만 열리며 실패한 초기화가 Manager나 UI에 남지 않는다.

### 4단계: EditorPreview와 PreviewScene

- [ ] World Tick을 Editor / Game·PIE / EditorPreview로 명시적으로 구분한다.
- [ ] 프리뷰에 허용할 컴포넌트 갱신을 정한다.
- [ ] 월드·프리뷰 액터·컴포넌트·환경 생성과 정리를 구현한다.
- [ ] UObject 및 WorldContext 수명 정책을 확인한다.
- [ ] 여러 프리뷰가 게임 전역 처리를 중복 실행하지 않는지 확인한다.

완료 기준: 프리뷰 생성·제거가 레벨 월드와 게임플레이에 영향을 주지 않는다.

### 5단계: PreviewScene 지원 ViewportClient

- [ ] 선택적인 프리뷰 참조 및 필수 참조를 받는 파생 생성자를 추가한다.
- [ ] GetWorld와 내부 World 직접 참조를 정리한다.
- [ ] 프리뷰 상태에서 SetWorld의 동작을 제한한다.
- [ ] 카메라 이동·회전·바운드 맞춤·Resize를 구현한다.
- [ ] 기존 레벨 뷰포트 입력 회귀를 확인한다.

완료 기준: 각 클라이언트가 항상 의도한 월드를 사용한다.

### 6단계: StaticMeshEditor 전체 연결

- [ ] StaticMesh 애셋 조회와 프리뷰 컴포넌트 연결을 구현한다.
- [ ] 메시 바운드에 맞춰 초기 카메라를 배치한다.
- [ ] 탭 등록과 프리뷰 이미지 표시를 연결한다.
- [ ] 기본 정보·카메라 맞춤·탭 메뉴·닫기를 구현한다.
- [ ] 아직 저장 기능이 없다면 Save를 생략하거나 비활성화한다.
- [ ] 레벨 저장 콜백을 애셋 저장에 재사용하지 않는다.

### 7단계: 엔진 Tick·Render 연결

- [ ] Manager Tick과 별도 레벨 패널 Tick을 연결한다.
- [ ] RenderViewports 뒤에 Manager.Render를 연결한다.
- [ ] 프리뷰별 월드·타깃·렌더 옵션을 전달한다.
- [ ] 크기 0·숨김·다중 분리 창·최초 프레임을 처리한다.
- [ ] ImGui 렌더 완료 뒤에 종료 요청 객체를 파괴한다.

완료 기준: 레벨과 프리뷰가 동시에 올바르게 표시되며 PIE·Resize·분리에서도 유지된다.

### 8단계: SkeletalMeshEditor 확장

- [ ] SkeletalMesh의 로딩·등록 경로를 확인하고 필요한 부분을 연결한다.
- [ ] `USkeletalMeshComponent` 생성 및 애셋 연결을 구현한다.
- [ ] 기본 포즈·본 변환 갱신을 구현한다.
- [ ] 전용 뷰포트와 탭을 연결한다.
- [ ] 메시·본 개수 등 기본 정보를 표시한다.
- [ ] 애니메이션 재생 기능은 별도 후속 단계로 둔다.

완료 기준: 두 종류의 편집기를 동시에 열어도 카메라·메뉴·프리뷰 상태가 섞이지 않는다.

### 9단계: 탐색기 및 엔진 종료 연결

- [ ] ContentDrawer에 일반 애셋 열기 콜백을 추가한다.
- [ ] 메시 더블클릭을 Manager.OpenEditor로 연결한다.
- [ ] 기존 폴더 이동·Widget 열기 동작을 유지한다.
- [ ] 엔진 종료 시 새 요청을 차단하고 모든 편집기를 정리한다.
- [ ] Manager를 UI·ImGui·렌더 장치보다 먼저 정리한다.

## 9. 최종 검증 체크리스트

- [ ] 같은 애셋을 다시 열면 기존 탭과 분리된 창이 활성화된다.
- [ ] 같은 파일명을 가진 다른 경로의 애셋은 별도로 열린다.
- [ ] 여러 편집기의 분리·재도킹·활성화가 동작한다.
- [ ] 메인 메뉴와 분리된 창 메뉴가 올바른 탭을 따른다.
- [ ] 닫은 애셋을 다시 열 수 있고 오래된 참조가 남지 않는다.
- [ ] 초기화 실패·미지원 파일·잘못된 경로가 안전하게 처리된다.
- [ ] 프리뷰를 연 상태에서 레벨 변경과 PIE 시작·종료가 정상 동작한다.
- [ ] 여러 프리뷰가 전역 코루틴이나 게임 처리를 중복 실행하지 않는다.
- [ ] 크기 변경·크기 0·숨김·다중 분리 창이 처리된다.
- [ ] 편집기를 여러 개 연 채 엔진을 종료해도 자원 수명 문제가 없다.
- [ ] 적절한 구성에서 컴파일·링크하고 실제 화면 동작을 확인한다.

## 10. 권장 구현 묶음

1. 1~3단계: 공통 계약과 임시 탭을 통한 안전한 수명 관리.
2. 4~7단계: StaticMeshEditor 하나를 열기부터 렌더·닫기까지 완성.
3. 8단계: SkeletalMesh 기본 포즈 프리뷰로 확장.
4. 9단계 및 전체 검증: 탐색기 연결과 종료·회귀 확인.
5. 후속 기능: 애셋 수정·저장, 애니메이션 재생, 본 선택, LOD·머티리얼 편집 등.
