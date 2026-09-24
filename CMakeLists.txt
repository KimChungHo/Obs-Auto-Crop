cmake_minimum_required(VERSION 3.28)
project(obs-auto-crop VERSION 0.1.0 LANGUAGES C CXX)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(obs-auto-crop MODULE src/plugin-main.cpp src/black-border.cpp)
target_compile_features(obs-auto-crop PRIVATE cxx_std_17)
if(TARGET OBS::frontend-api)
  set(obs_frontend_target OBS::frontend-api)
else()
  set(obs_frontend_target OBS::obs-frontend-api)
endif()
target_link_libraries(obs-auto-crop PRIVATE OBS::libobs ${obs_frontend_target} Qt6::Core Qt6::Widgets)
set_target_properties(obs-auto-crop PROPERTIES PREFIX "" OUTPUT_NAME "obs-auto-crop")

install(TARGETS obs-auto-crop LIBRARY DESTINATION obs-plugins/64bit RUNTIME DESTINATION obs-plugins/64bit)
install(DIRECTORY data/ DESTINATION data/obs-plugins/obs-auto-crop)
