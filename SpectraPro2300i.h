#ifndef SpectraPro2300i_h	//check for multiple inclusions
#define SpectraPro2300i_h

#include "Arduino.h"

typedef void ( *FinishedListener )();

class SpectraPro2300i
{
	public:
		enum class CommandsType : uint8_t {GoWave, Grat, Wave, Count, Error};
		enum class CommandValueType : uint8_t {None, Integer, Float, String, Count};
		enum class TransmissionType : uint8_t {Get, Set, Error, None, Count};
		enum class ReplyValueType : uint8_t {None, Integer, Float, String, Error, Count};
		enum class RecievingPart : uint8_t {Echo, Value, Unit, Status, Count};
		union CommandValue
		{
			uint8_t IntegerValue;
			bool BoolValue;
			float FloatValue;
			char* StringPointer;
		};
		union ReplyValue
		{
			uint8_t IntegerValue;
			bool BoolValue;
			float FloatValue;
			char* StringPointer;
		};
		struct CommandStringList
		{
			CommandsType Command;
			CommandValueType ValueType;
			const char* String;
			uint8_t Count;
			bool ReplyHasUnits;
		};
		struct Command
		{
			CommandsType Command;
			TransmissionType Type;
			CommandValue Value;
			CommandValueType ValueType;
			bool ReplyHasUnits;
		};
		struct Reply
		{
			CommandsType Command;
			TransmissionType Type;
			ReplyValue Value;
			ReplyValueType ValueType;
		};
		struct ReplyReturn
		{
			ReplyValue Value;
			ReplyValueType ValueType;
		};
		SpectraPro2300i(HardwareSerial* serial); //Invoke with CS260(&SerialN);
		~SpectraPro2300i();
		bool SendSetWavelength(float Wavelength);
		bool SendGetWavelength();
		bool SendSetGrating(uint8_t GratingNumber);
		bool SendGetGrating();
		bool SetRecievedCallback(FinishedListener Finished);
		bool IsBusy();
		void CheckSerial();
		float GetCurrentWavelength();
	private:
		HardwareSerial* _HardwareSerial;
		RecievingPart CurrentRecievingPart;
		FinishedListener RecievedCallback;
		Command CurrentCommand;
		Reply CurrentReply;
		uint32_t TransmitTime;
		uint8_t ReplyBufferIndex;
		uint8_t CommandRetries;
		uint8_t WavelengthSetRetries;
		bool PollingWavelength;
		bool Busy;
		bool ExpectReply;
		char* ReplyBuffer;
		char* IntFloatBuffer;
		float CurrentWavelength;
		bool CheckWavelength;
		uint8_t IntToCharPointer(uint8_t Input, char* Buffer, size_t BufferSize);
		uint8_t FloatToCharPointer(float Input, char* Buffer, size_t BufferSize);
		uint8_t CharPointerToInt(char* Buffer, size_t BufferSize);
		float CharPointerToFloat(char* Buffer, size_t BufferSize);
		void ParseEcho(char Character);
		void ParseValue(char Character);
		void ParseUnit(char Character);
		void ParseStatus(char Character);
		void CheckReply();
		bool SendCurrentCommand();
		void SetupForSending(bool ResetWavelengthSetRetries);
		bool AboutEqual(float x, float y);
		static const CommandStringList CommandLibrary[];
		static const char CarriageReturnCharacter;
		static const char EndOfLineCharacter;
		static const char SpaceCharacter;
		static const char GetCharacter;
		static const char PeriodCharacter;
		static const char* const WavelengthUnits;
		static const char* const OKAcknowledgement;
		static const uint32_t TimeOut;
		static const uint8_t MaxCommandRetries;
		static const uint8_t MaxWavelengthSetRetires;
		static const float WavelengthMin;
		static const float WavelengthMax;
};
#endif