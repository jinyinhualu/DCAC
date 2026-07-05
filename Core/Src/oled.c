/**
 * @file oled.c
 * @brief 娉㈢壒寰嬪姩OLED椹卞姩(CH1116) - 淇敼涓篠PI鎺ュ彛
 * @anchor 娉㈢壒寰嬪姩(keysking 鍗氬摜鍦ㄥ涔?
 * @version 1.0
 * @date 2023-08-04
 * @license MIT License
 *
 * @attention
 * 鏈┍鍔ㄥ簱閽堝娉㈢壒寰嬪姩路keysking鐨凷TM32鏁欑▼瀛︿範濂椾欢杩涜寮€鍙?
 * 鍦ㄥ叾浠栧钩鍙版垨椹卞姩鑺墖涓婁娇鐢ㄥ彲鑳介渶瑕佽繘琛岀Щ妞?
 *
 * @note
 * 浣跨敤娴佺▼:
 * 1. STM32鍒濆鍖朣PI瀹屾垚鍚庤皟鐢∣LED_Init()鍒濆鍖朞LED. 娉ㄦ剰STM32鍚姩姣擮LED涓婄數蹇? 鍙瓑寰?0ms鍐嶅垵濮嬪寲OLED
 * 2. 璋冪敤OLED_NewFrame()寮€濮嬬粯鍒舵柊鐨勪竴甯?
 * 3. 璋冪敤OLED_DrawXXX()绯诲垪鍑芥暟缁樺埗鍥惧舰鍒版樉瀛?璋冪敤OLED_Printxxx()绯诲垪鍑芥暟缁樺埗鏂囨湰鍒版樉瀛?
 * 4. 璋冪敤OLED_ShowFrame()灏嗘樉瀛樺唴瀹规樉绀哄埌OLED
 *
 * @note
 * 涓轰繚璇佷腑鏂囨樉绀烘甯?璇峰皢缂栬瘧鍣ㄧ殑瀛楃闆嗚缃负UTF-8
 *
 */
#include "main.h"
#include "oled.h"
#include "spi.h"
#include <string.h>
#include <stdlib.h>

// SPI閫氫俊闇€瑕佺殑澶栭儴澹版槑
extern SPI_HandleTypeDef hspi3;

// OLED鍙傛暟
#define OLED_PAGE 8            // OLED椤垫暟
#define OLED_ROW 8 * OLED_PAGE // OLED琛屾暟
#define OLED_COLUMN 128        // OLED鍒楁暟
#define OLED_COLUMN_OFFSET 2   // OLED鍒楀亸绉?鐢变簬OLED鐨勬樉绀哄尯鍩熸瘮瀹為檯鐨勫垪鏁板皯2鍒?鍥犳闇€瑕佸亸绉?鍒?

// 鏄惧瓨
uint8_t OLED_GRAM[OLED_PAGE][OLED_COLUMN];

// ========================== 搴曞眰閫氫俊鍑芥暟 ==========================

/**
 * @brief 鍚慜LED鍙戦€佹暟鎹殑鍑芥暟(SPI鐗堟湰)
 * @param data 瑕佸彂閫佺殑鏁版嵁
 * @param len 瑕佸彂閫佺殑鏁版嵁闀垮害
 * @return None
 * @note 姝ゅ嚱鏁版槸绉绘鏈┍鍔ㄦ椂鐨勯噸瑕佸嚱鏁?灏嗘湰椹卞姩搴撶Щ妞嶅埌鍏朵粬骞冲彴鏃跺簲鏍规嵁瀹為檯鎯呭喌淇敼姝ゅ嚱鏁?
 */
void OLED_Send(uint8_t *data, uint8_t len) {
  HAL_SPI_Transmit(&hspi3, data, len, HAL_MAX_DELAY);
}

/**
 * @brief 鍚慜LED鍙戦€佹寚浠?SPI鐗堟湰)
 */
void OLED_SendCmd(uint8_t cmd) {
  // DC鎷変綆琛ㄧず鍛戒护妯″紡
  HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_RESET);
  // CS鎷変綆閫変腑璁惧
  HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
  // 鍙戦€佸懡浠?
  HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);
  // 绛夊緟SPI浼犺緭瀹屾垚鍐嶆媺楂楥S锛屽惁鍒欐渶鍚庡嚑涓瓧鑺傚彲鑳戒涪澶?
  while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_BSY)) {}
  // CS鎷夐珮缁撴潫浼犺緭
  HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
}

// ========================== OLED椹卞姩鍑芥暟 ==========================

/**
 * @brief 鍒濆鍖朞LED(SPI鐗堟湰)
 * @note 姝ゅ嚱鏁版槸绉绘鏈┍鍔ㄦ椂鐨勯噸瑕佸嚱鏁?灏嗘湰椹卞姩搴撶Щ妞嶅埌鍏朵粬椹卞姩鑺墖鏃跺簲鏍规嵁瀹為檯鎯呭喌淇敼姝ゅ嚱鏁?
 */
void OLED_Init() {
  // 纭欢澶嶄綅OLED
  HAL_GPIO_WritePin(SCREEN_RES_GPIO_Port, SCREEN_RES_Pin, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(SCREEN_RES_GPIO_Port, SCREEN_RES_Pin, GPIO_PIN_SET);
  HAL_Delay(100);

  OLED_SendCmd(0xAE); /*鍏抽棴鏄剧ず display off*/

  OLED_SendCmd(0x00); /*璁剧疆鍒楄捣濮嬪湴鍧€浣庝綅 set lower column address*/
  OLED_SendCmd(0x10); /*璁剧疆鍒楄捣濮嬪湴鍧€楂樹綅 set higher column address*/

  OLED_SendCmd(0x40); /*璁剧疆璧峰琛?set display start line*/

  OLED_SendCmd(0xB0); /*璁剧疆椤靛湴鍧€ set page address*/

  OLED_SendCmd(0x81); /*璁剧疆瀵规瘮搴?contract control*/
  OLED_SendCmd(0xCF); /*128*/

  OLED_SendCmd(0xA1); /*璁剧疆鍒嗘閲嶆槧灏?浠庡彸鍒板乏 set segment remap*/

  OLED_SendCmd(0xA6); /*姝ｅ悜鏄剧ず normal / reverse*/

  OLED_SendCmd(0xA8); /*澶氳矾澶嶇敤鐜?multiplex ratio*/
  OLED_SendCmd(0x3F); /*duty = 1/64*/

  OLED_SendCmd(0x8D); /*璁剧疆鍚姩鐢佃嵎娉?set charge pump enable*/
  OLED_SendCmd(0x14); /*鍚姩DC-DC */

  OLED_SendCmd(0x33); /*璁剧疆娉电數鍘?set VPP 10V */

  OLED_SendCmd(0xC8); /*璁剧疆杈撳嚭鎵弿鏂瑰悜 COM[N-1]鍒癈OM[0] Com scan direction*/

  OLED_SendCmd(0xD3); /*璁剧疆鏄剧ず鍋忕Щ set display offset*/
  OLED_SendCmd(0x00); /* 0x00 */

  OLED_SendCmd(0xD5); /*璁剧疆鍐呴儴鏃堕挓棰戠巼 set osc frequency*/
  OLED_SendCmd(0xC0);

  OLED_SendCmd(0xD9); /*璁剧疆鏀剧數/棰勫厖鐢垫椂闂?set pre-charge period*/
  OLED_SendCmd(0x1F); /*0x22*/

  OLED_SendCmd(0xDA); /*璁剧疆寮曡剼甯冨眬 set COM pins*/
  OLED_SendCmd(0x12);

  OLED_SendCmd(0xDB); /*璁剧疆鐢靛钩 set vcomh*/
  OLED_SendCmd(0x40);

  OLED_NewFrame();
  OLED_ShowFrame();

  OLED_SendCmd(0xAF); /*寮€鍚樉绀?display ON*/
}

/**
 * @brief 寮€鍚疧LED鏄剧ず
 */
void OLED_DisPlay_On() {
  OLED_SendCmd(0x8D); // 鐢佃嵎娉典娇鑳?
  OLED_SendCmd(0x14); // 寮€鍚數鑽锋车
  OLED_SendCmd(0xAF); // 鐐逛寒灞忓箷
}

/**
 * @brief 鍏抽棴OLED鏄剧ず
 */
void OLED_DisPlay_Off() {
  OLED_SendCmd(0x8D); // 鐢佃嵎娉典娇鑳?
  OLED_SendCmd(0x10); // 鍏抽棴鐢佃嵎娉?
  OLED_SendCmd(0xAE); // 鍏抽棴灞忓箷
}

/**
 * @brief 璁剧疆棰滆壊妯″紡 榛戝簳鐧藉瓧鎴栫櫧搴曢粦瀛?
 * @param ColorMode 棰滆壊妯″紡COLOR_NORMAL/COLOR_REVERSED
 * @note 姝ゅ嚱鏁扮洿鎺ヨ缃睆骞曠殑棰滆壊妯″紡
 */
void OLED_SetColorMode(OLED_ColorMode mode) {
  if (mode == OLED_COLOR_NORMAL) {
    OLED_SendCmd(0xA6); // 姝ｅ父鏄剧ず
  }
  if (mode == OLED_COLOR_REVERSED) {
    OLED_SendCmd(0xA7); // 鍙嶈壊鏄剧ず
  }
}

// ========================== 鏄惧瓨鎿嶄綔鍑芥暟 ==========================

/**
 * @brief 娓呯┖鏄惧瓨 缁樺埗鏂扮殑涓€甯?
 */
void OLED_NewFrame() {
  memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

/**
 * @brief 灏嗗綋鍓嶆樉瀛樻樉绀哄埌灞忓箷涓?SPI鐗堟湰)
 * @note 姝ゅ嚱鏁版槸绉绘鏈┍鍔ㄦ椂鐨勯噸瑕佸嚱鏁?灏嗘湰椹卞姩搴撶Щ妞嶅埌鍏朵粬椹卞姩鑺墖鏃跺簲鏍规嵁瀹為檯鎯呭喌淇敼姝ゅ嚱鏁?
 */
void OLED_ShowFrame() {
  uint8_t cmd;
  // 鍏ㄧ▼淇濇寔CS浣? 涓€娆℃€т紶杈撴暣甯?椤垫暟鎹? 浠呴€氳繃DC鍒囨崲鍛戒护/鏁版嵁妯″紡
  // 閬垮厤閫愰〉鍒囨崲CS瀵艰嚧鐨凜S楂樿剦鍐茶繃鐭?CH1116瑕佹眰鈮?00ns), 娑堥櫎椤甸棿鏁版嵁閿欎贡
  HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_RESET);
  for (uint8_t i = 0; i < OLED_PAGE; i++) {
    // ===== 鍛戒护闃舵: DC浣?=====
    HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_RESET);

    // 璁剧疆椤靛湴鍧€
    cmd = 0xB0 + i;
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);

    // 璁剧疆鍒楀湴鍧€浣?浣?(0x00-0x0F)
    cmd = (OLED_COLUMN_OFFSET & 0x0F);
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);

    // 璁剧疆鍒楀湴鍧€楂?浣?(0x10-0x1F)
    cmd = 0x10 | ((OLED_COLUMN_OFFSET >> 4) & 0x0F);
    HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY);

    // 绛夊緟鍛戒护鍙戦€佸畬鎴愬啀鍒囨崲DC
    while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_BSY)) {}
    while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_TXE) == RESET);

    // ===== 鏁版嵁闃舵: DC鎷夐珮 =====
    HAL_GPIO_WritePin(SCREEN_DC_GPIO_Port, SCREEN_DC_Pin, GPIO_PIN_SET);

    // 鍙戦€?28瀛楄妭鏁版嵁
    HAL_SPI_Transmit(&hspi3, OLED_GRAM[i], OLED_COLUMN, HAL_MAX_DELAY);

    // 绛夊緟鏈〉鏁版嵁鍙戦€佸畬鎴愬啀鍒囨崲DC(涓轰笅涓€椤靛懡浠ゅ仛鍑嗗)
    while (__HAL_SPI_GET_FLAG(&hspi3, SPI_FLAG_BSY)) {}
  }
  // 鏁村抚浼犺緭瀹屾瘯, 鎷夐珮CS
  HAL_GPIO_WritePin(SCREEN_CS_GPIO_Port, SCREEN_CS_Pin, GPIO_PIN_SET);
}

/**
 * @brief 璁剧疆涓€涓儚绱犵偣
 * @param x 妯潗鏍?
 * @param y 绾靛潗鏍?
 * @param color 棰滆壊
 */
void OLED_SetPixel(uint8_t x, uint8_t y, OLED_ColorMode color) {
  if (x >= OLED_COLUMN || y >= OLED_ROW) return;
  if (!color) {
    OLED_GRAM[y / 8][x] |= 1 << (y % 8);
  } else {
    OLED_GRAM[y / 8][x] &= ~(1 << (y % 8));
  }
}

/**
 * @brief 璁剧疆鏄惧瓨涓竴瀛楄妭鏁版嵁鐨勬煇鍑犱綅
 * @param page 椤靛湴鍧€
 * @param column 鍒楀湴鍧€
 * @param data 鏁版嵁
 * @param start 璧峰浣?
 * @param end 缁撴潫浣?
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁板皢鏄惧瓨涓殑鏌愪竴瀛楄妭鐨勭start浣嶅埌绗琫nd浣嶈缃负涓巇ata鐩稿悓
 * @note start鍜宔nd鐨勮寖鍥翠负0-7, start蹇呴』灏忎簬绛変簬end
 * @note 姝ゅ嚱鏁颁笌OLED_SetByte_Fine鐨勫尯鍒湪浜庢鍑芥暟鍙兘璁剧疆鏄惧瓨涓殑鏌愪竴鐪熷疄瀛楄妭
 */
void OLED_SetByte_Fine(uint8_t page, uint8_t column, uint8_t data, uint8_t start, uint8_t end, OLED_ColorMode color) {
  static uint8_t temp;
  if (page >= OLED_PAGE || column >= OLED_COLUMN) return;
  if (color) data = ~data;

  temp = data | (0xff << (end + 1)) | (0xff >> (8 - start));
  OLED_GRAM[page][column] &= temp;
  temp = data & ~(0xff << (end + 1)) & ~(0xff >> (8 - start));
  OLED_GRAM[page][column] |= temp;
  // 浣跨敤OLED_SetPixel瀹炵幇
  // for (uint8_t i = start; i <= end; i++) {
  //   OLED_SetPixel(column, page * 8 + i, !((data >> i) & 0x01));
  // }
}

/**
 * @brief 璁剧疆鏄惧瓨涓殑涓€瀛楄妭鏁版嵁
 * @param page 椤靛湴鍧€
 * @param column 鍒楀湴鍧€
 * @param data 鏁版嵁
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁板皢鏄惧瓨涓殑鏌愪竴瀛楄妭璁剧疆涓篸ata鐨勫€?
 */
void OLED_SetByte(uint8_t page, uint8_t column, uint8_t data, OLED_ColorMode color) {
  if (page >= OLED_PAGE || column >= OLED_COLUMN) return;
  if (color) data = ~data;
  OLED_GRAM[page][column] = data;
}

/**
 * @brief 璁剧疆鏄惧瓨涓殑涓€瀛楄妭鏁版嵁鐨勬煇鍑犱綅
 * @param x 妯潗鏍?
 * @param y 绾靛潗鏍?
 * @param data 鏁版嵁
 * @param len 浣嶆暟
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁板皢鏄惧瓨涓粠(x,y)寮€濮嬪悜涓嬫暟len浣嶈缃负涓巇ata鐩稿悓
 * @note len鐨勮寖鍥翠负1-8
 * @note 姝ゅ嚱鏁颁笌OLED_SetByte_Fine鐨勫尯鍒湪浜庢鍑芥暟鐨勬í鍧愭爣鍜岀旱鍧愭爣鏄互鍍忕礌涓哄崟浣嶇殑, 鍙兘鍑虹幇璺ㄤ袱涓湡瀹炲瓧鑺傜殑鎯呭喌(璺ㄩ〉)
 */
void OLED_SetBits_Fine(uint8_t x, uint8_t y, uint8_t data, uint8_t len, OLED_ColorMode color) {
  uint8_t page = y / 8;
  uint8_t bit = y % 8;
  if (bit + len > 8) {
    OLED_SetByte_Fine(page, x, data << bit, bit, 7, color);
    OLED_SetByte_Fine(page + 1, x, data >> (8 - bit), 0, len + bit - 1 - 8, color);
  } else {
    OLED_SetByte_Fine(page, x, data << bit, bit, bit + len - 1, color);
  }
  // 浣跨敤OLED_SetPixel瀹炵幇
  // for (uint8_t i = 0; i < len; i++) {
  //   OLED_SetPixel(x, y + i, !((data >> i) & 0x01));
  // }
}

/**
 * @brief 璁剧疆鏄惧瓨涓竴瀛楄妭闀垮害鐨勬暟鎹?
 * @param x 妯潗鏍?
 * @param y 绾靛潗鏍?
 * @param data 鏁版嵁
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁板皢鏄惧瓨涓粠(x,y)寮€濮嬪悜涓嬫暟8浣嶈缃负涓巇ata鐩稿悓
 * @note 姝ゅ嚱鏁颁笌OLED_SetByte鐨勫尯鍒湪浜庢鍑芥暟鐨勬í鍧愭爣鍜岀旱鍧愭爣鏄互鍍忕礌涓哄崟浣嶇殑, 鍙兘鍑虹幇璺ㄤ袱涓湡瀹炲瓧鑺傜殑鎯呭喌(璺ㄩ〉)
 */
void OLED_SetBits(uint8_t x, uint8_t y, uint8_t data, OLED_ColorMode color) {
  uint8_t page = y / 8;
  uint8_t bit = y % 8;
  OLED_SetByte_Fine(page, x, data << bit, bit, 7, color);
  if (bit) {
    OLED_SetByte_Fine(page + 1, x, data >> (8 - bit), 0, bit - 1, color);
  }
}

/**
 * @brief 璁剧疆涓€鍧楁樉瀛樺尯鍩?
 * @param x 璧峰妯潗鏍?
 * @param y 璧峰绾靛潗鏍?
 * @param data 鏁版嵁鐨勮捣濮嬪湴鍧€
 * @param w 瀹藉害
 * @param h 楂樺害
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁板皢鏄惧瓨涓粠(x,y)寮€濮嬬殑w*h涓儚绱犺缃负data涓殑鏁版嵁
 * @note data鐨勬暟鎹簲璇ラ噰鐢ㄥ垪琛屽紡鎺掑垪
 */
void OLED_SetBlock(uint8_t x, uint8_t y, const uint8_t *data, uint8_t w, uint8_t h, OLED_ColorMode color) {
  uint8_t fullRow = h / 8; // 瀹屾暣鐨勮鏁?
  uint8_t partBit = h % 8; // 涓嶅畬鏁寸殑瀛楄妭涓殑鏈夋晥浣嶆暟
  for (uint8_t i = 0; i < w; i++) {
    for (uint8_t j = 0; j < fullRow; j++) {
      OLED_SetBits(x + i, y + j * 8, data[i + j * w], color);
    }
  }
  if (partBit) {
    uint16_t fullNum = w * fullRow; // 瀹屾暣鐨勫瓧鑺傛暟
    for (uint8_t i = 0; i < w; i++) {
      OLED_SetBits_Fine(x + i, y + (fullRow * 8), data[fullNum + i], partBit, color);
    }
  }
  // 浣跨敤OLED_SetPixel瀹炵幇
  // for (uint8_t i = 0; i < w; i++) {
  //   for (uint8_t j = 0; j < h; j++) {
  //     for (uint8_t k = 0; k < 8; k++) {
  //       if (j * 8 + k >= h) break; // 闃叉瓒婄晫(涓嶅畬鏁寸殑瀛楄妭
  //       OLED_SetPixel(x + i, y + j * 8 + k, !((data[i + j * w] >> k) & 0x01));
  //     }
  //   }
  // }
}

// ========================== 鍥惧舰缁樺埗鍑芥暟 ==========================
/**
 * @brief 缁樺埗涓€鏉＄嚎娈?
 * @param x1 璧峰鐐规í鍧愭爣
 * @param y1 璧峰鐐圭旱鍧愭爣
 * @param x2 缁堟鐐规í鍧愭爣
 * @param y2 缁堟鐐圭旱鍧愭爣
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁颁娇鐢˙resenham绠楁硶缁樺埗绾挎
 */
void OLED_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, OLED_ColorMode color) {
  static uint8_t temp = 0;
  if (x1 == x2) {
    if (y1 > y2) {
      temp = y1;
      y1 = y2;
      y2 = temp;
    }
    for (uint8_t y = y1; y <= y2; y++) {
      OLED_SetPixel(x1, y, color);
    }
  } else if (y1 == y2) {
    if (x1 > x2) {
      temp = x1;
      x1 = x2;
      x2 = temp;
    }
    for (uint8_t x = x1; x <= x2; x++) {
      OLED_SetPixel(x, y1, color);
    }
  } else {
    // Bresenham鐩寸嚎绠楁硶
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    int16_t ux = ((dx > 0) << 1) - 1;
    int16_t uy = ((dy > 0) << 1) - 1;
    int16_t x = x1, y = y1, eps = 0;
    dx = abs(dx);
    dy = abs(dy);
    if (dx > dy) {
      for (x = x1; x != x2; x += ux) {
        OLED_SetPixel(x, y, color);
        eps += dy;
        if ((eps << 1) >= dx) {
          y += uy;
          eps -= dx;
        }
      }
    } else {
      for (y = y1; y != y2; y += uy) {
        OLED_SetPixel(x, y, color);
        eps += dx;
        if ((eps << 1) >= dy) {
          x += ux;
          eps -= dy;
        }
      }
    }
  }
}

/**
 * @brief 缁樺埗涓€涓煩褰?
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param w 鐭╁舰瀹藉害
 * @param h 鐭╁舰楂樺害
 * @param color 棰滆壊
 */
void OLED_DrawRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_ColorMode color) {
  OLED_DrawLine(x, y, x + w, y, color);
  OLED_DrawLine(x, y + h, x + w, y + h, color);
  OLED_DrawLine(x, y, x, y + h, color);
  OLED_DrawLine(x + w, y, x + w, y + h, color);
}

/**
 * @brief 缁樺埗涓€涓～鍏呯煩褰?
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param w 鐭╁舰瀹藉害
 * @param h 鐭╁舰楂樺害
 * @param color 棰滆壊
 */
void OLED_DrawFilledRectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h, OLED_ColorMode color) {
  for (uint8_t i = 0; i < h; i++) {
    OLED_DrawLine(x, y+i, x+w-1, y+i, color);
  }
}

/**
 * @brief 缁樺埗涓€涓笁瑙掑舰
 * @param x1 绗竴涓偣妯潗鏍?
 * @param y1 绗竴涓偣绾靛潗鏍?
 * @param x2 绗簩涓偣妯潗鏍?
 * @param y2 绗簩涓偣绾靛潗鏍?
 * @param x3 绗笁涓偣妯潗鏍?
 * @param y3 绗笁涓偣绾靛潗鏍?
 * @param color 棰滆壊
 */
void OLED_DrawTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, OLED_ColorMode color) {
  OLED_DrawLine(x1, y1, x2, y2, color);
  OLED_DrawLine(x2, y2, x3, y3, color);
  OLED_DrawLine(x3, y3, x1, y1, color);
}

/**
 * @brief 缁樺埗涓€涓～鍏呬笁瑙掑舰
 * @param x1 绗竴涓偣妯潗鏍?
 * @param y1 绗竴涓偣绾靛潗鏍?
 * @param x2 绗簩涓偣妯潗鏍?
 * @param y2 绗簩涓偣绾靛潗鏍?
 * @param x3 绗笁涓偣妯潗鏍?
 * @param y3 绗笁涓偣绾靛潗鏍?
 * @param color 棰滆壊
 */
void OLED_DrawFilledTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, OLED_ColorMode color) {
  uint8_t a = 0, b = 0, y = 0, last = 0;
  if (y1 > y2) {
    a = y2;
    b = y1;
  } else {
    a = y1;
    b = y2;
  }
  y = a;
  for (; y <= b; y++) {
    if (y <= y3) {
      OLED_DrawLine(x1 + (y - y1) * (x2 - x1) / (y2 - y1), y, x1 + (y - y1) * (x3 - x1) / (y3 - y1), y, color);
    } else {
      last = y - 1;
      break;
    }
  }
  for (; y <= b; y++) {
    OLED_DrawLine(x2 + (y - y2) * (x3 - x2) / (y3 - y2), y, x1 + (y - last) * (x3 - x1) / (y3 - last), y, color);
  }
}

/**
 * @brief 缁樺埗涓€涓渾
 * @param x 鍦嗗績妯潗鏍?
 * @param y 鍦嗗績绾靛潗鏍?
 * @param r 鍦嗗崐寰?
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁颁娇鐢˙resenham绠楁硶缁樺埗鍦?
 */
void OLED_DrawCircle(uint8_t x, uint8_t y, uint8_t r, OLED_ColorMode color) {
  int16_t a = 0, b = r, di = 3 - (r << 1);
  while (a <= b) {
    OLED_SetPixel(x - b, y - a, color);
    OLED_SetPixel(x + b, y - a, color);
    OLED_SetPixel(x - a, y + b, color);
    OLED_SetPixel(x - b, y - a, color);
    OLED_SetPixel(x - a, y - b, color);
    OLED_SetPixel(x + b, y + a, color);
    OLED_SetPixel(x + a, y - b, color);
    OLED_SetPixel(x + a, y + b, color);
    OLED_SetPixel(x - b, y + a, color);
    a++;
    if (di < 0) {
      di += 4 * a + 6;
    } else {
      di += 10 + 4 * (a - b);
      b--;
    }
    OLED_SetPixel(x + a, y + b, color);
  }
}

/**
 * @brief 缁樺埗涓€涓～鍏呭渾
 * @param x 鍦嗗績妯潗鏍?
 * @param y 鍦嗗績绾靛潗鏍?
 * @param r 鍦嗗崐寰?
 * @param color 棰滆壊
 * @note 姝ゅ嚱鏁颁娇鐢˙resenham绠楁硶缁樺埗鍦?
 */
void OLED_DrawFilledCircle(uint8_t x, uint8_t y, uint8_t r, OLED_ColorMode color) {
  int16_t a = 0, b = r, di = 3 - (r << 1);
  while (a <= b) {
    for (int16_t i = x - b; i <= x + b; i++) {
      OLED_SetPixel(i, y + a, color);
      OLED_SetPixel(i, y - a, color);
    }
    for (int16_t i = x - a; i <= x + a; i++) {
      OLED_SetPixel(i, y + b, color);
      OLED_SetPixel(i, y - b, color);
    }
    a++;
    if (di < 0) {
      di += 4 * a + 6;
    } else {
      di += 10 + 4 * (a - b);
      b--;
    }
  }
}

/**
 * @brief 缁樺埗涓€涓き鍦?
 * @param x 妞渾涓績妯潗鏍?
 * @param y 妞渾涓績绾靛潗鏍?
 * @param a 妞渾闀胯酱
 * @param b 妞渾鐭酱
 */
void OLED_DrawEllipse(uint8_t x, uint8_t y, uint8_t a, uint8_t b, OLED_ColorMode color) {
  int xpos = 0, ypos = b;
  int a2 = a * a, b2 = b * b;
  int d = b2 + a2 * (0.25 - b);
  while (a2 * ypos > b2 * xpos) {
    OLED_SetPixel(x + xpos, y + ypos, color);
    OLED_SetPixel(x - xpos, y + ypos, color);
    OLED_SetPixel(x + xpos, y - ypos, color);
    OLED_SetPixel(x - xpos, y - ypos, color);
    if (d < 0) {
      d = d + b2 * ((xpos << 1) + 3);
      xpos += 1;
    } else {
      d = d + b2 * ((xpos << 1) + 3) + a2 * (-(ypos << 1) + 2);
      xpos += 1, ypos -= 1;
    }
  }
  d = b2 * (xpos + 0.5) * (xpos + 0.5) + a2 * (ypos - 1) * (ypos - 1) - a2 * b2;
  while (ypos > 0) {
    OLED_SetPixel(x + xpos, y + ypos, color);
    OLED_SetPixel(x - xpos, y + ypos, color);
    OLED_SetPixel(x + xpos, y - ypos, color);
    OLED_SetPixel(x - xpos, y - ypos, color);
    if (d < 0) {
      d = d + b2 * ((xpos << 1) + 2) + a2 * (-(ypos << 1) + 3);
      xpos += 1, ypos -= 1;
    } else {
      d = d + a2 * (-(ypos << 1) + 3);
      ypos -= 1;
    }
  }
}

/**
 * @brief 缁樺埗涓€寮犲浘鐗?
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param img 鍥剧墖
 * @param color 棰滆壊
 */
void OLED_DrawImage(uint8_t x, uint8_t y, const Image *img, OLED_ColorMode color) {
  OLED_SetBlock(x, y, img->data, img->w, img->h, color);
}

// ================================ 鏂囧瓧缁樺埗 ================================

/**
 * @brief 缁樺埗涓€涓狝SCII瀛楃
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param ch 瀛楃
 * @param font 瀛椾綋
 * @param color 棰滆壊
 */
void OLED_PrintASCIIChar(uint8_t x, uint8_t y, char ch, const ASCIIFont *font, OLED_ColorMode color) {
  OLED_SetBlock(x, y, font->chars + (ch - ' ') * (((font->h + 7) / 8) * font->w), font->w, font->h, color);
}

/**
 * @brief 缁樺埗涓€涓狝SCII瀛楃涓?
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param str 瀛楃涓?
 * @param font 瀛椾綋
 * @param color 棰滆壊
 */
void OLED_PrintASCIIString(uint8_t x, uint8_t y, char *str, const ASCIIFont *font, OLED_ColorMode color) {
  uint8_t x0 = x;
  while (*str) {
    OLED_PrintASCIIChar(x0, y, *str, font, color);
    x0 += font->w;
    str++;
  }
}

void OLED_PrintNumber(uint8_t x, uint8_t y, int num, OLED_ColorMode color) {
  char buffer[12];
  uint8_t index = 0;
  uint8_t start = 0;
  uint32_t value = (uint32_t)num;

  if (num < 0) {
    buffer[index++] = '-';
    value = (uint32_t)(-(int64_t)num);
    start = 1;
  }

  do {
    buffer[index++] = (char)('0' + (value % 10));
    value /= 10;
  } while ((value > 0) && (index < (sizeof(buffer) - 1U)));

  buffer[index] = '\0';

  for (uint8_t left = start, right = index - 1U; left < right; left++, right--) {
    char temp = buffer[left];
    buffer[left] = buffer[right];
    buffer[right] = temp;
  }

  OLED_PrintASCIIString(x, y, buffer, &afont16x8, color);
}

/**
 * @brief 鑾峰彇UTF-8缂栫爜鐨勫瓧绗﹂暱搴?
 */
uint8_t _OLED_GetUTF8Len(char *string) {
  if ((string[0] & 0x80) == 0x00) {
    return 1;
  } else if ((string[0] & 0xE0) == 0xC0) {
    return 2;
  } else if ((string[0] & 0xF0) == 0xE0) {
    return 3;
  } else if ((string[0] & 0xF8) == 0xF0) {
    return 4;
  }
  return 0;
}

/**
 * @brief 缁樺埗瀛楃涓?
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param str 瀛楃涓?
 * @param font 瀛椾綋
 * @param color 棰滆壊
 *
 * @note 涓轰繚璇佸瓧绗︿覆涓殑涓枃浼氳鑷姩璇嗗埆骞剁粯鍒? 闇€:
 * 1. 缂栬瘧鍣ㄥ瓧绗﹂泦璁剧疆涓篣TF-8
 * 2. 浣跨敤娉㈢壒寰嬪姩LED鍙栨ā宸ュ叿鐢熸垚瀛楁ā(https://led.baud-dance.com)
 */
/**
 * @brief 缁樺埗瀛楃涓?
 * @param x 璧峰鐐规í鍧愭爣
 * @param y 璧峰鐐圭旱鍧愭爣
 * @param str 瀛楃涓?
 * @param font 瀛椾綋
 * @param color 棰滆壊
 *
 * @note 涓轰繚璇佸瓧绗︿覆涓殑涓枃浼氳鑷姩璇嗗埆骞剁粯鍒? 闇€:
 * 1. 缂栬瘧鍣ㄥ瓧绗﹂泦璁剧疆涓篣TF-8
 * 2. 浣跨敤娉㈢壒寰嬪姩LED鍙栨ā宸ュ叿鐢熸垚瀛楁ā(https://led.baud-dance.com)
 */
void OLED_PrintString(uint8_t x, uint8_t y, char *str, const Font *font, OLED_ColorMode color) {
  uint16_t i = 0;                                       // 瀛楃涓茬储寮?
  uint8_t oneLen = (((font->h + 7) / 8) * font->w) + 4; // 涓€涓瓧妯″崰澶氬皯瀛楄妭
  uint8_t found;                                        // 鏄惁鎵惧埌瀛楁ā
  uint8_t utf8Len;                                      // UTF-8缂栫爜闀垮害
  uint8_t *head;                                        // 瀛楁ā澶存寚閽?
  while (str[i]) {
    found = 0;
    utf8Len = _OLED_GetUTF8Len(str + i);
    if (utf8Len == 0) break; // 鏈夐棶棰樼殑UTF-8缂栫爜

    // 瀵绘壘瀛楃  TODO 浼樺寲鏌ユ壘绠楁硶, 浜屽垎鏌ユ壘鎴栬€卙ash
    for (uint8_t j = 0; j < font->len; j++) {
      head = (uint8_t *)(font->chars) + (j * oneLen);
      if (memcmp(str + i, head, utf8Len) == 0) {
        OLED_SetBlock(x, y, head + 4, font->w, font->h, color);
        // 绉诲姩鍏夋爣
        x += font->w;
        i += utf8Len;
        found = 1;
        break;
      }
    }

    // 鑻ユ湭鎵惧埌瀛楁ā,涓斾负ASCII瀛楃, 鍒欑己鐪佹樉绀篈SCII瀛楃
    if (found == 0) {
      if (utf8Len == 1) {
        OLED_PrintASCIIChar(x, y, str[i], font->ascii, color);
        // 绉诲姩鍏夋爣
        x += font->ascii->w;
        i += utf8Len;
      } else {
        OLED_PrintASCIIChar(x, y, ' ', font->ascii, color);
        x += font->ascii->w;
        i += utf8Len;
      }
    }
  }
}






