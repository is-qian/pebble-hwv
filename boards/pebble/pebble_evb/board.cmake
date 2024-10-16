if(CONFIG_SOC_NRF5340_CPUAPP_QKAA)
  board_runner_args(jlink "--device=nrf5340_xxaa_app" "--speed=4000")
endif()

if(CONFIG_SOC_NRF5340_CPUNET_QKAA)
  board_runner_args(jlink "--device=nrf5340_xxaa_net" "--speed=4000")
endif()

board_runner_args(openocd --cmd-pre-init "source [find ${BOARD_DIR}/openocd.cfg]")
board_runner_args(openocd --cmd-pre-init "source [find target/nrf53.cfg]")
if(CONFIG_SOC_NRF5340_CPUNET_QKAA)
  board_runner_args(openocd --cmd-pre-init "targets nrf53.cpunet")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/nrfjprog.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
