

/*
 *
 * lineboye
 *
 * created with FontCreator
 * written by F. Maximilian Thiele
 *
 * http://www.apetech.de/fontCreator
 * me@apetech.de
 *
 * File Name           : lineboye.h
 * Date                : 27.04.2020
 * Font size in bytes  : 375
 * Font width          : 8
 * Font height         : -5
 * Font first char     : 48
 * Font last char      : 57
 * Font used chars     : 9
 *
 * The font data are defined as
 *
 * struct _FONT_ {
 *     uint16_t   font_Size_in_Bytes_over_all_included_Size_it_self;
 *     uint8_t    font_Width_in_Pixel_for_fixed_drawing;
 *     uint8_t    font_Height_in_Pixel_for_all_characters;
 *     unit8_t    font_First_Char;
 *     uint8_t    font_Char_Count;
 *
 *     uint8_t    font_Char_Widths[font_Last_Char - font_First_Char +1];
 *                  // for each character the separate width in pixels,
 *                  // characters < 128 have an implicit virtual right empty row
 *
 *     uint8_t    font_data[];
 *                  // bit field of all characters
 */

#ifndef lineboye_H
#define lineboye_H

#define lineboye_WIDTH 8
#define lineboye_HEIGHT -5

GLCDFONTDECL(lineboye) = {
    0x0, 0x0,   // size
    0x08,       // width
    0x05,       // height
    0x30,       // first char
    0x09,       // char count

    // font data
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 48
    0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 49
    0x20, 0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 50
    0x20, 0x20, 0xF8, 0x20, 0x20, 0x20, 0x20, 0x20, // 51
    0x20, 0x20, 0x20, 0xF8, 0x20, 0x20, 0x20, 0x20, // 52
    0x20, 0x20, 0x20, 0x20, 0xF8, 0x20, 0x20, 0x20, // 53
    0x20, 0x20, 0x20, 0x20, 0x20, 0xF8, 0x20, 0x20, // 54
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xF8, 0x20, // 55
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xF8  // 56

};

#endif
