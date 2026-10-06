# Keep the kb-1 power LED separate from the settings-reset firmware.
if(CONFIG_SHIELD_KB_1)
  zephyr_library()
  zephyr_library_sources(src/power_led.c)
endif()
