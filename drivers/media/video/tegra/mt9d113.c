/*
 * kernel/drivers/media/video/tegra
 *
 * Aptina MT9D113 sensor driver
 *
 * Copyright (C) 2012 MALATA Corporation
 *
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <media/mt9d113.h>

#define SENSOR_NAME     "mt9d113"
#define DEV(x)          "/dev/"x
#define SENSOR_PATH     DEV(SENSOR_NAME)

#define SENSOR_WAIT_MS      0  /* special number to indicate the wait time */
#define SENSOR_TABLE_END    1  /* special number to indicate the end of table */
#define SENSOR_MAX_RETRIES  3 /* max counter for retry I2C access */

#define SENSOR_640_WIDTH_VAL   0x280
#define SENSOR_720_WIDTH_VAL   0x500
#define SENSOR_1600_WIDTH_VAL  0x640

struct sensor_reg {
	u16 addr;
	u16 val;
};

struct sensor_info {
	int mode;
	struct i2c_client *i2c_client;
	struct mt9p113_platform_data *pdata;
};

static struct sensor_info *info;

enum {
	YUV_ColorEffect = 0,
	YUV_Whitebalance,
	YUV_SceneMode,
	YUV_Exposure
};

enum {
	YUV_ColorEffect_Invalid = 0,
	YUV_ColorEffect_Aqua,
	YUV_ColorEffect_Blackboard,
	YUV_ColorEffect_Mono,
	YUV_ColorEffect_Negative,
	YUV_ColorEffect_None,
	YUV_ColorEffect_Posterize,
	YUV_ColorEffect_Sepia,
	YUV_ColorEffect_Solarize,
	YUV_ColorEffect_Whiteboard
};

enum {
	YUV_Whitebalance_Invalid = 0,
	YUV_Whitebalance_Auto,
	YUV_Whitebalance_Incandescent,
	YUV_Whitebalance_Fluorescent,
	YUV_Whitebalance_WarmFluorescent,
	YUV_Whitebalance_Daylight,
	YUV_Whitebalance_CloudyDaylight,
	YUV_Whitebalance_Shade,
	YUV_Whitebalance_Twilight,
	YUV_Whitebalance_Custom
};

enum {
	YUV_SceneMode_Invalid = 0,
	YUV_SceneMode_Auto,
	YUV_SceneMode_Action,
	YUV_SceneMode_Portrait,
	YUV_SceneMode_Landscape,
	YUV_SceneMode_Beach,
	YUV_SceneMode_Candlelight,
	YUV_SceneMode_Fireworks,
	YUV_SceneMode_Night,
	YUV_SceneMode_NightPortrait,
	YUV_SceneMode_Party,
	YUV_SceneMode_Snow,
	YUV_SceneMode_Sports,
	YUV_SceneMode_SteadyPhoto,
	YUV_SceneMode_Sunset,
	YUV_SceneMode_Theatre,
	YUV_SceneMode_Barcode
};

enum {
	YUV_Exposure_Invalid = 0,
	YUV_Exposure_Positive2,
	YUV_Exposure_Positive1,
	YUV_Exposure_Number0,
	YUV_Exposure_Negative1,
	YUV_Exposure_Negative2,
};

static struct sensor_reg mode_1600x1200[] = {
{0x098C, 0xA115},
{0x0990, 0x0002},
{0x098C, 0xA103},
{0x0990, 0x0002},
{SENSOR_WAIT_MS, 100},
{SENSOR_TABLE_END, 0x00}
};

/*
 * CTS testing will set the largest resolution mode only during testZoom.
 * Fast switchig function will break the testing and fail.
 * To workaround this, we need to create a 1600x1200 table
 */
static struct sensor_reg CTS_ZoomTest_mode_1600x1200[] = {
{0x098C, 0xA115},
{0x0990, 0x0002},
{0x098C, 0xA103},
{0x0990, 0x0002},
{SENSOR_WAIT_MS, 100},
{SENSOR_TABLE_END, 0x00}
};

static struct sensor_reg mode_1280x720[] = {
{0x001A, 0x0001},
{SENSOR_WAIT_MS, 10},
{0x001A, 0x0000},
{SENSOR_WAIT_MS, 50},
/* select output interface */
{0x001A, 0x0050},
{0x001A, 0x0058},
{0x0014, 0x21F9},
{0x0010, 0x0115},
{0x0012, 0x00F5},
{0x0014, 0x2545},
{0x0014, 0x2547},
{0x0014, 0x2447},
{SENSOR_WAIT_MS, 100},
{0x0014, 0x2047},
{0x0014, 0x2046},
{0x0018, 0x402D},
{SENSOR_WAIT_MS, 100},
{0x0018, 0x402c},
{SENSOR_WAIT_MS, 100},
{0x098C, 0x2703},
{0x0990, 0x0640},
{0x098C, 0x2705},
{0x0990, 0x04B0},
{0x098C, 0x2707},
{0x0990, 0x0640},
{0x098C, 0x2709},
{0x0990, 0x04B0},
{0x098C, 0x270D},
{0x0990, 0x0000},
{0x098C, 0x270F},
{0x0990, 0x0000},
{0x098C, 0x2711},
{0x0990, 0x04BB},
{0x098C, 0x2713},
{0x0990, 0x064B},
{0x098C, 0x2715},
{0x0990, 0x0111},
{0x098C, 0x2717},
{0x0990, 0x0024},
{0x098C, 0x2719},
{0x0990, 0x003A},
{0x098C, 0x271B},
{0x0990, 0x00F6},
{0x098C, 0x271D},
{0x0990, 0x008B},
{0x098C, 0x271F},
{0x0990, 0x0521},
{0x098C, 0x2721},
{0x0990, 0x08EC},
{SENSOR_WAIT_MS, 100},
{SENSOR_TABLE_END, 0x00}
};

static struct sensor_reg mode_800x600[] = {
{0x0014, 0xA0FB},
{SENSOR_WAIT_MS, 10},
{0x0014, 0xA0F9},
{SENSOR_WAIT_MS, 10},
{0x0014, 0x2545},
{0x0010, 0x0110},
{0x0012, 0x1FF7},
{0x0014, 0x2547},
{0x0014, 0x2447},
{SENSOR_WAIT_MS, 1},
{0x0014, 0x2047},
{0x0014, 0x2046},
{SENSOR_WAIT_MS, 1},
{0x0018, 0x4028},
{ 0x001E, 0x0700},
{SENSOR_WAIT_MS, 50},
/* errata for rev2 */
{0x3084, 0x240C},
{0x3092, 0x0A4C},
{0x3094, 0x4C4C},
{0x3096, 0x4C54},
/* LSC */
{0x364E, 0x03F0},
{0x3650, 0x2D85},
{0x3652, 0x138F},
{0x3654, 0xA80F},
{0x3656, 0x8D10},
{0x3658, 0x00F0},
{0x365A, 0x3C24},
{0x365C, 0x206F},
{0x365E, 0x9070},
{0x3660, 0x49D0},
{0x3662, 0x00D0},
{0x3664, 0xA0CB},
{0x3666, 0x766E},
{0x3668, 0xC1CF},
{0x366A, 0x404D},
{0x366C, 0x00D0},
{0x366E, 0x2769},
{0x3670, 0x732E},
{0x3672, 0xB08F},
{0x3674, 0xCC2F},
{0x3676, 0xE16A},
{0x3678, 0x8670},
{0x367A, 0xD62E},
{0x367C, 0x45D2},
{0x367E, 0x2251},
{0x3680, 0x090A},
{0x3682, 0x128F},
{0x3684, 0xD4AB},
{0x3686, 0xDCF1},
{0x3688, 0x5452},
{0x368A, 0xBC2B},
{0x368C, 0xF2CE},
{0x368E, 0x84EF},
{0x3690, 0x7B8F},
{0x3692, 0x5E72},
{0x3694, 0x9E0C},
{0x3696, 0x162E},
{0x3698, 0x61AE},
{0x369A, 0x1AAF},
{0x369C, 0x556D},
{0x369E, 0x4B72},
{0x36A0, 0xB0B0},
{0x36A2, 0x0EF3},
{0x36A4, 0x1775},
{0x36A6, 0xBC57},
{0x36A8, 0x4672},
{0x36AA, 0xF350},
{0x36AC, 0x6133},
{0x36AE, 0x5895},
{0x36B0, 0xEC97},
{0x36B2, 0x4872},
{0x36B4, 0xBAF0},
{0x36B6, 0xD5B1},
{0x36B8, 0x0AD5},
{0x36BA, 0xCCB6},
{0x36BC, 0x58F2},
{0x36BE, 0xC06F},
{0x36C0, 0x3732},
{0x36C2, 0x07D5},
{0x36C4, 0xB357},
{0x36C6, 0x22D0},
{0x36C8, 0x7C71},
{0x36CA, 0x1A73},
{0x36CC, 0xDD54},
{0x36CE, 0x9F96},
{0x36D0, 0x73CF},
{0x36D2, 0x2D2C},
{0x36D4, 0x12B4},
{0x36D6, 0x06B4},
{0x36D8, 0xCC97},
{0x36DA, 0x1330},
{0x36DC, 0xCE10},
{0x36DE, 0x3E72},
{0x36E0, 0x3CB5},
{0x36E2, 0x8BD7},
{0x36E4, 0x75D0},
{0x36E6, 0x1412},
{0x36E8, 0x8EAF},
{0x36EA, 0xE015},
{0x36EC, 0xDA73},
{0x36EE, 0x93F4},
{0x36F0, 0x5934},
{0x36F2, 0x9EF8},
{0x36F4, 0xE538},
{0x36F6, 0x1FDB},
{0x36F8, 0x8F33},
{0x36FA, 0x2D55},
{0x36FC, 0xAFF8},
{0x36FE, 0xA8B9},
{0x3700, 0x2E1B},
{0x3702, 0xDF93},
{0x3704, 0x1695},
{0x3706, 0xD217},
{0x3708, 0x8079},
{0x370A, 0x61FA},
{0x370C, 0xAC54},
{0x370E, 0x4B74},
{0x3710, 0x8C58},
{0x3712, 0xD818},
{0x3714, 0x16DB},
{0x3644, 0x02C8},
{0x3642, 0x0240},
{0x3210, 0x01B8},
/* low light setting */
{0x098C, 0x2B28},
{0x0990, 0x35E8},
{0x098C, 0x2B2A},
{0x0990, 0xB3B0},
{0x098C, 0xAB20},
{0x0990, 0x0042},
{0x098C, 0xAB24},
{0x0990, 0x0000},
{0x098C, 0xAB25},
{0x0990, 0x00FF},
{0x098C, 0xAB30},
{0x0990, 0x001E},
{0x098C, 0xAB31},
{0x0990, 0x001E},
{0x098C, 0xAB32},
{0x0990, 0x001E},
{0x098C, 0xAB33},
{0x0990, 0x001E},
{0x098C, 0xAB34},
{0x0990, 0x0080},
{0x098C, 0xAB35},
{0x0990, 0x00FF},
{0x098C, 0xAB36},
{0x0990, 0x0014},
{0x098C, 0xAB37},
{0x0990, 0x0003},
{0x098C, 0x2B38},
{0x0990, 0x1E14},
{0x098C, 0x2B3A},
{0x0990, 0x6590},
{0x098C, 0x2B62},
{0x0990, 0xFFFE},
{0x098C, 0x2B64},
{0x0990, 0xFFFF},
/* AWB and CCM */
{0x098C, 0x2306},
{0x0990, 0x01D6},
{0x098C, 0x2308},
{0x0990, 0xFF89},
{0x098C, 0x230A},
{0x0990, 0xFFA1},
{0x098C, 0x230C},
{0x0990, 0xFFB3},
{0x098C, 0x230E},
{0x0990, 0x01A3},
{0x098C, 0x2310},
{0x0990, 0xFFAA},
{0x098C, 0x2312},
{0x0990, 0xFFE3},
{0x098C, 0x2314},
{0x0990, 0xFF1E},
{0x098C, 0x2316},
{0x0990, 0x01FF},
{0x098C, 0x2318},
{0x0990, 0x0024},
{0x098C, 0x231A},
{0x0990, 0x0036},
{0x098C, 0x231C},
{0x0990, 0x000C},
{0x098C, 0x231E},
{0x0990, 0xFFDA},
{0x098C, 0x2320},
{0x0990, 0x001B},
{0x098C, 0x2322},
{0x0990, 0x000F},
{0x098C, 0x2324},
{0x0990, 0x001C},
{0x098C, 0x2326},
{0x0990, 0xFFD5},
{0x098C, 0x2328},
{0x0990, 0x0008},
{0x098C, 0x232A},
{0x0990, 0x006C},
{0x098C, 0x232C},
{0x0990, 0xFF8C},
{0x098C, 0x232E},
{0x0990, 0x0004},
{0x098C, 0x2330},
{0x0990, 0xFFEF},
{0x098C, 0xA348},
{0x0990, 0x0008},
{0x098C, 0xA349},
{0x0990, 0x0002},
{0x098C, 0xA363},
{0x0990, 0x00D2},
{0x098C, 0xA364},
{0x0990, 0x00EE},
{0x3244, 0x0328},
{0x323E, 0xC22C},
{0x098C, 0xA36B},
{0x0990, 0x0084},
{0x098C, 0xA366},
{0x0990, 0x006C},
{0x098C, 0xA367},
{0x0990, 0x0070},
{0x098C, 0xA368},
{0x0990, 0x0072},
{SENSOR_WAIT_MS, 10},
/* Patch */
{0x098C, 0x0415},
{0x0990, 0xF601},
{0x0992, 0x42C1},
{0x0994, 0x0326},
{0x0996, 0x11F6},
{0x0998, 0x0143},
{0x099A, 0xC104},
{0x099C, 0x260A},
{0x099E, 0xCC04},
{0x098C, 0x0425},
{0x0990, 0x33BD},
{0x0992, 0xA362},
{0x0994, 0xBD04},
{0x0996, 0x3339},
{0x0998, 0xC6FF},
{0x099A, 0xF701},
{0x099C, 0x6439},
{0x099E, 0xFE01},
{0x098C, 0x0435},
{0x0990, 0x6918},
{0x0992, 0xCE03},
{0x0994, 0x25CC},
{0x0996, 0x0013},
{0x0998, 0xBDC2},
{0x099A, 0xB8CC},
{0x099C, 0x0489},
{0x099E, 0xFD03},
{0x098C, 0x0445},
{0x0990, 0x27CC},
{0x0992, 0x0325},
{0x0994, 0xFD01},
{0x0996, 0x69FE},
{0x0998, 0x02BD},
{0x099A, 0x18CE},
{0x099C, 0x0339},
{0x099E, 0xCC00},
{0x098C, 0x0455},
{0x0990, 0x11BD},
{0x0992, 0xC2B8},
{0x0994, 0xCC04},
{0x0996, 0xC8FD},
{0x0998, 0x0347},
{0x099A, 0xCC03},
{0x099C, 0x39FD},
{0x099E, 0x02BD},
{0x098C, 0x0465},
{0x0990, 0xDE00},
{0x0992, 0x18CE},
{0x0994, 0x00C2},
{0x0996, 0xCC00},
{0x0998, 0x37BD},
{0x099A, 0xC2B8},
{0x099C, 0xCC04},
{0x099E, 0xEFDD},
{0x098C, 0x0475},
{0x0990, 0xE6CC},
{0x0992, 0x00C2},
{0x0994, 0xDD00},
{0x0996, 0xC601},
{0x0998, 0xF701},
{0x099A, 0x64C6},
{0x099C, 0x03F7},
{0x099E, 0x0165},
{0x098C, 0x0485},
{0x0990, 0x7F01},
{0x0992, 0x6639},
{0x0994, 0x3C3C},
{0x0996, 0x3C34},
{0x0998, 0xCC32},
{0x099A, 0x3EBD},
{0x099C, 0xA558},
{0x099E, 0x30ED},
{0x098C, 0x0495},
{0x0990, 0x04BD},
{0x0992, 0xB2D7},
{0x0994, 0x30E7},
{0x0996, 0x06CC},
{0x0998, 0x323E},
{0x099A, 0xED00},
{0x099C, 0xEC04},
{0x099E, 0xBDA5},
{0x098C, 0x04A5},
{0x0990, 0x44CC},
{0x0992, 0x3244},
{0x0994, 0xBDA5},
{0x0996, 0x585F},
{0x0998, 0x30ED},
{0x099A, 0x02CC},
{0x099C, 0x3244},
{0x099E, 0xED00},
{0x098C, 0x04B5},
{0x0990, 0xF601},
{0x0992, 0xD54F},
{0x0994, 0xEA03},
{0x0996, 0xAA02},
{0x0998, 0xBDA5},
{0x099A, 0x4430},
{0x099C, 0xE606},
{0x099E, 0x3838},
{0x098C, 0x04C5},
{0x0990, 0x3831},
{0x0992, 0x39BD},
{0x0994, 0xD661},
{0x0996, 0xF602},
{0x0998, 0xF4C1},
{0x099A, 0x0126},
{0x099C, 0x0BFE},
{0x099E, 0x02BD},
{0x098C, 0x04D5},
{0x0990, 0xEE10},
{0x0992, 0xFC02},
{0x0994, 0xF5AD},
{0x0996, 0x0039},
{0x0998, 0xF602},
{0x099A, 0xF4C1},
{0x099C, 0x0226},
{0x099E, 0x0AFE},
{0x098C, 0x04E5},
{0x0990, 0x02BD},
{0x0992, 0xEE10},
{0x0994, 0xFC02},
{0x0996, 0xF7AD},
{0x0998, 0x0039},
{0x099A, 0x3CBD},
{0x099C, 0xB059},
{0x099E, 0xCC00},
{0x098C, 0x04F5},
{0x0990, 0x28BD},
{0x0992, 0xA558},
{0x0994, 0x8300},
{0x0996, 0x0027},
{0x0998, 0x0BCC},
{0x099A, 0x0026},
{0x099C, 0x30ED},
{0x099E, 0x00C6},
{0x098C, 0x0505},
{0x0990, 0x03BD},
{0x0992, 0xA544},
{0x0994, 0x3839},
{0x098C, 0x2006},
{0x0990, 0x0415},
{0x098C, 0xA005},
{0x0990, 0x0001},
{SENSOR_WAIT_MS, 50},
/* POLL MON_PATCH_ID_0 => 0x01 */
{0x321C, 0x0003},
{0x98C, 0x2703},
{0x990, 0x0320},
{0x98C, 0x2705},
{0x990, 0x0258},
{0x98C, 0x2707},
{0x990, 0x0640},
{0x98C, 0x2709},
{0x990, 0x04B0},
{0x98C, 0x270D},
{0x990, 0x0000},
{0x98C, 0x270F},
{0x990, 0x0000},
{0x98C, 0x2711},
{0x990, 0x04BD},
{0x98C, 0x2713},
{0x990, 0x064D},
{0x98C, 0x2715},
{0x990, 0x0111},
{0x98C, 0x2717},
{0x990, 0x046C},
{0x98C, 0x2719},
{0x990, 0x005A},
{0x98C, 0x271B},
{0x990, 0x01BE},
{0x98C, 0x271D},
{0x990, 0x0131},
{0x98C, 0x271F},
{0x990, 0x02B3},
{0x98C, 0x2721},
{0x990, 0x07BC},
{0x98C, 0x2723},
{0x990, 0x0004},
{0x98C, 0x2725},
{0x990, 0x0004},
{0x98C, 0x2727},
{0x990, 0x04BB},
{0x98C, 0x2729},
{0x990, 0x064B},
{0x98C, 0x272B},
{0x990, 0x0111},
{0x98C, 0x272D},
{0x990, 0x0024},
{0x98C, 0x272F},
{0x990, 0x003A},
{0x98C, 0x2731},
{0x990, 0x00F6},
{0x98C, 0x2733},
{0x990, 0x008B},
{0x98C, 0x2735},
{0x990, 0x059A},
{0x98C, 0x2737},
{0x990, 0x07BC},
{0x98C, 0x2739},
{0x990, 0x0000},
{0x98C, 0x273B},
{0x990, 0x031F},
{0x98C, 0x273D},
{0x990, 0x0000},
{0x98C, 0x273F},
{0x990, 0x0257},
{0x98C, 0x2747},
{0x990, 0x0000},
{0x98C, 0x2749},
{0x990, 0x063F},
{0x98C, 0x274B},
{0x990, 0x0000},
{0x98C, 0x274D},
{0x990, 0x04AF},
{0x98C, 0x222D},
{0x990, 0x0065},
{0x98C, 0xA408},
{0x990, 0x0018},
{0x98C, 0xA409},
{0x990, 0x001A},
{0x98C, 0xA40A},
{0x990, 0x001D},
{0x98C, 0xA40B},
{0x990, 0x001F},
{0x98C, 0x2411},
{0x990, 0x0065},
{0x98C, 0x2413},
{0x990, 0x0079},
{0x98C, 0x2415},
{0x990, 0x0065},
{0x98C, 0x2417},
{0x990, 0x0079},
{0x98C, 0xA404},
{0x990, 0x0010},
{0x98C, 0xA40D},
{0x990, 0x0002},
{0x98C, 0xA40E},
{0x990, 0x0003},
{0x98C, 0xA410},
{0x990, 0x000A},
{0x098C, 0xA34A},
{0x0990, 0x0059},
{0x098C, 0xA34B},
{0x0990, 0x00C6},
{0x098C, 0xA34C},
{0x0990, 0x0059},
{0x098C, 0xA34D},
{0x0990, 0x00A6},
{0x098C, 0xA351},
{0x0990, 0x0000},
{0x098C, 0xA352},
{0x0990, 0x007F},
{0x098C, 0xAB3C},
{0x0990, 0x0000},
{0x098C, 0xAB3D},
{0x0990, 0x000A},
{0x098C, 0xAB3E},
{0x0990, 0x001D},
{0x098C, 0xAB3F},
{0x0990, 0x0037},
{0x098C, 0xAB40},
{0x0990, 0x0058},
{0x098C, 0xAB41},
{0x0990, 0x0071},
{0x098C, 0xAB42},
{0x0990, 0x0086},
{0x098C, 0xAB43},
{0x0990, 0x0098},
{0x098C, 0xAB44},
{0x0990, 0x00A7},
{0x098C, 0xAB45},
{0x0990, 0x00B5},
{0x098C, 0xAB46},
{0x0990, 0x00C0},
{0x098C, 0xAB47},
{0x0990, 0x00CB},
{0x098C, 0xAB48},
{0x0990, 0x00D4},
{0x098C, 0xAB49},
{0x0990, 0x00DD},
{0x098C, 0xAB4A},
{0x0990, 0x00E4},
{0x098C, 0xAB4B},
{0x0990, 0x00EC},
{0x098C, 0xAB4C},
{0x0990, 0x00F3},
{0x098C, 0xAB4D},
{0x0990, 0x00F9},
{0x098C, 0xAB4E},
{0x0990, 0x00FF},
{0x098C, 0xAB4F},
{0x0990, 0x0000},
{0x098C, 0xAB50},
{0x0990, 0x000A},
{0x098C, 0xAB51},
{0x0990, 0x001D},
{0x098C, 0xAB52},
{0x0990, 0x0037},
{0x098C, 0xAB53},
{0x0990, 0x0058},
{0x098C, 0xAB54},
{0x0990, 0x0071},
{0x098C, 0xAB55},
{0x0990, 0x0086},
{0x098C, 0xAB56},
{0x0990, 0x0098},
{0x098C, 0xAB57},
{0x0990, 0x00A7},
{0x098C, 0xAB58},
{0x0990, 0x00B5},
{0x098C, 0xAB59},
{0x0990, 0x00C0},
{0x098C, 0xAB5A},
{0x0990, 0x00CB},
{0x098C, 0xAB5B},
{0x0990, 0x00D4},
{0x098C, 0xAB5C},
{0x0990, 0x00DD},
{0x098C, 0xAB5D},
{0x0990, 0x00E4},
{0x098C, 0xAB5E},
{0x0990, 0x00EC},
{0x098C, 0xAB5F},
{0x0990, 0x00F3},
{0x098C, 0xAB60},
{0x0990, 0x00F9},
{0x098C, 0xAB61},
{0x0990, 0x00FF},
{0x098C, 0xA369},
{0x0990, 0x0078},
{0x098C, 0xA36B},
{0x0990, 0x0084},
{0x098C, 0xA366},
{0x0990, 0x007a},
{0x098C, 0x275F},
{0x0990, 0x0195},
/* Set normal mode, Frame rate >=15fps */
{0x098C, 0xA20C},
{0x0990, 0x000B},
{0x098C, 0xA215},
{0x0990, 0x000A},
{0x098C, 0xAB22},
{0x0990, 0x0005},
{0x098C, 0xA24F},
{0x0990, 0x0050},
/* Denoise */
{0x098C, 0x2212},
{0x0990, 0x0080},
{SENSOR_WAIT_MS, 100},
{ 0x098C, 0xA103},
{ 0x0990, 0x0006},
{SENSOR_WAIT_MS, 200},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg mode_640x480[] = {
{0x0014, 0xA0FB},
{SENSOR_WAIT_MS, 20},
{0x0014, 0xA0F9},
{SENSOR_WAIT_MS, 20},
{0x0014, 0x2545},
{0x0010, 0x0110},
{0x0012, 0x1FF7},
{0x0014, 0x2547},
{0x0014, 0x2447},
{SENSOR_WAIT_MS,10},
{0x0014, 0x2047},
{0x0014, 0x2046},
{SENSOR_WAIT_MS, 10},
{0x0018, 0x4028},
{ 0x001E, 0x0700},
{SENSOR_WAIT_MS, 50},
/* errata for rev2 */
{0x3084, 0x240C},
{0x3092, 0x0A4C},
{0x3094, 0x4C4C},
{0x3096, 0x4C54},
/* LSC */
{0x364E, 0x03F0},
{0x3650, 0x2D85},
{0x3652, 0x138F},
{0x3654, 0xA80F},
{0x3656, 0x8D10},
{0x3658, 0x00F0},
{0x365A, 0x3C24},
{0x365C, 0x206F},
{0x365E, 0x9070},
{0x3660, 0x49D0},
{0x3662, 0x00D0},
{0x3664, 0xA0CB},
{0x3666, 0x766E},
{0x3668, 0xC1CF},
{0x366A, 0x404D},
{0x366C, 0x00D0},
{0x366E, 0x2769},
{0x3670, 0x732E},
{0x3672, 0xB08F},
{0x3674, 0xCC2F},
{0x3676, 0xE16A},
{0x3678, 0x8670},
{0x367A, 0xD62E},
{0x367C, 0x45D2},
{0x367E, 0x2251},
{0x3680, 0x090A},
{0x3682, 0x128F},
{0x3684, 0xD4AB},
{0x3686, 0xDCF1},
{0x3688, 0x5452},
{0x368A, 0xBC2B},
{0x368C, 0xF2CE},
{0x368E, 0x84EF},
{0x3690, 0x7B8F},
{0x3692, 0x5E72},
{0x3694, 0x9E0C},
{0x3696, 0x162E},
{0x3698, 0x61AE},
{0x369A, 0x1AAF},
{0x369C, 0x556D},
{0x369E, 0x4B72},
{0x36A0, 0xB0B0},
{0x36A2, 0x0EF3},
{0x36A4, 0x1775},
{0x36A6, 0xBC57},
{0x36A8, 0x4672},
{0x36AA, 0xF350},
{0x36AC, 0x6133},
{0x36AE, 0x5895},
{0x36B0, 0xEC97},
{0x36B2, 0x4872},
{0x36B4, 0xBAF0},
{0x36B6, 0xD5B1},
{0x36B8, 0x0AD5},
{0x36BA, 0xCCB6},
{0x36BC, 0x58F2},
{0x36BE, 0xC06F},
{0x36C0, 0x3732},
{0x36C2, 0x07D5},
{0x36C4, 0xB357},
{0x36C6, 0x22D0},
{0x36C8, 0x7C71},
{0x36CA, 0x1A73},
{0x36CC, 0xDD54},
{0x36CE, 0x9F96},
{0x36D0, 0x73CF},
{0x36D2, 0x2D2C},
{0x36D4, 0x12B4},
{0x36D6, 0x06B4},
{0x36D8, 0xCC97},
{0x36DA, 0x1330},
{0x36DC, 0xCE10},
{0x36DE, 0x3E72},
{0x36E0, 0x3CB5},
{0x36E2, 0x8BD7},
{0x36E4, 0x75D0},
{0x36E6, 0x1412},
{0x36E8, 0x8EAF},
{0x36EA, 0xE015},
{0x36EC, 0xDA73},
{0x36EE, 0x93F4},
{0x36F0, 0x5934},
{0x36F2, 0x9EF8},
{0x36F4, 0xE538},
{0x36F6, 0x1FDB},
{0x36F8, 0x8F33},
{0x36FA, 0x2D55},
{0x36FC, 0xAFF8},
{0x36FE, 0xA8B9},
{0x3700, 0x2E1B},
{0x3702, 0xDF93},
{0x3704, 0x1695},
{0x3706, 0xD217},
{0x3708, 0x8079},
{0x370A, 0x61FA},
{0x370C, 0xAC54},
{0x370E, 0x4B74},
{0x3710, 0x8C58},
{0x3712, 0xD818},
{0x3714, 0x16DB},
{0x3644, 0x02C8},
{0x3642, 0x0240},
{0x3210, 0x01B8},
/* low light setting */
{0x098C, 0x2B28},
{0x0990, 0x35E8},
{0x098C, 0x2B2A},
{0x0990, 0xB3B0},
{0x098C, 0xAB20},
{0x0990, 0x0042},
{0x098C, 0xAB24},
{0x0990, 0x0000},
{0x098C, 0xAB25},
{0x0990, 0x00FF},
{0x098C, 0xAB30},
{0x0990, 0x001E},
{0x098C, 0xAB31},
{0x0990, 0x001E},
{0x098C, 0xAB32},
{0x0990, 0x001E},
{0x098C, 0xAB33},
{0x0990, 0x001E},
{0x098C, 0xAB34},
{0x0990, 0x0080},
{0x098C, 0xAB35},
{0x0990, 0x00FF},
{0x098C, 0xAB36},
{0x0990, 0x0014},
{0x098C, 0xAB37},
{0x0990, 0x0003},
{0x098C, 0x2B38},
{0x0990, 0x1E14},
{0x098C, 0x2B3A},
{0x0990, 0x6590},
{0x098C, 0x2B62},
{0x0990, 0xFFFE},
{0x098C, 0x2B64},
{0x0990, 0xFFFF},
/* AWB and CCM */
{0x098C, 0x2306},
{0x0990, 0x01D6},
{0x098C, 0x2308},
{0x0990, 0xFF89},
{0x098C, 0x230A},
{0x0990, 0xFFA1},
{0x098C, 0x230C},
{0x0990, 0xFFB3},
{0x098C, 0x230E},
{0x0990, 0x01A3},
{0x098C, 0x2310},
{0x0990, 0xFFAA},
{0x098C, 0x2312},
{0x0990, 0xFFE3},
{0x098C, 0x2314},
{0x0990, 0xFF1E},
{0x098C, 0x2316},
{0x0990, 0x01FF},
{0x098C, 0x2318},
{0x0990, 0x0024},
{0x098C, 0x231A},
{0x0990, 0x0036},
{0x098C, 0x231C},
{0x0990, 0x000C},
{0x098C, 0x231E},
{0x0990, 0xFFDA},
{0x098C, 0x2320},
{0x0990, 0x001B},
{0x098C, 0x2322},
{0x0990, 0x000F},
{0x098C, 0x2324},
{0x0990, 0x001C},
{0x098C, 0x2326},
{0x0990, 0xFFD5},
{0x098C, 0x2328},
{0x0990, 0x0008},
{0x098C, 0x232A},
{0x0990, 0x006C},
{0x098C, 0x232C},
{0x0990, 0xFF8C},
{0x098C, 0x232E},
{0x0990, 0x0004},
{0x098C, 0x2330},
{0x0990, 0xFFEF},
{0x098C, 0xA348},
{0x0990, 0x0008},
{0x098C, 0xA349},
{0x0990, 0x0002},
{0x098C, 0xA363},
{0x0990, 0x00D2},
{0x098C, 0xA364},
{0x0990, 0x00EE},
{0x3244, 0x0328},
{0x323E, 0xC22C},
{0x098C, 0xA36B},
{0x0990, 0x0084},
{0x098C, 0xA366},
{0x0990, 0x006C},
{0x098C, 0xA367},
{0x0990, 0x0070},
{0x098C, 0xA368},
{0x0990, 0x0072},
{SENSOR_WAIT_MS, 10},
/* Patch */
{0x098C, 0x0415},
{0x0990, 0xF601},
{0x0992, 0x42C1},
{0x0994, 0x0326},
{0x0996, 0x11F6},
{0x0998, 0x0143},
{0x099A, 0xC104},
{0x099C, 0x260A},
{0x099E, 0xCC04},
{0x098C, 0x0425},
{0x0990, 0x33BD},
{0x0992, 0xA362},
{0x0994, 0xBD04},
{0x0996, 0x3339},
{0x0998, 0xC6FF},
{0x099A, 0xF701},
{0x099C, 0x6439},
{0x099E, 0xFE01},
{0x098C, 0x0435},
{0x0990, 0x6918},
{0x0992, 0xCE03},
{0x0994, 0x25CC},
{0x0996, 0x0013},
{0x0998, 0xBDC2},
{0x099A, 0xB8CC},
{0x099C, 0x0489},
{0x099E, 0xFD03},
{0x098C, 0x0445},
{0x0990, 0x27CC},
{0x0992, 0x0325},
{0x0994, 0xFD01},
{0x0996, 0x69FE},
{0x0998, 0x02BD},
{0x099A, 0x18CE},
{0x099C, 0x0339},
{0x099E, 0xCC00},
{0x098C, 0x0455},
{0x0990, 0x11BD},
{0x0992, 0xC2B8},
{0x0994, 0xCC04},
{0x0996, 0xC8FD},
{0x0998, 0x0347},
{0x099A, 0xCC03},
{0x099C, 0x39FD},
{0x099E, 0x02BD},
{0x098C, 0x0465},
{0x0990, 0xDE00},
{0x0992, 0x18CE},
{0x0994, 0x00C2},
{0x0996, 0xCC00},
{0x0998, 0x37BD},
{0x099A, 0xC2B8},
{0x099C, 0xCC04},
{0x099E, 0xEFDD},
{0x098C, 0x0475},
{0x0990, 0xE6CC},
{0x0992, 0x00C2},
{0x0994, 0xDD00},
{0x0996, 0xC601},
{0x0998, 0xF701},
{0x099A, 0x64C6},
{0x099C, 0x03F7},
{0x099E, 0x0165},
{0x098C, 0x0485},
{0x0990, 0x7F01},
{0x0992, 0x6639},
{0x0994, 0x3C3C},
{0x0996, 0x3C34},
{0x0998, 0xCC32},
{0x099A, 0x3EBD},
{0x099C, 0xA558},
{0x099E, 0x30ED},
{0x098C, 0x0495},
{0x0990, 0x04BD},
{0x0992, 0xB2D7},
{0x0994, 0x30E7},
{0x0996, 0x06CC},
{0x0998, 0x323E},
{0x099A, 0xED00},
{0x099C, 0xEC04},
{0x099E, 0xBDA5},
{0x098C, 0x04A5},
{0x0990, 0x44CC},
{0x0992, 0x3244},
{0x0994, 0xBDA5},
{0x0996, 0x585F},
{0x0998, 0x30ED},
{0x099A, 0x02CC},
{0x099C, 0x3244},
{0x099E, 0xED00},
{0x098C, 0x04B5},
{0x0990, 0xF601},
{0x0992, 0xD54F},
{0x0994, 0xEA03},
{0x0996, 0xAA02},
{0x0998, 0xBDA5},
{0x099A, 0x4430},
{0x099C, 0xE606},
{0x099E, 0x3838},
{0x098C, 0x04C5},
{0x0990, 0x3831},
{0x0992, 0x39BD},
{0x0994, 0xD661},
{0x0996, 0xF602},
{0x0998, 0xF4C1},
{0x099A, 0x0126},
{0x099C, 0x0BFE},
{0x099E, 0x02BD},
{0x098C, 0x04D5},
{0x0990, 0xEE10},
{0x0992, 0xFC02},
{0x0994, 0xF5AD},
{0x0996, 0x0039},
{0x0998, 0xF602},
{0x099A, 0xF4C1},
{0x099C, 0x0226},
{0x099E, 0x0AFE},
{0x098C, 0x04E5},
{0x0990, 0x02BD},
{0x0992, 0xEE10},
{0x0994, 0xFC02},
{0x0996, 0xF7AD},
{0x0998, 0x0039},
{0x099A, 0x3CBD},
{0x099C, 0xB059},
{0x099E, 0xCC00},
{0x098C, 0x04F5},
{0x0990, 0x28BD},
{0x0992, 0xA558},
{0x0994, 0x8300},
{0x0996, 0x0027},
{0x0998, 0x0BCC},
{0x099A, 0x0026},
{0x099C, 0x30ED},
{0x099E, 0x00C6},
{0x098C, 0x0505},
{0x0990, 0x03BD},
{0x0992, 0xA544},
{0x0994, 0x3839},
{0x098C, 0x2006},
{0x0990, 0x0415},
{0x098C, 0xA005},
{0x0990, 0x0001},
{SENSOR_WAIT_MS, 50},
/* POLL MON_PATCH_ID_0 => 0x01 */
{0x321C, 0x0003},
{0x98C, 0x2703},
{0x990, 0x0280},
{0x98C, 0x2705},
{0x990, 0x01E0},
{0x98C, 0x2707},
{0x990, 0x0640},
{0x98C, 0x2709},
{0x990, 0x04B0},
{0x98C, 0x270D},
{0x990, 0x0000},
{0x98C, 0x270F},
{0x990, 0x0000},
{0x98C, 0x2711},
{0x990, 0x04BD},
{0x98C, 0x2713},
{0x990, 0x064D},
{0x98C, 0x2715},
{0x990, 0x0111},
{0x98C, 0x2717},
{0x990, 0x046C},
{0x98C, 0x2719},
{0x990, 0x005A},
{0x98C, 0x271B},
{0x990, 0x01BE},
{0x98C, 0x271D},
{0x990, 0x0131},
{0x98C, 0x271F},
{0x990, 0x02B3},
{0x98C, 0x2721},
{0x990, 0x07BC},
{0x98C, 0x2723},
{0x990, 0x0004},
{0x98C, 0x2725},
{0x990, 0x0004},
{0x98C, 0x2727},
{0x990, 0x04BB},
{0x98C, 0x2729},
{0x990, 0x064B},
{0x98C, 0x272B},
{0x990, 0x0111},
{0x98C, 0x272D},
{0x990, 0x0024},
{0x98C, 0x272F},
{0x990, 0x003A},
{0x98C, 0x2731},
{0x990, 0x00F6},
{0x98C, 0x2733},
{0x990, 0x008B},
{0x98C, 0x2735},
{0x990, 0x059A},
{0x98C, 0x2737},
{0x990, 0x07BC},
{0x98C, 0x2739},
{0x990, 0x0000},
{0x98C, 0x273B},
{0x990, 0x031F},
{0x98C, 0x273D},
{0x990, 0x0000},
{0x98C, 0x273F},
{0x990, 0x0257},
{0x98C, 0x2747},
{0x990, 0x0000},
{0x98C, 0x2749},
{0x990, 0x063F},
{0x98C, 0x274B},
{0x990, 0x0000},
{0x98C, 0x274D},
{0x990, 0x04AF},
{0x98C, 0x222D},
{0x990, 0x0065},
{0x98C, 0xA408},
{0x990, 0x0018},
{0x98C, 0xA409},
{0x990, 0x001A},
{0x98C, 0xA40A},
{0x990, 0x001D},
{0x98C, 0xA40B},
{0x990, 0x001F},
{0x98C, 0x2411},
{0x990, 0x0065},
{0x98C, 0x2413},
{0x990, 0x0079},
{0x98C, 0x2415},
{0x990, 0x0065},
{0x98C, 0x2417},
{0x990, 0x0079},
{0x98C, 0xA404},
{0x990, 0x0010},
{0x98C, 0xA40D},
{0x990, 0x0002},
{0x98C, 0xA40E},
{0x990, 0x0003},
{0x98C, 0xA410},
{0x990, 0x000A},
{0x098C, 0xA34A},
{0x0990, 0x0059},
{0x098C, 0xA34B},
{0x0990, 0x00C6},
{0x098C, 0xA34C},
{0x0990, 0x0059},
{0x098C, 0xA34D},
{0x0990, 0x00A6},
{0x098C, 0xA351},
{0x0990, 0x0000},
{0x098C, 0xA352},
{0x0990, 0x007F},
{0x098C, 0xAB3C},
{0x0990, 0x0000},
{0x098C, 0xAB3D},
{0x0990, 0x000A},
{0x098C, 0xAB3E},
{0x0990, 0x001D},
{0x098C, 0xAB3F},
{0x0990, 0x0037},
{0x098C, 0xAB40},
{0x0990, 0x0058},
{0x098C, 0xAB41},
{0x0990, 0x0071},
{0x098C, 0xAB42},
{0x0990, 0x0086},
{0x098C, 0xAB43},
{0x0990, 0x0098},
{0x098C, 0xAB44},
{0x0990, 0x00A7},
{0x098C, 0xAB45},
{0x0990, 0x00B5},
{0x098C, 0xAB46},
{0x0990, 0x00C0},
{0x098C, 0xAB47},
{0x0990, 0x00CB},
{0x098C, 0xAB48},
{0x0990, 0x00D4},
{0x098C, 0xAB49},
{0x0990, 0x00DD},
{0x098C, 0xAB4A},
{0x0990, 0x00E4},
{0x098C, 0xAB4B},
{0x0990, 0x00EC},
{0x098C, 0xAB4C},
{0x0990, 0x00F3},
{0x098C, 0xAB4D},
{0x0990, 0x00F9},
{0x098C, 0xAB4E},
{0x0990, 0x00FF},
{0x098C, 0xAB4F},
{0x0990, 0x0000},
{0x098C, 0xAB50},
{0x0990, 0x000A},
{0x098C, 0xAB51},
{0x0990, 0x001D},
{0x098C, 0xAB52},
{0x0990, 0x0037},
{0x098C, 0xAB53},
{0x0990, 0x0058},
{0x098C, 0xAB54},
{0x0990, 0x0071},
{0x098C, 0xAB55},
{0x0990, 0x0086},
{0x098C, 0xAB56},
{0x0990, 0x0098},
{0x098C, 0xAB57},
{0x0990, 0x00A7},
{0x098C, 0xAB58},
{0x0990, 0x00B5},
{0x098C, 0xAB59},
{0x0990, 0x00C0},
{0x098C, 0xAB5A},
{0x0990, 0x00CB},
{0x098C, 0xAB5B},
{0x0990, 0x00D4},
{0x098C, 0xAB5C},
{0x0990, 0x00DD},
{0x098C, 0xAB5D},
{0x0990, 0x00E4},
{0x098C, 0xAB5E},
{0x0990, 0x00EC},
{0x098C, 0xAB5F},
{0x0990, 0x00F3},
{0x098C, 0xAB60},
{0x0990, 0x00F9},
{0x098C, 0xAB61},
{0x0990, 0x00FF},
{0x098C, 0xA369},
{0x0990, 0x0078},
{0x098C, 0xA36B},
{0x0990, 0x0084},
{0x098C, 0xA366},
{0x0990, 0x007a},
{0x098C, 0x275F},
{0x0990, 0x0195},
/* Set normal mode, Frame rate >=15fps */
{0x098C, 0xA20C},
{0x0990, 0x000B},
{0x098C, 0xA215},
{0x0990, 0x000A},
{0x098C, 0xAB22},
{0x0990, 0x0005},
{0x098C, 0xA24F},
{0x0990, 0x0050},
/* Denoise */
{0x098C, 0x2212},
{0x0990, 0x0080},
{SENSOR_WAIT_MS, 100},
{ 0x098C, 0xA103},
{ 0x0990, 0x0006},
{SENSOR_WAIT_MS, 200},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_WAIT_MS, 200},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_None[] = {
{0x098C, 0x2759},
{0x0990, 0x6440},
{0x098C, 0x275B},
{0x0990, 0x6440},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Mono[] = {
{0x098C, 0x2759},
{0x0990, 0x6441},
{0x098C, 0x275B},
{0x0990, 0x6441},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Sepia[] = {
{0x098C, 0x2763},
{0x0990, 0xb023},
{0x098C, 0x2759},
{0x0990, 0x6442},
{0x098C, 0x275B},
{0x0990, 0x6442},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Negative[] = {
{0x098C, 0x2759},
{0x0990, 0x6443},
{0x098C, 0x275B},
{0x0990, 0x6443},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg ColorEffect_Solarize[] = {
{0x098C, 0x2759},
{0x0990, 0x6444},
{0x098C, 0x275B},
{0x0990, 0x6444},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

/* YUV Sensor ISP Not Support this function */
static struct sensor_reg ColorEffect_Posterize[] = {
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Auto[] = {
{0x098C, 0xA118},
{0x0990, 0x0001},
{0x098C, 0xA11e},
{0x0990, 0x0001},
{0x098C, 0xA124},
{0x0990, 0x0000},
{0x098C, 0xA12a},
{0x0990, 0x0001},
{0x098C, 0xA103},
{0x0990, 0x0005},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Incandescent[] = {
{0x098C, 0xA115},
{0x0990, 0x0000},
{0x098C, 0xA11F},
{0x0990, 0x0000},
{0x098C, 0xA103},
{0x0990, 0x0005},
{0x098C, 0xA353},
{0x0990, 0x002B},
{0x098C, 0xA34E},
{0x0990, 0x007B},
{0x098C, 0xA34F},
{0x0990, 0x0080},
{0x098C, 0xA350},
{0x0990, 0x007E},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Daylight[] = {
{0x098C, 0xA115},
{0x0990, 0x0000},
{0x098C, 0xA11F},
{0x0990, 0x0000},
{0x098C, 0xA103},
{0x0990, 0x0005},
{0x098C, 0xA353},
{0x0990, 0x007F},
{0x098C, 0xA34E},
{0x0990, 0x008E},
{0x098C, 0xA34F},
{0x0990, 0x0080},
{0x098C, 0xA350},
{0x0990, 0x0074},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Whitebalance_Fluorescent[] = {
{0x098C, 0xA115},
{0x0990, 0x0000},
{0x098C, 0xA11F},
{0x0990, 0x0000},
{0x098C, 0xA103},
{0x0990, 0x0005},
{0x098C, 0xA353},
{0x0990, 0x0036},
{0x098C, 0xA34E},
{0x0990, 0x0099},
{0x098C, 0xA34F},
{0x0990, 0x0080},
{0x098C, 0xA350},
{0x0990, 0x007D},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Exposure_2[] = {
{0x098C, 0xA24F},
{0x0990, 0x004f},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Exposure_1[] = {
{0x098C, 0xA24F},
{0x0990, 0x003e},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Exposure_0[] = {
{0x098C, 0xA24F},
{0x0990, 0x0032},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Exposure_Negative_1[] = {
{0x098C, 0xA24F},
{0x0990, 0x002a},
{SENSOR_TABLE_END, 0x0000}
};

static struct sensor_reg Exposure_Negative_2[] = {
{0x098C, 0xA24F},
{0x0990, 0x0022},
{SENSOR_TABLE_END, 0x0000}
};

enum {
	SENSOR_MODE_1600x1200,
	SENSOR_MODE_1280x720,
	SENSOR_MODE_800x600,
	SENSOR_MODE_640x480,
};

static struct sensor_reg *mode_table[] = {
	[SENSOR_MODE_1600x1200] = mode_1600x1200,
	[SENSOR_MODE_1280x720] = mode_1280x720,
	[SENSOR_MODE_800x600]  = mode_800x600,
	[SENSOR_MODE_640x480]   = mode_640x480,
};

static int sensor_read_reg(struct i2c_client *client, u16 addr, u16 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[4];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr >> 8);;
	data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 2;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

	/* swap high and low byte to match table format */
	swap(*(data+2),*(data+3));
	memcpy(val, data+2, 2);

	return 0;
}

static int sensor_write_reg(struct i2c_client *client, u16 addr, u16 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[4];
	int retry = 0;

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	data[2] = (u8) (val >> 8);
	data[3] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 4;
	msg.buf = data;

	do {
		err = i2c_transfer(client->adapter, &msg, 1);
		if (err == 1)
			return 0;
		retry++;
		pr_err("yuv_sensor : i2c transfer failed, retrying %x %x\n",
								addr, val);
		msleep(8);
	} while (retry <= SENSOR_MAX_RETRIES);

	return err;
}

static int sensor_write_table(struct i2c_client *client,
			const struct sensor_reg table[])
{
	int err;
	const struct sensor_reg *next;
	u16 val;

	for (next = table; next->addr != SENSOR_TABLE_END; next++) {
		if (next->addr == SENSOR_WAIT_MS) {
			msleep(next->val);
			continue;
		}

		val = next->val;
		err = sensor_write_reg(client, next->addr, val);
		if (err)
			return err;
	}

	return 0;
}

static int sensor_set_mode(struct sensor_info *info, struct mt9d113_mode *mode)
{
	int sensor_table;
	int err;
	u16 val;

	if (mode->xres == 1600 && mode->yres == 1200)
		sensor_table = SENSOR_MODE_1600x1200;
	else if (mode->xres == 1280 && mode->yres == 720)
		sensor_table = SENSOR_MODE_1280x720;
	else if (mode->xres == 800 && mode->yres == 600)
		sensor_table = SENSOR_MODE_800x600;
	else if (mode->xres == 640 && mode->yres == 480)
		sensor_table = SENSOR_MODE_640x480;
	else {
		pr_err("%s: invalid resolution supplied to set mode %d %d\n",
					__func__, mode->xres, mode->yres);
		sensor_table = SENSOR_MODE_640x480;
	}
	if (sensor_table != SENSOR_MODE_1600x1200) {
		if (info->pdata && info->pdata->reset)
			info->pdata->reset();
	}

	err = sensor_read_reg(info->i2c_client, 0x0000, &val);
	pr_info("%s: read out reg 0x0000,value is %x\n", __func__, val);
	err = sensor_write_table(info->i2c_client, mode_table[sensor_table]);
	if (err) {
		pr_err("sensor_write_table sensor_table is %d,err is %d\n",
							sensor_table,err);
		if (info->pdata && info->pdata->power_off)
			info->pdata->power_off();
		mdelay(20);
		info->mode = -1;
		return err;
	}
	/*
	 * if no table is programming before and request set to 1600x1200, then
	 * we must use 1600x1200 table to fix CTS testing issue
	 */
	if(!(val == SENSOR_640_WIDTH_VAL || val == SENSOR_720_WIDTH_VAL ||
					val == SENSOR_1600_WIDTH_VAL)
			&& sensor_table == SENSOR_MODE_1600x1200) {
		err = sensor_write_table(info->i2c_client,
				CTS_ZoomTest_mode_1600x1200);
		info->mode = sensor_table;
		return 0;
	}
	/*
	 * Check already program the sensor mode, Aptina support Context
	 * B fast switching capture mode back to preview mode we don't
	 * need to re-program the sensor mode for 640x480 table
	 */
	if(!(val == SENSOR_640_WIDTH_VAL &&
		sensor_table == SENSOR_MODE_640x480)) {
		err = sensor_write_table(info->i2c_client,
			mode_table[sensor_table]);
		if (err)
			return err;
	}

	info->mode = sensor_table;
	return 0;
}

static long sensor_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct sensor_info *info = file->private_data;
	int err=0;

	switch (cmd) {
	case MT9D113_IOCTL_SET_MODE:
	{
		struct mt9d113_mode mode;
		if (copy_from_user(&mode, (const void __user *)arg,
				sizeof(struct mt9d113_mode))) {
			return -EFAULT;
		}

		return sensor_set_mode(info, &mode);
	}
	case MT9D113_IOCTL_GET_STATUS:
		return 0;

	case MT9D113_IOCTL_SET_COLOR_EFFECT:
	{
		u8 coloreffect;

		if (copy_from_user(&coloreffect, (const void __user *)arg,
			sizeof(coloreffect))) {
			return -EFAULT;
		}
		switch(coloreffect) {
		case YUV_ColorEffect_None:
			err = sensor_write_table(info->i2c_client,
						ColorEffect_None);
			break;
		case YUV_ColorEffect_Mono:
			err = sensor_write_table(info->i2c_client,
						ColorEffect_Mono);
			break;
		case YUV_ColorEffect_Sepia:
			err = sensor_write_table(info->i2c_client,
						ColorEffect_Sepia);
			break;
		case YUV_ColorEffect_Negative:
			err = sensor_write_table(info->i2c_client,
						ColorEffect_Negative);
			break;
		case YUV_ColorEffect_Solarize:
			err = sensor_write_table(info->i2c_client,
						ColorEffect_Solarize);
			break;
		case YUV_ColorEffect_Posterize:
			err = sensor_write_table(info->i2c_client,
						ColorEffect_Posterize);
			break;
		default:
			break;
		}

		if (err)
			return err;

		return 0;
	}

	case MT9D113_IOCTL_SET_WHITE_BALANCE:
	{
		u8 whitebalance;

		if (copy_from_user(&whitebalance, (const void __user *)arg,
					sizeof(whitebalance))) {
			return -EFAULT;
		}

		switch(whitebalance) {
		case YUV_Whitebalance_Auto:
			err = sensor_write_table(info->i2c_client,
						Whitebalance_Auto);
			break;
		case YUV_Whitebalance_Incandescent:
			err = sensor_write_table(info->i2c_client,
					Whitebalance_Incandescent);
			break;
		case YUV_Whitebalance_Daylight:
			err = sensor_write_table(info->i2c_client,
					Whitebalance_Daylight);
			break;
		case YUV_Whitebalance_Fluorescent:
			err = sensor_write_table(info->i2c_client,
					Whitebalance_Fluorescent);
			break;
		default:
			break;
		}

		if (err)
			return err;

		return 0;
	}

	case MT9D113_IOCTL_SET_SCENE_MODE:
		return 0;

	case MT9D113_IOCTL_SET_EXPOSURE:
	{
		u8 exposure;

		if (copy_from_user(&exposure, (const void __user *)arg,
			sizeof(exposure))) {
			return -EFAULT;
		}
		switch(exposure) {
		case YUV_Exposure_Number0:
			err = sensor_write_table(info->i2c_client, Exposure_0);
			break;
		case YUV_Exposure_Positive1:
			err = sensor_write_table(info->i2c_client, Exposure_1);
			break;
		case YUV_Exposure_Positive2:
			err = sensor_write_table(info->i2c_client, Exposure_2);
			break;
		case YUV_Exposure_Negative1:
			err = sensor_write_table(info->i2c_client,
						Exposure_Negative_1);
			break;
		case YUV_Exposure_Negative2:
			err = sensor_write_table(info->i2c_client,
						Exposure_Negative_2);
			break;
		default:
			break;
		}

		if (err)
			return err;

		return 0;
	}
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_open(struct inode *inode, struct file *file)
{
	file->private_data = info;
	if (info->pdata && info->pdata->power_on)
		info->pdata->power_on();
	return 0;
}

static int sensor_release(struct inode *inode, struct file *file)
{
	if (info->mode == -1)
		return 0;
	if (info->pdata && info->pdata->power_off)
		info->pdata->power_off();
	info->mode = -1;
	file->private_data = NULL;
	return 0;
}


static const struct file_operations sensor_fileops = {
	.owner = THIS_MODULE,
	.open = sensor_open,
	.unlocked_ioctl = sensor_ioctl,
	.release = sensor_release,
};

static struct miscdevice sensor_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = SENSOR_NAME,
	.fops = &sensor_fileops,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int err;

	pr_info("mt9d113 %s\n", __func__);
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (!info) {
		pr_err("mt9d113 %s: Unable to allocate memory!\n", __func__);
		return -ENOMEM;
	}

	err = misc_register(&sensor_device);
	if (err) {
		pr_err("mt9d113 %s: Unable to register misc device!\n",
							__func__);
		kfree(info);
		return err;
	}

	info->pdata = client->dev.platform_data;
	info->i2c_client = client;

	i2c_set_clientdata(client, info);
	info->mode = -1;
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct sensor_info *info;

	info = i2c_get_clientdata(client);
	misc_deregister(&sensor_device);
	kfree(info);
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_i2c_driver = {
	.driver = {
		.name = SENSOR_NAME,
		.owner = THIS_MODULE,
	},
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};

static int __init mt9d113_sensor_init(void)
{
	pr_info("mt9d113 sensor init!\n");
	return i2c_add_driver(&sensor_i2c_driver);
}

static void __exit mt9d113_sensor_exit(void)
{
	pr_info("mt9d113 sensor exit!\n");
	i2c_del_driver(&sensor_i2c_driver);
}

module_init(mt9d113_sensor_init);
module_exit(mt9d113_sensor_exit);

