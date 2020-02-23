#include <IRremote.h>
const int IRReceiverPin = 11;
IRrecv irrecv(IRReceiverPin);
decode_results results;

const int PowerPin = 2;
const int LiftingUpPin = 4;
const int LiftingDownPin = 5;
const int EndStopPin = 1;

bool DeviceOn = false;
bool AutoLiftingDown = false;
bool AutoLiftingUp = false;
bool LiftingUp = false;
bool LiftingDown = false;
bool CheckIdleTime = false;
long ManualLiftingTimeOut = 250; // this var updates in methods isButtonUp() and isButtonDown()
long TurningOnTimeOut = 10000;
long IdleTimeOut = 60000;

long PressTime;
long TurningOnTime;
long IdleTime;

//bool Blink = false;
//long BlinkTime = 0;
//void BlinkWithoutDelay() {
//	if (BlinkTime == 0)BlinkTime = millis();
//	if (millis() - BlinkTime > 100)
//	{
//		Blink = !Blink;
//		BlinkTime = 0;
//	}
//}

void setup()
{
	//Serial.begin(9600);
	irrecv.enableIRIn(); // Start the receiver

	pinMode(PowerPin, OUTPUT);
	pinMode(LiftingUpPin, OUTPUT);
	pinMode(LiftingDownPin, OUTPUT);
	pinMode(EndStopPin, INPUT);

	digitalWrite(PowerPin, !false);
	TurnOffLifting();
}

void setRelayState(int relayPin, bool value)
{
	if (value)
	{
		digitalWrite(relayPin, LOW);
	}
	else
	{
		digitalWrite(relayPin, HIGH);
	}
}

bool getRelayState(int relayPin)
{
	if (digitalRead(relayPin))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool isButtonOff(unsigned long value)
{
	bool result = value == 0xA32AB931
		|| (value == 0x10EF38C7 && DeviceOn && !AutoLiftingUp);

	if (result)
	{
		OnAnyUserAction();
	}

	return result;
}

bool isButtonOn(unsigned long value)
{
	bool result = value == 0x143226DB
		|| (value == 0x10EF38C7 && (!DeviceOn || AutoLiftingUp));

	if (result)
	{
		OnAnyUserAction();
	}

	return result;
}

bool isButtonMode(unsigned long value)
{
	bool result = value == 0x371A3C86 || value == 0x10EF20DF;

	if (result)
	{
		OnAnyUserAction();
	}

	return result;
}

bool isButtonUp(unsigned long value)
{
	bool result = value == 0xE0984BB6 || value == 0x10EFB04F;

	if (result)
	{
		if (value == 0x10EFB04F)
		{
			ManualLiftingTimeOut = 350;
		}
		else if (value == 0xE0984BB6)
		{
			ManualLiftingTimeOut = 150;
		}

		OnAnyUserAction();
	}

	return result;
}

bool isButtonDown(unsigned long value)
{
	bool result = value == 0x39D41DC6 || value == 0x10EF08F7;

	if (result)
	{
		if (value == 0x10EF08F7)
		{
			ManualLiftingTimeOut = 350;
		}
		else if (value == 0x39D41DC6)
		{
			ManualLiftingTimeOut = 150;
		}

		OnAnyUserAction();
	}

	return result;
}

void OnAnyUserAction()
{
	if (DeviceOn)
	{
		digitalWrite(PowerPin, !true);
		IdleTime = millis();
		CheckIdleTime = true;
	}
}

void TurnOffLifting()
{
	setRelayState(LiftingUpPin, false);
	setRelayState(LiftingDownPin, false);
}

void StopAutoLiftDown()
{
	if (AutoLiftingDown)
	{
		AutoLiftingDown = false;
		TurnOffLifting();
	}
}

void StopAutoLiftUp(bool turnOff = true)
{
	if (AutoLiftingUp)
	{
		AutoLiftingUp = false;
		TurnOffLifting();

		if (turnOff)
		{
			DeviceOn = false;
			digitalWrite(PowerPin, !false);
		}
	}
}

void AutoLiftDown()
{
	if (!AutoLiftingDown)
	{
		TurnOffLifting();
		TurningOnTime = millis();
		AutoLiftingUp = false;
		AutoLiftingDown = true;
		setRelayState(LiftingDownPin, AutoLiftingDown);
	}
}

void AutoLiftUp()
{
	if (!AutoLiftingUp)
	{
		TurnOffLifting();

		if (AutoLiftingDown)
		{
			delay(200);
		}

		AutoLiftingUp = true;
		AutoLiftingDown = false;

		if (IsTopPosition())
		{
			StopAutoLiftUp();
		}
		else
		{
			setRelayState(LiftingUpPin, AutoLiftingUp);
		}
	}
}

bool IsTopPosition()
{
	if (getRelayState(LiftingUpPin))
	{
		if (analogRead(EndStopPin) == 0)
		{
			return true;
		}
	}

	return false;
}

void ProcessSignal(unsigned long signal)
{
	//Serial.println(signal, HEX);
	if (isButtonOff(signal) && DeviceOn)
	{
		AutoLiftUp();
		delay(200);

		return;
	}
	else if (isButtonOn(signal))
	{
		if (DeviceOn)
		{
			if (AutoLiftingUp)
			{
				StopAutoLiftUp(false);
			}
		}
		else
		{
			DeviceOn = true;
			digitalWrite(PowerPin, !true);
			OnAnyUserAction();
			AutoLiftDown();
		}
	}
	else if (DeviceOn)
	{
		if (isButtonUp(signal))
		{
			StopAutoLiftDown();
			StopAutoLiftUp(false);
			
			if (!LiftingUp)
			{
				setRelayState(LiftingUpPin, true);
				LiftingUp = true;
			}
			
			LiftingDown = false;
			PressTime = millis();
		}
		else if (isButtonDown(signal))
		{
			StopAutoLiftDown();
			StopAutoLiftUp(false);

			if (!LiftingDown)
			{
				setRelayState(LiftingDownPin, true);
				LiftingDown = true;
			}

			LiftingUp = false;
			PressTime = millis();
		}
	}
}

void loop() {
	/*Serial.print(LiftingUp);
	Serial.print(" : ");
	Serial.println(analogRead(EndStopPin), DEC);*/

	if (irrecv.decode(&results))
	{
		ProcessSignal(results.value);
		irrecv.resume();
	}

	if (LiftingUp || LiftingDown)
	{
		if ((millis() - PressTime > ManualLiftingTimeOut))
		{
			LiftingUp = false;
			LiftingDown = false;
			TurnOffLifting();
		}
	}

	if (AutoLiftingDown && millis() - TurningOnTime > TurningOnTimeOut)
	{
		StopAutoLiftDown();
	}

	if (AutoLiftingUp && IsTopPosition())
	{
		StopAutoLiftUp();
	}

	if (CheckIdleTime && (millis() - IdleTime > IdleTimeOut))
	{
		digitalWrite(PowerPin, !false);
		CheckIdleTime = false;
	}

	delay(20);
}