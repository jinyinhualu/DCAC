#ifndef __PLL_H__
#define __PLL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define PLL_PI 3.1415926f
#define PLL_ROOT_2 1.4142f

#define PLL_NOMINAL_FREQ_HZ 50.0f                   //工频
#define PLL_SAMPLE_FREQ_HZ 10000.0f                 //采样频率
#define PLL_SAMPLE_TIME (1.0f / PLL_SAMPLE_FREQ_HZ) //采样周期
#define PLL_SOGI_K PLL_ROOT_2                       //SOGI增益系数

#define PLL_BANDWIDTH_HZ 20.0f                      //锁相环带宽
#define PLL_DAMPING 0.707f                          //阻尼系数
#define PLL_MIN_FREQ_HZ 45.0f                       //最小频率
#define PLL_MAX_FREQ_HZ 55.0f                       //最大频率
#define PLL_MIN_SIGNAL 0.01f                        //最小信号幅值，低于该值时不进行相位误差计算

typedef struct SOGI {
    float ualfa_0;                                  //正弦点大小
    float SOGI_Ualfa;                               //alpha分量
    float SOGI_Ubeta;                               //beta分量

    float integral_2;                               //dalpha分量积分
    float integral_3;                               //dbeta分量积分

    float Ugird_W0;                                 //角频率，也是旋转角速度
    float samp_t;                                   //采样周期
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
    float wt;                                       //旋转角度
    float w0;                                       //旋转角速度    
    PID_t pid;                                      //PID控制器
    SOGI_t sogi;                                    //SOGI滤波器
    float sin_wt;                                   //sin(wt)，当前采样点的正弦值
    float cos_wt;                                   //cos(wt)，当前采样点的余弦值
    float frequency_hz;                             //频率(Hz)
} PLL_t;

void PLL_Init(PLL_t *pll);
void PLL_Update(PLL_t *pll, float input);
float PLL_GetRms(const PLL_t *pll);

#ifdef __cplusplus
}
#endif

#endif /* __PLL_H__ */
