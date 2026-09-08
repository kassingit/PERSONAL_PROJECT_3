#include <driver/i2c_master.h>
#include <esp_log.h>
#include "esp_rom_sys.h"

// SDA AND SCL PINS DEFINITION
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22

// THE I2C CONTROLLER CHOICE
#define I2C_MASTER_NUM I2C_NUM_0

// FREQUENCY DEFINITION (SCL)
#define I2C_MASTER_FREQ_HZ 100000

static const char *TAG = "I2C_DRIVER";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t lcd_handle;

esp_err_t i2c_master_init(void){
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true
    };

    esp_err_t err = i2c_new_master_bus(&i2c_bus_config, &bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "FAIL TO CREATE THE I2C DRIVER: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "I2C BUS SUCCESSFULLY CREATED (SDA = %d, SCL = %d)", I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO);
    return ESP_OK;
}

// FUNCTION FOR SCANNING THE CONNECTED DEVICES AND GET THEIR ADDRESS
void addressScan(void){
    esp_err_t val;
    for (int i = 0x08; i <= 0x77; i++) {
        val = i2c_master_probe(bus_handle, i, 50);
        if (val == ESP_OK) {
            ESP_LOGI(TAG, "DEVICE FOUND AT THE ADDRESS :%02X", i);
        }
    }
}

// TO SEND NIBBLE (4 BITS)
void lcd_write_nibble(uint8_t nibble, uint8_t rs)
{
    // BL=1, E=1, RW=0
    uint8_t octet_E1 = (nibble << 4) | (1 << 3) | (1 << 2) | rs;
    uint8_t octet_E0 = (nibble << 4) | (1 << 3) | rs; // E=0, déclenche la lecture

    uint8_t buffer1[1] = {octet_E1};
    uint8_t buffer2[1] = {octet_E0};

    i2c_master_transmit(lcd_handle, buffer1, 1, 100);
    esp_rom_delay_us(100);
    i2c_master_transmit(lcd_handle, buffer2, 1, 100);
    esp_rom_delay_us(100);
}

// SEND A BYTE
void lcd_send_byte(uint8_t byte, uint8_t rs){
    uint8_t first_nibble = (byte >> 4);
    lcd_write_nibble(first_nibble, rs);

    esp_rom_delay_us(100);

    uint8_t second_nibble = byte & 0x0F;
    lcd_write_nibble(second_nibble, rs);
}

void lcd_init(void){
    esp_rom_delay_us(50000); // 50ms, attente après power-on

    lcd_write_nibble(0x03, 0);
    esp_rom_delay_us(4500);

    lcd_write_nibble(0x03, 0);
    esp_rom_delay_us(4500);

    lcd_write_nibble(0x03, 0);
    esp_rom_delay_us(150);

    lcd_write_nibble(0x02, 0); // Passage en mode 4 bits

    lcd_send_byte(0x28, 0); // Function Set : 4 bits, 2 lignes, 5x8
    lcd_send_byte(0x0C, 0); // Display ON, curseur OFF, blink OFF

    lcd_send_byte(0x01, 0); // Clear Display
    esp_rom_delay_us(2000); // Délai supplémentaire, cette commande est lente

    lcd_send_byte(0x06, 0); // Entry Mode Set : incrément auto du curseur
}

void app_main(void){
    ESP_ERROR_CHECK(i2c_master_init());
    addressScan();

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x27,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
        .scl_wait_us = 1000
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &lcd_handle));
    ESP_LOGI(TAG, "LCD device added to I2C bus");

    lcd_init();

    esp_rom_delay_us(10000);

    const char* message = "HELLO WORLD !";
    for (int i = 0; message[i] != '\0'; i++) {
        lcd_send_byte(message[i], 1); // rs=1 pour les données
        esp_rom_delay_us(100);
    }
}