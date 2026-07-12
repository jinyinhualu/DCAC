#include "pll.h"

#include <math.h>

static void SOGI_Init(SOGI_t *sogi);
static void SOGI_Run(SOGI_t *sogi, float input);
static float SOGI_GetRms(const SOGI_t *sogi);
static void PID_Init(PID_t *pid, float kp, float ki, float kd);
static float PID_Update(PID_t *pid, float error, float output_min, float output_max);
static float ClampFloat(float value, float min_value, float max_value);

void PLL_Init(PLL_t *pll)
{
  float pll_bandwidth = 2.0f * PLL_PI * PLL_BANDWIDTH_HZ;

  SOGI_Init(&pll->sogi);
  PID_Init(&pll->pid,
           2.0f * PLL_DAMPING * pll_bandwidth,
           pll_bandwidth * pll_bandwidth * PLL_SAMPLE_TIME,
           0.0f);

  pll->wt = 0.0f;
  pll->w0 = 2.0f * PLL_PI * PLL_NOMINAL_FREQ_HZ;
  pll->sin_wt = 0.0f;
  pll->cos_wt = 1.0f;
  pll->ud = 0.0f;
  pll->uq = 0.0f;
  pll->frequency_hz = PLL_NOMINAL_FREQ_HZ;
}

void SOGI_Update(PLL_t *pll, float input)
{
  SOGI_Run(&pll->sogi, input);
}

void dq_pll(PLL_t *pll)
{
  float alpha = 0.0f;                                       //alpha分量
  float beta = 0.0f;                                        //beta分量
  float ud = 0.0f;                                          //d轴分量
  float uq = 0.0f;                                          //q轴分量
  float signal_peak = 0.0f;                                 //信号幅值
  float phase_error = 0.0f;                                 //相位误差，范围[-1, 1]
  float omega_nom = 2.0f * PLL_PI * PLL_NOMINAL_FREQ_HZ;    //参考角频率
  float omega_min = 2.0f * PLL_PI * PLL_MIN_FREQ_HZ;        //最小角频率
  float omega_max = 2.0f * PLL_PI * PLL_MAX_FREQ_HZ;        //最大角频率
  float omega_correction = 0.0f;                            //角频率修正量

  alpha = pll->sogi.SOGI_Ualfa;
  beta = pll->sogi.SOGI_Ubeta;
  signal_peak = sqrtf(alpha * alpha + beta * beta);
  ud = alpha * pll->cos_wt + beta * pll->sin_wt;             //Park变换d轴分量
  uq = beta * pll->cos_wt - alpha * pll->sin_wt;             //Park变换q轴分量
  pll->ud = ud;
  pll->uq = uq;

  if (signal_peak > PLL_MIN_SIGNAL)
  {
    phase_error = -uq / signal_peak;                            //q轴归零作为锁相误差
    phase_error = ClampFloat(phase_error, -1.0f, 1.0f);          //限制相位误差
  }
  omega_correction = PID_Update(&pll->pid,
                                phase_error,
                                omega_nom - omega_max,
                                omega_nom - omega_min); //角频率修正量计算
  pll->w0 = ClampFloat(omega_nom - omega_correction, omega_min, omega_max); //角频率更新
  pll->wt += pll->w0 * PLL_SAMPLE_TIME; //旋转角度更新
  while (pll->wt >= (2.0f * PLL_PI))
  {
    pll->wt -= 2.0f * PLL_PI;
  }
  while (pll->wt < 0.0f)
  {
    pll->wt += 2.0f * PLL_PI;
  }

  pll->sin_wt = sinf(pll->wt);
  pll->cos_wt = cosf(pll->wt);
  pll->frequency_hz = pll->w0 / (2.0f * PLL_PI);
}

void PLL_Update(PLL_t *pll, float input)
{
  SOGI_Update(pll, input);
  dq_pll(pll);
}

float PLL_GetRms(const PLL_t *pll)
{
  return SOGI_GetRms(&pll->sogi);
}

static void SOGI_Init(SOGI_t *sogi)
{
  sogi->ualfa_0 = 0.0f;
  sogi->SOGI_Ualfa = 0.0f;
  sogi->SOGI_Ubeta = 0.0f;
  sogi->integral_2 = 0.0f;
  sogi->integral_3 = 0.0f;
  sogi->Ugird_W0 = 2.0f * PLL_PI * PLL_NOMINAL_FREQ_HZ;
  sogi->samp_t = PLL_SAMPLE_TIME;
}

static void SOGI_Run(SOGI_t *sogi, float input)
{
  float alpha_error = 0.0f;

  sogi->ualfa_0 = input;
  alpha_error = sogi->ualfa_0 - sogi->SOGI_Ualfa;

  sogi->integral_2 += (PLL_SOGI_K * alpha_error - sogi->SOGI_Ubeta)
                    * sogi->Ugird_W0
                    * sogi->samp_t;
  sogi->SOGI_Ualfa = sogi->integral_2;

  sogi->integral_3 += sogi->SOGI_Ualfa
                    * sogi->Ugird_W0
                    * sogi->samp_t;
  sogi->SOGI_Ubeta = sogi->integral_3;
}

static float SOGI_GetRms(const SOGI_t *sogi)
{
  float peak = sqrtf(sogi->SOGI_Ualfa * sogi->SOGI_Ualfa + sogi->SOGI_Ubeta * sogi->SOGI_Ubeta);

  return peak / PLL_ROOT_2;
}

static void PID_Init(PID_t *pid, float kp, float ki, float kd)
{
  pid->kp = kp;
  pid->ki = ki;
  pid->kd = kd;
  pid->ek = 0.0f;
  pid->ek_1 = 0.0f;
  pid->ek_2 = 0.0f;
  pid->uk = 0.0f;
  pid->uk_1 = 0.0f;
}

static float PID_Update(PID_t *pid, float error, float output_min, float output_max)
{
  pid->ek = error;
  pid->uk = pid->uk_1
          + pid->kp * (pid->ek - pid->ek_1)
          + pid->ki * pid->ek
          + pid->kd * (pid->ek - 2.0f * pid->ek_1 + pid->ek_2);
  pid->uk = ClampFloat(pid->uk, output_min, output_max);

  pid->ek_2 = pid->ek_1;
  pid->ek_1 = pid->ek;
  pid->uk_1 = pid->uk;

  return pid->uk;
}

static float ClampFloat(float value, float min_value, float max_value)
{
  if (value < min_value)
  {
    return min_value;
  }
  if (value > max_value)
  {
    return max_value;
  }

  return value;
}
