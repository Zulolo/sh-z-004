#ifndef _SH_Z_H
#define _SH_Z_H

#define SH_Z_SN_LEN						16
#define DEVICE_SN_JSON_TAG		"sh_z_device_sn"
#define BITS_NUM_PER_BYTE			8

typedef struct {
  unsigned short bStaticIP : 1;
	unsigned short bReserved : 15;
	unsigned char uIP_Addr[4];
	unsigned char uNetmask[4];
	unsigned char uGateway[4];
	unsigned char uMAC_Addr[6];
} ETH_Conf_t;

#endif
