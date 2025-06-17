#include <stdio.h>
#include <string.h>
#include <zenoh-pico.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE "[PORTABLE] Pub from Zenoh-Pico!"

void main(void) {
    printf("=== Testing Zenoh Serial Transport ===\n");
    printf("Board: %s\n", CONFIG_BOARD);

    // Wait for console to be ready
    k_sleep(K_SECONDS(1));

    printf("Testing keyexpr creation...\n");
    z_owned_keyexpr_t keyexpr;
    if (z_keyexpr_from_str(&keyexpr, KEYEXPR) != 0) {
        printf("❌ Unable to create keyexpr!\n");
        return;
    }
    printf("✓ Keyexpr created successfully\n");

    printf("Testing config creation...\n");
    z_owned_config_t config;
    if (z_config_default(&config) != 0) {
        printf("❌ Unable to create config!\n");
        return;
    }
    printf("✓ Config created successfully\n");

    // Configure for serial transport with more debugging
    printf("Configuring serial transport...\n");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, "serial/console#baudrate=115200");
    printf("✓ Serial transport configured\n");

    printf("Opening Zenoh Session...\n");
    z_owned_session_t session;
    z_result_t result = z_open(&session, z_move(config), NULL);
    if (result != 0) {
        printf("❌ Unable to open session! Error: %d\n", result);
        return;
    }
    printf("✓ Zenoh session opened successfully!\n");

    printf("Declaring publisher for '%s'...\n", KEYEXPR);
    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(session), &pub, z_loan(keyexpr), NULL) != 0) {
        printf("❌ Unable to declare publisher!\n");
        z_drop(z_move(session));
        z_drop(z_move(keyexpr));
        return;
    }
    printf("✓ Publisher declared\n");

    printf("\n🚀 Ready! Publishing data every second...\n");

    char buf[256];
    int idx = 0;

    while (1) {
        k_sleep(K_SECONDS(1));
        snprintf(buf, sizeof(buf), "[%04d] %s (Board: %s)", idx++, VALUE, CONFIG_BOARD);

        printf("📤 [%04d] Publishing: '%s'\n", idx - 1, buf);

        z_owned_bytes_t payload;
        z_bytes_copy_from_buf(&payload, (const uint8_t *)buf, strlen(buf));
        z_result_t res = z_publisher_put(z_loan(pub), z_move(payload), NULL);

        if (res != 0) {
            printf("⚠️  Failed to publish (error: %d)\n", res);
        } else {
            printf("✅ Published successfully!\n");
        }
    }

    // Cleanup (never reached in this example)
    z_drop(z_move(pub));
    z_drop(z_move(session));
    z_drop(z_move(keyexpr));

    printf("✓ Test completed successfully!\n");
}