#ifndef ___S5K6AA_SENSOR_H__
#define ___S5K6AA_SENSOR_H__

#include <linux/ioctl.h>  /* For IOCTL macros */

#define S5K6AA_SetPage 0xFCFC
#define S5K6AA_P_APB 0xD000 /* Hardware registers */
#define S5K6AA_P_ROM 0x7000 /* Host-SW registers */

#define S5K6AA_R_FWdate 0x0134
#define S5K6AA_R_FWapiVer 0x0136
#define S5K6AA_R_FWrevision 0x0138
#define S5K6AA_R_FWpid 0x013A
#define S5K6AA_R_FWprjName 0x013C
#define S5K6AA_R_FW_compDate 0x0148
#define S5K6AA_R_FWSFC_VER 0x0154
#define S5K6AA_R_FWTC_VER 0x0156
#define S5K6AA_R_FWrealImageLine 0x0158
#define S5K6AA_R_FWsenId 0x015A
#define S5K6AA_R_FWusDevIdQaVersion 0x015C
#define S5K6AA_R_usFwCompilationBits 0x015E
#define S5K6AA_R_usSVNrevision 0x0160
#define S5K6AA_R_SVNpathRomAddress 0x0164

#define S5K6AA_IOCTL_SET_MODE		_IOW('o', 1, struct s5k6aa_mode)
#define S5K6AA_IOCTL_GET_STATUS		_IOR('o', 2, __u8)
#define S5K6AA_IOCTL_SET_COLOR_EFFECT   _IOW('o', 3, __u8)
#define S5K6AA_IOCTL_SET_WHITE_BALANCE  _IOW('o', 4, __u8)
#define S5K6AA_IOCTL_SET_SCENE_MODE     _IOW('o', 5, __u8)
#define S5K6AA_IOCTL_SET_AF_MODE        _IOW('o', 6, __u8)
#define S5K6AA_IOCTL_GET_AF_STATUS      _IOW('o', 7, __u8)
#define S5K6AA_IOCTL_SET_EXPOSURE       _IOW('o', 8, __u8)

struct s5k6aa_mode {
	int xres;
	int yres;
};

#ifdef __KERNEL__
struct s5k6aa_platform_data {
	int (*power_on)(void);
	int (*power_off)(void);
	int (*reset)(void);

};
#endif /* __KERNEL__ */

#endif  /* ___S5K6AA_SENSOR_H__ */

