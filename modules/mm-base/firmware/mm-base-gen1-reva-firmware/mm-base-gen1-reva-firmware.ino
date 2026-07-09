/*
  MakaniMotion MM-BASE GEN 1 Rev A Firmware

  Short button press:
    Cycle through PWM stages.

  Long button press:
    Toggle RGB status LEDs on/off.

  RGB LEDs:
    Non-blocking pulse effect during normal operation.
*/

const byte OC1A_PIN = 9;     // PWM output for fan control, D9 / OC1A
const byte BUTTON_PIN = 2;   // Button input, D2, active LOW

const byte RED_PIN = 5;      // RGB LED red channel
const byte GREEN_PIN = 6;    // RGB LED green channel
const byte BLUE_PIN = 3;     // RGB LED blue channel

const word PWM_FREQ_HZ = 25000;
const word TCNT1_TOP = F_CPU / (2 * PWM_FREQ_HZ);

// PWM stages in percent
const byte PWM_STAGES[] = {33, 66, 100};
const byte PWM_STAGE_COUNT = sizeof(PWM_STAGES) / sizeof(PWM_STAGES[0]);

byte currentStage = 0;

// Button handling
const unsigned long DEBOUNCE_DELAY_MS = 40;
const unsigned long LONG_PRESS_MS = 1000;

byte lastButtonReading = HIGH;
byte stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long buttonPressStartTime = 0;

bool longPressHandled = false;

// LED handling
bool ledsEnabled = true;

const unsigned long LED_PULSE_INTERVAL_MS = 15;
const byte LED_PULSE_MIN = 25;
const byte LED_PULSE_MAX = 255;
const byte LED_PULSE_STEP = 2;

unsigned long lastLedPulseUpdate = 0;
byte ledPulseValue = LED_PULSE_MIN;
int ledPulseDirection = 1;

void setup() {
  pinMode(OC1A_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  setupTimer1ForFanPwm();

  setPwmDuty(PWM_STAGES[currentStage]);
  updateLedPulse();
}

void loop() {
  handleButton();
  updateLedPulse();
}

void setupTimer1ForFanPwm() {
  // Reset Timer1 registers
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;

  // Phase Correct PWM on OC1A, no prescaler
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << CS10);

  // Set TOP value for 25 kHz PWM
  ICR1 = TCNT1_TOP;
}

void handleButton() {
  byte reading = digitalRead(BUTTON_PIN);

  // Debounce input
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
    if (reading != stableButtonState) {
      stableButtonState = reading;

      if (stableButtonState == LOW) {
        // Button was pressed
        buttonPressStartTime = millis();
        longPressHandled = false;
      } else {
        // Button was released
        if (!longPressHandled) {
          nextPwmStage();
        }
      }
    }

    // Long press while button is still held
    if (stableButtonState == LOW && !longPressHandled) {
      if ((millis() - buttonPressStartTime) >= LONG_PRESS_MS) {
        toggleLeds();
        longPressHandled = true;
      }
    }
  }

  lastButtonReading = reading;
}

void nextPwmStage() {
  currentStage = (currentStage + 1) % PWM_STAGE_COUNT;
  setPwmDuty(PWM_STAGES[currentStage]);

  // Restart pulse at a defined brightness after changing stage
  ledPulseValue = LED_PULSE_MIN;
  ledPulseDirection = 1;
}

void toggleLeds() {
  ledsEnabled = !ledsEnabled;

  if (!ledsEnabled) {
    setRgbRaw(0, 0, 0);
  } else {
    ledPulseValue = LED_PULSE_MIN;
    ledPulseDirection = 1;
    updateLedPulse();
  }
}

void setPwmDuty(byte duty) {
  OCR1A = (word)(duty * TCNT1_TOP) / 100;
}

void updateLedPulse() {
  if (!ledsEnabled) {
    setRgbRaw(0, 0, 0);
    return;
  }

  if ((millis() - lastLedPulseUpdate) < LED_PULSE_INTERVAL_MS) {
    return;
  }

  lastLedPulseUpdate = millis();

  int nextValue = ledPulseValue + (ledPulseDirection * LED_PULSE_STEP);

  if (nextValue >= LED_PULSE_MAX) {
    nextValue = LED_PULSE_MAX;
    ledPulseDirection = -1;
  } else if (nextValue <= LED_PULSE_MIN) {
    nextValue = LED_PULSE_MIN;
    ledPulseDirection = 1;
  }

  ledPulseValue = (byte)nextValue;

  setStageColorScaled(currentStage, ledPulseValue);
}

void setStageColorScaled(byte stage, byte brightness) {
  byte red = 0;
  byte green = 0;
  byte blue = 0;

  switch (stage) {
    case 0:
      // Stage 1: Blue
      red = 0;
      green = 0;
      blue = 255;
      break;

    case 1:
      // Stage 2: Green
      red = 0;
      green = 255;
      blue = 0;
      break;

    case 2:
      // Stage 3: Cyan
      red = 0;
      green = 255;
      blue = 255;
      break;
  }

  setRgbRaw(
    scaleColor(red, brightness),
    scaleColor(green, brightness),
    scaleColor(blue, brightness)
  );
}

byte scaleColor(byte color, byte brightness) {
  return (byte)((word)color * brightness / 255);
}

void setRgbRaw(byte red, byte green, byte blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}