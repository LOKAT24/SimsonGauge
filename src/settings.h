#ifndef SETTINGS_H
#define SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/** Open the persistent settings store (NVS). Call once in setup(). */
void  settings_begin(void);

/** RPM multiplier (frequency * multiplier = RPM). Default 1.0. */
float settings_get_multiplier(void);
void  settings_set_multiplier(float v);

/** Speed multiplier (frequency * multiplier = speed). Default 1.0. */
float settings_get_speed_multiplier(void);
void  settings_set_speed_multiplier(float v);

/** Active theme index (0 = NFS). Default 0. */
int   settings_get_theme(void);
void  settings_set_theme(int t);

/** Brightness (10 to 100). Default 100. */
int   settings_get_brightness(void);
void  settings_set_brightness(int b);

/** Measurement algorithm (0 = A: ISR/period, 1 = B: edge buffer). Default 0. */
int   settings_get_algorithm(void);
void  settings_set_algorithm(int a);

#ifdef __cplusplus
}
#endif

#endif // SETTINGS_H
