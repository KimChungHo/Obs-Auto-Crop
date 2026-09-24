# OBS Auto Crop

OBS 소스의 **변환 → 자동 크롭**을 선택하면 현재 프레임의 검은 가장자리를 찾아 해당 장면 항목의 크롭 값에 적용합니다. 한 번 실행하는 명령이며, OBS의 실행 취소로 되돌릴 수 있습니다.

## 빌드

Windows에서는 Visual Studio 2022, CMake 3.28 이상, OBS 개발 패키지(`libobs`, `obs-frontend-api`)와 OBS가 사용하는 Qt 6 개발 패키지가 필요합니다. 이 PC에 설치된 OBS는 32.2.2이므로 개발 패키지도 32.2.2와 x64 아키텍처에 맞추세요. [OBS 공식 플러그인 템플릿](https://github.com/obsproject/obs-plugintemplate)의 빌드 환경 안내를 참고할 수 있습니다.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/path/to/obs-sdk;C:/path/to/Qt/6.x/msvc2022_64"
cmake --build build --config RelWithDebInfo
```

생성된 `obs-auto-crop.dll`은 `C:/ProgramData/obs-studio/plugins/obs-auto-crop/bin/64bit/`에, `data/locale/`의 모든 언어 파일은 `C:/ProgramData/obs-studio/plugins/obs-auto-crop/data/locale/`에 넣습니다. CMake 설치를 사용한다면 `cmake --install build --config RelWithDebInfo --prefix "C:/ProgramData/obs-studio"`로 같은 구조를 만들 수 있습니다. OBS를 다시 시작하면 메뉴에 표시됩니다.

플러그인은 OBS 32.2.2의 77개 언어 코드에 맞춰 **자동 크롭** 메뉴 이름과 실행 취소 이름을 표시합니다. 오류 및 안내 문구는 한국어와 영어가 제공되며, 다른 언어에서는 영어로 표시됩니다.

검은 여백 감지 로직은 OBS 없이 따로 테스트할 수 있습니다.

```powershell
cmake -S tests -B build/tests -G "Visual Studio 17 2022" -A x64
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

## 자동 릴리즈

`.github/workflows/release.yml`은 `v1.0.0`처럼 `v`로 시작하는 세 자리 버전 태그를 푸시하면 Windows x64, macOS universal(Apple Silicon 및 Intel), Ubuntu 26.04 x86_64용 플러그인을 각각 빌드합니다. 세 빌드와 테스트가 모두 성공하면 GitHub Release를 만들고 OS별 설치 파일(`.exe`, `.pkg`, `.deb`)과 수동 설치용 압축 파일(`.zip`, `.zip`, `.tar.gz`)을 함께 첨부합니다. macOS 패키지와 플러그인은 서명하거나 공증하지 않았습니다.

```bash
git tag v1.0.0
git push origin v1.0.0
```

태그를 만들기 전에 워크플로 파일이 기본 브랜치에 푸시되어 있어야 합니다. GitHub 저장소의 Actions 권한에서 `GITHUB_TOKEN`에 Release 생성 권한(`contents: write`)이 허용되어야 합니다. Actions 화면에서 수동으로 빌드 검증을 실행할 수도 있으며, 수동 실행은 Release를 만들지 않습니다.

다운로드한 파일의 설치 방법은 다음과 같습니다. 설치 파일과 압축 파일 중 하나만 사용하세요.

| OS | 설치 파일 | 압축 파일로 수동 설치 |
| --- | --- | --- |
| Windows | `obs-auto-crop-버전-windows-x64-setup.exe`를 실행하고 마법사에서 `bin/64bit/obs64.exe`가 들어 있는 OBS 설치 폴더를 선택합니다. | Windows ZIP의 `plugins/` 폴더를 `C:/ProgramData/obs-studio/`에 풀어 `plugins/obs-auto-crop/`이 만들어지게 합니다. |
| macOS | `.pkg`를 열어 현재 사용자 홈에 설치합니다. OBS 앱의 설치 위치와 무관합니다. | macOS ZIP의 `obs-auto-crop.plugin`을 `~/Library/Application Support/obs-studio/plugins/`에 복사합니다. |
| Ubuntu 26.04 | `.deb`를 패키지 설치 앱으로 열거나 `sudo apt install ./obs-auto-crop-v1.0.0-ubuntu-26.04-amd64.deb`를 실행합니다. | tar.gz의 `lib/`와 `share/`를 `/usr/` 아래에 풉니다. 예: `sudo tar -C /usr -xzf obs-auto-crop-v1.0.0-ubuntu-26.04-x86_64.tar.gz`. |

Windows 설치 파일은 선택한 OBS 폴더 안에, ZIP은 `C:/ProgramData/obs-studio/`에 설치합니다. Windows ZIP을 사용하다 설치 파일로 전환한다면 `C:/ProgramData/obs-studio/plugins/obs-auto-crop/`의 기존 복사본을 먼저 삭제하세요. 설치 후 OBS를 다시 시작합니다.

압축 파일 내용은 OS별 OBS 플러그인 경로에 맞춰 다릅니다. Windows ZIP은 `plugins/` 트리, macOS ZIP은 `.plugin` 번들, Ubuntu tar.gz는 `lib/`와 `share/` 트리를 담습니다. Windows와 macOS는 OBS 32.2.2 개발 SDK로, Ubuntu는 배포판의 `libobs-dev` 패키지로 빌드합니다. Ubuntu `.deb`는 시스템 경로에 설치하므로 설치 마법사에서 경로를 변경하지 않습니다. 다른 배포판의 OBS 및 Qt 버전과 호환되지 않을 수 있습니다.

## 동작 범위

- 비디오 소스 한 개를 선택했을 때 사용할 수 있습니다. 잠긴 항목과 그룹은 제외합니다.
- 현재 프레임을 최대 1280픽셀로 축소해 분석합니다. 아주 얇은 테두리는 감지하지 못할 수 있습니다.
- 검은 화면이나 페이드 중에는 실행하지 않는 편이 좋습니다. 감지할 내용이 없으면 크롭을 적용하지 않습니다.
- OBS 프런트엔드의 `transformMenu` Qt 객체를 사용합니다. OBS 버전 변경으로 메뉴 구조가 바뀌면 연결 코드를 조정해야 합니다.
