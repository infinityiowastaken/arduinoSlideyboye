

/*
 *
 * circle
 *
 * created with FontCreator
 * written by F. Maximilian Thiele
 *
 * http://www.apetech.de/fontCreator
 * me@apetech.de
 *
 * File Name           : circle.h
 * Date                : 29.04.2020
 * Font size in bytes  : 654
 * Font width          : 8
 * Font height         : -10
 * Font first char     : 48
 * Font last char      : 56
 * Font used chars     : 8
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

#ifndef CIRCLE_H
#define CIRCLE_H

#define CIRCLE_WIDTH 8
#define CIRCLE_HEIGHT -10

GLCDFONTDECL(circle) = {
    0x00, 0x00, // size
    0x07,       // width
    0x07,       // height
    0x30,       // first char
    0x0A,       // char count

    // font data
    0x38, 0x44, 0x82, 0x80, 0x80, 0x44, 0x38, // encoder 0-7
    0x38, 0x44, 0x82, 0x82, 0x82, 0x40, 0x30, // 
    0x38, 0x44, 0x82, 0x82, 0x82, 0x44, 0x08, // 
    0x38, 0x44, 0x82, 0x82, 0x02, 0x04, 0x38, // 
    0x38, 0x44, 0x02, 0x02, 0x82, 0x44, 0x38, // 
    0x18, 0x04, 0x82, 0x82, 0x82, 0x44, 0x38, // 
    0x20, 0x44, 0x82, 0x82, 0x82, 0x44, 0x38, // 
    0x38, 0x40, 0x80, 0x82, 0x82, 0x44, 0x38, //

    0x38, 0x44, 0x82, 0x82, 0x82, 0x44, 0x38, // circle
    0x38, 0x7C, 0xFE, 0xFE, 0xFE, 0x7C, 0x38  // circle, filled

};

#endif
