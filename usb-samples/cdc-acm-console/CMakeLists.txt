cmake_minimum_required(VERSION 3.20.0)

include(ztest_build_extensions)

zephyr_include_module(:self)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(cdc-acm-console)

target_sources(app PRIVATE src/main.c)