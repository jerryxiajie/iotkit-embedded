/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */
#ifdef DEPRECATED_LINKKIT
#include "deprecated/solo.c"
#else
#include "stdio.h"
#include "iot_export_linkkit.h"
#include "cJSON.h"
#include "app_entry.h"

#include <stdlib.h>


#if defined(OTA_ENABLED) && defined(BUILD_AOS)
    #include "ota_service.h"
#endif

#define USE_CUSTOME_DOMAIN      (0)

// for demo only
#define PRODUCT_KEY      "a1A83PCMVng"
#define PRODUCT_SECRET   "DXxguDoumhnH5tYO"
#define DEVICE_NAME      "ali_test"
#define DEVICE_SECRET    "nAHRe1LAY1rLE51fLbccbYNHQBfKtpJF"

#define SAMPLE_LED_1      1
#define SAMPLE_LED_2      2

#if USE_CUSTOME_DOMAIN
    #define CUSTOME_DOMAIN_MQTT     "iot-as-mqtt.cn-shanghai.aliyuncs.com"
    #define CUSTOME_DOMAIN_HTTP     "iot-auth.cn-shanghai.aliyuncs.com"
#endif

#define USER_EXAMPLE_YIELD_TIMEOUT_MS (200)

#define EXAMPLE_TRACE(...)                               \
do {                                                     \
    HAL_Printf("\033[1;32;40m%s.%d: ", __func__, __LINE__);  \
    HAL_Printf(__VA_ARGS__);                                 \
    HAL_Printf("\033[0m\r\n");                                   \
} while (0)

void user_post_property(int id, int status);

typedef struct {
    int master_devid;
    int cloud_connected;
    int master_initialized;
} user_example_ctx_t;

static user_example_ctx_t g_user_example_ctx;

static user_example_ctx_t *user_example_get_ctx(void)
{
    return &g_user_example_ctx;
}

void *example_malloc(size_t size)
{
    return HAL_Malloc(size);
}

void example_free(void *ptr)
{
    HAL_Free(ptr);
}

static int user_connected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Connected");
    user_example_ctx->cloud_connected = 1;
#if defined(OTA_ENABLED) && defined(BUILD_AOS)
    ota_service_init(NULL);
#endif
    return 0;
}

static int user_disconnected_event_handler(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    EXAMPLE_TRACE("Cloud Disconnected");

    user_example_ctx->cloud_connected = 0;

    return 0;
}

static int user_down_raw_data_arrived_event_handler(const int devid, const unsigned char *payload,
    const int payload_len)
{
    EXAMPLE_TRACE("Down Raw Message, Devid: %d, Payload Length: %d", devid, payload_len);
    return 0;
}

static int user_property_set_event_handler(const int devid, const char *request, const int request_len)
{
    int res = 0;
    cJSON *root = NULL, *item_light = NULL;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    EXAMPLE_TRACE("Property Set Received, Devid: %d, Request: %s", devid, request);

    root = cJSON_Parse(request);
    if(root == NULL || !cJSON_IsObject(root))
    {
        EXAMPLE_TRACE("cJson Parse Error! \n");
        return -1;
    }

    item_light = cJSON_GetObjectItem(root,"LightSwitch1");
    if (item_light == NULL)
    {
        EXAMPLE_TRACE("Can't get LigthSwitch1!");
        item_light = cJSON_GetObjectItem(root,"LightSwitch2");
        if (item_light == NULL)
        {
            EXAMPLE_TRACE("Can't get valid Item!");
            exit(-1);
        }
        else
        {
            if(item_light->valueint == 0)
            {
                system("echo 0 > /sys/class/gpio/gpio12/value");
                user_post_property(SAMPLE_LED_2,0);
            }
            else if (item_light->valueint == 1)
            {
                system("echo 1 > /sys/class/gpio/gpio12/value");
                user_post_property(SAMPLE_LED_2,1);
            }
            else
            {
                EXAMPLE_TRACE("Invalid LED control status!");
                exit(-1);
            }
        }
    }
    else
    {
        if(item_light->valueint == 0)
        {
            system("echo 0 > /sys/class/gpio/gpio16/value");
            user_post_property(SAMPLE_LED_1,0);
        }
        else if (item_light->valueint == 1)
        {
            system("echo 1 > /sys/class/gpio/gpio16/value");
            user_post_property(SAMPLE_LED_1,1);
        }
        else
        {
            EXAMPLE_TRACE("Invalid LED control status!");
            exit(-1);
        }
    }

    EXAMPLE_TRACE("LightSwitch : %d \n",item_light->valueint);

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,(unsigned char *)request, request_len);
    EXAMPLE_TRACE("Post Property Message ID: %d", res);

    return 0;
}

static int user_report_reply_event_handler(const int devid, const int msgid, const int code, const char *reply,
    const int reply_len)
{
    const char *reply_value = (reply == NULL) ? ("NULL") : (reply);
    const int reply_value_len = (reply_len == 0) ? (strlen("NULL")) : (reply_len);

    EXAMPLE_TRACE("Message Post Reply Received, Devid: %d, Message ID: %d, Code: %d, Reply: %.*s", devid, msgid, code,
      reply_value_len,
      reply_value);
    return 0;
}

static int user_timestamp_reply_event_handler(const char *timestamp)
{
    EXAMPLE_TRACE("Current Timestamp: %s", timestamp);

    return 0;
}

static int user_initialized(const int devid)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    EXAMPLE_TRACE("Device Initialized, Devid: %d", devid);

    if (user_example_ctx->master_devid == devid) {
        user_example_ctx->master_initialized = 1;
    }

    return 0;
}

static uint64_t user_update_sec(void)
{
    static uint64_t time_start_ms = 0;

    if (time_start_ms == 0) {
        time_start_ms = HAL_UptimeMs();
    }

    return (HAL_UptimeMs() - time_start_ms) / 1000;
}

void user_post_property(int led_id, int status)
{
    int res = 0;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *property_payload = "NULL";

    if(led_id == SAMPLE_LED_1)
    {
        if (status == 0)
        {
            property_payload = "{\"LightSwitch1\":0}";
        }
        else if (status == 1)
        {
            property_payload = "{\"LightSwitch1\":1}";
        }
        else
        {
            EXAMPLE_TRACE("SAMPLE_LED_1: User post property status invalid!");
            exit(-1);
        }

    }
    else if (led_id == SAMPLE_LED_2)
    {
        if (status == 0)
        {
            property_payload = "{\"LightSwitch2\":0}";
        }
        else if (status == 1)
        {
            property_payload = "{\"LightSwitch2\":1}";
        }
        else
        {
            EXAMPLE_TRACE("SAMPLE_LED_2: User post property status invalid!");
            exit(-1);
        }
    }
    else
    {
        EXAMPLE_TRACE("User post property LED_ID invalid!");
        exit(-1);
    }

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,(unsigned char *)property_payload, strlen(property_payload));
    EXAMPLE_TRACE("Post Property Message ID: %d", res);
}

/* Turn on led */
void turn_on_led(int led_id)
{
    if (led_id == SAMPLE_LED_1)
    {
        system("echo 1 > /sys/class/gpio/gpio16/value");
    }
    else if (led_id == SAMPLE_LED_2)
    {
        system("echo 1 > /sys/class/gpio/gpio12/value");
    }
    else
    {
        EXAMPLE_TRACE("Turn on led LED_ID invalid!");
        exit(-1);
    }
}

/* Turn off led */
void turn_off_led(int led_id)
{
    if (led_id == SAMPLE_LED_1)
    {
        system("echo 0 > /sys/class/gpio/gpio16/value");
    }
    else if (led_id == SAMPLE_LED_2)
    {
        system("echo 0 > /sys/class/gpio/gpio12/value");
    }
    else
    {
        EXAMPLE_TRACE("Turn off led LED_ID invalid!");
        exit(-1);
    }
}

float get_temperature(void)
{
    int i;
    FILE * file_fd;
    char * FILE_NAME = "/sys/bus/w1/devices/28-020592461ab5/w1_slave";
    char file_buffer[256];
    float temp;

    file_fd = fopen(FILE_NAME,"r");
    if(file_fd == NULL)
    {
        printf("File open failed! \n");
        exit(0);
    }
    else
    {
        printf("File open success! \n");
    }

    fread(file_buffer,128,1,file_fd);
    fclose(file_fd);
    i = 0;
    while(file_buffer[i++]  != 't');
    temp = atof(&file_buffer[i+1]);

    return temp/1000; 
}

void user_post_temp_property(void)
{
    int res = 0;
    float temperature;
    char * response;
    int length;
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();
    char *property_payload = "{\"temperature\":%.1f}";

    temperature = get_temperature();
    length = strlen(property_payload)+sizeof(float)+1;
    response = (char *)HAL_Malloc(length);
    if(response ==NULL){
     exit(-1);
    }
    memset(response,0,length);
    HAL_Snprintf(response, length, property_payload, temperature);

    res = IOT_Linkkit_Report(user_example_ctx->master_devid, ITM_MSG_POST_PROPERTY,(unsigned char *)response, length);
    EXAMPLE_TRACE("Post Property Message ID: %d", res);
}


static int user_master_dev_available(void)
{
    user_example_ctx_t *user_example_ctx = user_example_get_ctx();

    if (user_example_ctx->cloud_connected && user_example_ctx->master_initialized) {
        return 1;
    }

    return 0;
}

void set_iotx_info()
{
    HAL_SetProductKey(PRODUCT_KEY);
    HAL_SetProductSecret(PRODUCT_SECRET);
    HAL_SetDeviceName(DEVICE_NAME);
    HAL_SetDeviceSecret(DEVICE_SECRET);
}

static int max_running_seconds = 86400;
int linkkit_main(void *paras)
{

    uint64_t                        time_prev_sec = 0, time_now_sec = 0;
    uint64_t                        time_begin_sec = 0;
    int                             res = 0;
    iotx_linkkit_dev_meta_info_t    master_meta_info;
    user_example_ctx_t             *user_example_ctx = user_example_get_ctx();
#if defined(__UBUNTU_SDK_DEMO__)
    int                             argc = ((app_main_paras_t *)paras)->argc;
    char                          **argv = ((app_main_paras_t *)paras)->argv;

    if (argc > 1) {
        int     tmp = atoi(argv[1]);

        if (tmp >= 60) {
            max_running_seconds = tmp;
            EXAMPLE_TRACE("set [max_running_seconds] = %d seconds\n", max_running_seconds);
        }
    }
#endif

#if !defined(WIFI_PROVISION_ENABLED) || !defined(BUILD_AOS)
    set_iotx_info();
#endif

    memset(user_example_ctx, 0, sizeof(user_example_ctx_t));

    IOT_SetLogLevel(IOT_LOG_DEBUG);

    /* Register Callback */
    IOT_RegisterCallback(ITE_CONNECT_SUCC, user_connected_event_handler);
    IOT_RegisterCallback(ITE_DISCONNECTED, user_disconnected_event_handler);
    IOT_RegisterCallback(ITE_RAWDATA_ARRIVED, user_down_raw_data_arrived_event_handler);
    IOT_RegisterCallback(ITE_PROPERTY_SET, user_property_set_event_handler);
    IOT_RegisterCallback(ITE_REPORT_REPLY, user_report_reply_event_handler);
    IOT_RegisterCallback(ITE_TIMESTAMP_REPLY, user_timestamp_reply_event_handler);
    IOT_RegisterCallback(ITE_INITIALIZE_COMPLETED, user_initialized);

    memset(&master_meta_info, 0, sizeof(iotx_linkkit_dev_meta_info_t));
    memcpy(master_meta_info.product_key, PRODUCT_KEY, strlen(PRODUCT_KEY));
    memcpy(master_meta_info.product_secret, PRODUCT_SECRET, strlen(PRODUCT_SECRET));
    memcpy(master_meta_info.device_name, DEVICE_NAME, strlen(DEVICE_NAME));
    memcpy(master_meta_info.device_secret, DEVICE_SECRET, strlen(DEVICE_SECRET));

    /* Choose Login Server, domain should be configured before IOT_Linkkit_Open() */
    int domain_type = IOTX_CLOUD_REGION_SHANGHAI;
    IOT_Ioctl(IOTX_IOCTL_SET_DOMAIN, (void *)&domain_type);

    /* Choose Login Method */
    int dynamic_register = 0;
    IOT_Ioctl(IOTX_IOCTL_SET_DYNAMIC_REGISTER, (void *)&dynamic_register);

    /* Choose Whether You Need Post Property/Event Reply */
    int post_event_reply = 1;
    IOT_Ioctl(IOTX_IOCTL_RECV_EVENT_REPLY, (void *)&post_event_reply);

    /* Create Master Device Resources */
    user_example_ctx->master_devid = IOT_Linkkit_Open(IOTX_LINKKIT_DEV_TYPE_MASTER, &master_meta_info);
    if (user_example_ctx->master_devid < 0) {
        EXAMPLE_TRACE("IOT_Linkkit_Open Failed\n");
        return -1;
    }

    /* Start Connect Aliyun Server */
    res = IOT_Linkkit_Connect(user_example_ctx->master_devid);
    if (res < 0) {
        EXAMPLE_TRACE("IOT_Linkkit_Connect Failed\n");
        return -1;
    }    

    /* Init the leds status and post the status to cloud */
    turn_off_led(SAMPLE_LED_1);
    user_post_property(SAMPLE_LED_1,0);
    turn_off_led(SAMPLE_LED_2);
    user_post_property(SAMPLE_LED_2,0);
    user_post_temp_property();

    /* Init the time_begain_sec */
    time_begin_sec = user_update_sec();

    /* main loop */
    while (1) {
        /* Recive message from cloud */
        IOT_Linkkit_Yield(USER_EXAMPLE_YIELD_TIMEOUT_MS);

        time_now_sec = user_update_sec();
        if (time_prev_sec == time_now_sec)
        {
            continue;
        }
        if (max_running_seconds && (time_now_sec - time_begin_sec > max_running_seconds))
        {
            EXAMPLE_TRACE("Example Run for Over %d Seconds, Break Loop!\n", max_running_seconds);
            break;
        }

        /*Upate temperature every minutes*/
        if (time_now_sec % 60 == 0 && user_master_dev_available())
        {
            user_post_temp_property();
        }

        time_prev_sec = time_now_sec;
    }

    IOT_Linkkit_Close(user_example_ctx->master_devid);

    IOT_DumpMemoryStats(IOT_LOG_DEBUG);
    IOT_SetLogLevel(IOT_LOG_NONE);

    return 0;
}
#endif
