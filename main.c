#include <stdio.h>
#include <math.h>
#include "miniaudio.h"

double phase = 0.0;
double freq = 440.0;
double sample_rate = 48000.0;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
  //process frameCount frames

  float* interleaved_samples = (float*)pOutput;
  
  for (int i = 0; i < frameCount; ++i) {
    float current_sample = (float)(sin(phase)*0.5);

    phase += (2.0 * 3.14159 * freq) / sample_rate;

    for (int channel = 0; channel < 2; ++channel) {
      *interleaved_samples++ = current_sample;
    }
  }
}

int main(int argc, char** argv) {
  printf("hello, feedback!\n");

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = 2;
  config.sampleRate = 48000;
  config.dataCallback = data_callback;

  ma_device device;
  if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
    return -1; //failure to init device
  }

  ma_device_start(&device);

  while (1) {
    //main loop
  }

  ma_device_uninit(&device);

  return 0;
}
