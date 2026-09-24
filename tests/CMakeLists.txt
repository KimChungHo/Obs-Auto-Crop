cmake_minimum_required(VERSION 3.20)
project(obs_auto_crop_detection_tests LANGUAGES CXX)

enable_testing()
add_executable(black-border-tests black-border-tests.cpp ../src/black-border.cpp)
target_compile_features(black-border-tests PRIVATE cxx_std_17)
add_test(NAME black-border-tests COMMAND black-border-tests)
