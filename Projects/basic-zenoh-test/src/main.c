#include <stdio.h>
#include <string.h>
#include <zenoh-pico.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE "[PORTABLE] Pub from Zenoh-Pico!"

void main(void) {
    printf("=== Portable Zenoh Publisher ===\n");
    printf("Board: %s\n", CONFIG_BOARD);
    printf("Console device: %s\n",
#ifdef CONFIG_USB_CDC_ACM
           "USB CDC ACM"
#else
           "UART"
#endif
    );

    // Wait for console to be ready
    k_sleep(K_SECONDS(2));

    // Configure Zenoh session
    z_owned_config_t config;
    if (z_config_default(&config) != 0) {
        printf("Unable to create config!\n");
        return;
    }

    // Use a generic serial endpoint - the transport will use the active console device
    zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, "client");
    zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, "serial/console#baudrate=115200");

    printf("Opening Zenoh Session...\n");
    z_owned_session_t session;
    if (z_open(&session, z_move(config), NULL) != 0) {
        printf("Unable to open session!\n");
        printf("Make sure zenohd is running and connected to this device\n");
        return;
    }
    printf("✓ Zenoh session opened\n");

    printf("Declaring publisher for '%s'...\n", KEYEXPR);
    z_owned_keyexpr_t keyexpr;
    if (z_keyexpr_from_str(&keyexpr, KEYEXPR) != 0) {
        printf("Unable to create keyexpr!\n");
        return;
    }

    z_owned_publisher_t pub;
    if (z_declare_publisher(z_loan(session), &pub, z_loan(keyexpr), NULL) != 0) {
        printf("Unable to declare publisher!\n");
        return;
    }
    printf("✓ Publisher declared\n");

    printf("\n🚀 Ready! Publishing data every second...\n");
    printf("💡 Usage:\n");
    printf("   1. Connect this device via USB/UART\n");
    printf("   2. Run: zenohd\n");
    printf("   3. Listen: z_sub \"demo/example/**\"\n\n");

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
        }
    }
}