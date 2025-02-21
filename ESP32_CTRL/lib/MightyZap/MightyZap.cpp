/*
 * MightyZap.cpp
 *
 *  Created on: 2016 12. 28.
 *      Author: BG. Shim
 */

#include "Arduino.h"
#include "MightyZap.h"

#include <SoftwareSerial.h>

MightyZap::MightyZap(HardwareSerial  *dev_serial, int DirectionPin) {
MightyZap_Serial_Type=0;
	MightyZap_Serial=dev_serial;
	MightyZap_DirPin=DirectionPin;
	MightyZap_DirPin_Level_Tx = HIGH;//tx
	MightyZap_DirPin_Level_Rx = LOW;
}



MightyZap::MightyZap(SoftwareSerial  *dev_serial, int DirectionPin) {
  //SoftwareSerial mySerial(receivePin, transmitPin); // RX, TX
  MightyZap_Serial_Type=1;
  MightyZap_Serial=dev_serial;
  MightyZap_DirPin=DirectionPin;
  MightyZap_DirPin_Level_Tx = HIGH;//tx
  MightyZap_DirPin_Level_Rx = LOW;
}

MightyZap::MightyZap(HardwareSerial  *dev_serial, int DirectionPin,bool TxLevel) {
	MightyZap_Serial_Type=0;
	MightyZap_Serial=dev_serial;
	MightyZap_DirPin=DirectionPin;
	MightyZap_DirPin_Level_Tx = TxLevel;//tx
	MightyZap_DirPin_Level_Rx = !TxLevel;
}



MightyZap::MightyZap(SoftwareSerial  *dev_serial, int DirectionPin,bool TxLevel) {
	//SoftwareSerial mySerial(receivePin, transmitPin); // RX, TX
	MightyZap_Serial_Type=1;
	MightyZap_Serial=dev_serial;
	MightyZap_DirPin=DirectionPin;
	MightyZap_DirPin_Level_Tx = TxLevel;//tx
	MightyZap_DirPin_Level_Rx = !TxLevel;
}

MightyZap::~MightyZap() {
	// TODO Auto-generated destructor stub
}
void MightyZap::begin(int baud){

	pinMode(MightyZap_DirPin, OUTPUT);
	if(MightyZap_Serial_Type)
	((SoftwareSerial *)MightyZap_Serial)->begin(irp_get_baudrate(baud));
	else
	((HardwareSerial *)MightyZap_Serial)->begin(irp_get_baudrate(baud));

	digitalWrite( MightyZap_DirPin, MightyZap_DirPin_Level_Tx);// TX Enable

	mIRPtxrxStatus = 0;
	mBusUsed = 0;// only 1 when tx/rx is operated
	SmartDelayFlag=1;
	this->setPacketType();
	
}


int MightyZap::readRaw(void){
int temp=0;
if (MightyZap_Serial->available()) {
	temp = MightyZap_Serial->read();
	}
	return temp;
}
void MightyZap::writeRaw(int value){

MightyZap_Serial->write(value);
}

/*
 * @brief : if data coming from irp bus, returns 1, or if not, returns 0.
 *
 */
int MightyZap::available(void){

		return MightyZap_Serial->available();
}
void MightyZap::irpTxEnable(void){

	digitalWrite( MightyZap_DirPin, MightyZap_DirPin_Level_Tx );// RX Disable

}
void MightyZap::irpTxDisable(void){

	digitalWrite( MightyZap_DirPin, MightyZap_DirPin_Level_Rx );// RX Enable

}

void MightyZap::clearBuffer(void){
	while((this->available()))
	{
		MightyZap_Serial->read();
	}
}

void MightyZap::setPacketType(){

		mPktIdIndex = 3; //2;
		mPktLengthIndex = 4; //3;
		mPktInstIndex = 5; //4;
		mPktErrorIndex = 5; //4;

}
int MightyZap::getPacketType(void){
	return mPacketType;
}
int MightyZap::checkPacketType(void){

	return 0;//default is 1.0 protocol IRP_PACKET_TYPE1
}


int MightyZap::getTxRxStatus(void)
{
	return mIRPtxrxStatus;
}

/*
 * Use getTxRxStatus() instead of getResult()
 * */
 
int  MightyZap::getResult(void){
	//	return mCommStatus;
	return this->getTxRxStatus();
}



int MightyZap::txPacket(int bID, int bInstruction, int bParameterLength){

    int bCount,bCheckSum,bPacketLength;
	
	//int bCount,bCheckSum,bPacketLength;

    int offsetParamIndex;
   
	mTxBuffer[0] = 0xff;
	mTxBuffer[1] = 0xff;
	mTxBuffer[2] = 0xff; //
	mTxBuffer[3] = bID;  //[2]
	mTxBuffer[4] = bParameterLength+2; //[3] //2(int) <- instruction(1int) + checksum(1int) 
	mTxBuffer[5] = bInstruction; //[4]

	offsetParamIndex = 6; //5
	bPacketLength = bParameterLength+3+4; //+2+4;

  
    //copy parameters from mParamBuffer to mTxBuffer
    for(bCount = 0; bCount < bParameterLength; bCount++)
    {
    	mTxBuffer[bCount+offsetParamIndex] = mParamBuffer[bCount];
    }

	// chech sum
    bCheckSum = 0;
    for(bCount = 3; bCount < bPacketLength-1; bCount++){ //except 0xff,checksum //bCount = 2;
		bCheckSum += mTxBuffer[bCount];
	}
    mTxBuffer[bCount] = ~bCheckSum; //Writing Checksum with Bit Inversion
  
    
    this->irpTxEnable(); // this define is declared in irp.h

    for(bCount = 0; bCount < bPacketLength; bCount++)
    {
        writeRaw(mTxBuffer[bCount]);
    }
	flushPacket();
	//delay(1);
    this->irpTxDisable();// this define is declared in irp.h

    return(bPacketLength); // return packet length
}
int MightyZap::rxPacket(int bRxLength){

	unsigned long ulCounter, ulTimeLimit;
	int bCount, bLength, bChecksum;
	//int bCount, bLength, bChecksum; 

	int bTimeout;

	//#if false
	bTimeout = 0;
	if(bRxLength == 255 || bRxLength == 0xffff) 
		ulTimeLimit = RX_TIMEOUT_COUNT1;
	else
		ulTimeLimit = RX_TIMEOUT_COUNT2;
	for(bCount = 0; bCount < bRxLength; bCount++)
	{
		ulCounter = 0;
		while(!( MightyZap_Serial->available()))
		{
			nDelay(NANO_TIME_DELAY); 
			if(ulCounter++ > ulTimeLimit)
			{
				bTimeout = 1;
				break;
			}
			uDelay(0); 
		}
		if(bTimeout) break;
		mRxBuffer[bCount] = this->readRaw();  // get packet data from USART device

	}
	//Serial.println(" ");
	bLength = bCount;
	bChecksum = 0;
	if( mTxBuffer[mPktIdIndex] != IRP_BROADCAST_ID )
	{
	
		if(bTimeout && bRxLength != 255)
		{
				
			mIRPtxrxStatus |= (1<<COMM_RXTIMEOUT);
			clearBuffer();			
			return 0;
		}
		
		if(bLength > 3) //checking available length.
		{
			
			if(mRxBuffer[0] != 0xff || mRxBuffer[1] != 0xff || mRxBuffer[2] != 0xff ) mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXHEADER); //|| mRxBuffer[2] != 0xff
			else if(mRxBuffer[mPktIdIndex] != mTxBuffer[mPktIdIndex] ) mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXID);
			else if(mRxBuffer[mPktLengthIndex] != bLength-mPktInstIndex) mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXLENGTH);
			else{
				for(bCount = 3; bCount < bLength; bCount++){ //bCount = 2
					bChecksum += mRxBuffer[bCount]; //Calculate checksum of received data for compare
				}
				if(bChecksum != 0xff) mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXCHECKSUM);
				//return 0;
			}
			
			
			
			if(mRxBuffer[0] != 0xff || mRxBuffer[1] != 0xff || mRxBuffer[2] != 0xff ){ //|| 
				mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXHEADER);
				clearBuffer();
				return 0;
			}
			

			if(mRxBuffer[mPktIdIndex] != mTxBuffer[mPktIdIndex] )  //id check
			{
					
				mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXID);
				clearBuffer();
				return 0;
			}

			if(mRxBuffer[mPktLengthIndex] != bLength-mPktInstIndex) // status packet length check
			{			
						
				mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXLENGTH);
				clearBuffer();
				return 0;
			}
	
			for(bCount = 3; bCount < bLength; bCount++){ //bCount = 2
				bChecksum += mRxBuffer[bCount]; //Calculate checksum of received data for compare
			}

			bChecksum &= 0xff;

			if(bChecksum != 0xff)
			{			
				//TxDStringC("[RxChksum Error]");			
				mIRPtxrxStatus |= (1<<COMM_RXCORRUPT);//RXCHECKSUM);
				clearBuffer();
				return 0;
			}
		//end of checksum
		}//(bLength > 3)
	}//end of Rx status packet check

	return bLength;
}


int MightyZap::txRxPacket(int bID, int bInst, int bTxParaLen){
//#if false
	mIRPtxrxStatus = 0;

	int bTxLen, bRxLenEx, bTryCount;

	mBusUsed = 1;
	mRxLength = bRxLenEx = bTxLen = 0;

//	for(bTryCount = 0; bTryCount < gbIRPNumberTxRxAttempts; bTryCount++)
	for(bTryCount = 0; bTryCount < 1; bTryCount++)
	{
		while((this->available())){
			MightyZap_Serial->read();
		}

		/**************************************   Transfer packet  ***************************************************/
		bTxLen = this->txPacket(bID, bInst, bTxParaLen);
		
		if (bTxLen == (bTxParaLen+4+3))	mIRPtxrxStatus = (1<<COMM_TXSUCCESS); //+4+2
				
		if(bInst == CMD_PING){		
			if(bID == IRP_BROADCAST_ID)	mRxLength = bRxLenEx = 0xff;
			else mRxLength = bRxLenEx = 7; //6; // basic response packet length			
		}
		else if(bInst == CMD_READ){
			mRxLength = bRxLenEx = 7+mParamBuffer[1]; //6+
		}
		else if( bID == IRP_BROADCAST_ID ){
			if(bInst == CMD_SYNC_READ || bInst == CMD_BULK_READ) mRxLength = bRxLenEx = 0xffff; //only 2.0 case
			else mRxLength = bRxLenEx = 0; // no response packet
		}
		else{
			if (gbIRPStatusReturnLevel>1){
				if(mPacketType == IRP_PACKET_TYPE1) mRxLength = bRxLenEx = 7; //6 //+mParamBuffer[1];
				else mRxLength = bRxLenEx = 11;
			}
			else{
				mRxLength = bRxLenEx = 0;
			}
		}


		if(bRxLenEx){
			if(SmartDelayFlag == 1)
				delay(150);
			/**************************************   Receive packet  ***************************************************/
			mRxLength = this->rxPacket(bRxLenEx);

		}//bRxLenEx is exist
	} //for() gbIRPNumberTxRxAttempts

	mBusUsed = 0;
	
	if((mRxLength != bRxLenEx) && (mTxBuffer[mPktIdIndex] != IRP_BROADCAST_ID))
	{
		
	//	return 0;
	}else if((mRxLength == 0) && (mTxBuffer[mPktInstIndex] == CMD_PING)){ 
		return 0;
	}
	

	mIRPtxrxStatus = (1<<COMM_RXSUCCESS);

	//gbLengthForPacketMaking =0;
//	#endif
	return 1;
}


int MightyZap::Dummy(int tmp){
	return tmp;
}
void MightyZap::uDelay(int uTime){
	int cnt, max;
		static int tmp = 0;

		for( max=0; max < uTime; max++)
		{
			for( cnt=0; cnt < 10 ; cnt++ )
			{
				tmp +=Dummy(cnt);
			}
		}
		//tmpdly = tmp;
}
void MightyZap::nDelay(int nTime){
	int cnt, max;
		cnt=0;
		static int tmp = 0;

		for( max=0; max < nTime; max++)
		{
			//for( cnt=0; cnt < 10 ; cnt++ )
			//{
				tmp +=Dummy(cnt);
			//}
		}
		//tmpdly = tmp;
}


int  MightyZap::ping(int  bID ){

	if(this->txRxPacket(bID, CMD_PING, 0)){
		if(mPacketType == IRP_PACKET_TYPE1) return (mRxBuffer[3]); //1.0
		else return IRP_MAKEint(mRxBuffer[9],mRxBuffer[10]); 
	}else{
		return 0xff;  //no irp in bus.
	}

}

int  MightyZap::writeByte(int bID, int bAddress, int bData){
	int param_length = 0;

	mParamBuffer[0] = bAddress;
	mParamBuffer[1] = bData;
	param_length = 2;
	
	return this->txRxPacket(bID, CMD_WRITE, param_length);
}

int MightyZap::readByte(int bID, int bAddress){
	this->clearBuffer();
	
	mParamBuffer[0] = bAddress;
	mParamBuffer[1] = 1;
	if( this->txRxPacket(bID, CMD_READ, 2 )){
		mCommStatus = 1;
		return(mRxBuffer[6]);// [5] //refer to 1.0 packet structure
	}
	else{
		mCommStatus = 0;
		return 0xff;
	}
}



int MightyZap::writeint(int bID, int bAddress, short wData){
    int param_length = 0;
    this->clearBuffer();

	mParamBuffer[0] = bAddress;
	mParamBuffer[1] = IRP_LOBYTE(wData);//(int)(wData&0xff);
	mParamBuffer[2] = IRP_HIBYTE(wData);//(int)((wData>>8)&0xff);
	param_length = 3;
	
	return this->txRxPacket(bID, CMD_WRITE, param_length);

}



int MightyZap::readint(int bID, int bAddress){
	this->clearBuffer();
	
	mParamBuffer[0] = bAddress;
	mParamBuffer[1] = 2;
	if(this->txRxPacket(bID, CMD_READ, 2)){
		return IRP_MAKEint(mRxBuffer[6],mRxBuffer[7]);//( (((int)mRxBuffer[6])<<8)+ mRxBuffer[5] );
	}
	else{
		return 0xffff;
	}	
}


void MightyZap::setTxPacketId(int id){
	mbIDForPacketMaking = id;

}
void MightyZap::setTxPacketInstruction(int instruction){
	mbInstructionForPacketMaking = instruction;

}
void MightyZap::setTxPacketParameter( int index, int value ){
	mParamBuffer[index] = value;

}
void MightyZap::setTxPacketLength( int length ){
	mbLengthForPacketMaking = length;

}
int MightyZap::txrxPacket(void){
	mCommStatus = this->txRxPacket(mbIDForPacketMaking, mbInstructionForPacketMaking, mbLengthForPacketMaking);
	return mCommStatus;
}

int MightyZap::getRxPacketParameter( int index ){
	//return irp_get_rxpacket_parameter( index );
	return mRxBuffer[6 + index]; //5
}
int MightyZap::getRxPacketLength(void){
	//return irp_get_rxpacket_length();
	return mRxBuffer[4]; //length index is 3 in status packet //3
}
/****************************************************************/
/* Model Number													*/
/* Type : Read Only												*/
/**********************************************Version******************/

// Data memory Map
int MightyZap::getModelNumber(int bID){
	return this->readint(bID, 0);
}
/****************************************************************/
/* Model Virsion												*/
/* Type : Read Only												*/
/****************************************************************/
int MightyZap::Version(int bID){
	return this->readByte(bID, 2);
}
/****************************************************************/
/* ServoID														*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::ServoID(int bID, int new_ID){
	this->writeByte(bID, 3, new_ID);
}
int MightyZap::ServoID(int bID){
	return this->readByte(bID, 3);
}
/****************************************************************/
/* BaudRate														*/
/*  16 : 115200													*/
/*  32 :  57600													*/
/*  64 :  19200													*/
/* 128 :   9600													*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::BaudRate(int bID, int baud_num){
	this->writeByte(bID, 4, baud_num);
}
int MightyZap::BaudRate(int bID){
	return this->readByte(bID, 4);
}

/****************************************************************/
/* Strok Limit                                                  */
/* Dir : Stroke.Short, Stroke.Long								*/
/* Short Strok Adrress 0x06~0x07								*/
/* Long Strok Adrress 0x06~0x07									*/
/****************************************************************/

// Short Stroke Limit
void MightyZap::ShortStrokeLimit(int bID, int position){
	this->writeint(bID, 6, position);
}
int MightyZap::ShortStrokeLimit(int bID){
	return this->readint(bID, 6);
}
// Long Stroke Limit
void MightyZap::LongStrokeLimit(int bID, int position){
	this->writeint(bID, 8, position);
}
int MightyZap::LongStrokeLimit(int bID){
	return this->readint(bID, 8);
}




/****************************************************************/
/* Alarm LED													*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::alarmLed(int bID, int value){	
	this->writeByte(bID, 17, value);		
}
int MightyZap::alarmLed(int bID){
	return this->readByte(bID, 17);
}
/****************************************************************/
/* Alarm Shutdown												*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::alarmShutdown(int bID,int option){
	this->writeByte(bID, 18, option);
}
int MightyZap::alarmShutdown(int bID){
	return this->readByte(bID, 18);
}

// Parameter Map
/****************************************************************/
/* Force Enable													*/
/* Type : Read/write											*/
/****************************************************************/
void MightyZap::forceEnable(int bID,int value){
	this->writeByte(bID, 0x80, value);
}
int MightyZap::forceEnable(int bID){
	return this->readByte(bID, 0x80);
}

/****************************************************************/
/* LED On/Off													*/
/* Type : Read/write											*/
/****************************************************************/
void MightyZap::ledOn(int bID,int value){
	this->writeByte(bID, 0x81, value);//2 4 8
}
int MightyZap::ledOn(int bID){
	return this->readByte(bID, 0x81);
}

/****************************************************************/
/* Start Compliance Margin										*/
/* Type : Read/write											*/
/****************************************************************/
void MightyZap::StartMargin(int bID, int value){	
	this->writeByte(bID, 0x13, value);	
}
int MightyZap::StartMargin(int bID){	
	return this->readByte(bID, 0x13);
}
/****************************************************************/
/* End Compliance Margin										*/
/* Type : Read/write											*/
/****************************************************************/
void MightyZap::EndMargin(int bID, int value){	
	this->writeByte(bID, 0x14, value);	
}
int MightyZap::EndMargin(int bID){	
	return this->readByte(bID, 0x14);
}

/****************************************************************/
/* Goal Position												*/
/* Type : Read/write											*/
/****************************************************************/
void MightyZap::GoalPosition(int bID,int value){	
	this->writeint(bID, 0x86, value);	
}
int MightyZap::GoalPosition(int bID){	
	return this->readint(bID, 0x86);
}
/****************************************************************/
/* Goal Speed													*/
/* Type : Read/write											*/
/****************************************************************/
void MightyZap::GoalSpeed(int bID,int value){	
	this->writeint(bID, 0x88, value);	
}
int MightyZap::GoalSpeed(int bID){	
	return this->readint(bID, 0x88);
}
void MightyZap::SpeedLimit(int bID,int value){	
	this->writeint(bID, 0x15, value);	
}
int MightyZap::SpeedLimit(int bID){	
	return this->readint(bID, 0x15);
}
/****************************************************************/
/* Goal Current													*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::GoalCurrent(int bID, int value){	
	this->writeint(bID, 0x8a, value);	
}
int MightyZap::GoalCurrent(int bID){	

	return this->readint(bID, 0x8a);		
}
void MightyZap::CurrentLimit(int bID, int value){	
	this->writeint(bID, 0x34, value);	
}
int MightyZap::CurrentLimit(int bID){	

	return this->readint(bID, 0x34);		
}
/****************************************************************/
/* Acceleration													*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::Acceleration(int bID, int value){	
	this->writeByte(bID, 0x21, value);	
}
int MightyZap::Acceleration(int bID){	

	return this->readByte(bID, 0x21);		
}
/****************************************************************/
/* Deceleration													*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::Deceleration(int bID, int value){	
	this->writeByte(bID, 0x22, value);	
}
int MightyZap::Deceleration(int bID){	

	return this->readByte(bID, 0x22);		
}




/****************************************************************/
/* Present Position 											*/
/* Type : Read Only												*/
/****************************************************************/
int MightyZap::presentPosition(int bID){	
	return this->readint(bID, 0x8c);

}
/****************************************************************/
/* Present OperatingRate	 												*/
/* Type : Read Only												*/
/****************************************************************/
int MightyZap::presentOperatingRate(int bID){	
	return this->readint(bID, 0x90);
}


/****************************************************************/
/* Present Temperature											*/
/* Type : Read Only												*/
/****************************************************************/
int MightyZap::presentTemperature(int bID){	
	return this->readByte(bID, 0x93);
}

/****************************************************************/
/* Moving														*/
/* Type : Read Only												*/
/****************************************************************/
int MightyZap::Moving(int bID){	
	return this->readByte(bID, 0x96);
}
/****************************************************************/
/* Lock															*/
/* Type : Read/Write											*/
/****************************************************************/
void MightyZap::Lock(int bID,int value){	
	this->writeByte(bID, 0x97, value);	
}

/*
 * @brief initialize parameter and get ID, instruction for making packet
 * */
void MightyZap::initPacket(int bID, int bInst){
	mbLengthForPacketMaking = 0;
	mbIDForPacketMaking = bID;
	mbInstructionForPacketMaking = bInst;
	mCommStatus = 0;
}
/*
 * @brief just push parameters, individual ID or moving data, individual data length
 * */
void MightyZap::pushByte(int value){
	
	if(mbLengthForPacketMaking > 255)//prevent violation of memory access
		return;
	mParamBuffer[mbLengthForPacketMaking++] = value;
}
void MightyZap::pushParam(byte value){
	if(mbLengthForPacketMaking > 255)//prevent violation of memory access
			return;
	mParamBuffer[mbLengthForPacketMaking++] = value;
}
void MightyZap::pushParam(int value){

	if(mbLengthForPacketMaking > 255)//prevent violation of memory access
			return;
	mParamBuffer[mbLengthForPacketMaking++] = (unsigned char)IRP_LOBYTE(value);
	mParamBuffer[mbLengthForPacketMaking++] = (unsigned char)IRP_HIBYTE(value);
}
/*
 * @brief transfers packets to MightyZap bus
 * */
int MightyZap::flushPacket(void){

	//TxDString("\r\n");
	//TxD_Dec_U8(gbLengthForPacketMaking);
	MightyZap_Serial->flush();             // Waiting to transmit packet
	return 0;
}
/*
 * @brief return current the total packet length
 * */

