# Apply the BLE fix to the pinned ZMK source before it is compiled.
# Keep the reusable upstream build workflow and local west builds consistent.
if(CONFIG_ZMK_BLE)
  find_package(Git REQUIRED)
  set(ble_patch "${CMAKE_CURRENT_LIST_DIR}/patches/zmk-ble-profile-advertising-restart.patch")
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --check "${ble_patch}"
    WORKING_DIRECTORY "${APPLICATION_SOURCE_DIR}"
    RESULT_VARIABLE patch_check
    ERROR_QUIET
  )
  if(patch_check EQUAL 0)
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" apply "${ble_patch}"
      WORKING_DIRECTORY "${APPLICATION_SOURCE_DIR}"
      RESULT_VARIABLE patch_result
    )
    if(NOT patch_result EQUAL 0)
      message(FATAL_ERROR "Failed to apply the BLE profile advertising patch")
    endif()
  else()
    # Cached west checkouts may already contain the patch.
    execute_process(
      COMMAND "${GIT_EXECUTABLE}" apply --reverse --check "${ble_patch}"
      WORKING_DIRECTORY "${APPLICATION_SOURCE_DIR}"
      RESULT_VARIABLE reverse_check
      ERROR_QUIET
    )
    if(NOT reverse_check EQUAL 0)
      message(FATAL_ERROR "BLE profile advertising patch does not match the ZMK source")
    endif()
  endif()
endif()

# Keep the kb-1 power LED separate from the settings-reset firmware.
if(CONFIG_SHIELD_KB_1)
  zephyr_library()
  zephyr_library_sources(src/power_led.c)
  zephyr_library_sources(src/trackball_diagnostics.c)
endif()
