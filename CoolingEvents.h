#ifndef COOLING_EVENTS_H
#define COOLING_EVENTS_H

/* Bits of Rte CoolFaults: written by SWC_CoolingCtrl, read by the DEM monitor */
#define COOL_EVT_SHT_COMM     0x01U
#define COOL_EVT_SHT_RANGE    0x02U
#define COOL_EVT_LVL_LOW      0x04U
#define COOL_EVT_LVL_EMPTY    0x08U
#define COOL_EVT_LVL_SENSOR   0x10U
#define COOL_EVT_OVERTEMP_W   0x20U
#define COOL_EVT_OVERTEMP_C   0x40U
#define COOL_EVT_INEFFECTIVE  0x80U

#endif
