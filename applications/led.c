#include <agile_led.h>
#include <stdlib.h>
#include "led.h"
#include "pin_config.h"
#include <agile_led.h>

static agile_led_t *led0 = RT_NULL;
static agile_led_t *led1 = RT_NULL;
static agile_led_t *beep = RT_NULL;
static agile_led_t *singlebeep = RT_NULL;
static agile_led_t *learnbeep = RT_NULL;
static agile_led_t *singleled0 = RT_NULL;
static agile_led_t *beep_three = RT_NULL;
static agile_led_t *led0_three = RT_NULL;
static agile_led_t *lossled0 = RT_NULL;
static agile_led_t *wifi_G = RT_NULL;
static agile_led_t *wifi_G_Com = RT_NULL;
static agile_led_t *wifi_R = RT_NULL;

#define DBG_TAG "led"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

uint8_t led_id_temp = 0;
uint8_t led_mode_temp = 0;
extern uint32_t Gateway_ID;

void led_Init(void)
{
    if(led0 == RT_NULL)
    {
        led0 = agile_led_create(LED3_PIN, PIN_LOW, "200,200", -1);
        singleled0 = agile_led_create(LED3_PIN, PIN_LOW, "200,1", 1);
        lossled0 = agile_led_create(LED3_PIN, PIN_LOW, "200,200,200,5000", -1);
        led0_three = agile_led_create(LED3_PIN, PIN_LOW, "200,200", 3);
        LOG_D("LED_0 Init Success\r\n");
    }

    if(led1 == RT_NULL)
    {
        led1 = agile_led_create(LED0_PIN, PIN_LOW, "200,200", -1);
        LOG_D("LED_1 Init Success\r\n");
    }

    if(beep == RT_NULL)
    {
        beep = agile_led_create(BEEP_PIN, PIN_HIGH, "200,200", -1);
        singlebeep = agile_led_create(BEEP_PIN, PIN_HIGH, "200,1", 1);
        beep_three = agile_led_create(BEEP_PIN, PIN_HIGH, "200,200", 3);
        learnbeep = agile_led_create(BEEP_PIN, PIN_HIGH, "50,50,200,200", 3);
        LOG_D("Beep Init Success\r\n");
    }
    if(wifi_G == RT_NULL)
    {
        wifi_G = agile_led_create(LED4_PIN, PIN_LOW, "200,200", -1);
        wifi_G_Com = agile_led_create(LED4_PIN, PIN_LOW, "200,200", -1);
        LOG_D("wifi_G Init Success\r\n");
    }
    if(wifi_R == RT_NULL)
    {
        wifi_R = agile_led_create(LED5_PIN, PIN_LOW, "200,200", -1);
        LOG_D("wifi_R Init Success\r\n");
    }
}
void loss_led_start(void)
{
    agile_led_start(lossled0);
}
void loss_led_stop(void)
{
    agile_led_stop(lossled0);
}
void beep_three_times(void)
{
    agile_led_start(beep_three);
    agile_led_start(led0_three);
}
void wifi_G_resume_callback(agile_led_t *led)
{
    extern uint8_t Heart_Flag;
    if(Heart_Flag)
    {
        wifi_led(1);
    }
    else
    {
        wifi_led(2);
    }
}
void wifi_led(uint8_t num)
{
    switch(num)
    {
    case 0://无设备
        agile_led_stop(wifi_R);
        agile_led_stop(wifi_G);
        agile_led_off(wifi_R);
        agile_led_off(wifi_G);
        break;
    case 1://心跳成功
        agile_led_stop(wifi_R);
        agile_led_set_light_mode(wifi_G, "200,0", -1);
        agile_led_start(wifi_G);
        break;
    case 2://心跳失败
        agile_led_stop(wifi_G);
        agile_led_set_light_mode(wifi_R, "200,0", -1);
        agile_led_start(wifi_R);
        break;
    case 3://正在通讯
        agile_led_stop(wifi_G);
        agile_led_set_light_mode(wifi_G, "50,50", 1);
        if(Gateway_ID)
        {
            agile_led_set_compelete_callback(wifi_G,wifi_G_resume_callback);
        }
        agile_led_start(wifi_G);
        break;
    }
}
void beep_start(uint8_t led_id,int mode)
{
    agile_led_stop(singlebeep);
    agile_led_stop(singleled0);
    led_id_temp = led_id;
    led_mode_temp = mode;
    switch (mode)
    {
    case 0://短叫一声
        if(led_id)//绿灯
        {
            agile_led_set_light_mode(beep, "200,200", 1);
            agile_led_start(beep);
            agile_led_set_light_mode(led1, "200,200", 1);
            agile_led_start(led1);
        }
        else//红灯
        {
            agile_led_set_light_mode(beep, "200,15000", -1);
            agile_led_start(beep);
            agile_led_set_light_mode(led0, "200,15000", -1);
            agile_led_start(led0);
        }
        break;

    case 1://短叫两声
        agile_led_set_light_mode(beep, "200,200,200,5000", -1);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200,200,5000", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,200,200,5000", -1);
            agile_led_start(led0);
        }
        break;

    case 2://短叫三声
        agile_led_set_light_mode(beep, "200,200,200,200,200,5000", -1);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200,200,200,200,5000", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,200,200,200,200,5000", -1);
            agile_led_start(led0);
        }
        break;
    case 3://短叫四声
        agile_led_set_light_mode(beep, "200,200,200,200,200,200,200,5000", -1);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led0);
        }
        break;
    case 4://五声
        agile_led_set_light_mode(beep, "200,200", 5);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200,200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,200", 75);
            agile_led_start(led0);
        }
        break;
    case 5://四声
        agile_led_set_light_mode(beep, "200,200,200,200,200,200,200,5000", -1);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led0);
        }
        break;
    case 6:
        agile_led_set_light_mode(beep, "200,200", 1);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200", 1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_stop(led0);
            agile_led_set_light_mode(led0, "200,200", 1);
            agile_led_start(led0);
        }
        break;
    case 7:
        agile_led_set_light_mode(beep, "200,200", 3);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200", 3);
            agile_led_start(led1);
        }
        else
        {
            agile_led_stop(led0);
            agile_led_set_light_mode(led0, "200,200", 3);
            agile_led_start(led0);
        }
        break;
    case 8:
        agile_led_set_light_mode(beep, "200,200", 5);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200", 5);
            agile_led_start(led1);
        }
        else
        {
            agile_led_stop(led0);
            agile_led_set_light_mode(led0, "200,200", 5);
            agile_led_start(led0);
        }
        break;

    case 9://六声
        agile_led_set_light_mode(beep, "200,200,200,200,200,200,200,200,200,200,200,5000", -1);
        agile_led_start(beep);
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,200,200,200,200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,200,200,200,200,200,200,200,200,200,200,5000", -1);
            agile_led_start(led0);
        }
        break;
    case 10://短叫五声
        agile_led_set_light_mode(beep, "100,300", 4);
        agile_led_set_light_mode(led1, "100,300", 4);
        agile_led_set_light_mode(led0, "100,300", 4);
        agile_led_start(beep);
        agile_led_start(led1);
        agile_led_start(led0);
        break;
    case 11://短叫三声
        agile_led_set_light_mode(beep, "100,300", 2);
        agile_led_set_light_mode(led1, "100,300", 2);
        agile_led_set_light_mode(led0, "100,300", 2);
        agile_led_start(beep);
        agile_led_start(led1);
        agile_led_start(led0);
    case 12://短叫三声
        agile_led_set_light_mode(beep, "100,300", 1);
        agile_led_set_light_mode(led1, "100,300", 1);
        agile_led_set_light_mode(led0, "100,300", 1);
        agile_led_start(beep);
        agile_led_start(led1);
        agile_led_start(led0);
    case 13://慢闪
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,400", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,400", -1);
            agile_led_start(led0);
        }
    case 14://快闪
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,100", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,100", -1);
            agile_led_start(led0);
        }
    case 15://1声间隔五秒
        if(led_id)
        {
            agile_led_set_light_mode(led1, "200,100", -1);
            agile_led_start(led1);
        }
        else
        {
            agile_led_set_light_mode(led0, "200,5000", -1);
            agile_led_set_light_mode(beep, "200,5000", -1);
            agile_led_start(led0);
            agile_led_start(beep);
        }
    }
}
void beep_stop(void)
{
    agile_led_stop(beep);
    rt_pin_write(BEEP_PIN,0);
}
void key_down(void)
{
    agile_led_start(singlebeep);
    agile_led_start(singleled0);
}
void just_ring(void)
{
    agile_led_start(singlebeep);
}
void learn_fail_ring(void)
{
    agile_led_start(learnbeep);
}
void Relearn(void)
{
    agile_led_set_light_mode(led0, "200,200", 75);
    agile_led_start(led0);
}
void NTC_Ring(void)
{
    agile_led_set_light_mode(beep, "50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50\
                                    ,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,2000", -1);
    agile_led_set_light_mode(led0, "50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50\
                                    ,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,2000", -1);
    agile_led_set_light_mode(led1, "50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50\
                                    ,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,50,2000", -1);
    agile_led_start(led0);
    agile_led_start(led1);
    agile_led_start(beep);
}
void Factory_WarningRing(void)
{
    agile_led_stop(led1);
    agile_led_set_light_mode(beep, "200,200", 1);
    agile_led_set_light_mode(led0, "200,200", 1);
    agile_led_start(led0);
    agile_led_start(beep);
}
void Factory_NormalRing(void)
{
    agile_led_stop(led0);
    agile_led_set_light_mode(beep, "200,200", 1);
    agile_led_set_light_mode(led1, "200,200", 1);
    agile_led_set_light_mode(wifi_G, "200,200", 1);
    agile_led_set_light_mode(wifi_R, "200,200", 1);
    agile_led_start(wifi_G);
    agile_led_start(wifi_R);
    agile_led_start(led1);
    agile_led_start(beep);
}
void led_Long_Start(uint8_t led_id)
{
    switch (led_id)
    {
    case 0:
        agile_led_set_light_mode(led0, "200,200", -1);
        agile_led_start(led0);
        led_Stop(1);
        break;

    case 1:
        agile_led_set_light_mode(led1, "200,0", -1);
        agile_led_start(led1);
        led_Stop(0);
        break;
    }
}
void Green_Long_Start(void)
{
    agile_led_set_light_mode(led1, "200,0", -1);
    agile_led_start(led1);
}
void led_Slow_Start(uint8_t led_id,int count)
{
    switch (led_id)
    {
    case 0:
        agile_led_set_light_mode(led0, "200,2000", count);
        agile_led_start(led0);
        led_Stop(1);
        break;

    case 1:
        agile_led_set_light_mode(led1, "200,2000", count);
        agile_led_start(led1);
        led_Stop(0);
        break;
    }
}
void led_Fast_Start(uint8_t led_id,int count)
{
    switch (led_id)
    {
    case 0:
        agile_led_set_light_mode(led0, "200,300", count);
        agile_led_start(led0);
        led_Stop(1);
        break;

    case 1:
        agile_led_set_light_mode(led1, "200,300", count);
        agile_led_start(led1);
        led_Stop(0);
        break;
    }

}
void led_Stop(uint8_t led_id)
{
    switch (led_id)
    {
    case 0:
        agile_led_stop(led0);
        rt_pin_write(LED3_PIN, 1);
        break;
    case 1:
        agile_led_stop(led1);
        rt_pin_write(LED0_PIN, 1);
        break;
    case 2:
        agile_led_pause(led0);
        agile_led_pause(led1);
        agile_led_pause(beep);
        agile_led_pause(singlebeep);
        agile_led_pause(singleled0);
        break;
    }
}

void led_on(uint8_t id)
{
    switch(id)
    {
    case 0:agile_led_on(led0);agile_led_off(led1);agile_led_set_light_mode(beep, "70,30,70,30,70,30,70,500", -1);agile_led_start(beep);break;
    case 1:agile_led_on(led1);agile_led_off(led0);agile_led_set_light_mode(beep, "70,30,70,30,70,30,70,500", -1);agile_led_start(beep);break;
    }

}
