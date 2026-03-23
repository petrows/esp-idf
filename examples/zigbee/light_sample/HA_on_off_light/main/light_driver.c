/*
 * SPDX-FileCopyrightText: 2021-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: LicenseRef-Included
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Espressif Systems
 *    integrated circuit in a product or a software update for such product,
 *    must reproduce the above copyright notice, this list of conditions and
 *    the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * 4. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "esp_log.h"
#include "led_strip.h"
#include "light_driver.h"

static const char *TAG = "LIGHT_DRIVER";

static led_strip_handle_t s_led_strip;
static bool    s_power      = false;
static uint8_t s_hue        = 0;       /* ZCL hue: 0-254 */
static uint8_t s_saturation = 0;       /* ZCL saturation: 0-254 */
static uint8_t s_level      = 254;     /* ZCL level: 0-254 */

/* Convert HSV (hue 0-360, sat 0-255, val 0-255) to RGB (each 0-255) */
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v,
                       uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s == 0) {
        *r = *g = *b = v;
        return;
    }
    uint16_t region  = h / 60;
    uint16_t rem     = (h - region * 60) * 255 / 60;
    uint8_t  p       = (uint32_t)v * (255 - s) / 255;
    uint8_t  q       = (uint32_t)v * (255 - ((uint32_t)s * rem) / 255) / 255;
    uint8_t  t_val   = (uint32_t)v * (255 - ((uint32_t)s * (255 - rem)) / 255) / 255;
    switch (region) {
    case 0:  *r = v;     *g = t_val; *b = p;     break;
    case 1:  *r = q;     *g = v;     *b = p;     break;
    case 2:  *r = p;     *g = v;     *b = t_val; break;
    case 3:  *r = p;     *g = q;     *b = v;     break;
    case 4:  *r = t_val; *g = p;     *b = v;     break;
    default: *r = v;     *g = p;     *b = q;     break;
    }
}

static void light_driver_update(void)
{
    uint8_t r = 0, g = 0, b = 0;
    if (s_power) {
        /* ZCL hue 0-254 → degrees 0-360 */
        uint16_t hue_deg = (uint16_t)s_hue * 360 / 254;
        /* ZCL level 0-254 → brightness 0-255 */
        uint8_t brightness = s_level;
        hsv_to_rgb(hue_deg, s_saturation, brightness, &r, &g, &b);
    }
    ESP_LOGD(TAG, "LED update: power=%d hue=%d sat=%d level=%d -> r=%d g=%d b=%d",
             s_power, s_hue, s_saturation, s_level, r, g, b);
    ESP_ERROR_CHECK(led_strip_set_pixel(s_led_strip, 0, r, g, b));
    ESP_ERROR_CHECK(led_strip_refresh(s_led_strip));
}

void light_driver_init(bool power)
{
    led_strip_config_t led_strip_conf = {
        .max_leds = CONFIG_EXAMPLE_STRIP_LED_NUMBER,
        .strip_gpio_num = CONFIG_EXAMPLE_STRIP_LED_GPIO,
    };
    led_strip_rmt_config_t rmt_conf = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&led_strip_conf, &rmt_conf, &s_led_strip));
    s_power = power;
    light_driver_update();
}

void light_driver_set_power(bool power)
{
    s_power = power;
    light_driver_update();
}

void light_driver_set_level(uint8_t level)
{
    s_level = level;
    light_driver_update();
}

void light_driver_set_color_hue(uint8_t hue)
{
    s_hue = hue;
    light_driver_update();
}

void light_driver_set_color_saturation(uint8_t saturation)
{
    s_saturation = saturation;
    light_driver_update();
}
