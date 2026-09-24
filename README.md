# OBS Auto Crop

OBS 소스의 **변환 → 자동 크롭**을 선택하면 현재 프레임의 검은 가장자리를 찾아 해당 장면 항목의 크롭 값에 적용합니다. 한 번 실행하는 명령이며, OBS의 실행 취소로 되돌릴 수 있습니다.

## 빌드

Windows에서는 Visual Studio 2022, CMake 3.28 이상, OBS 개발 패키지(`libobs`, `obs-frontend-api`)와 OBS가 사용하는 Qt 6 개발 패키지가 필요합니다. 이 PC에 설치된 OBS는 32.2.2이므로 개발 패키지도 32.2.2와 x64 아키텍처에 맞추세요. [OBS 공식 플러그인 템플릿](https://github.com/obsproject/obs-plugintemplate)의 빌드 환경 안내를 참고할 수 있습니다.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/path/to/obs-sdk;C:/path/to/Qt/6.x/msvc2022_64"
cmake --build build --config RelWithDebInfo
```

생성된 `obs-auto-crop.dll`은 OBS 설치 폴더의 `obs-plugins/64bit/`에, `data/locale/ko-KR.ini`는 `data/obs-plugins/obs-auto-crop/locale/`에 넣습니다. OBS를 다시 시작하면 메뉴에 표시됩니다.

검은 여백 감지 로직은 OBS 없이 따로 테스트할 수 있습니다.

```powershell
cmake -S tests -B build/tests -G "Visual Studio 17 2022" -A x64
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

## 동작 범위

- 비디오 소스 한 개를 선택했을 때 사용할 수 있습니다. 잠긴 항목과 그룹은 제외합니다.
- 현재 프레임을 최대 1280픽셀로 축소해 분석합니다. 아주 얇은 테두리는 감지하지 못할 수 있습니다.
- 검은 화면이나 페이드 중에는 실행하지 않는 편이 좋습니다. 감지할 내용이 없으면 크롭을 적용하지 않습니다.
- OBS 프런트엔드의 `transformMenu` Qt 객체를 사용합니다. OBS 버전 변경으로 메뉴 구조가 바뀌면 연결 코드를 조정해야 합니다.
