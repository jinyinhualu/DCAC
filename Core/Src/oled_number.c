#include "oled.h"

void OLED_PrintNumber(uint8_t x, uint8_t y, int num, const ASCIIFont *font, OLED_ColorMode color)
{
  char buffer[12];
  char reversed[10];
  uint8_t length = 0U;
  uint8_t digits = 0U;
  unsigned int magnitude;

  if (num < 0)
  {
    buffer[length++] = '-';
    magnitude = (unsigned int)(-(num + 1)) + 1U;
  }
  else
  {
    magnitude = (unsigned int)num;
  }

  if (magnitude == 0U)
  {
    buffer[length++] = '0';
  }
  else
  {
    while ((magnitude > 0U) && (digits < sizeof(reversed)))
    {
      reversed[digits++] = (char)('0' + (magnitude % 10U));
      magnitude /= 10U;
    }

    while (digits > 0U)
    {
      buffer[length++] = reversed[--digits];
    }
  }

  buffer[length] = '\0';
  OLED_PrintASCIIString(x, y, buffer, font, color);
}
