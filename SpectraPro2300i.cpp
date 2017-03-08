#include "SpectraPro2300i.h"

#define ReplyBufferSize 16
#define IntFloatBufferSize 16
#define AlmostEqualDifference 0.2f

const char SpectraPro2300i::CarriageReturnCharacter = '\r';
const char SpectraPro2300i::EndOfLineCharacter = '\n';
const char SpectraPro2300i::SpaceCharacter = ' ';
const char SpectraPro2300i::GetCharacter = '?';
const char SpectraPro2300i::PeriodCharacter = '.';
const uint32_t SpectraPro2300i::TimeOut = 10000000;
const uint8_t SpectraPro2300i::MaxCommandRetries = 8;
const uint8_t SpectraPro2300i::MaxWavelengthSetRetires = 8;
const float SpectraPro2300i::WavelengthMin = 190;
const float SpectraPro2300i::WavelengthMax = 1000;
const char* const SpectraPro2300i::WavelengthUnits = "nm";
const char* const SpectraPro2300i::OKAcknowledgement = "ok";
const SpectraPro2300i::CommandStringList SpectraPro2300i::CommandLibrary[] =
{
	{CommandsType::GoWave,CommandValueType::Float,"GOTO",4,false},
	{CommandsType::Grat,CommandValueType::Integer,"GRATING",7,false},
	{CommandsType::Wave,CommandValueType::Float,"NM",4,true} //Need to poll wavelength for this must be false, or endless loop occurs.
};
SpectraPro2300i::SpectraPro2300i(HardwareSerial *serial)
{
	_HardwareSerial = serial;
	CurrentRecievingPart = RecievingPart::Echo;
	ReplyBuffer = new char[ReplyBufferSize]();
	IntFloatBuffer = new char[IntFloatBufferSize]();
	RecievedCallback = 0;
	ReplyBufferIndex = 0;
	CommandRetries = 0;
	TransmitTime = 0;
	CurrentWavelength = 0.0;
	WavelengthSetRetries = 0;
	Busy = false;
	ExpectReply = false;
}
SpectraPro2300i::~SpectraPro2300i()
{

}
void SpectraPro2300i::SetupForSending(bool ResetWavelengthSetRetries)
{
	Busy = true;
	CommandRetries = 0;
	if (ResetWavelengthSetRetries)
	{
		WavelengthSetRetries = 0;
	}
}
bool SpectraPro2300i::SendSetWavelength(float Wavelength)
{
	if (!Busy)
	{
		SetupForSending(false);
		Wavelength = constrain(Wavelength, WavelengthMin, WavelengthMax);
		CurrentWavelength = Wavelength;
		uint8_t Index = static_cast<uint8_t>(CommandsType::GoWave);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.ReplyHasUnits = CommandLibrary[Index].ReplyHasUnits;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.FloatValue = Wavelength;
		CurrentCommand.ValueType = CommandValueType::Float;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool SpectraPro2300i::SendGetWavelength()
{
	if (!Busy)
	{
		SetupForSending(false);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Wave);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.ReplyHasUnits = CommandLibrary[Index].ReplyHasUnits;
		CurrentCommand.Type = TransmissionType::Get;
		CurrentCommand.ValueType = CommandValueType::Float;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool SpectraPro2300i::SendSetGrating(uint8_t GratingNumber)
{
	if (!Busy)
	{
		SetupForSending(true);
		GratingNumber = constrain(GratingNumber, 1, 3);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Grat);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.ReplyHasUnits = CommandLibrary[Index].ReplyHasUnits;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.IntegerValue = GratingNumber;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool SpectraPro2300i::SendGetGrating()
{
	if (!Busy)
	{
		SetupForSending(true);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Grat);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.ReplyHasUnits = CommandLibrary[Index].ReplyHasUnits;
		CurrentCommand.Type = TransmissionType::Get;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool SpectraPro2300i::SetRecievedCallback(FinishedListener Finished)
{
	if (!Finished)
	{
		return false;
	}
	RecievedCallback = Finished;
	return true;
}
bool SpectraPro2300i::SendCurrentCommand()
{
	const uint8_t CommandIndex = static_cast<uint8_t>(CurrentCommand.Command);
	const uint8_t* StringToSend = reinterpret_cast<const uint8_t*>(CommandLibrary[CommandIndex].String);
	const uint8_t CharsToSend = CommandLibrary[CommandIndex].Count;
	if (CurrentCommand.Type == TransmissionType::Get)
	{
		_HardwareSerial->write(GetCharacter);
		//Serial.print(GetCharacter);
		switch (CurrentCommand.ValueType)
		{
			case CommandValueType::Float:
				CurrentReply.ValueType = ReplyValueType::Float;
				break;
			case CommandValueType::Integer:
				CurrentReply.ValueType = ReplyValueType::Integer;
				break;
			default:
				CurrentReply.ValueType = ReplyValueType::None;
				break;
		}
	}
	else if (CurrentCommand.Type == TransmissionType::Set)
	{
		if (CurrentCommand.ValueType == CommandValueType::Integer)
		{
			int Count = IntToCharPointer(CurrentCommand.Value.IntegerValue, IntFloatBuffer, IntFloatBufferSize);
			_HardwareSerial->write(reinterpret_cast<const uint8_t*>(IntFloatBuffer),Count);
			CurrentReply.ValueType = ReplyValueType::Integer;
			//Serial.print(IntFloatBuffer);
		}
		else if (CurrentCommand.ValueType == CommandValueType::Float)
		{
			int Count = FloatToCharPointer(CurrentCommand.Value.FloatValue, IntFloatBuffer, IntFloatBufferSize);
			_HardwareSerial->write(reinterpret_cast<const uint8_t*>(IntFloatBuffer),Count);
			CurrentReply.ValueType = ReplyValueType::Float;
			//Serial.print(IntFloatBuffer);
		}
		else
		{
			return false;
		}
		_HardwareSerial->write(SpaceCharacter);
		//Serial.print(SpaceCharacter);
	}
	else if (CurrentCommand.Type == TransmissionType::None)
	{

	}
	else
	{
		return false;
	}
	_HardwareSerial->write(StringToSend, CharsToSend);
	//Serial.print("M:");
	//Serial.print(String((char*)StringToSend));
	_HardwareSerial->write(CarriageReturnCharacter);
	//_HardwareSerial->write(EndOfLineCharacter);
	_HardwareSerial->flush();
	//Serial.print(EndOfLineCharacter);
	//Serial.print(CarriageReturnCharacter);
	TransmitTime = micros();
	ReplyBufferIndex = 0;
	CurrentReply.Type = CurrentCommand.Type;
	if (CurrentCommand.Type == TransmissionType::Set)
	{
		CurrentRecievingPart = RecievingPart::Value;
	}
	else
	{
		CurrentRecievingPart = RecievingPart::Echo;
	}
	ExpectReply = true;
	return true;
}
uint8_t SpectraPro2300i::IntToCharPointer(uint8_t Input, char* Buffer, size_t BufferSize)
{
	memset(Buffer,0,BufferSize);
	return sprintf(Buffer, "%u", Input);
}
uint8_t SpectraPro2300i::FloatToCharPointer(float Input, char* Buffer, size_t BufferSize)
{
	memset(Buffer,0,BufferSize);
	return sprintf(Buffer, "%1.3f", Input);
}
uint8_t SpectraPro2300i::CharPointerToInt(char* Buffer, size_t BufferSize)
{
	Buffer[BufferSize-1] = '\0';
	return (uint8_t) atoi(Buffer);
}
float SpectraPro2300i::CharPointerToFloat(char* Buffer, size_t BufferSize)
{
	Buffer[BufferSize-1] = '\0';
    float sign = 1.0;
    float value;
    while (isblank(*Buffer) )
    {
        Buffer++;
    }
    if (*Buffer == '-')
    {
        sign = -1.0;
        Buffer++;
    }
    for (value = 0.0; isdigit(*Buffer); Buffer++)
    {
        value = value * 10.0 + (*Buffer - '0');
    }
    if (*Buffer == '.')
    {
        double pow10 = 10.0;
        Buffer++;
        while (isdigit(*Buffer))
        {
            value += (*Buffer - '0') / pow10;
            pow10 *= 10.0;
            Buffer++;
        }
    }
    return sign * value;
}
void SpectraPro2300i::CheckSerial()
{
	//if (_HardwareSerial->available() > 0)
	//{
	//	Serial.print((char)_HardwareSerial->peek());
	//}
	if (ExpectReply)
	{
		uint32_t TimeDifference = micros()  - TransmitTime;
		if(_HardwareSerial->available() > 0)
		{
			char Character = (char)_HardwareSerial->read();
			//Serial.print(Character);
			switch (CurrentRecievingPart)
			{
				case RecievingPart::Echo:
					//Serial.print("e");
					//Serial.print(Character);
					ParseEcho(Character);
					break;
				case RecievingPart::Value:
					//Serial.print("v");
					//Serial.print(Character);
					ParseValue(Character);
					break;
				case RecievingPart::Unit:
					//Serial.print("u");
					//Serial.print(Character);
					ParseUnit(Character);
					break;
				case RecievingPart::Status:
					//Serial.print("s");
					//Serial.print(Character);
					ParseStatus(Character);
					break;
				default:
					break;
			}
		}
		else if ( ( (CurrentRecievingPart != RecievingPart::Status) && (TimeDifference > TimeOut) ) || ((TimeDifference > 10*TimeOut)) )
		{
			Serial.println("Timeout on monochromator.");
			CurrentReply.Command = CommandsType::Error;
			CurrentReply.Type = TransmissionType::Error;
			CurrentReply.ValueType = ReplyValueType::Error;
			ReplyBufferIndex = 0;
			CurrentRecievingPart = RecievingPart::Echo;
			CheckReply();
		}
	}
	else
	{
		if(_HardwareSerial->available() > 0)
		{
			//while (_HardwareSerial->available() > 0)
			//{
			//	Serial.print("~");
			//	Serial.print((char)_HardwareSerial->read());
			//}
			_HardwareSerial->clear();
		}
	}
}
void SpectraPro2300i::ParseEcho(char Character)
{
	//Serial.print(Character);
	if (Character == GetCharacter)
	{
		CurrentReply.Type = TransmissionType::Get;
	}
	else if (isalpha(Character))
	{
		ReplyBuffer[ReplyBufferIndex] = Character;
		if (ReplyBufferIndex < (ReplyBufferSize-2) )
		{
			ReplyBufferIndex++;
		}
	}
	else if ( (Character == SpaceCharacter) )
	{
		if (ReplyBufferIndex > 0)
		{
			bool FoundCommand = false;
			ReplyBuffer[ReplyBufferIndex]='\0';
			//Serial.print("!B:");
			//Serial.print(ReplyBuffer);
			//Serial.print(":");
			for (uint8_t Index = 0; Index < static_cast<uint8_t>(CommandsType::Count); Index++)
			{
				if (strcmp(ReplyBuffer,CommandLibrary[Index].String)==0)
				{
					FoundCommand = true;
					CurrentReply.Command = CommandLibrary[Index].Command;
					break;
				}
			}
			if (CurrentReply.Type == TransmissionType::Get)
			{
				CurrentRecievingPart = RecievingPart::Value;
			}
			else
			{
				CurrentRecievingPart = RecievingPart::Status;
			}
			if (!FoundCommand)
			{
				//Serial.print("!NF!");
				CurrentReply.Command = CommandsType::Error;
				CurrentReply.Type = TransmissionType::Error;
				CurrentReply.ValueType = ReplyValueType::Error;
				CurrentRecievingPart = RecievingPart::Echo;
				CheckReply();
			}
			ReplyBufferIndex = 0;
		}
	}
}
void SpectraPro2300i::ParseValue(char Character)
{
	//Serial.print(Character);
	if (isdigit(Character) || (Character == PeriodCharacter) )
	{
		ReplyBuffer[ReplyBufferIndex] = Character;
		if (ReplyBufferIndex < (ReplyBufferSize-1) )
		{
			ReplyBufferIndex++;
		}
	}
	else if ( (Character == CarriageReturnCharacter) || (Character == EndOfLineCharacter) || (Character == SpaceCharacter))
	{
		if (ReplyBufferIndex > 0)
		{
			if (CurrentReply.ValueType == ReplyValueType::Integer)
			{
				CurrentReply.Value.IntegerValue = CharPointerToInt(ReplyBuffer,ReplyBufferIndex);
				//Serial.print("!I:");
				//Serial.print(CurrentReply.Value.IntegerValue);
			}
			else if (CurrentReply.ValueType == ReplyValueType::Float)
			{
				CurrentReply.Value.FloatValue = CharPointerToFloat(ReplyBuffer,ReplyBufferIndex);
				//Serial.print("!F:");
				//Serial.print(CurrentReply.Value.FloatValue);
			}
			ReplyBufferIndex = 0;
			if ( CurrentReply.Type == TransmissionType::Get )
			{
				CurrentRecievingPart = RecievingPart::Unit;
			}
			else
			{
				CurrentRecievingPart = RecievingPart::Echo;
			}
		}
	}
	else
	{
		CurrentReply.Command = CommandsType::Error;
		CurrentReply.Type = TransmissionType::Error;
		CurrentReply.ValueType = ReplyValueType::Error;
		CurrentRecievingPart = RecievingPart::Echo;
		CheckReply();
	}
}
void SpectraPro2300i::ParseUnit(char Character)
{
	//Serial.print(Character);
	if (isalpha(Character))
	{
		ReplyBuffer[ReplyBufferIndex] = Character;
		if (ReplyBufferIndex < (ReplyBufferSize-1) )
		{
			ReplyBufferIndex++;
		}
	}
	else if ( (Character == CarriageReturnCharacter) || (Character == EndOfLineCharacter) || (Character == SpaceCharacter))
	{
		if (ReplyBufferIndex > 0)
		{
			ReplyBuffer[ReplyBufferIndex] = '\0';
			if (strcmp(ReplyBuffer, WavelengthUnits) != 0)
			{
				CurrentReply.Command = CommandsType::Error;
				CurrentReply.Type = TransmissionType::Error;
				CurrentReply.ValueType = ReplyValueType::Error;
			}
			ReplyBufferIndex = 0;
			CurrentRecievingPart = RecievingPart::Echo;
		}
	}
	else
	{
		CurrentReply.Command = CommandsType::Error;
		CurrentReply.Type = TransmissionType::Error;
		CurrentReply.ValueType = ReplyValueType::Error;
		CurrentRecievingPart = RecievingPart::Echo;
		CheckReply();
	}
}
void SpectraPro2300i::ParseStatus(char Character)
{
	//Serial.print(Character);
	if (Character == SpaceCharacter)
	{

	}
	else if (isalpha(Character))
	{
		ReplyBuffer[ReplyBufferIndex] = Character;
		if (ReplyBufferIndex < (ReplyBufferSize-1) )
		{
			ReplyBufferIndex++;
		}
	}
	else if ( (Character == CarriageReturnCharacter) || (Character == EndOfLineCharacter))
	{
		if (ReplyBufferIndex > 0)
		{
			ReplyBuffer[ReplyBufferIndex]='\0';
			if (strcmp(ReplyBuffer,OKAcknowledgement)!=0)
			{
				CurrentReply.Command = CommandsType::Error;
				CurrentReply.Type = TransmissionType::Error;
				CurrentReply.ValueType = ReplyValueType::Error;
			}
			ReplyBufferIndex = 0;
			CurrentRecievingPart = RecievingPart::Echo;
			CheckReply();
		}
	}
	else
	{
		CurrentReply.Command = CommandsType::Error;
		CurrentReply.Type = TransmissionType::Error;
		CurrentReply.ValueType = ReplyValueType::Error;
		CurrentRecievingPart = RecievingPart::Echo;
		CheckReply();
	}
}
float SpectraPro2300i::GetCurrentWavelength()
{
	return CurrentWavelength;
}
void SpectraPro2300i::CheckReply()
{
	ExpectReply = false;
	//Serial.print("!C:");
	if (CurrentReply.Command == CommandsType::Error)
	{
		//Serial.println("E!");
		if (CommandRetries < MaxCommandRetries)
		{
			CommandRetries++;
			SendCurrentCommand();
		}
		else
		{
			CommandRetries = 0;
			Serial.println("Monochromator transmission failed.");
		}
	}
	else
	{
		if ( (CurrentReply.Command == CommandsType::Wave) || (CurrentReply.Command == CommandsType::GoWave) )
		{
			CurrentWavelength = CurrentReply.Value.FloatValue;
		}
		Busy = false;
		CommandRetries = 0;
		if (RecievedCallback)
		{
			RecievedCallback();
		}
	}
}
bool SpectraPro2300i::AboutEqual(float x, float y)
{
	if ( (x > y ? x - y : y - x) > AlmostEqualDifference)
	{
		return false;
	}
	else
	{
		return true;
	}
}