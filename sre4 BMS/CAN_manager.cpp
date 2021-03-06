//cpp file for CAN_manager_singleton.h

#include "CAN_manager.h"

CAN_manager_singleton* CAN_manager_singleton::instance = NULL;

static CAN_manager_singleton&  CAN_manager_singleton::getInstance()
{
  if (instance == NULL)
  {
    instance = new CAN_manager_singleton();
  }
  return *instance;
}

CAN_manager_singleton::CAN_manager_singleton()
{
  //initialize varibles for extended message format
  BmsToCharger_message.message_id = 0x1806E5F4; //fixed
	BmsToCharger_message.data_length_byte = 8; //data = 8 bytes
	BmsToCharger_message.data = {0}; //clear all data in the data frame

  ChargerToBms_message.message_id = 0; //clear message ID
	ChargerToBms_message.data_length_byte = 0; //clear data length byte
	ChargerToBms_message.data = {0}; //clear all data in the data frame
}

bool CAN_manager_singleton::CAN_manager_init_bus(MCP_CAN& CAN_BUS)
{
  //MCP_ANY means the bus can accept both standard and extended format messages (disables masks and filters)
  bool initialization_failed = true;
  initialization_failed =  CAN_BUS.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  CAN_BUS.setMode(MCP_NORMAL);
  return initialization_failed;
}

void CAN_manager_singleton::BMS_to_charger_set_max_charging_parameters(float max_charging_voltage, float max_charging_current)
{
  int voltage = max_charging_voltage * 10;
  BmsToCharger_message.data.voltage_to_charger_LSB = voltage & 0xff;
  BmsToCharger_message.data.voltage_to_charger_MSB = (voltage >> 8) & 0xff;

  int current = max_charging_current * 10;
  BmsToCharger_message.data.current_to_charger_LSB = current & 0xff;
  BmsToCharger_message.data.current_to_charger_MSB = (current >> 8) & 0xff;
}
void CAN_manager_singleton::BMS_to_charger_disable_charger()
{
BmsToCharger_message.data.control = 1;
}
void CAN_manager_singleton::BMS_to_charger_enable_charger()
{
BmsToCharger_message.data.control = 0;
}

bool CAN_manager_singleton::BMS_to_charger_message_send(MCP_CAN& CAN_BUS)
{
return CAN_BUS.sendMsgBuf(BmsToCharger_message.message_id, BmsToCharger_message.data_length_byte, BmsToCharger_message.data.dword);
}

void CAN_manager_singleton::charger_to_BMS_message_receive(MCP_CAN& CAN_BUS)
{
  if (CAN_BUS.checkReceive()) //if the data is available
  {
    uint32_t temp_mid = 0;
    uint8_t temp_data_length = 0;
    uint8_t temp_dword[8] = {0};

    //CAN_BUS.readMsgBuf(&(ChargerToBms_message.message_id), &(ChargerToBms_message.data_length_byte), ChargerToBms_message.data.dword);
    CAN_BUS.readMsgBuf(&temp_mid, &temp_data_length, temp_dword);
    if (temp_mid == CHARGER_MID) //store into struct only if msg id matches
    {
      ChargerToBms_message.message_id = temp_mid;
      ChargerToBms_message.data_length_byte = temp_data_length;
      memcpy(ChargerToBms_message.data.dword, temp_dword, 8);
    }
  }
}

BMS_to_charger_CAN_message& CAN_manager_singleton::get_BMS_to_charger_CAN_message()
{
  return BmsToCharger_message;
}

charger_to_BMS_CAN_message& CAN_manager_singleton::get_charger_to_BMS_CAN_message()
{
  return ChargerToBms_message;
}

bool CAN_manager_singleton::BMS_to_car_message_send(MCP_CAN& CAN_BUS, standard_traction_pack_message& stpm)
{
  BMS_standard_message msg = {0};
  uint16_t raw = 0;
  uint8_t msgs_send_successful = 0;

  //0x620 -soc, max discharge and regen currents
  msg.message_id = 0x620;
  msg.data_length_byte = 8;
  msg.data[0] = stpm.SoC;
  raw = stpm.max_discharge_current * 10;
  msg.data[1] = (raw >> 8) & 0xFF;
  msg.data[2] = (raw & 0xFF);
  raw = stpm.max_regen_current * 10;
  msg.data[3] = (raw >> 8) & 0xFF;
  msg.data[4] = raw & 0xFF;
  uint32_t raw_32 = stpm.battery_power;
  msg.data[5] = (raw_32 >> 16) & 0xFF;
  msg.data[6] = (raw_32 >> 8) & 0xFF;
  msg.data[7] = raw_32 & 0xFF;
  msgs_send_successful |= CAN_BUS.sendMsgBuf(msg.message_id, msg.data_length_byte, msg.data);

  //0x621 -battery temp, flags, total voltage
  msg.message_id = 0x621;
  msg.data_length_byte = 7;
  raw = stpm.battery_temp * 10;
  msg.data[0] = (raw >> 8) & 0xFF;
  msg.data[1] = (raw & 0xFF);
  msg.data[2] = stpm.flag >> 8 & 0xFF;
  msg.data[3] = stpm.flag & (0xFF);
  raw = stpm.total_voltage * 10;
  msg.data[4] = (raw >> 8) & 0xFF;
  msg.data[5] = (raw & 0xFF);
  raw = stpm.total_current * 10;
  msg.data[6] = (raw >> 8) & 0xFF;
  msg.data[7] = (raw & 0xFF);
  msgs_send_successful |= CAN_BUS.sendMsgBuf(msg.message_id, msg.data_length_byte, msg.data);

  return msgs_send_successful; //should be 0 if all msgs are send successfully
}

//at the moment only 2 messages are received - from PCANView to initiate BSPD testing
//and values to toggle the DAC for BSPD testing
//TODO: setting up pcan message ID and message to send
void CAN_manager_singleton::read_CAN_bus_std_format(MCP_CAN& CAN_BUS, uint32_t& msg_ID, uint8_t& msg_length, uint8_t buffer[8])
{
  if (CAN_BUS.checkReceive()) //if the data is available
  {
    CAN_BUS.readMsgBuf(&msg_ID, &msg_length, buffer);
  }
}
