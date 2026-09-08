//===== TEST GITHUB
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

static const char *TAG = "[I2C_DRIVER]";
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

    esp_err_t err = i2c_new_master_bus(&i2c_bus_config,&bus_handle);
    if(err!=ESP_OK){
        ESP_LOGE(TAG,"FAIL TO CREATE THE I2C DRIVER",esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG,"I2C BUS SUCCESSFULLY CREATED (SDA = %d, SCL = %d)",I2C_MASTER_SDA_IO,I2C_MASTER_SCL_IO);
    return ESP_OK;
}


// FUNCTION FOR SCANNING THE CONNECTED DEVICES AND GET THEIR ADDRESS
void addressScan(){
    esp_err_t val;
    for (int i =0x08; i<=0x77; i++){
        val = i2c_master_probe(bus_handle,i,50);

        if(val== ESP_OK){
            ESP_LOGI(TAG,"DEVICE FOUND AT THE ADDRESS :%02X",i);
        }
    

    }
}

// TO SEND NIBLLE (4 BITS)
void lcd_write_nibble(uint8_t nibble, uint8_t rs)
{
    // BL=1, E=1, RW=0
    uint8_t octet_E1 = (nibble << 4) | (1 << 3) | (1 << 2) | 0 | rs; // nibble110rs = nibbleBLERWrs
    uint8_t octet_E0 = (nibble << 4) | (1 << 3) | 0| 0 | rs;// E= 0 for launching the reading

    uint8_t buffer1[1] = {octet_E1};
    uint8_t buffer2[1] = {octet_E0};

    i2c_master_transmit(lcd_handle, buffer1,1, 100);
    esp_rom_delay_us(100);
    i2c_master_transmit(lcd_handle, buffer2,1, 100);
    esp_rom_delay_us(100);

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

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle,&dev_config,&lcd_handle));
    ESP_LOGI(TAG,"LCD device added to I2C bus");
}