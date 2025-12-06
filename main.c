#include <stdio.h>
#include <math.h>
#include <stdatomic.h>
#include "miniaudio.h"

#define FREQ 440.0
#define SAMPLE_RATE 48000.0

typedef struct {
  _Atomic double amp;

  double phase;
  double freq;
  double sample_rate;
} engine_t;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  
  engine_t* engine = (engine_t*)(pDevice->pUserData);
  float* interleaved_samples = (float*)pOutput;
  
  for (int i = 0; i < frameCount; ++i) {
    float current_sample = (float)(sin(engine->phase)*atomic_load(&engine->amp));

    engine->phase += (2.0 * 3.14159 * FREQ) / SAMPLE_RATE;

    for (int channel = 0; channel < 2; ++channel) {
      *interleaved_samples++ = current_sample;
    }
  }
}

int main(int argc, char** argv) {
  printf("hello, feedback!\n");

  engine_t engine = {
    .phase = 0.0,
    .freq = FREQ,
    .sample_rate = SAMPLE_RATE
  };

  atomic_store(&engine.amp, 0.5);
  
  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
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
