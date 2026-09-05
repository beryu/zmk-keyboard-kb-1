// SPDX-License-Identifier: GPL-2.0-or-later
// copyright (C) 2025 sekigon-gonnoc

#include <zephyr/sys/util_macro.h>

#if IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/logging/log.h>
#include <zephyr/input/input.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/usb.h>

LOG_MODULE_REGISTER(split_power_mgmt, CONFIG_ZMK_LOG_LEVEL);

#define SLEEP1_TIMEOUT_MS 5000   // 5 seconds to sleep1 from active
#define SLEEP2_TIMEOUT_MS 15000  // 15 seconds to sleep2 from sleep1  
#define SLEEP3_TIMEOUT_MS 30000  // 30 seconds to sleep3 from sleep2
#define ACTIVE_CONN_INTERVAL CONFIG_ZMK_SPLIT_BLE_PREF_INT
#define SLEEP1_CONN_INTERVAL (CONFIG_ZMK_SPLIT_BLE_PREF_INT*2)
#define SLEEP2_CONN_INTERVAL (CONFIG_ZMK_SPLIT_BLE_PREF_INT*4)
#define SLEEP3_CONN_INTERVAL (CONFIG_ZMK_SPLIT_BLE_PREF_INT*8)
#define CONN_LATENCY CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY
#define SLEEP1_CONN_LATENCY ((CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY+1)/2)
#define SLEEP2_CONN_LATENCY ((CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY+3)/4)  
#define SLEEP3_CONN_LATENCY ((CONFIG_ZMK_SPLIT_BLE_PREF_LATENCY+7)/8) 
#define SUPERVISION_TIMEOUT CONFIG_ZMK_SPLIT_BLE_PREF_TIMEOUT

enum power_mode {
    POWER_MODE_ACTIVE,
    POWER_MODE_SLEEP1,
    POWER_MODE_SLEEP2,
    POWER_MODE_SLEEP3,
};

static struct k_work_delayable power_mode_work;
static enum power_mode current_mode = POWER_MODE_ACTIVE;
static int64_t last_activity_time = 0;
static struct bt_conn *split_conn = NULL;
K_MUTEX_DEFINE(power_state_mutex);

static bool update_mode_if_connected(struct bt_conn *conn, enum power_mode mode) {
    bool connected;

    k_mutex_lock(&power_state_mutex, K_FOREVER);
    connected = (split_conn == conn);
    if (connected) {
        current_mode = mode;
    }
    k_mutex_unlock(&power_state_mutex);

    return connected;
}

// Power mode transition handler
static void power_mode_transition(struct k_work *work) {
    struct bt_conn *conn;
    enum power_mode mode;
    int64_t activity_time;

    ARG_UNUSED(work);

    k_mutex_lock(&power_state_mutex, K_FOREVER);
    conn = split_conn == NULL ? NULL : bt_conn_ref(split_conn);
    mode = current_mode;
    activity_time = last_activity_time;
    k_mutex_unlock(&power_state_mutex);

    if (conn == NULL) {
        return;
    }
    
    // Stay in active mode when USB power is connected
    if (zmk_usb_is_powered()) {
        LOG_DBG("USB power detected, staying in active mode");
        if (mode != POWER_MODE_ACTIVE) {
            // Return to active mode
            struct bt_le_conn_param param = {
                .interval_min = ACTIVE_CONN_INTERVAL,
                .interval_max = ACTIVE_CONN_INTERVAL,
                .latency = CONN_LATENCY,
                .timeout = SUPERVISION_TIMEOUT,
            };
            
            int err = bt_conn_le_param_update(conn, &param);
            if (err == 0) {
                update_mode_if_connected(conn, POWER_MODE_ACTIVE);
                LOG_INF("Returned to active mode due to USB power");
            }
        }
        
        // Periodic check while USB power is present
        k_work_schedule(&power_mode_work, K_MSEC(5000));
        bt_conn_unref(conn);
        return;
    }
    
    int64_t idle_time = k_uptime_get() - activity_time;
    enum power_mode target_mode;
    
    // Determine target mode based on idle time
    if (idle_time >= SLEEP3_TIMEOUT_MS) {
        target_mode = POWER_MODE_SLEEP3;
    } else if (idle_time >= SLEEP2_TIMEOUT_MS) {
        target_mode = POWER_MODE_SLEEP2;
    } else if (idle_time >= SLEEP1_TIMEOUT_MS) {
        target_mode = POWER_MODE_SLEEP1;
    } else {
        target_mode = POWER_MODE_ACTIVE;
    }
    
    // Only update if different from current mode
    if (target_mode == mode) {
        // Schedule next transition
        int32_t next_timeout;
        switch (mode) {
        case POWER_MODE_ACTIVE:
            next_timeout = SLEEP1_TIMEOUT_MS - idle_time;
            break;
        case POWER_MODE_SLEEP1:
            next_timeout = SLEEP2_TIMEOUT_MS - idle_time;
            break;
        case POWER_MODE_SLEEP2:
            next_timeout = SLEEP3_TIMEOUT_MS - idle_time;
            break;
        default:
            bt_conn_unref(conn);
            return; // No further transitions from SLEEP3
        }
        
        if (next_timeout > 0) {
            k_work_schedule(&power_mode_work, K_MSEC(next_timeout));
        }
        bt_conn_unref(conn);
        return;
    }
    
    // Configure connection parameters
    struct bt_le_conn_param param;
    const char *mode_name;
    
    switch (target_mode) {
    case POWER_MODE_ACTIVE:
        param.interval_min = param.interval_max = ACTIVE_CONN_INTERVAL;
        param.latency = CONN_LATENCY;
        mode_name = "active";
        break;
    case POWER_MODE_SLEEP1:
        param.interval_min = param.interval_max = SLEEP1_CONN_INTERVAL;
        param.latency = SLEEP1_CONN_LATENCY;
        mode_name = "sleep1";
        break;
    case POWER_MODE_SLEEP2:
        param.interval_min = param.interval_max = SLEEP2_CONN_INTERVAL;
        param.latency = SLEEP2_CONN_LATENCY;
        mode_name = "sleep2";
        break;
    case POWER_MODE_SLEEP3:
        param.interval_min = param.interval_max = SLEEP3_CONN_INTERVAL;
        param.latency = SLEEP3_CONN_LATENCY;
        mode_name = "sleep3";
        break;
    }
    
    param.timeout = SUPERVISION_TIMEOUT;
    
    LOG_INF("Entering %s mode - updating connection parameters", mode_name);
    
    int err = bt_conn_le_param_update(conn, &param);
    if (err == 0) {
        bool connected = update_mode_if_connected(conn, target_mode);
        LOG_INF("%s mode activated", mode_name);
        
        // Schedule next transition
        int32_t next_timeout;
        switch (target_mode) {
        case POWER_MODE_ACTIVE:
            next_timeout = SLEEP1_TIMEOUT_MS - idle_time;
            break;
        case POWER_MODE_SLEEP1:
            next_timeout = SLEEP2_TIMEOUT_MS - idle_time;
            break;
        case POWER_MODE_SLEEP2:
            next_timeout = SLEEP3_TIMEOUT_MS - idle_time;
            break;
        default:
            bt_conn_unref(conn);
            return; // No further transitions from SLEEP3
        }
        
        if (connected && next_timeout > 0) {
            k_work_schedule(&power_mode_work, K_MSEC(next_timeout));
        }
    } else {
        LOG_WRN("Failed to update connection parameters for %s mode: %d", mode_name, err);
    }

    bt_conn_unref(conn);
}

// Reset activity timer on user input
static void reset_idle_timer(void) {
    enum power_mode mode;
    bool connected;

    LOG_DBG("Activity detected - resetting idle timer");

    k_mutex_lock(&power_state_mutex, K_FOREVER);
    last_activity_time = k_uptime_get();
    mode = current_mode;
    connected = (split_conn != NULL);
    k_mutex_unlock(&power_state_mutex);

    if (!connected) {
        k_work_cancel_delayable(&power_mode_work);
        return;
    }
    
    if (mode != POWER_MODE_ACTIVE) {
        // Queue an immediate transition instead of calling the work handler
        // directly, which could race with an already-running transition.
        k_work_reschedule(&power_mode_work, K_NO_WAIT);
    } else {
        // Schedule transition to SLEEP1 from active mode
        k_work_reschedule(&power_mode_work, K_MSEC(SLEEP1_TIMEOUT_MS));
    }
}

static int position_state_changed_listener(const zmk_event_t *eh) {
    ARG_UNUSED(eh);
    reset_idle_timer();
    return ZMK_EV_EVENT_BUBBLE;
}


ZMK_LISTENER(split_power_mgmt_position, position_state_changed_listener);
ZMK_SUBSCRIPTION(split_power_mgmt_position, zmk_position_state_changed);

static bool is_split_peripheral_conn(struct bt_conn *conn) {
    struct bt_conn_info info;
    if (bt_conn_get_info(conn, &info) != 0) {
        return false;
    }
    
    return (info.role == BT_CONN_ROLE_CENTRAL && info.type == BT_CONN_TYPE_LE);
}

static void power_mgmt_bt_conn_connected_cb(struct bt_conn *conn, uint8_t err) {
    struct bt_conn *old_conn;

    if (err || !is_split_peripheral_conn(conn)) {
        return;
    }
    
    LOG_INF("Split peripheral connection detected");

    k_mutex_lock(&power_state_mutex, K_FOREVER);
    old_conn = split_conn;
    split_conn = bt_conn_ref(conn);
    last_activity_time = k_uptime_get();
    current_mode = POWER_MODE_ACTIVE;
    k_mutex_unlock(&power_state_mutex);

    if (old_conn != NULL) {
        bt_conn_unref(old_conn);
    }

    k_work_reschedule(&power_mode_work, K_MSEC(SLEEP1_TIMEOUT_MS));
}

static void power_mgmt_bt_conn_disconnected_cb(struct bt_conn *conn, uint8_t reason) {
    struct bt_conn *old_conn;

    k_mutex_lock(&power_state_mutex, K_FOREVER);
    if (conn != split_conn) {
        k_mutex_unlock(&power_state_mutex);
        return;
    }

    old_conn = split_conn;
    split_conn = NULL;
    current_mode = POWER_MODE_ACTIVE;
    k_mutex_unlock(&power_state_mutex);

    LOG_INF("Split peripheral disconnected (reason: %d)", reason);

    k_work_cancel_delayable(&power_mode_work);
    bt_conn_unref(old_conn);
}

static struct bt_conn_cb power_mgmt_bt_conn_callbacks = {
    .connected = power_mgmt_bt_conn_connected_cb,
    .disconnected = power_mgmt_bt_conn_disconnected_cb,
};

static void mouse_input_callback(struct input_event *evt) {
    ARG_UNUSED(evt);
    reset_idle_timer();
}

static int split_power_mgmt_init(void) {
    bool connected;

    LOG_INF("Initializing split power management");
    
    k_work_init_delayable(&power_mode_work, power_mode_transition);
    
    bt_conn_cb_register(&power_mgmt_bt_conn_callbacks);
    
    k_mutex_lock(&power_state_mutex, K_FOREVER);
    connected = (split_conn != NULL);
    if (connected) {
        last_activity_time = k_uptime_get();
    }
    k_mutex_unlock(&power_state_mutex);

    if (connected) {
        k_work_schedule(&power_mode_work, K_MSEC(SLEEP1_TIMEOUT_MS));
        LOG_INF("Split power management initialized with existing connection");
    } else {
        LOG_INF("Split power management initialized - waiting for connection");
    }
    
    return 0;
}

INPUT_CALLBACK_DEFINE(DEVICE_DT_GET_OR_NULL(DT_NODELABEL(pointing_device)), mouse_input_callback);

SYS_INIT(split_power_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif /* CONFIG_ZMK_SPLIT_ROLE_CENTRAL */
