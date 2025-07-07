// SPDX-License-Identifier: MIT
// Wokwi Custom Chip - OpAmp with Attribute Controls
#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct {
  pin_t vinp, vinn, vout, vcc, vee;
  timer_t tid;
  uint32_t gain_attr;
  uint32_t period_attr;
  float last_gain;
  int last_period;
} opamp_chip_t;

static void opamp_simulate(void *user_data);

void chip_init() {
  opamp_chip_t *chip = malloc(sizeof(opamp_chip_t));
  chip->vinp = pin_init("IN+", ANALOG);
  chip->vinn = pin_init("IN-", ANALOG);
  chip->vout = pin_init("OUT", OUTPUT);
  chip->vcc  = pin_init("VCC", ANALOG);
  chip->vee  = pin_init("VEE", ANALOG);

  // Liga aos "id" definidos em chip.json (controls)
  chip->gain_attr   = attr_init("gain",   100000);
  chip->period_attr = attr_init("period", 10);

  chip->last_gain   = 100000.0f;
  chip->last_period = 10;

  // Configuração do timer
  const timer_config_t tcfg = {
    .callback  = opamp_simulate,
    .user_data = chip,
  };
  chip->tid = timer_init(&tcfg);
  timer_start(chip->tid, chip->last_period, true);

  printf("[chip-opamp] OpAmp custom pronto com controles via atributos!\n");
}

static void opamp_simulate(void *user_data) {
  opamp_chip_t *chip = (opamp_chip_t*)user_data;

  float gain = (float)attr_read(chip->gain_attr);
  int period = (int)attr_read(chip->period_attr);
  if (period <= 0) period = 1; // Evita timer zero

  // Atualiza timer se o período mudou
  if (period != chip->last_period) {
    timer_stop(chip->tid);
    timer_start(chip->tid, period, true);
    chip->last_period = period;
    printf("[chip-opamp] Novo período do timer: %d ms\n", period);
  }

  if (fabs(gain - chip->last_gain) > 1.0f) {
    printf("[chip-opamp] Novo ganho: %.0fx\n", gain);
    chip->last_gain = gain;
  }

  float vinp = pin_adc_read(chip->vinp);
  float vinn = pin_adc_read(chip->vinn);
  float vcc  = chip->vcc ? pin_adc_read(chip->vcc) : 5.0f;
  float vee  = chip->vee ? pin_adc_read(chip->vee) : 0.0f;

  float diff = vinp - vinn;
  float out = diff * gain;

  // Saturação em VCC/VEE
  if (out > vcc) out = vcc;
  if (out < vee) out = vee;

  pin_write(chip->vout, out);

  printf("[chip-opamp] VINP=%.3fV VINN=%.3fV VOUT=%.3fV (Ganho=%.0fx, Periodo=%dms)\n",
         vinp, vinn, out, gain, period);
}