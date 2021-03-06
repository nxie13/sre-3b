//header file to send and receive CAN messages between the BMS and the charger
//By Nuoya Xie

#ifndef BMS_DATATYPES_H_
#define BMS_DATATYPES_H_

#include <stdint.h>
#include "Linduino.h"
#include "LT_SPI.h"
#include "LTC6811.h"
#include <Wire.h>

//addresses for various ICs
#define TEMP_SENSORS_BASE_ADDRESS 0x14 //decimal(20).  0x14(hex),  0010100(binary), according to Alex
#define BSPD_DAC_5321_ADDRESS 0xC
#define CURRENT_SENSOR_2946_ADDRESS 0xDE

//pins
#define CAN_TRANSCEIVER_PIN 9
// TODO: #define DAC_PIN
#define BMS_FAULT_PIN 6
#define CHARGING_ACTIVE_PIN 5
#define ISO_CS_PIN 7

//Individual cell specifications (from Hg2 data sheet)
#define CELL_OVER_VOLTAGE_THRESHOLD_V 4.16
#define CELL_UNDER_VOLTAGE_THRESHOLD_V 2.6
#define CELL_ABSOLUTE_MINIMUM_VOLTAGE_V 2.5
#define CELL_ABSOLUTE_MAXIMUM_VOLTAGE_V 4.2
#define CELL_ALMOST_FULL_VOLTAGE_V 4.0
#define CELL_OVER_TEMP_THRESHOLD_CHARGE_C 50
#define CELL_OVER_TEMP_THRESHOLD_DISCHARGE_C 75
#define TOTAL_BATTERY_VOLTAGE 299.7
#define CHARGER_OUTPUT_CURRENT_SPEC 7.5
#define CHARGER_OUTPUT_VOLTAGE_SPEC 288

//self-defined variables for 6811 Inc
#define TOTAL_CELL_GROUP 10 //9 cell group/module, but 6th one is zero and 10th one is the one with battery
#define ENABLED 1
#define DISABLED 0
//#define DATALOG_ENABLED 1
//#define DATALOG_DISABLED 0

//setup variables and constants needed for the linduino 6811 APIs, created by LT
const uint8_t TOTAL_IC = 8;//!<number of ICs in the daisy chain, 8 in our case
//ADC Command Configurations
const uint8_t ADC_OPT = ADC_OPT_DISABLED; // See LTC6811_daisy.h for Options
const uint8_t ADC_CONVERSION_MODE = MD_7KHZ_3KHZ;//MD_7KHZ_3KHZ; //MD_26HZ_2KHZ;//MD_7KHZ_3KHZ; // See LTC6811_daisy.h for Options
const uint8_t ADC_DCP = DCP_DISABLED; // See LTC6811_daisy.h for Options
const uint8_t CELL_CH_TO_CONVERT = CELL_CH_ALL; // See LTC6811_daisy.h for Options
//const uint8_t AUX_CH_TO_CONVERT = AUX_CH_ALL; // See LTC6811_daisy.h for Options
//const uint8_t STAT_CH_TO_CONVERT = STAT_CH_ALL; // See LTC6811_daisy.h for Options

//const uint16_t MEASUREMENT_LOOP_TIME = 500;//milliseconds(mS)

//Under Voltage and Over Voltage Thresholds
//const uint16_t OV_THRESHOLD = 41000; // Over voltage threshold ADC Code. LSB = 0.0001
//const uint16_t UV_THRESHOLD = 30000; // Under voltage threshold ADC Code. LSB = 0.0001

//Loop Measurement Setup These Variables are ENABLED or DISABLED Remember ALL CAPS
//const uint8_t WRITE_CONFIG = DISABLED; // This is ENABLED or DISABLED
//const uint8_t READ_CONFIG = DISABLED; // This is ENABLED or DISABLED
//const uint8_t MEASURE_CELL = ENABLED; // This is ENABLED or DISABLED
//const uint8_t MEASURE_AUX = DISABLED; // This is ENABLED or DISABLED
//const uint8_t MEASURE_STAT = DISABLED; //This is ENABLED or DISABLED
//const uint8_t PRINT_PEC = DISABLED; //This is ENABLED or DISABLED

//This struct contains the voltage information and over-voltage flag for each cell group
/*typedef struct
{
	float cell_group_voltage;
	float cell_group_temp;
	bool over_voltage_threshold_flag;
	bool over_temp_threshold_flag;
	bool cell_group_battery_full_flag;
}cell_group;*/

//state of the battery - on the charger or inside the car?
typedef enum
{
	discharging = 0, //this means battery is in the car, not charging
	charging = 1, //this means battery is off the car for charging
  regening = 2, //this means the battery is being charged by the regen process
  testing = 3, //this means the battery is in testing BPSD
  idle = 4 //idle state
}battery_state;

//standard traction pack message for the VCU
struct standard_traction_pack_message{
  //MID 620 //change soc from percentage to voltage
  uint8_t  SoC; //byte 0
  float  max_discharge_current; //byte 1 and 2
  float  max_regen_current; //byte 3 and 4
  uint32_t battery_power; //byte 5 to 7

  //MID 621
  float battery_temp; //byte 0 and 1
  union { //byte 2 and 3
    uint16_t flag;
    struct {
          byte cell_voltage_reach_4_flag : 1; //bit 0
          byte cell_voltage_reach_4_16_flag : 1; //bit 1
          byte cell_voltage_too_high_flag : 1; //bit 2
          byte cell_voltage_too_low_flag : 1; //bit 3
          byte cell_voltage_below_2_6_flag : 1; //bit 4
          byte pack_voltage_high_flag : 1; //bit 5
          byte pack_voltage_low_flag : 1; //bit 6
          byte battery_temp_too_high_for_charging_flag : 1; //bit 7
          byte battery_temp_too_high_for_discharging_flag : 1; //bit 8
          byte discharge_disabled_flag : 1; //bit 9
          byte recharge_disabled_flag : 1; //bit 10
          byte BMS_fault_flag: 1; //bit 11
          byte : 1; //bit 12
          byte : 1; //bit 13
          byte : 1; //bit 14
          byte : 1; //bit 15
      }__attribute__((packed));
  };
  float total_voltage; //byte 4 and 5
  float total_current; //byte 6  and 7
};

#endif
