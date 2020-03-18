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
const long ManualSignalTimeOut = 150;
const long TurningOnTimeOut = 10600;
const long IdleTimeOut = 60000;
const long EndStopChecks = 10;

long TurningOnTime;
long IdleTime;
long EndStopPressMillis = 0;
long EndStopCheckNumber = 0;
long LastIrSignaMillis = 0;

void setup() {
	//Serial.begin(115200);
	irrecv.enableIRIn(); // Start the receiver

	pinMode(PowerPin, OUTPUT);
	pinMode(LiftingUpPin, OUTPUT);
	pinMode(LiftingDownPin, OUTPUT);
	pinMode(EndStopPin, INPUT);

	digitalWrite(PowerPin, !false);
	TurnOffLifting();
}

void setRelayState(int relayPin, bool value) {
	if (value) {
		digitalWrite(relayPin, LOW);
	} else {
		digitalWrite(relayPin, HIGH);
	}
}

bool getRelayState(int relayPin) {
	if (digitalRead(relayPin)) {
		return false;
	} else {
		return true;
	}
}

bool isButtonOff(unsigned long value) {
  //                     fan                    projector
	bool result = value == 0xA32AB931 || value == 0xC1AA9A65;

	if (result) {
		OnAnyUserAction();
	}

	return result;
}

bool isButtonOn(unsigned long value) {
  //                     fan                    projector
	bool result = value == 0x143226DB || value == 0xC1AA7A85;

	if (result) {
		OnAnyUserAction();
	}

	return result;
}

bool isButtonMode(unsigned long value) {
  //                     fan                    projector
	bool result = value == 0x371A3C86 || value == 0xC1AADA25;

	if (result) {
		OnAnyUserAction();
	}

	return result;
}

bool isButtonUp(unsigned long value) {
  //                     fan                    projector
	bool result = value == 0xE0984BB6 || value == 0xC1AA5AA5;

	if (result) {
		OnAnyUserAction();
	}

	return result;
}

bool isButtonDown(unsigned long value) {
  //                     fan                    projector
	bool result = value == 0x39D41DC6 || value == 0xC1AA3AC5;

	if (result) {
		OnAnyUserAction();
	}

	return result;
}

void OnAnyUserAction() {
	if (DeviceOn) {
		digitalWrite(PowerPin, !true);
		IdleTime = millis();
		CheckIdleTime = true;
	}
}

void TurnOffLifting() {
	setRelayState(LiftingUpPin, false);
	setRelayState(LiftingDownPin, false);
}

void StopAutoLiftDown() {
	if (AutoLiftingDown) {
		AutoLiftingDown = false;
		TurnOffLifting();
	}
}

void StopAutoLiftUp(bool turnOff = true) {
	if (AutoLiftingUp) {
		AutoLiftingUp = false;
		TurnOffLifting();

		if (turnOff) {
			DeviceOn = false;
			digitalWrite(PowerPin, !false);
		}
	}
}

void AutoLiftDown() {
	if (!AutoLiftingDown) {
		TurnOffLifting();
		TurningOnTime = millis();
		AutoLiftingUp = false;
		AutoLiftingDown = true;
		setRelayState(LiftingDownPin, AutoLiftingDown);
	}
}

void AutoLiftUp() {
	if (!AutoLiftingUp) {
		TurnOffLifting();

		if (AutoLiftingDown) {
			delay(200);
		}

		AutoLiftingUp = true;
		AutoLiftingDown = false;

		if (IsTopPosition()) {
			StopAutoLiftUp();
		} else {
			setRelayState(LiftingUpPin, AutoLiftingUp);
		}
	}
}

bool IsTopPosition() {
	if (getRelayState(LiftingUpPin)) {
		int endStopValue = analogRead(EndStopPin);
		//Serial.print("endStopValue = ");
		//Serial.println(endStopValue, DEC);
		if (endStopValue == 0) {
			EndStopCheckNumber++;
			return EndStopCheckNumber >= EndStopChecks;
		} else {
			EndStopCheckNumber = 0;
		}
	}

	return false;
}

void ProcessSignal(unsigned long signal) {
	//Serial.println(signal, HEX);

	if (isButtonOff(signal) && DeviceOn) { // off
		AutoLiftUp();
		delay(200);
		return;
	} else if (isButtonOn(signal)) { // on
		if (DeviceOn) {
			if (AutoLiftingUp) {
				StopAutoLiftUp(false);
			}
		} else {
			DeviceOn = true;
			digitalWrite(PowerPin, !true);
			OnAnyUserAction();
			AutoLiftDown();
		}
	} else if (DeviceOn) {
		if (isButtonUp(signal)) { // up
			StopAutoLiftDown();
			StopAutoLiftUp(false);

			if (!LiftingUp) {
				setRelayState(LiftingUpPin, true);
        setRelayState(LiftingDownPin, false);
				LiftingUp = true;
			}

			LiftingDown = false;
		} else if (isButtonDown(signal)) { // down
			StopAutoLiftDown();
			StopAutoLiftUp(false);

			if (!LiftingDown) {
        setRelayState(LiftingUpPin, false);
				setRelayState(LiftingDownPin, true);
				LiftingDown = true;
			}

			LiftingUp = false;
		}
	}
}

void loop() {
	if (irrecv.decode( & results)) {
    LastIrSignaMillis = millis();
		ProcessSignal(results.value);
		irrecv.resume();
	}

	if (LiftingUp || LiftingDown) {
		if ((millis() - LastIrSignaMillis > ManualSignalTimeOut)) {
			LiftingUp = false;
			LiftingDown = false;
			TurnOffLifting();
		}
	}

	if (AutoLiftingDown && millis() - TurningOnTime > TurningOnTimeOut) {
		StopAutoLiftDown();
	}

	if (AutoLiftingUp && IsTopPosition()) {
		StopAutoLiftUp();
	}

	if (CheckIdleTime && (millis() - IdleTime > IdleTimeOut)) {
		digitalWrite(PowerPin, !false);
		CheckIdleTime = false;
	}

	delay(20);
}
