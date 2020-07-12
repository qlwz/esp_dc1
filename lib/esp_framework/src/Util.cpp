#include "Util.h"

char *Util::strlowr(char *str)
{
    char *orign = str;
    for (; *str != '\0'; str++)
        *str = tolower(*str);
    return orign;
}

char *Util::strupr(char *str)
{
    char *orign = str;
    for (; *str != '\0'; str++)
        *str = toupper(*str);
    return orign;
}

uint16_t Util::hex2Str(uint8_t *bin, uint16_t bin_size, char *buff, bool needBlank)
{
    const char *set = "0123456789ABCDEF";
    char *nptr = buff;
    if (NULL == buff)
    {
        return -1;
    }
    uint16_t len = needBlank ? (bin_size * 2 + bin_size) : (bin_size * 2 + 1);
    while (bin_size--)
    {
        *nptr++ = set[(*bin) >> 4];
        *nptr++ = set[(*bin++) & 0xF];
        if (needBlank && bin_size > 0)
        {
            *nptr++ = ' ';
        }
    }
    *nptr = '\0';
    return len;
}

char *Util::dtostrfd(double number, unsigned char prec, char *s)
{
    if ((isnan(number)) || (isinf(number)))
    { // Fix for JSON output (https://stackoverflow.com/questions/1423081/json-left-out-infinity-and-nan-json-status-in-ecmascript)
        strcpy(s, PSTR("null"));
        return s;
    }
    else
    {
        return dtostrf(number, 1, prec, s);
    }
}

uint32_t Util::SqrtInt(uint32_t num)
{
    if (num <= 1)
    {
        return num;
    }

    uint32_t x = num / 2;
    uint32_t y;
    do
    {
        y = (x + num / x) / 2;
        if (y >= x)
        {
            return x;
        }
        x = y;
    } while (true);
}

uint32_t Util::RoundSqrtInt(uint32_t num)
{
    uint32_t s = SqrtInt(4 * num);
    if (s & 1)
    {
        s++;
    }
    return s / 2;
}

bool Util::endWith(char *str, const char *suffix, uint16_t strLen)
{
    if (strLen == 0)
    {
        strLen = strlen(str);
    }
    size_t suffixLen = strlen(suffix);
    return suffixLen <= strLen && strncmp(str + strLen - suffixLen, suffix, suffixLen) == 0 ? true : false;
}
