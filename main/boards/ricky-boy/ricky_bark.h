#ifndef RICKY_BARK_H_
#define RICKY_BARK_H_

/** PCM 24 kHz mono int16 (khớp test_rb / AUDIO_OUTPUT_SAMPLE_RATE). */
void RickyPlayBarkShortAsync();
void RickyPlayBarkLongAsync();

/** Chọn ngẫu nhiên sủa ngắn hoặc dài; dùng kèm preset cơ bản 1–10. */
void RickyPlayBarkRandomForPresetBasic();

#endif  // RICKY_BARK_H_
