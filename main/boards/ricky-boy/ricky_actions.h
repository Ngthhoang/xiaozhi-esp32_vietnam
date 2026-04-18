#ifndef RICKY_ACTIONS_H_
#define RICKY_ACTIONS_H_

class RickyMotor;

/** Preset test_rb: index 0..11 (MCP 1..12); thêm bài 13: index 12 (MCP preset 13, Intro blend 13a). */
void RickyRunPreset(RickyMotor& m, int index_0_to_12);
/** Dance base (không âm thanh nền): index 0..6 tương ứng động tác 1,2,3,4,6,8,10. */
void RickyRunDanceBase(RickyMotor& m, int index_0_to_6);
/** 7 động tác mắt: index 0..6 (MCP có thể gọi 1..7). */
void RickyRunEyeAction(RickyMotor& m, int index_0_to_6);

#endif  // RICKY_ACTIONS_H_
