cmake_minimum_required(VERSION 3.13.1)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(app LANGUAGES C)

target_sources(
  app
  PRIVATE
    src/main.c
    src/ble.c
    src/buttons.c
    src/display.c
    src/flash.c
    src/haptic.c
    src/press.c
)
