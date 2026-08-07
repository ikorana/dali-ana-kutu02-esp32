#include "eth5500.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nvs.h"

#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_netif_ip_addr.h"
#include "freertos/event_groups.h"

#include "lwip/ip4_addr.h"  // IP4_ADDR makrosu için temel tanım
#include "esp_netif_ip_addr.h"  // ESP-IDF özel tanımlar

#include "esp_eth_phy_w5500.h"
#include "esp_eth_mac_w5500.h"

//#include "w5500.h"

ethernet_config_t *config;
EventGroupHandle_t Event;

#define CONFIG_EXAMPLE_ETH_SPI_HOST 1
#define CONFIG_EXAMPLE_ETH_SPI_SCLK GPIO_NUM_21
#define CONFIG_EXAMPLE_ETH_SPI_MOSI GPIO_NUM_19
#define CONFIG_EXAMPLE_ETH_SPI_MISO GPIO_NUM_18
#define CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ 16
#define CONFIG_EXAMPLE_ETH_SPI_CS GPIO_NUM_22
#define CONFIG_EXAMPLE_ETH_SPI_INT GPIO_NUM_23
#define CONFIG_EXAMPLE_ETH_SPI_POLLING_MS 0
#define CONFIG_EXAMPLE_ETH_SPI_PHY_RST GPIO_NUM_5
#define CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR 1

#define SPI_ETHERNETS_NUM 1
#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config, num)                                      \
    do {                                                                                        \
        eth_module_config[num].spi_cs_gpio = CONFIG_EXAMPLE_ETH_SPI_CS;           \
        eth_module_config[num].int_gpio = CONFIG_EXAMPLE_ETH_SPI_INT ;             \
        eth_module_config[num].polling_ms = CONFIG_EXAMPLE_ETH_SPI_POLLING_MS;         \
        eth_module_config[num].phy_reset_gpio = CONFIG_EXAMPLE_ETH_SPI_PHY_RST;   \
        eth_module_config[num].phy_addr = CONFIG_EXAMPLE_ETH_SPI_PHY_ADDR;                \
    } while(0)

typedef struct {
    uint8_t spi_cs_gpio;
    int8_t int_gpio;
    uint32_t polling_ms;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
    uint8_t *mac_addr;
} spi_eth_module_config_t;

static const char *TAG = "w5500_init";
static bool gpio_isr_svc_init_by_eth = false;

#define NVS_NAMESPACE "mac_config"
#define NVS_KEY "custom_mac"

// NVS'ye MAC adresini yaz
esp_err_t save_mac_to_nvs(uint8_t *mac) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(nvs_handle, NVS_KEY, mac, 6);
    if (err == ESP_OK) nvs_commit(nvs_handle);

    nvs_close(nvs_handle);
    return err;
}

// NVS'den MAC adresini oku
esp_err_t load_mac_from_nvs(uint8_t *mac) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) return err;

    size_t len = 6;
    err = nvs_get_blob(nvs_handle, NVS_KEY, mac, &len);
    nvs_close(nvs_handle);
    return err;
}

void set_custom_mac(bool change) {
    uint8_t custom_mac[6];
    
    // NVS'den MAC'i oku (yoksa rastgele oluştur)
    esp_err_t stat = load_mac_from_nvs(custom_mac);
    if (change) stat = ESP_ERR_INVALID_STATE;
    if (stat != ESP_OK) {
        custom_mac[0] = 0x02; // Local MAC (LAA)
        for (int i = 1; i < 6; i++) {
            custom_mac[i] = esp_random() % 256;
        }
        save_mac_to_nvs(custom_mac); // NVS'ye kaydet
    }

    // MAC adresini ayarla
    esp_base_mac_addr_set(custom_mac);
}

static esp_err_t spi_bus_init(void)
{
    esp_err_t ret = ESP_OK;
    ret = gpio_install_isr_service(0);
    if (ret == ESP_OK) {
        gpio_isr_svc_init_by_eth = true;
    } else if (ret == ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "GPIO ISR handler has been already installed");
        ret = ESP_OK; // ISR handler has been already installed so no issues
    } else {
        ESP_LOGE(TAG, "GPIO ISR handler install failed");
        goto err;
    }

    // Init SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_EXAMPLE_ETH_SPI_MISO,
        .mosi_io_num = CONFIG_EXAMPLE_ETH_SPI_MOSI,
        .sclk_io_num = CONFIG_EXAMPLE_ETH_SPI_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_GOTO_ON_ERROR(spi_bus_initialize(CONFIG_EXAMPLE_ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO),
                        err, TAG, "SPI host #%d init failed", CONFIG_EXAMPLE_ETH_SPI_HOST);

err:
    return ret;
}

static esp_eth_handle_t eth_init_spi(spi_eth_module_config_t *spi_eth_module_config, esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    esp_eth_handle_t ret = NULL;

    nvs_flash_init();

    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    // Update PHY config based on board specific configuration
    phy_config.phy_addr = spi_eth_module_config->phy_addr;
    phy_config.reset_gpio_num = spi_eth_module_config->phy_reset_gpio;

    // Configure SPI interface for specific SPI module
    spi_device_interface_config_t spi_devcfg = {
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_ETH_SPI_CLOCK_MHZ * 1000 * 1000,
        .queue_size = 20,
        .spics_io_num = spi_eth_module_config->spi_cs_gpio
    };
    // Init vendor specific MAC config to default, and create new SPI Ethernet MAC instance
    // and new PHY instance based on board configuration

    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(CONFIG_EXAMPLE_ETH_SPI_HOST, &spi_devcfg);
    w5500_config.int_gpio_num = spi_eth_module_config->int_gpio;
    w5500_config.poll_period_ms = spi_eth_module_config->polling_ms;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);

    // Init Ethernet driver to default and install it
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_GOTO_ON_FALSE(esp_eth_driver_install(&eth_config_spi, &eth_handle) == ESP_OK, NULL, err, TAG, "SPI Ethernet driver install failed");

    // The SPI Ethernet module might not have a burned factory MAC address, we can set it manually.
    if (spi_eth_module_config->mac_addr != NULL) {
        ESP_GOTO_ON_FALSE(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, spi_eth_module_config->mac_addr) == ESP_OK,
                                        NULL, err, TAG, "SPI Ethernet MAC address config failed");
    }

    if (mac_out != NULL) {
        *mac_out = mac;
    }
    if (phy_out != NULL) {
        *phy_out = phy;
    }
    return eth_handle;
err:
    if (eth_handle != NULL) {
        esp_eth_driver_uninstall(eth_handle);
    }
    if (mac != NULL) {
        mac->del(mac);
    }
    if (phy != NULL) {
        phy->del(phy);
    }
    return ret;
}

esp_err_t eth_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out, bool change_mac)
{
    esp_err_t ret = ESP_OK;
    esp_eth_handle_t *eth_handles = NULL;
    uint8_t eth_cnt = 0;

    nvs_flash_init();
    set_custom_mac(change_mac);

    ESP_GOTO_ON_FALSE(eth_handles_out != NULL && eth_cnt_out != NULL, ESP_ERR_INVALID_ARG,
                        err, TAG, "invalid arguments: initialized handles array or number of interfaces");
    eth_handles = calloc(SPI_ETHERNETS_NUM, sizeof(esp_eth_handle_t));
    ESP_GOTO_ON_FALSE(eth_handles != NULL, ESP_ERR_NO_MEM, err, TAG, "no memory");


    ESP_GOTO_ON_ERROR(spi_bus_init(), err, TAG, "SPI bus init failed");
    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config[SPI_ETHERNETS_NUM] = { 0 };
    INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 0);
    // The SPI Ethernet module(s) might not have a burned factory MAC address, hence use manually configured address(es).
    // In this example, Locally Administered MAC address derived from ESP32x base MAC address is used.
    // Note that Locally Administered OUI range should be used only when testing on a LAN under your control!
    uint8_t base_mac_addr[ETH_ADDR_LEN];
    //ESP_GOTO_ON_ERROR(esp_efuse_mac_get_default(base_mac_addr), err, TAG, "get EFUSE MAC failed");
    ESP_GOTO_ON_ERROR(esp_base_mac_addr_get(base_mac_addr), err, TAG, "get EFUSE MAC failed");
    uint8_t local_mac_1[ETH_ADDR_LEN];
    esp_derive_local_mac(local_mac_1, base_mac_addr);
    spi_eth_module_config[0].mac_addr = local_mac_1;

    for (int i = 0; i < SPI_ETHERNETS_NUM; i++) {
        eth_handles[eth_cnt] = eth_init_spi(&spi_eth_module_config[i], NULL, NULL);
        ESP_GOTO_ON_FALSE(eth_handles[eth_cnt], ESP_FAIL, err, TAG, "SPI Ethernet init failed");
        eth_cnt++;
    }

    *eth_handles_out = eth_handles;
    *eth_cnt_out = eth_cnt;
    return ret;

err:
    free(eth_handles);
    return ret;
}

esp_err_t example_eth_deinit(esp_eth_handle_t *eth_handles, uint8_t eth_cnt)
{
    ESP_RETURN_ON_FALSE(eth_handles != NULL, ESP_ERR_INVALID_ARG, TAG, "array of Ethernet handles cannot be NULL");
    for (int i = 0; i < eth_cnt; i++) {
        esp_eth_mac_t *mac = NULL;
        esp_eth_phy_t *phy = NULL;
        if (eth_handles[i] != NULL) {
            esp_eth_get_mac_instance(eth_handles[i], &mac);
            esp_eth_get_phy_instance(eth_handles[i], &phy);
            ESP_RETURN_ON_ERROR(esp_eth_driver_uninstall(eth_handles[i]), TAG, "Ethernet %p uninstall failed", eth_handles[i]);
        }
        if (mac != NULL) {
            mac->del(mac);
        }
        if (phy != NULL) {
            phy->del(phy);
        }
    }

    spi_bus_free(CONFIG_EXAMPLE_ETH_SPI_HOST);
    if (gpio_isr_svc_init_by_eth) {
        ESP_LOGW(TAG, "uninstalling GPIO ISR service!");
        gpio_uninstall_isr_service();
    }
    free(eth_handles);
    return ESP_OK;
}

esp_err_t ethernet_init(ethernet_config_t *conf)
{
    config = conf;
    uint8_t eth_port_cnt = 0;
    esp_eth_handle_t *eth_handles;
    Event = xEventGroupCreate();
    xEventGroupClearBits(Event, ETH_CONNECTED_BIT | ETH_FAIL_BIT);

    ESP_ERROR_CHECK(eth_init(&eth_handles, &eth_port_cnt, config->mac_change));
    //ESP_ERROR_CHECK(esp_netif_init());
   // ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *eth_netifs[eth_port_cnt];
    esp_eth_netif_glue_handle_t eth_netif_glues[eth_port_cnt];


    if (config->dhcp)
      {
        //Dynamic ip
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);
        eth_netif_glues[0] = esp_eth_new_netif_glue(eth_handles[0]);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
      } else {
        //Static ip
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);

        esp_netif_dhcpc_stop(eth_netifs[0]);
        esp_netif_ip_info_t ip_info={};
        ip_info.ip.addr = esp_ip4addr_aton((const char *)config->ip);
        ip_info.netmask.addr = esp_ip4addr_aton((const char *)config->netmask);
        ip_info.gw.addr = esp_ip4addr_aton((const char *)config->gw);
        esp_netif_set_ip_info(eth_netifs[0], &ip_info);

        esp_netif_dns_info_t dns;
        dns.ip.type = ESP_IPADDR_TYPE_V4; 
        dns.ip.u_addr.ip4.addr = esp_ip4addr_aton((const char *)config->dns);
        ESP_ERROR_CHECK(esp_netif_set_dns_info(eth_netifs[0], ESP_NETIF_DNS_MAIN, &dns));

        esp_netif_dns_info_t bdns;
        bdns.ip.type = ESP_IPADDR_TYPE_V4; 
        bdns.ip.u_addr.ip4.addr = esp_ip4addr_aton((const char *)config->backup_dns);
        ESP_ERROR_CHECK(esp_netif_set_dns_info(eth_netifs[0], ESP_NETIF_DNS_BACKUP, &bdns));
 
        eth_netif_glues[0] = esp_eth_new_netif_glue(eth_handles[0]);
        ESP_ERROR_CHECK(esp_netif_attach(eth_netifs[0], eth_netif_glues[0]));
      }

    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, config->eth_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, config->ip_handler, NULL));

    // Start Ethernet driver state machine
    for (int i = 0; i < eth_port_cnt; i++) {
        ESP_ERROR_CHECK(esp_eth_start(eth_handles[i]));
    }  

    /* esp_netif_dns_info_t dns_info;
    if (esp_netif_get_dns_info(eth_netifs[0], ESP_NETIF_DNS_MAIN, &dns_info) == ESP_OK) {
        ESP_LOGI("DNS", "Main DNS: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    }
    if (esp_netif_get_dns_info(eth_netifs[0], ESP_NETIF_DNS_BACKUP, &dns_info) == ESP_OK) {
        ESP_LOGI("DNS", "Backup DNS: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    }  */
    const TickType_t xTicksToWait = 10000 / portTICK_PERIOD_MS;
    EventBits_t uxBits;

    uxBits =  xEventGroupWaitBits(Event,
    		ETH_CONNECTED_BIT | ETH_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            xTicksToWait);
    if (uxBits==0) return ESP_FAIL;

    xEventGroupClearBits(Event, ETH_CONNECTED_BIT | ETH_FAIL_BIT);

    return ESP_OK;
}

void set_ethernet_connect_bit(void) {
        xEventGroupSetBits(Event, ETH_CONNECTED_BIT);
}

void set_ethernet_fail_bit(void) {
    xEventGroupSetBits(Event, ETH_FAIL_BIT);
}