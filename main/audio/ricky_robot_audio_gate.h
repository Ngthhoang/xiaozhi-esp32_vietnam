#ifndef RICKY_ROBOT_AUDIO_GATE_H_
#define RICKY_ROBOT_AUDIO_GATE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ricky_boy: robot dùng chung I2S/codec với TTS. Gọi Enter trước khi phát bark/nhạc,
 * Leave khi xong. Lần Enter đầu tiên gọi AudioService::ResetDecoder() để xóa TTS đang chờ.
 */
void RickyRobotAudioGateEnter(void);
void RickyRobotAudioGateLeave(void);
bool RickyRobotAudioGateIsHeld(void);

#ifdef __cplusplus
}
#endif

#endif  // RICKY_ROBOT_AUDIO_GATE_H_
