/*
 * Copyright (C) 2015-2018 Alibaba Group Holding Limited
 */

#ifndef MQTTPACKET_H_
#define MQTTPACKET_H_

#if defined(__cplusplus) /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

#if defined(WIN32_DLL) || defined(WIN64_DLL)
#define DLLImport __declspec(dllimport)
#define DLLExport __declspec(dllexport)
#elif defined(LINUX_SO)
#define DLLImport extern
#define DLLExport  __attribute__ ((visibility ("default")))
#else
#define DLLImport
#define DLLExport
#endif

enum errors {
    MQTTPACKET_BUFFER_TOO_SHORT = -2,
    MQTTPACKET_READ_ERROR = -1,
    MQTTPACKET_READ_COMPLETE
};


/* CPT, control packet type */
enum msgTypes {
    MQTT_CPT_RESERVED = 0, CONNECT = 1, CONNACK, PUBLISH, PUBACK, PUBREC, PUBREL,
    PUBCOMP, SUBSCRIBE, SUBACK, UNSUBSCRIBE, UNSUBACK,
    PINGREQ, PINGRESP, DISCONNECT
};

/**
 * Bitfields for the MQTT header byte.
 */
typedef union
{
	unsigned char byte;	                /**< the whole byte */
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	struct
	{
		unsigned int type : 4;			/**< message type nibble */
		unsigned int dup : 1;				/**< DUP flag bit */
		unsigned int qos : 2;				/**< QoS value, 0, 1 or 2 */
		unsigned int retain : 1;		/**< retained flag bit */
	} bits;
#else
	struct
	{
		unsigned int retain : 1;		/**< retained flag bit */
		unsigned int qos : 2;				/**< QoS value, 0, 1 or 2 */
		unsigned int dup : 1;				/**< DUP flag bit */
		unsigned int type : 4;			/**< message type nibble */
	} bits;
#endif
} MQTTHeader;

typedef struct {
    int len;
    char *data;
} MQTTLenString;

typedef struct {
    char *cstring;
    MQTTLenString lenstring;
} MQTTString;

#define MQTTString_initializer {NULL, {0, NULL}}

int MQTTstrlen(MQTTString mqttstring);

#include "MQTTConnect.h"
#include "MQTTPublish.h"
#include "MQTTSubscribe.h"
#include "MQTTUnsubscribe.h"

int MQTTSerialize_ack(unsigned char *buf, int buflen, unsigned char type, unsigned char dup, unsigned short packetid);
int MQTTDeserialize_ack(unsigned char *packettype, unsigned char *dup, unsigned short *packetid, unsigned char *buf,
                        int buflen);

int MQTTPacket_len(int rem_len);
int MQTTPacket_equals(MQTTString *a, char *b);

int MQTTPacket_encode(unsigned char *buf, int length);
int MQTTPacket_decode(int (*getcharfn)(unsigned char *, int), int *value);
int MQTTPacket_decodeBuf(unsigned char *buf, int *value);

int readInt(unsigned char **pptr);
char readChar(unsigned char **pptr);
void writeChar(unsigned char **pptr, char c);
void writeInt(unsigned char **pptr, int anInt);
int readMQTTLenString(MQTTString *mqttstring, unsigned char **pptr, unsigned char *enddata);
void writeCString(unsigned char **pptr, const char *string);
void writeMQTTString(unsigned char **pptr, MQTTString mqttstring);

typedef struct {
    int (*getfn)(void *, unsigned char *,
                 int); /* must return -1 for error, 0 for call again, or the number of bytes read */
    void *sck;  /* pointer to whatever the system may use to identify the transport */
    int multiplier;
    int rem_len;
    int len;
    char state;
} MQTTTransport;

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
}
#endif


#endif /* MQTTPACKET_H_ */

