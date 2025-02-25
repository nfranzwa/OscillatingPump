/*
 * MightyZap.h
 *
 *  Created on: 2016 12. 28.
 *      Author: BG. Shim
 */

#ifndef MightyZap_H_
#define MightyZap_H_

#include "utility/IR_Protocol.h"
#include "HardwareSerial.h"
#include <SoftwareSerial.h>

/*
typedef struct data {
    int             iID;
    int				iAddr;
    int             iLength;
    int             iError;
    int  			iData[8];
} BulkData, *PBulkData;

*/
enum stroke_type{
		Short =0,
		Lowest =0,
		Highest=1,
		Long=1,
		Center=2,
		Middle =2
	};
enum level_type{
	Low =0,
	High,	
};
enum gain{
	dGain,
	iGain,
	pGain
};	
enum RGB{
	off=0,
	RED=1,
	GREEN=2,
    BLUE =3
};

class MightyZap {
public:
	MightyZap(HardwareSerial  *dev_serial, int DirectionPin);
	MightyZap(SoftwareSerial  *dev_serial, int DirectionPin);
	MightyZap(HardwareSerial  *dev_serial, int DirectionPin,bool TxLevel);
	MightyZap(SoftwareSerial  *dev_serial, int DirectionPin,bool TxLevel);
	virtual ~MightyZap();

	/////////// Device control methods /////////////
	void begin(int buad);
	int  ping(int  bID);
	int readRaw(void);
	int available(void);
	void writeRaw(int value);
	int getError(int errbit);

	int txRxPacket(int bID, int bInst, int bTxParaLen);
	int txPacket(int bID, int bInstruction, int bParameterLength);
	int rxPacket(int bRxLength);
	int  getResult(void);
	int getTxRxStatus(void);
	void setPacketType();
	int getPacketType(void);
	//// High communication methods ////////
	int readByte(int bID, int bAddress);
	int writeByte(int bID, int bAddress, int bData);
	int readint(int bID, int bAddress);
	int writeint(int bID, int bAddress, short wData);
	//int writeDint( int bID, int wAddress,  unsigned long value );
	//unsigned long readDint( int bID, int wAddress );
	
	

	/////// Methods for making a packet ////////
	void setTxPacketId( int id );
	void setTxPacketInstruction( int instruction );
	void setTxPacketParameter( int index, int value );
	void setTxPacketLength( int length );
	int txrxPacket(void);
	int getRxPacketParameter( int index );
	int getRxPacketLength(void);

	//Easy Functions for IRP
	
	int getModelNumber(int bID);
	int Version(int bID);
	void ServoID(int bID, int new_ID);
	int ServoID(int bID);
	void BaudRate(int bID, int baud_num);
	int BaudRate(int bID);


	// Stroke Limit	
	void ShortStrokeLimit(int bID, int position);
	int ShortStrokeLimit(int bID);	
	void LongStrokeLimit(int bID, int position);
	int LongStrokeLimit(int bID);

	void alarmLed(int bID, int value);
	int alarmLed(int bID);	
	void alarmShutdown(int bID,int option);
	int alarmShutdown(int bID);


	void forceEnable(int bID,int value);
	int forceEnable(int bID);
	void ledOn(int bID,int value);
	int ledOn(int bID);	
	void StartMargin(int bID,int value);
	int StartMargin(int bID);
	void EndMargin(int bID,int value);
	int EndMargin(int bID);
	void GoalPosition(int bID,int value);
	int GoalPosition(int bID);
	void GoalSpeed(int bID,int value);
	int GoalSpeed(int bID);
	void SpeedLimit(int bID,int value);
	int SpeedLimit(int bID);
	void GoalCurrent(int bID,int value);
	int GoalCurrent(int bID);	
	void CurrentLimit(int bID,int value);
	int CurrentLimit(int bID);	
	int presentPosition(int bID);
	int presentOperatingRate(int bID);
	int presentTemperature(int bID);
	int Moving(int bID);
	void Lock(int bID,int value);
	int Lock(int bID);
	void Acceleration(int bID, int value);
	int Acceleration(int bID);
	void Deceleration(int bID, int value);
	int Deceleration(int bID);

	
	void initPacket(int bID, int bInst);
	void pushByte(int value);
	int flushPacket(void);
	void pushParam(byte value);
	void pushParam(int value);
	

	/*
	 * Utility methods for MightyZap
	 */


private:
void printBuffer(int *bpPrintBuffer, int bLength);
int Dummy(int tmp);
	void uDelay(int uTime);
	void nDelay(int nTime);
	void irpTxEnable(void);
	void irpTxDisable(void);
	void clearBuffer(void);
	int checkPacketType(void);
	int MightyZap_Serial_Type;
	Stream  *MightyZap_Serial;
	//HardwareSerial  *MightyZap_Serial;
	//SoftwareSerial  *MightyZap_Serial;
	int MightyZap_DirPin;
	bool MightyZap_DirPin_Level_Tx;
	bool MightyZap_DirPin_Level_Rx;
	int mRxBuffer[IRP_RX_BUF_SIZE];
	int mTxBuffer[IRP_RX_BUF_SIZE];
	int mParamBuffer[IRP_PARAMETER_BUF_SIZE];
	int mBusUsed;
	int mRxLength;

	// additions to return proper COMM_* status
	int mIRPtxrxStatus;
	// additions to permit non-default Status Return Level settings without returning errors
	int gbIRPStatusReturnLevel;
	// additions to adjust number of txrx attempts
	int gbIRPNumberTxRxAttempts;

	int mPacketType;  

	int mPktIdIndex;
	int mPktLengthIndex;
	int mPktInstIndex;
	int mPktErrorIndex;
	//int mRxLengthOffset;

	int mbLengthForPacketMaking;
	int mbIDForPacketMaking;
	int mbInstructionForPacketMaking;
	int mCommStatus;

	int SmartDelayFlag;
};


#endif /* MightyZap_H_ */
