#ifndef __PLL_H__
#define __PLL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PLL_PI 3.1415926f
#define PLL_ROOT_2 1.4142f

#define PLL_NOMINAL_FREQ_HZ 50.0f
#define PLL_SAMPLE_FREQ_HZ 10000.0f
#define PLL_SAMPLE_TIME (1.0f / PLL_SAMPLE_FREQ_HZ)
#define PLL_SOGI_K PLL_ROOT_2

#define PLL_BANDWIDTH_HZ 20.0f
#define PLL_DAMPING 0.707f
#define PLL_MIN_FREQ_HZ 45.0f
#define PLL_MAX_FREQ_HZ 55.0f
#define PLL_MIN_SIGNAL 0.01f

typedef struct SOGI {
    float ualfa_0;
    float SOGI_Ualfa;
    float SOGI_Ubeta;

    float integral_2;
    float integral_3;

    float Ugird_W0;
    float samp_t;
} SOGI_t;

typedef struct {
    float kp;
    float ki;
    float kd;
    float ek;
    float ek_1;
    float ek_2;
    float uk;
    float uk_1;
} PID_t;

typedef struct {
    float wt;
    float w0;
    PID_t pid;
    SOGI_t sogi;
    float sin_wt;
    float cos_wt;
    float frequency_hz;
} PLL_t;

void PLL_Init(PLL_t *pll);
void PLL_Update(PLL_t *pll, float input);
float PLL_GetRms(const PLL_t *pll);

#ifdef __cplusplus
}
#endif

#endif /* __PLL_H__ */
