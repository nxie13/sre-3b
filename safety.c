#include <stdlib.h>  //Needed for malloc
#include <math.h>
#include "IO_RTC.h"

#include "safety.h"
#include "mathFunctions.h"

#include "sensors.h"

#include "torqueEncoder.h"
#include "brakePressureSensor.h"


/*****************************************************************************
* SafetyChecker object
******************************************************************************
*
****************************************************************************/
struct _SafetyChecker {
	bool tpsOutOfRange;
	bool tpsCalibrated;
	bool tpsOutOfSync;

	bool bpsOutOfRange;
	bool bpsCalibrated;

	bool tpsbpsImplausible;
};

/*****************************************************************************
* Torque Encoder (TPS) functions
* RULE EV2.3.5:
* If an implausibility occurs between the values of these two sensors the power to the motor(s) must be immediately shut down completely.
* It is not necessary to completely deactivate the tractive system, the motor controller(s) shutting down the power to the motor(s) is sufficient.
****************************************************************************/
SafetyChecker* SafetyChecker_new(void)
{
    SafetyChecker* me = (SafetyChecker*)malloc(sizeof(struct _SafetyChecker));

	//Initialize all safety checks to FAIL 
	me->tpsOutOfRange = TRUE;
	me->tpsOutOfSync = TRUE; //Torque Encoder Plausibility Check
	me->bpsOutOfRange = TRUE;
	me->tpsbpsImplausible = TRUE;


    return me;
}

//Updates all values based on sensor readings, safety checks, etc
void SafetyChecker_update(SafetyChecker* me, TorqueEncoder* tps, BrakePressureSensor* bps)
{
	//===================================================================
	//Get calibration status
	//===================================================================
	me->tpsCalibrated = tps->calibrated;
	me->bpsCalibrated = bps->calibrated;

	//===================================================================
	//Make sure raw sensor readings are within operating range
	//===================================================================
	//RULE: EV2.3.10 - signal outside of operating range is considered a failure
	//  This refers to SPEC SHEET values, not calibration values
	//Note: IC cars may continue to drive for up to 100ms until valid readings are restored, but EVs must immediately cut power
	//Note: We need to decide how to report errors and how to perform actions when those errors occur.  For now, I'm calling an imaginary Err.Report function
	
	//-------------------------------------------------------------------
	//Torque Encoder
	//-------------------------------------------------------------------
	me->tpsOutOfRange = TRUE;
	if (tps->tps0->sensorValue >= tps->tps0->specMin && tps->tps0->sensorValue <= tps->tps0->specMax
	&&  tps->tps1->sensorValue >= tps->tps1->specMin && tps->tps1->sensorValue <= tps->tps1->specMax)
	{
		me->tpsOutOfRange = FALSE;
	}

	//-------------------------------------------------------------------
	//Brake Pressure Sensor
	//-------------------------------------------------------------------
	me->bpsOutOfRange = TRUE;
	if (bps->bps0->sensorValue >= bps->bps0->specMin && bps->bps0->sensorValue <= bps->bps0->specMax)
	{
		me->bpsOutOfRange = FALSE;
	}


	//===================================================================
	// Make sure calibrated TPS readings are in sync with each other
	//===================================================================
	// EV2.3.5 If an implausibility occurs between the values of these two sensors
	//  the power to the motor(s) must be immediately shut down completely. It is not necessary 
	//  to completely deactivate the tractive system, the motor controller(s) shutting down the 
	//  power to the motor(s) is sufficient.
	// EV2.3.6 Implausibility is defined as a deviation of more than 10 % pedal travel between the sensors.
	//-------------------------------------------------------------------
	
	//Check for implausibility (discrepancy > 10%)
	//RULE: EV2.3.6 Implausibility is defined as a deviation of more than 10% pedal travel between the sensors.
	float4 tps0 = -10;  //Set invalid values just in case
	float4 tps1 = 99;
	TorqueEncoder_getIndividualSensorPercent(tps, 0, &tps0);
	TorqueEncoder_getIndividualSensorPercent(tps, 1, &tps1);
	if (fabs(tps0 - tps1) > .1)  //Note: Individual TPS readings don't go negative, otherwise this wouldn't work
	{
		//Err.Report(Err.Codes.TPSDiscrepancy, "TPS discrepancy of over 10%", Motor.Stop);
		me->tpsOutOfSync = TRUE;
	}



	//===================================================================
	//Torque Encoder <-> Brake Pedal Plausibility Check
	//===================================================================
	// EV2.5 Torque Encoder / Brake Pedal Plausibility Check
	//  The power to the motors must be immediately shut down completely, if the mechanical brakes 
	//  are actuated and the torque encoder signals more than 25 % pedal travel at the same time.
	//  This must be demonstrated when the motor controllers are under load.
	// EV2.5.1 The motor power shut down must remain active until the torque encoder signals less than 5 % pedal travel,
	//  no matter whether the brakes are still actuated or not.
	//-------------------------------------------------------------------
	//Implausibility if..
	if (bps->percent > .02 && tps->percent > .25) //If mechanical brakes actuated && tps > 25%
	{
		me->tpsbpsImplausible = TRUE;
		//From here, assume that motor controller will check for implausibility before accepting commands
	}

	//Clear implausibility if...
	if (me->tpsbpsImplausible == TRUE)
	{
		if (tps->percent < .05) //TPS is reduced to < 5%
		{
			me->tpsbpsImplausible = FALSE;
		}
	}


}


//Updates all values based on sensor readings, safety checks, etc
bool SafetyChecker_allSafe(SafetyChecker* me)
{
	bool allSafe = FALSE;
	if (me->tpsOutOfRange == FALSE 
		&& me->bpsOutOfRange == FALSE 
		&& me->tpsOutOfSync == FALSE 
		&& me->tpsbpsImplausible == FALSE
		&& me->tpsCalibrated == TRUE
		&& me->bpsCalibrated == TRUE)
	{
		allSafe = TRUE;
	}
	return allSafe;
}
