//###########################################################################
// Description:
//! \addtogroup f2803xADC
//! <h1> ADC Start of Conversion (adc_soc)</h1>
//!
//! This ADC example uses ePWM1 to generate a periodic ADC SOC - ADCINT1.
//! Two channels are converted, ADCINA4 and ADCINA2.
//!
//! \b Watch \b Variables \n
//! - Voltage1[10]    - Last 10 ADCRESULT0 values
//! - Voltage2[10]    - Last 10 ADCRESULT1 values
//! - ConversionCount - Current result number 0-9
//! - LoopCount       - Idle loop counter
//
//
//###########################################################################

// $Copyright:
// Copyright (C) 2017 Defa Incorporated - http://www.defa.com/

//###########################################################################

// Included headerfiles.
#include "DSP28x_Project.h"     // Device Headerfile and Examples Include File


extern void DSP28x_usDelay(Uint32 Count);    // Definition of this function is in the file named "DSP280x_usDelay.asm".

#pragma CODE_SECTION(F2803XADCRead, "ramfuncs");     // Function named  "F2803XADCRead" will be run from RAM.
#pragma CODE_SECTION(CalACValue, "ramfuncs");
#pragma CODE_SECTION(CalHVDCValue, "ramfuncs");
//#pragma CODE_SECTION(CalBATValue, "ramfuncs");
//#pragma CODE_SECTION(CalHeatSinkTempValue, "ramfuncs");

Uint16 ADCBuf[16];
_iq giq16_ADCGain = _IQ16(1.0);
_iq giq16_ADCOffset = _IQ16(0);
Uint16 ADCIAC_Raw = _IQ16(0);
Uint16 ADCVAC_Raw = _IQ16(0);
Uint16 ADCVHVDC_Raw = _IQ16(0);

#define HW_IAC_OFFSET		2069
#define HW_VAC_OFFSET		2118
#define HW_VHVDC_OFFSET		39
#define HW_IHVDC_OFFSET		2062
#define HW_IBAT_OFFSET		2066


Uint16 CurrentOldValue = 2048;
Uint16 CurrentOldValueNoOffset = 2048;

Uint16 VoltageACOldValue = 2048;
Uint16 VoltageACOldValueNoOffset = 2048;

Uint16 VoltageHVDCOldValue = 0;
Uint16 VoltageHVDCOldValueNoOffset = 0;

Uint16	VHVDCCoeffA = _IQ16(0.9409);
Uint16	VHVDCCoeffB = _IQ16(0.0591);
_iq VHVDCNormalFiltered = _IQ16(0);

Uint16	IHVDCCoeffA = _IQ16(0.9409);
Uint16	IHVDCCoeffB = _IQ16(0.0591);
_iq IHVDCNormalFiltered = _IQ16(0);

Uint16	VBATCoeffA = _IQ16(0.9409);
Uint16	VBATCoeffB = _IQ16(0.0591);
_iq VBATNormalFiltered = _IQ16(0);

Uint16	IBATCoeffA = _IQ16(0.9409);
Uint16	IBATCoeffB = _IQ16(0.0591);
_iq IBATNormalFiltered = _IQ16(0);

Uint16	TEMPCoeffA = _IQ16(0.9409);
Uint16	TEMPCoeffB = _IQ16(0.0591);
_iq TEMPNormalFiltered = _IQ16(0);



#define CPU_CLOCK_SPEED      6.000L   // 10.000L for a 100MHz CPU clock speed
#define ADC_usDELAY 10000L

//---------------------------------------------------------------------------
// F2803X_drv_init:
//---------------------------------------------------------------------------
// This function initializes the ADC registers to a known state, which should combine the hardware.

void F2803XADCInit(ADCVALptr p)
{

		DELAY_US(ADC_usDELAY);
		AdcRegs.ADCCTL1.all=ADC_RESET_FLAG;
		asm(" NOP ");
		asm(" NOP ");

		EALLOW;
		AdcRegs.ADCCTL1.bit.ADCBGPWD	= 1;	/* Power up band gap */

		DELAY_US(ADC_usDELAY);					/* Delay before powering up rest of ADC */

		AdcRegs.ADCCTL1.bit.ADCREFSEL	= 0;
		AdcRegs.ADCCTL1.bit.ADCREFPWD	= 1;	/* Power up reference */
		AdcRegs.ADCCTL1.bit.ADCPWDN 	= 1;	/* Power up rest of ADC */
		AdcRegs.ADCCTL1.bit.ADCENABLE	= 1;	/* Enable ADC */

		asm(" RPT#100 || NOP");

		AdcRegs.ADCCTL1.bit.INTPULSEPOS=1;
		AdcRegs.ADCCTL1.bit.TEMPCONV=0;

		DELAY_US(ADC_usDELAY);

//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN0 = 1;   // Simultaneous Sampling Mode for SOC0 and SOC1
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN2 = 1;   // Simultaneous Sampling Mode for SOC2 and SOC3
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN4 = 1;   // Simultaneous Sampling Mode for SOC4 and SOC5
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN6 = 1;   // Simultaneous Sampling Mode for SOC6 and SOC7
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN8 = 1;   // Simultaneous Sampling Mode for SOC8 and SOC9
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN10 = 1;   // Simultaneous Sampling Mode for SOC10 and SOC11
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN12 = 1;   // Simultaneous Sampling Mode for SOC12 and SOC13
//		AdcRegs.ADCSAMPLEMODE.bit.SIMULEN14 = 1;   // Simultaneous Sampling Mode for SOC14 and SOC15

		/******* CHANNEL SELECT *******/
		AdcRegs.ADCSOC0CTL.bit.CHSEL = IAC_D_CHAN;	/* ChSelect: ADC A0-> Current Output AC*/
		AdcRegs.ADCSOC1CTL.bit.CHSEL = VAC_D_CHAN;
		AdcRegs.ADCSOC2CTL.bit.CHSEL = IHVDC_D_CHAN;
		AdcRegs.ADCSOC3CTL.bit.CHSEL = VHVDC_D_CHAN;
		AdcRegs.ADCSOC4CTL.bit.CHSEL = IBAT_D_CHAN;
		AdcRegs.ADCSOC5CTL.bit.CHSEL = VBAT_D_CHAN;
		AdcRegs.ADCSOC6CTL.bit.CHSEL = NTC_TEMP_TRANS1_D_CHAN;
		AdcRegs.ADCSOC7CTL.bit.CHSEL = NTC_TEMP_HS1_D_CHAN;
		AdcRegs.ADCSOC8CTL.bit.CHSEL = NTC_TEMP_TRANS2_D_CHAN;
		AdcRegs.ADCSOC9CTL.bit.CHSEL = NTC_TEMP_HS2_D_CHAN;
		AdcRegs.ADCSOC10CTL.bit.CHSEL = REF2V5_D_CHAN;
		AdcRegs.ADCSOC11CTL.bit.CHSEL = REF0V_D_CHAN;
//		AdcRegs.ADCSOC12CTL.bit.CHSEL = ADrsvd2;
//		AdcRegs.ADCSOC13CTL.bit.CHSEL = ADrsvd3;
//		AdcRegs.ADCSOC14CTL.bit.CHSEL = ADrsvd5;
//		AdcRegs.ADCSOC15CTL.bit.CHSEL = ADrsvd6;

		AdcRegs.ADCSOC0CTL.bit.TRIGSEL  = 5;	/* Set SOC0 start trigger on EPWM1A, due to round-robin SOC0 converts first then SOC1*/
		AdcRegs.ADCSOC1CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC2CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC3CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC4CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC5CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC6CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC7CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC8CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC9CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC10CTL.bit.TRIGSEL  = 5;
		AdcRegs.ADCSOC11CTL.bit.TRIGSEL  = 5;
//		AdcRegs.ADCSOC12CTL.bit.TRIGSEL  = 5;
//		AdcRegs.ADCSOC13CTL.bit.TRIGSEL  = 5;
//		AdcRegs.ADCSOC14CTL.bit.TRIGSEL  = 5;
//		AdcRegs.ADCSOC15CTL.bit.TRIGSEL  = 5;

		AdcRegs.ADCSOC0CTL.bit.ACQPS    = 6;	/* Set SOC0 S/H Window to 7 ADC Clock Cycles, (6 ACQPS plus 1)*/
		AdcRegs.ADCSOC1CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC2CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC3CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC4CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC5CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC6CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC7CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC8CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC9CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC10CTL.bit.ACQPS    = 6;
		AdcRegs.ADCSOC11CTL.bit.ACQPS    = 6;
//		AdcRegs.ADCSOC12CTL.bit.ACQPS    = 6;
//		AdcRegs.ADCSOC13CTL.bit.ACQPS    = 6;
//		AdcRegs.ADCSOC14CTL.bit.ACQPS    = 6;
//		AdcRegs.ADCSOC15CTL.bit.ACQPS    = 6;

		EDIS;


		/* Set up Event Trigger with CNT_zero enable for Time-base of EPWM1 */
		EPwm1Regs.ETSEL.bit.SOCAEN = 1;     /* Enable SOCA */
		EPwm1Regs.ETSEL.bit.SOCASEL = 1;    /* Enable CNT_zero event for SOCA */
		EPwm1Regs.ETPS.bit.SOCAPRD = 1;     /* Generate SOCA on the 1st event */
		EPwm1Regs.ETCLR.bit.SOCA = 1;       /* Clear SOCA flag */
}


void F2803XADCRead(ADCVALptr p)
{
	Uint16 Temp;
	int16 ctr;
	Uint16 *ResultPtr;

	ResultPtr = (Uint16 *) (&(AdcResult.ADCRESULT0));
	for (ctr = 0; ctr < NUM_ADC_CHANS; ctr++)
	{
		ADCBuf[ctr] = *ResultPtr++;
	}

	Temp = ADCBuf[REF2V5_D_VALUE] - ADCBuf[REF0V_D_VALUE];

	giq16_ADCGain = _IQ16div(0x0c1F, Temp);//0x0c1F = 2.5/3.3 * 4096 _IQ16

	giq16_ADCOffset = _IQ16mpy(ADCBuf[REF0V_D_VALUE], giq16_ADCGain);

	ADCIAC_Raw = _IQ16mpy(ADCBuf[IAC_D_VALUE], giq16_ADCGain) - giq16_ADCOffset;

	ADCVAC_Raw = _IQ16mpy(ADCBuf[VAC_D_VALUE], giq16_ADCGain) - giq16_ADCOffset;

	ADCVHVDC_Raw = _IQ16mpy(ADCBuf[VHVDC_D_VALUE], giq16_ADCGain) - giq16_ADCOffset;
}

//---------------------------------------------------------------------------
// SetOffset:
//---------------------------------------------------------------------------
// This function calculate some offset values.

void SetOffset(ADCVALptr p)
{
	int16 i;
	int16 Temp;

	for(i=0; i < 30000; i++)
	{
			F2803XADCRead(p);

			//IAC

			Temp = (ADCIAC_Raw + CurrentOldValue) >> 1;

			CurrentOldValue = ADCIAC_Raw;

			CurrentOldValueNoOffset = (CurrentOldValueNoOffset + Temp) >> 1;

			Temp = CurrentOldValueNoOffset - HW_IAC_OFFSET;

			p->IACOffset = p->IACGain * Temp;  // Tmp = gain*dat => Q24 = Q12*Q12


			//VAC

			Temp = (ADCVAC_Raw + VoltageACOldValue) >> 1;

			VoltageACOldValue = ADCVAC_Raw;

			VoltageACOldValueNoOffset = (VoltageACOldValueNoOffset + Temp) >> 1;

			Temp = VoltageACOldValueNoOffset - HW_VAC_OFFSET;

			p->VACOffset = p->VACGain * Temp;  // Tmp = gain*dat => Q24 = Q12*Q12


			//VHVDC

			Temp = (ADCVHVDC_Raw + VoltageHVDCOldValue) >> 1;

			VoltageHVDCOldValue = ADCVHVDC_Raw;

			VoltageHVDCOldValueNoOffset = (VoltageHVDCOldValueNoOffset + Temp) >> 1;

			Temp = VoltageHVDCOldValueNoOffset - HW_VHVDC_OFFSET;

			p->VHVDCOffset = p->VHVDCGainIQ12 * Temp;  // Tmp = gain*dat => Q24 = Q12*Q12
	}
}

//---------------------------------------------------------------------------
// SetUVWOffset:
//---------------------------------------------------------------------------
//Call this function at pwm close state

void SetVACOffset(ADCVALptr p)
{
	p->IACOffset += p->IACOut;
	p->VACOffset += p->VACOut;
}



void CalACValue(ADCVALptr p)
{

	int16 Temp;

	//IAC

	Temp = (ADCIAC_Raw + CurrentOldValue) >> 1;

	CurrentOldValue = ADCIAC_Raw;

	CurrentOldValueNoOffset = (CurrentOldValueNoOffset + Temp) >> 1;

	Temp = CurrentOldValueNoOffset - HW_IAC_OFFSET;

	p->IACOut =  - (Temp * p->IACGain - p->IACOffset);  // Tmp = gain*dat => Q24 = Q12*Q12

	invIiInst = p->IACOut;

	//VAC

	Temp = (ADCVAC_Raw + VoltageACOldValue) >> 1;

	VoltageACOldValue = ADCVAC_Raw;

	VoltageACOldValueNoOffset = (VoltageACOldValueNoOffset + Temp) >> 1;

	Temp = VoltageACOldValueNoOffset - HW_VAC_OFFSET;

	p->VACOut =  - (Temp * p->VACGain - p->VACOffset);  // Tmp = gain*dat => Q24 = Q12*Q12



	invVoInst = p->VACOut;

}


void CalACNoFilterValue(ADCVALptr p)
{

	int16 Temp;

	Temp = ADCIAC_Raw - HW_IAC_OFFSET;

	p->IACNoFilter = Temp * p->IACGain - p->IACOffset;// Tmp = gain*dat => Q24 = Q12*Q12

	Temp = ADCVAC_Raw - HW_VAC_OFFSET;

	p->VACNoFilter = Temp * p->VACGain - p->VACOffset;// Tmp = gain*dat=> Q24 = Q12*Q12


}



void CalHVDCValue(ADCVALptr p)
{

	Uint32 Temp;

	Temp = _IQ16mpy(ADCBuf[IHVDC_D_VALUE], giq16_ADCGain) - giq16_ADCOffset - HW_IHVDC_OFFSET;

	Temp = p->IHVDCGain * Temp;

	IHVDCNormalFiltered = _IQ16mpy(IHVDCNormalFiltered, IHVDCCoeffA) + _IQ16mpy(Temp, IHVDCCoeffB);

	p->IHVDCOut = _IQ16mpy(_IQ16(IHVDC_SCALE_MAX_SENSE),IHVDCNormalFiltered);




	Temp = (ADCVHVDC_Raw + VoltageHVDCOldValue) >> 1;

	VoltageHVDCOldValue = ADCVHVDC_Raw;

	VoltageHVDCOldValueNoOffset = (VoltageHVDCOldValueNoOffset + Temp) >> 1;

	Temp = VoltageHVDCOldValueNoOffset - HW_VHVDC_OFFSET;

	Temp = p->VHVDCGainIQ12 * Temp - p->VHVDCOffset;  // Tmp = gain*dat => Q24 = Q12*Q12

	invVbusInst = Temp;



	Temp = _IQ16mpy(ADCBuf[VHVDC_D_VALUE], giq16_ADCGain) - giq16_ADCOffset;

	Temp = p->VHVDCGainIQ16 * Temp ;

	VHVDCNormalFiltered = _IQ16mpy(VHVDCNormalFiltered, VHVDCCoeffA) + _IQ16mpy(Temp, VHVDCCoeffB);		// cutoff frequency

	p->VHVDCOut = _IQ16mpy(_IQ16mpy(_IQ16(VHVDC_SCALE_MAX_SENSE),VHVDCNormalFiltered), _IQ16(1.066));		// Just for test with 143oHm gain with 8 * 2.2


}


void CalBATValue(ADCVALptr p)
{
	Uint32 Temp;

	Temp = _IQ16mpy(ADCBuf[IBAT_D_VALUE], giq16_ADCGain) - giq16_ADCOffset - HW_IBAT_OFFSET;

	Temp = p->IBATGain * Temp;


	IBATNormalFiltered = _IQ16mpy(IBATNormalFiltered, IBATCoeffA) + _IQ16mpy(Temp, IBATCoeffB);

	p->IBATOut = _IQ16mpy(_IQ16(IBAT_SCALE_MAX_SENSE),IBATNormalFiltered);



	Temp = _IQ16mpy(ADCBuf[VBAT_D_VALUE], giq16_ADCGain) - giq16_ADCOffset;

	Temp = p->VBATGain * Temp;

	VBATNormalFiltered = _IQ16mpy(VBATNormalFiltered, VBATCoeffA) + _IQ16mpy(Temp, VBATCoeffB);

	p->VBATOut = _IQ16mpy(_IQ16(VBAT_SCALE_MAX_SENSE),VBATNormalFiltered);

}


int32 CalHeatSinkTempValue(Uint16 n)
{

	int32 Temp;

	Temp = _IQ16mpy(ADCBuf[n],giq16_ADCGain) - giq16_ADCOffset;

	if(Temp > 3180)
	{//-55 ~ -1 degree			// y = -7E-05x2 + 0.448x - 730.33		0x00000004 = _IQ16(0.00007) 0x000072B0 = _IQ16(0.448)
		Temp = Temp * 0x000072B0 - Temp * Temp * 0x00000004 - _IQ16(730.33);

	}
	else if(Temp > 1400)
	{//0 ~ 40 degree			// y = -0.0228x + 71.925

		Temp = 0x0047ECCB - Temp * 0x05D7; // 0x05D7 = _IQ16(0.02281282080)		0x0047ECCB = _IQ16(71.92498779)

	}
	 else if(Temp > 256)
	{//41 ~ 100 degree			// y = 4E-05x2 - 0.1068x + 122.52		0x00000002 = _IQ16(0.00004) 0x00001B57 = _IQ16(0.1068)
		Temp = Temp * Temp * 0x00000004 - Temp * 0x00001B57  + _IQ16(122.52);
	}
	else
	{//101 ~ 150 degree			// y = 0.0011x2 - 0.6021x + 186.88		0x00000048 = _IQ16(0.0011) 0x00009A23 = _IQ16(0.6021)
		Temp = Temp * Temp * 0x00000048 - Temp * 0x00009A23  + _IQ16(186.88);
	}

//	TEMPNormalFiltered = _IQ16mpy(TEMPNormalFiltered, TEMPCoeffA) + _IQ16mpy(Temp, TEMPCoeffB);		// cutoff frequency

	return(Temp);

}






//===========================================================================
// End of file.
//===========================================================================
