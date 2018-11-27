#ifndef ZG_ZCL_H
#define ZG_ZCL_H

#define ZCL_BROADCAST_SHORT_ADDR                0xFFFF
#define ZCL_BROADCAST_ENDPOINT                  0xFE
#define ZCL_BROADCAST_INTER_PAN                 0xFFFF

#define ZCL_ZDP_ENDPOINT                        0x00

/* Cluster identifiers */
#define ZCL_CLUSTER_SIMPLE_DESCRIPTOR_REQUEST   0x0004
#define ZCL_CLUSTER_ACTIVE_ENDPOINTS_REQUEST    0x0005
#define ZCL_CLUSTER_ON_OFF                      0x0006
#define ZCL_CLUSTER_COLOR_CONTROL               0x0300
#define ZCL_CLUSTER_TEMPERATURE_MEASUREMENT     0x0402
#define ZCL_CLUSTER_PRESSURE_MEASUREMENT        0x0403
#define ZCL_CLUSTER_HUMIDITY_MEASUREMENT        0x0405
#define ZCL_CLUSTER_TOUCHLINK_COMMISSIONING     0x1000

typedef struct
{
    uint8_t endpoint;
    uint16_t profile;
    uint16_t device_id;
    uint8_t num_clusters_in;
    uint8_t num_clusters_out;
    uint8_t *in_clusters_list;
    uint8_t *out_clusters_list;
} ZgSimpleDescriptor;

#endif

