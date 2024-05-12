/* edid.h
   Purpose: EDID block definition, used by Intel GPU driver. */
#pragma once

#include "../types.h"

#ifndef _POWER_EDID_H
#define _POWER_EDID_H 1

struct __kpacked edid_header {
    __uchar Signature[8];
    __uchar Manufacturer[2];
    __uint16 ProductCode;
    __uint32 SerialNumber;
    __uchar ManufacturingWeek;
    __uchar ManufacturingYear; /*< epoch is 1990 */
    __uchar Version;
    __uchar Revision;
};

struct __kpacked edid_display {
    union {
        __uchar DisplayType; /*< Bit 7 set is digital, clear is analog*/
        struct {
            __uchar BitDepthInterface;
        } Digital;

        struct {
            __uchar VideoWhiteLevel;
        } Analog;
    };

    __uchar HorizScreenSize;
    __uchar VerticalScreenSize;
    __uchar Gamma;
    __uchar SupportedFeatures;
};

struct __kpacked edid_chroma { /*< 2º CIE 1931 */
    __uchar RedGreenLeastSignificant; /*< See the EDID spec */
    __uchar BlueWhiteLeastSignificant;
    __uchar RedXMostSignificant;
    __uchar RedYMostSignificant;
    __uchar GreenXMostSignificant;
    __uchar GreenYMostSignificant;
    __uchar BlueXMostSignificant;
    __uchar BlueYMostSignificant;
    __uchar WhitePointX;
    __uchar WhitePointY;
};

struct __kpacked edid_standard_timing {
    __uchar XResolution;
    __uchar AspectRatio;
};

#define EDID_DETAIL_ACTIVE_SHIFT 4
#define EDID_DETAIL_ACTIVE_UPPER_BITS (0b1111 << EDID_DETAIL_ACTIVE_SHIFT)
#define EDID_DETAIL_BLANK_UPPER_BITS (0b1111)

#define EDID_DETAIL_VPORCH_MIDDLE_SHIFT 4
#define EDID_DETAIL_VPORCH_MIDDLE_BITS (0b1111 << EDID_DETAIL_VPORCH_MIDDLE_SHIFT)

#define EDID_DETAIL_VSYNC_MIDDLE_BITS (0b1111)

#define EDID_DETAIL_HPORCH_UPPER_SHIFT 6
#define EDID_DETAIL_HPORCH_UPPER_BITS (0b11 << EDID_DETAIL_HPORCH_UPPER_SHIFT)
#define EDID_DETAIL_HSYNC_UPPER_SHIFT 4
#define EDID_DETAIL_HSYNC_UPPER_BITS (0b11 << EDID_DETAIL_HSYNC_UPPER_SHIFT)
#define EDID_DETAIL_VPORCH_UPPER_SHIFT 2
#define EDID_DETAIL_VPORCH_UPPER_BITS (0b11 << EDID_DETAIL_VPORCH_UPPER_SHIFT)
#define EDID_DETAIL_VSYNC_UPPER_BITS (0b11)

#define EDID_DETAIL_HORIZONTAL_IMAGE_SIZE_SHIFT 4
#define EDID_DETAIL_HORIZONTAL_IMAGE_SIZE_BITS                                       \
    (0b1111 << EDID_DETAIL_HORIZONTAL_IMAGE_SIZE_SHIFT)
#define EDID_DETAIL_VERTICAL_IMAGE_SIZE_BITS (0b1111)

struct __kpacked edid_detailed_timing {
    __uint16 PixelClock; /*< in 10 kHz units. */
    __uchar HorizActivePixelLower;
    __uchar HorizBlankPixelLower;
    __uchar HorizBlankActiveUpper4; /*< Upper 4 bits of ActivePixel and
                                       BlankPixel */

    __uchar VerticalActivePixelLower;
    __uchar VerticalBlankPixelLower;
    __uchar VerticalBlankActiveUpper4;

    __uchar HorizFrontPorchLower;
    __uchar HorizSyncPulseLower;
    __uchar VerticalFrontPorchLower4; /*< Also has the vertical sync pulse
                                        lower 4 bits */
    __uchar HorizVertFrontPorch; /*< Horizontal/Vertical Front Porch/Sync Pulse
                                    upper 2 bits each */

    __uchar HorizImageSizeLower; /*< In millimeters. */
    __uchar VerticalImageSizeLower; /*< In millimeters. */
    __uchar HorizVertImageSizeUpper4; /*< Upper 4 bits of vertical/horizontal
                                         image size */

    __uchar HorizontalBorderPixels; /*< Per side */
    __uchar VerticalBorderPixels; /*< Per side */

    __uchar Features;
};

struct edid {
    struct edid_header Header;
    struct edid_display Display;
    struct edid_chroma Chromacity;

    __uchar EstablishedTimings[3]; /*< Bitmap */

    struct edid_standard_timing StandardTimings[8];
    struct edid_detailed_timing Detailed[4];

    __uchar ExtensionCount;
    __uchar Checksum;
};

#endif
