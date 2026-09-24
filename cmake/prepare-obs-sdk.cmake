# Build the OBS 32.2.2 development component used by the Windows and macOS CI jobs.
# Dependency archive checksums come from the OBS Studio and obs-deps releases.
if(NOT DEFINED SDK_ROOT OR NOT DEFINED PLATFORM)
  message(FATAL_ERROR "Set SDK_ROOT and PLATFORM (windows or macos)")
endif()

get_filename_component(SDK_ROOT "${SDK_ROOT}" ABSOLUTE)
file(MAKE_DIRECTORY "${SDK_ROOT}")
set(obs_version "32.2.2")
set(deps_version "2026-07-15")
set(obs_archive "${obs_version}.zip")
set(obs_hash "f15f001f1fa526405318835f44f9910046502f496ebc3a30d5296a5018b831aa")

if(PLATFORM STREQUAL "windows")
  set(arch "x64")
  set(deps_archive "windows-deps-${deps_version}-x64.zip")
  set(deps_hash "6f90e9598fa10cff5ad23cdcfae49b87868c07bf896b02cd464582b4ce2f2ba9")
  set(qt_archive "windows-deps-qt6-${deps_version}-x64.zip")
  set(qt_hash "7c7f985711d80467bdc1795b6592275a27d5b0e5a2c7a61db1f2c1d08d6a5579")
  set(generator "Visual Studio 17 2022")
  set(platform_args -A x64)
elseif(PLATFORM STREQUAL "macos")
  set(arch "universal")
  set(deps_archive "macos-deps-${deps_version}-universal.tar.xz")
  set(deps_hash "4ecb4c598dfa853168df6c2a0c4e0ffec8495a81fbd1ba051ef88ecd5e0f7e53")
  set(qt_archive "macos-deps-qt6-${deps_version}-universal.tar.xz")
  set(qt_hash "d4b8058612a7067e44b2205fe7925ee24e9ec6b15ac8c29d3e6230c70030b102")
  set(generator "Xcode")
  set(platform_args "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64" "-DCMAKE_OSX_DEPLOYMENT_TARGET=13.0"
    "-DCMAKE_FRAMEWORK_PATH=${SDK_ROOT}/Frameworks")
else()
  message(FATAL_ERROR "Unsupported PLATFORM: ${PLATFORM}")
endif()

function(fetch_archive name url checksum destination)
  if(EXISTS "${destination}")
    return()
  endif()
  set(archive "${SDK_ROOT}/${name}")
  if(NOT EXISTS "${archive}")
    message(STATUS "Downloading ${url}")
    file(DOWNLOAD "${url}" "${archive}" EXPECTED_HASH "SHA256=${checksum}" STATUS result SHOW_PROGRESS)
    list(GET result 0 status_code)
    if(NOT status_code EQUAL 0)
      file(REMOVE "${archive}")
      message(FATAL_ERROR "Download failed: ${url}: ${result}")
    endif()
  endif()
  if(name STREQUAL "${obs_archive}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${SDK_ROOT}")
  else()
    file(MAKE_DIRECTORY "${destination}")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${destination}")
  endif()
endfunction()

set(deps_dir "${SDK_ROOT}/obs-deps-${deps_version}-${arch}")
set(qt_dir "${SDK_ROOT}/obs-deps-qt6-${deps_version}-${arch}")
set(obs_source "${SDK_ROOT}/obs-studio-${obs_version}")
fetch_archive("${deps_archive}"
  "https://github.com/obsproject/obs-deps/releases/download/${deps_version}/${deps_archive}"
  "${deps_hash}" "${deps_dir}")
fetch_archive("${qt_archive}"
  "https://github.com/obsproject/obs-deps/releases/download/${deps_version}/${qt_archive}"
  "${qt_hash}" "${qt_dir}")
fetch_archive("${obs_archive}"
  "https://github.com/obsproject/obs-studio/archive/refs/tags/${obs_archive}"
  "${obs_hash}" "${obs_source}")

if(PLATFORM STREQUAL "macos")
  execute_process(COMMAND xattr -rc "${SDK_ROOT}" COMMAND_ERROR_IS_FATAL ANY)
endif()

set(prefixes "${deps_dir};${qt_dir};${SDK_ROOT}")
set(obs_build "${SDK_ROOT}/obs-build-${arch}")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${obs_source}" -B "${obs_build}" -G "${generator}"
  ${platform_args} "-DCMAKE_PREFIX_PATH=${prefixes}" "-DOBS_CMAKE_VERSION=3.0.0"
  "-DOBS_VERSION_OVERRIDE=${obs_version}" "-DENABLE_PLUGINS=OFF" "-DENABLE_FRONTEND=OFF"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${obs_build}" --target obs-frontend-api
  --config Release COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${CMAKE_COMMAND}" --install "${obs_build}" --component Development
  --config Release --prefix "${SDK_ROOT}" COMMAND_ERROR_IS_FATAL ANY)
