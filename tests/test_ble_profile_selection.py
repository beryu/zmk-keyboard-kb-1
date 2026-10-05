"""Exercise the patched ZMK functions with mocked BLE calls.

Usage: python3 tests/test_ble_profile_selection.py /path/to/zmk/app/src/ble.c
Pass an unpatched v0.3 source; no radio or Zephyr SDK is required.
"""
import pathlib
import subprocess
import sys
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    workspace = pathlib.Path(directory)
    source = workspace / 'app/src/ble.c'
    source.parent.mkdir(parents=True)
    source.write_text(pathlib.Path(sys.argv[1]).read_text())
    subprocess.run(['git', 'init', '-q', directory], check=True)
    subprocess.run(['git', 'apply', str(root / 'patches/zmk-ble-profile-advertising-restart.patch')], cwd=workspace, check=True)
    text = source.read_text()
    stop_macro = text[text.index('#define CHECKED_ADV_STOP()'):text.index('#define CHECKED_DIR_ADV()')]
    functions = text[text.index('static int update_profile_advertising('):text.index('int zmk_ble_prof_next(')]
    harness = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#define ZMK_BLE_PROFILE_COUNT 4
#define LOG_DBG(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define ZMK_ADV_NONE 0
static uint8_t active_profile;
static int advertising_status;
static bool connected[4];
static int stops, updates, saves, events, stop_error, update_error;
static bool zmk_ble_active_profile_is_connected(void) { return connected[active_profile]; }
static int bt_le_adv_stop(void) { stops++; return stop_error; }
static int update_advertising(void) { updates++; return update_error; }
static int ble_save_profile(void) { saves++; return 0; }
static void raise_profile_changed_event(void) { events++; }
'''
    harness += stop_macro + functions
    harness += r'''
static void reset(void) {
    active_profile = 0;
    advertising_status = 2;
    for (int i = 0; i < 4; i++) connected[i] = false;
    stops = updates = saves = events = stop_error = update_error = 0;
}
int main(void) {
    reset();
    assert(zmk_ble_prof_select(4) == -ERANGE);
    assert(stops == 0 && updates == 0 && saves == 0 && events == 0);
    reset(); connected[0] = true;
    assert(zmk_ble_prof_select(0) == 0);
    assert(stops == 0 && updates == 0 && saves == 0 && events == 0 && connected[0]);
    reset(); connected[0] = connected[1] = true;
    assert(zmk_ble_prof_select(1) == 0);
    assert(active_profile == 1 && stops == 0 && updates == 1 && saves == 1 && events == 1);
    assert(connected[0] && connected[1]);
    reset();
    assert(zmk_ble_prof_select(0) == 0);
    assert(stops == 1 && updates == 1 && advertising_status == ZMK_ADV_NONE);
    assert(saves == 0 && events == 0);
    reset(); advertising_status = ZMK_ADV_NONE;
    assert(zmk_ble_prof_select(0) == 0);
    assert(stops == 1 && updates == 1);
    reset(); connected[0] = true;
    assert(zmk_ble_prof_select(1) == 0);
    assert(active_profile == 1 && stops == 1 && updates == 1 && saves == 1 && events == 1);
    assert(connected[0]);
    reset(); stop_error = -EIO;
    assert(zmk_ble_prof_select(1) == -EIO);
    assert(stops == 1 && updates == 0 && saves == 1 && events == 1);
    reset(); update_error = -ENOMEM;
    assert(zmk_ble_prof_select(0) == -ENOMEM);
    assert(stops == 1 && updates == 1 && saves == 0 && events == 0);
    return 0;
}
'''
    test = workspace / 'test.c'
    test.write_text(harness)
    binary = workspace / 'test'
    subprocess.run(['cc', '-std=c99', '-Wall', '-Wextra', '-Werror', str(test), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print('PASS: 8 BLE profile selection scenarios')
    # Verify module patch application, repeat configuration, and mismatch failure.
    cmake_args = ['cmake', '-DCONFIG_ZMK_BLE=ON', f'-DAPPLICATION_SOURCE_DIR={workspace / "app"}', '-P', str(root / 'CMakeLists.txt')]
    subprocess.run(['git', 'apply', '--reverse', str(root / 'patches/zmk-ble-profile-advertising-restart.patch')], cwd=workspace, check=True)
    subprocess.run(cmake_args, check=True)
    subprocess.run(cmake_args, check=True)
    source.write_text('/* incompatible upstream source */\n')
    assert subprocess.run(cmake_args, capture_output=True).returncode != 0
    subprocess.run(['cmake', '-DCONFIG_ZMK_BLE=OFF', '-P', str(root / 'CMakeLists.txt')], check=True)
    print('PASS: patch application, idempotence, mismatch rejection, BLE-disabled build')
