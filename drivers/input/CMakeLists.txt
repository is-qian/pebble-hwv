zephyr_library_amend()
zephyr_library_sources_ifdef(CONFIG_INPUT_LSM6DSO_TAP input_lsm6dso_tap.c)

if(CONFIG_INPUT_LSM6DSO_TAP)
  # TODO: fix hal_st glue code location upstream
  zephyr_library_include_directories(${ZEPHYR_BASE}/drivers/sensor/st/stmemsc)
  if(NOT CONFIG_SENSOR)
    zephyr_library_sources(
      ${ZEPHYR_BASE}/drivers/sensor/st/stmemsc/stmemsc_spi.c
      ${ZEPHYR_BASE}/drivers/sensor/st/stmemsc/stmemsc_mdelay.c
    )
  endif()
endif()
