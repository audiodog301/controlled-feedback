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
  float prior_l; // for feedback
  float prior_r;
  size_t input;  // buffer index for writing to
  _Atomic size_t output; // buffer index for reading from (atomic because we change the delay time by moving around the output pointer)
} delay_t;

typedef struct { // this is where we store all of our state
  _Atomic double amp;
  _Atomic double feedback;

  delay_t delay;

  double goal; // what feedback value are we moving towards?
} engine_t;

void delay_set_time_samples(delay_t* delay, int samples) {
  atomic_store(&delay->output, (delay->input - samples) % BUFFER_SIZE); // todo: bitwise operations instead of expensive mod
}

float clip(float in) { //bipolar clipping
  return fmaxf(-1.0, fminf(in, 1.0));
}

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  
  engine_t* engine = (engine_t*)(pDevice->pUserData); //we passed along a pointer to our state struct
  float* interleaved_samples = (float*)pOutput; //pointers to our blocks of input and output samples
  float* interleaved_input = (float*)pInput;
  
  for (int i = 0; i < frameCount; ++i) { // loop through all of our frames
    for (int channel = 0; channel < 2; ++channel) { // and then each sample per frame
      if (channel == 0) { //case: left channel
	engine->delay.buffer_l[engine->delay.input] = clip(*interleaved_input + atomic_load(&engine->feedback)*engine->delay.prior_l); // write into our delay buffer
	*interleaved_samples = clip(engine->delay.buffer_l[atomic_load(&engine->delay.output)] * atomic_load(&engine->amp)); // write our output samples
      } else { //case: right channel
	engine->delay.buffer_r[engine->delay.input] = clip(*interleaved_input + atomic_load(&engine->feedback)*engine->delay.prior_r); // ditto for right channel
	*interleaved_samples = clip(engine->delay.buffer_r[atomic_load(&engine->delay.output)] * atomic_load(&engine->amp));
      }

      interleaved_samples++; // iterate our pointers. we trust miniaudio that we won't segfault so long as our 2d loop goes through frameCount*channels samples
      interleaved_input++;
    }

    if (*interleaved_samples > 0.95) { // NEED TO TUNE THIS
      engine->goal = 0.01;
    } else if (*interleaved_samples > 0.5) {
      engine->goal = 0.5;
    } else {
      engine->goal = 1.0;
    }

    if (atomic_load(&engine->feedback) < engine->goal) { // todo: implement proper slew-limited motion rather than these jumps
      atomic_fetch_add(&engine->feedback, 1.0);
    } else {
      atomic_store(&engine->feedback, 0.5);
    }

    // update dsp state
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
    },

    .goal = 0.5
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
  
  while (1) { // main loop :)
    printf("volume, feedback, delay time: ");
    scanf("%lf %lf %d", &stored_amp, &stored_feedback, &stored_delay_time); // todo make this actually safe
    
    atomic_store(&engine.amp, stored_amp);
    atomic_store(&engine.feedback, stored_feedback);
    delay_set_time_samples(&engine.delay, stored_delay_time);
  }

  ma_device_uninit(&device);

  return 0;
}
