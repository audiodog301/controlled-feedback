#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <stdatomic.h>
#include "miniaudio.h"

#define FREQ 440.0
#define SAMPLE_RATE 48000.0
#define BUFFER_SIZE 48000

typedef struct {
  float buffer_l[BUFFER_SIZE];
  float buffer_r[BUFFER_SIZE];
  size_t input;
  size_t output;
} delay_t;

typedef struct {
  _Atomic double amp;

  delay_t delay;
  
  double phase;
  double freq;
  double sample_rate;
} engine_t;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  
  engine_t* engine = (engine_t*)(pDevice->pUserData);
  float* interleaved_samples = (float*)pOutput;
  float* interleaved_input = (float*)pInput;
  
  for (int i = 0; i < frameCount; ++i) {
    float current_sample = (float)(sin(engine->phase)*atomic_load(&engine->amp));

    engine->phase += (2.0 * 3.14159 * FREQ) / SAMPLE_RATE;

    for (int channel = 0; channel < 2; ++channel) {
      *interleaved_samples++ = (current_sample * 0.2) + *interleaved_input++;
    }
  }
}

int main(int argc, char** argv) {
  printf("hello, feedback!\n");

  engine_t engine = {
    .phase = 0.0,
    .freq = FREQ,
    .sample_rate = SAMPLE_RATE,

    .delay = {
      .buffer_l = {0.0},
      .buffer_r = {0.0},

      .input = 0,
      .output = 0
    }
  };

  atomic_store(&engine.amp, 0.5);
  
  ma_device_config config = ma_device_config_init(ma_device_type_duplex);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.capture.format = ma_format_f32;
  config.capture.channels = 2;
  config.sampleRate = SAMPLE_RATE;
  config.dataCallback = data_callback;
  config.pUserData = &engine;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    return -1; //failure to init device
  }

  ma_device_start(&device);

  double stored_amp = 0.0;

  while (1) {
    printf("volume: ");
    scanf("%lf", &stored_amp);
    
    atomic_store(&engine.amp, stored_amp);
  }

  ma_device_uninit(&device);

  return 0;
}
