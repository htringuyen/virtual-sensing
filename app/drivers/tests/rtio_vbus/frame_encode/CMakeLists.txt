cmake_minimum_required(VERSION 3.20.0)

include(ztest_build_extensions)

zephyr_include_module(:self)

find_package(Zephyr COMPONENTS ztest_build_ext HINTS $ENV{ZEPHYR_BASE})

project(test_vbus_frame_encode)

target_sources(app PRIVATE src/main.c)