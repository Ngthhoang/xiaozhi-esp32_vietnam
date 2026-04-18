#ifndef RICKY_SPEED_SETTINGS_H_
#define RICKY_SPEED_SETTINGS_H_

#include <string>

/** % tốc độ pace=normal cho tiến (NVS hoặc RICKY_SPEED_DEFAULT). */
int RickySpeedGetStoredForwardNormal();

/** % tốc độ pace=normal cho lùi (NVS hoặc RICKY_SPEED_DEFAULT). */
int RickySpeedGetStoredBackwardNormal();

/** Lưu vĩnh viễn (NVS namespace ricky). Trả false nếu giá trị ngoài 45–100. */
bool RickySpeedStoreForwardNormal(int percent);
bool RickySpeedStoreBackwardNormal(int percent);

/** Xóa khóa NVS → dùng lại mặc định firmware (config.h). */
void RickySpeedResetToFactoryDefaults();

/** JSON ngắn: {"forward":..,"backward":..,"nvs":true/false} */
std::string RickySpeedGetStatusJson();

#endif  // RICKY_SPEED_SETTINGS_H_
