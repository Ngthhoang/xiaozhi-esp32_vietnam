#ifndef RICKY_ACTION13_H_
#define RICKY_ACTION13_H_

/** Cùng test_rb ACTION13_VARIANT_RUN_MS: thời lượng mỗi biến thể bài 13 (~24s). */
#ifndef RICKY_ACTION13_VARIANT_RUN_MS
#define RICKY_ACTION13_VARIANT_RUN_MS 24000
#endif

class RickyMotor;

/**
 * Bài 13a từ test_rb run_fun_action case 12:
 * Intro blend (Action 1 super shake + Action 6 elastic), ~RICKY_ACTION13_VARIANT_RUN_MS.
 */
void RickyRunAction13IntroBlend(RickyMotor& m);

#endif  // RICKY_ACTION13_H_
