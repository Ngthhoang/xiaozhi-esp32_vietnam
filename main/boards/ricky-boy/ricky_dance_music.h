#ifndef RICKY_DANCE_MUSIC_H_
#define RICKY_DANCE_MUSIC_H_

#include <cstdint>

/** Phát lặp PCM dog_dance2 (24 kHz mono) trong duration_ms, song song với motor. */
void RickyPlayDogDance2MusicForMs(uint32_t duration_ms);

/** Phát lặp wholetdogout (test_rb audio op 3), 24 kHz mono — dùng cho preset 13. */
void RickyPlayWholetdogoutMusicForMs(uint32_t duration_ms);
/** Yêu cầu dừng ngay nhạc dance đang chạy (dùng khi wake word kích hoạt lắng nghe). */
void RickyStopDanceMusicNow();

#endif  // RICKY_DANCE_MUSIC_H_
