#include "DeskRoboAudio.h"

#include <math.h>
#include <string.h>

#include <driver/i2s_std.h>

namespace {
// Waveshare ESP32-S3-LCD-1.85 audio pins (wiki)
constexpr gpio_num_t MIC_WS = GPIO_NUM_2;
constexpr gpio_num_t MIC_BCLK = GPIO_NUM_15;
constexpr gpio_num_t MIC_DIN = GPIO_NUM_39;

constexpr gpio_num_t SPK_LRCK = GPIO_NUM_38;
constexpr gpio_num_t SPK_BCLK = GPIO_NUM_48;
constexpr gpio_num_t SPK_DOUT = GPIO_NUM_47;

constexpr uint32_t AUDIO_SR = 16000;

i2s_chan_handle_t s_rx_chan = nullptr;
i2s_chan_handle_t s_tx_chan = nullptr;
bool s_mic_ok = false;
bool s_spk_ok = false;
float s_mic_rms = 0.0f;
float s_mic_peak = 0.0f;
uint32_t s_last_mic_ms = 0;
uint16_t s_beep_level = 1600;  // 0..8000 conservative default

bool init_mic() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
  chan_cfg.dma_desc_num = 6;
  chan_cfg.dma_frame_num = 256;
  if (i2s_new_channel(&chan_cfg, nullptr, &s_rx_chan) != ESP_OK) {
    return false;
  }

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SR),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = MIC_BCLK,
              .ws = MIC_WS,
              .dout = I2S_GPIO_UNUSED,
              .din = MIC_DIN,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };

  if (i2s_channel_init_std_mode(s_rx_chan, &std_cfg) != ESP_OK) {
    return false;
  }
  if (i2s_channel_enable(s_rx_chan) != ESP_OK) {
    return false;
  }
  return true;
}

bool init_speaker() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.dma_desc_num = 6;
  chan_cfg.dma_frame_num = 256;
  if (i2s_new_channel(&chan_cfg, &s_tx_chan, nullptr) != ESP_OK) {
    return false;
  }

  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(AUDIO_SR),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = SPK_BCLK,
              .ws = SPK_LRCK,
              .dout = SPK_DOUT,
              .din = I2S_GPIO_UNUSED,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };

  if (i2s_channel_init_std_mode(s_tx_chan, &std_cfg) != ESP_OK) {
    return false;
  }
  if (i2s_channel_enable(s_tx_chan) != ESP_OK) {
    return false;
  }
  return true;
}
}  // namespace

void DeskRoboAudio_Init() {
  s_mic_ok = init_mic();
  s_spk_ok = init_speaker();
  Serial.printf("[AUDIO] mic=%s speaker=%s\n", s_mic_ok ? "ok" : "fail", s_spk_ok ? "ok" : "fail");
}

void DeskRoboAudio_Loop() {
  if (!s_mic_ok) return;
  const uint32_t now = millis();
  if ((now - s_last_mic_ms) < 90) return;
  s_last_mic_ms = now;

  int16_t samples[256];
  size_t bytes_read = 0;
  memset(samples, 0, sizeof(samples));
  if (i2s_channel_read(s_rx_chan, samples, sizeof(samples), &bytes_read, 0) != ESP_OK || bytes_read == 0) {
    return;
  }

  const size_t n = bytes_read / sizeof(int16_t);
  float sum_sq = 0.0f;
  float peak = 0.0f;
  for (size_t i = 0; i < n; ++i) {
    const float v = (float) samples[i] / 32768.0f;
    const float av = fabsf(v);
    sum_sq += v * v;
    if (av > peak) peak = av;
  }
  const float rms = sqrtf(sum_sq / (float) n);

  // low-pass smoothing for stable UI readout
  s_mic_rms = s_mic_rms * 0.75f + rms * 0.25f;
  s_mic_peak = s_mic_peak * 0.65f + peak * 0.35f;
}

bool DeskRoboAudio_MicReady() { return s_mic_ok; }
bool DeskRoboAudio_SpeakerReady() { return s_spk_ok; }
float DeskRoboAudio_MicRms() { return s_mic_rms; }
float DeskRoboAudio_MicPeak() { return s_mic_peak; }

bool DeskRoboAudio_Beep(uint16_t hz, uint16_t duration_ms) {
  if (!s_spk_ok) return false;
  if (hz < 80) hz = 80;
  if (hz > 4000) hz = 4000;
  if (duration_ms < 20) duration_ms = 20;
  if (duration_ms > 1200) duration_ms = 1200;

  const uint32_t total_samples = (AUDIO_SR * (uint32_t) duration_ms) / 1000U;
  const float step = 2.0f * 3.14159265f * (float) hz / (float) AUDIO_SR;
  float phase = 0.0f;
  int16_t buf[128];

  uint32_t written_samples = 0;
  while (written_samples < total_samples) {
    const uint32_t remain = total_samples - written_samples;
    const uint32_t chunk = (remain > 128U) ? 128U : remain;
    for (uint32_t i = 0; i < chunk; ++i) {
      buf[i] = (int16_t) (sinf(phase) * (float) s_beep_level);
      phase += step;
      if (phase > 6.2831853f) phase -= 6.2831853f;
    }
    size_t bytes_written = 0;
    if (i2s_channel_write(s_tx_chan, buf, chunk * sizeof(int16_t), &bytes_written, 50) != ESP_OK) {
      return false;
    }
    written_samples += (uint32_t) (bytes_written / sizeof(int16_t));
  }
  return true;
}

void DeskRoboAudio_SetBeepLevel(uint16_t level) {
  if (level > 8000) level = 8000;
  s_beep_level = level;
}
