#include <ctype.h>

UCHAR _ctype[ 256 ] =
{
    /* 00 */ _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,
    /* 08 */ _IS_CTL, _IS_SP | _IS_BLK, _IS_CTL |_IS_SP,  _IS_CTL |_IS_SP,  _IS_CTL |_IS_SP, _IS_CTL |_IS_SP, _IS_CTL, _IS_CTL,
    /* 10 */ _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,
    /* 18 */ _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL, _IS_CTL,
    /* 20 */ _IS_SP | _IS_BLK, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN,
    /* 28 */ _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN,
    /* 30 */ _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG,
    /* 38 */ _IS_HEX |_IS_DIG, _IS_HEX |_IS_DIG, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN,
    /* 40 */ _IS_PUN, _IS_HEX |_IS_UPP, _IS_HEX |_IS_UPP, _IS_HEX |_IS_UPP, _IS_HEX |_IS_UPP, _IS_HEX |_IS_UPP, _IS_HEX |_IS_UPP, _IS_UPP,
    /* 48 */ _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP,
    /* 50 */ _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP, _IS_UPP,
    /* 58 */ _IS_UPP, _IS_UPP, _IS_UPP, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN,
    /* 60 */ _IS_PUN, _IS_HEX |_IS_LOW, _IS_HEX |_IS_LOW, _IS_HEX |_IS_LOW, _IS_HEX |_IS_LOW, _IS_HEX |_IS_LOW, _IS_HEX |_IS_LOW, _IS_LOW,
    /* 68 */ _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW,
    /* 70 */ _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW, _IS_LOW,
    /* 78 */ _IS_LOW, _IS_LOW, _IS_LOW, _IS_PUN, _IS_PUN, _IS_PUN, _IS_PUN, _IS_CTL,
};

