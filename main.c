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
  float prior_l;
  float prior_r;
  size_t input;
  _Atomic size_t output;
} delay_t;

typedef struct {
  _Atomic double amp;
  _Atomic double feedback;

  delay_t delay;
} engine_t;

void delay_set_time_samples(delay_t* delay, int samples) {
  atomic_store(&delay->output, (delay->input - samples) % BUFFER_SIZE);
}

float clip(float in) {
  return fmaxf(-1.0, fminf(in, 1.0));
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  
  engine_t* engine = (engine_t*)(pDevice->pUserData);
  float* interleaved_samples = (float*)pOutput;
  float* interleaved_input = (float*)pInput;
  
  for (int i = 0; i < frameCount; ++i) {
    for (int channel = 0; channel < 2; ++channel) {
      if (channel == 0) { //case: left channel
	engine->delay.buffer_l[engine->delay.input] = clip(*interleaved_input + atomic_load(&engine->feedback)*engine->delay.prior_l);
	*interleaved_samples = clip(engine->delay.buffer_l[atomic_load(&engine->delay.output)] * atomic_load(&engine->amp));
      } else { //case: right channel
	engine->delay.buffer_r[engine->delay.input] = clip(*interleaved_input + atomic_load(&engine->feedback)*engine->delay.prior_r);
	*interleaved_samples = clip(engine->delay.buffer_r[atomic_load(&engine->delay.output)] * atomic_load(&engine->amp));
      }

      interleaved_samples++;
      interleaved_input++;
    }

    if (*interleaved_samples > 0.9) {
      atomic_store(&engine->feedback, 0.0);
    } else if (*interleaved_samples > 0.7) {
      atomic_store(&engine->feedback, 0.4);
    } else {
      atomic_fetch_add(&engine->feedback, 0.0001);
    }

    engine->delay.prior_l = engine->delay.buffer_l[engine->delay.output];
    engine->delay.prior_r = engine->delay.buffer_r[engine->delay.output];

    engine->delay.input = (engine->delay.input + 1)%BUFFER_SIZE;
    atomic_store(&engine->delay.output, (atomic_load(&engine->delay.output) + 1)%BUFFER_SIZE);
  }
}

int main(int argc, char** argv) {
  printf("hello, feedback!\n");

  engine_t engine = {
    .delay = {
      .buffer_l = {0.0},
      .buffer_r = {0.0},
      .prior_l = 0.0,
      .prior_r = 0.0,

      .input = 0,
      .output = 0,
    }
  };

  atomic_store(&engine.amp, 0.5);
  atomic_store(&engine.feedback, 0.0);
  delay_set_time_samples(&engine.delay, 24000);
  
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

  double stored_amp;
  double stored_feedback;
  int stored_delay_time;
  
  while (1) {
    printf("volume, feedback, delay time: ");
    scanf("%lf %lf %d", &stored_amp, &stored_feedback, &stored_delay_time);
    
    atomic_store(&engine.amp, stored_amp);
    atomic_store(&engine.feedback, stored_feedback);
    delay_set_time_samples(&engine.delay, stored_delay_time);
  }

  ma_device_uninit(&device);

  return 0;
}
