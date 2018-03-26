
#include <elapsedMillis.h>
#include <SPI.h>

#define PWM_IN_PIN 2
#define POT_IN_PIN 0
#define CS_PIN 10
#define SCK_PIN 15
#define MOSI_PIN 16
#define AVG_LEN 15


//#define DEBUG

SPISettings spiSet(5000000, MSBFIRST, SPI_MODE0);

volatile int onTime = 0;
volatile int offTime = 0;
volatile long lastTime = 0;
elapsedMillis outputTimer;
elapsedMillis printTimer;



float pwmPcts[AVG_LEN];
float outputPct;
float pwmPct;
float potPct;

void interrupt () {
    long now = micros();
    long elapsed = lastTime - now;
    bool state = digitalRead(PWM_IN_PIN);

    if (state) {
        offTime = elapsed;
    } else {
        onTime = elapsed;
    }

    lastTime = now;
}

void setup() {

    pinMode(PWM_IN_PIN, INPUT_PULLUP);
    pinMode(CS_PIN, OUTPUT);

    SPI.begin();
    setOutput(0);

    attachInterrupt(digitalPinToInterrupt(PWM_IN_PIN), interrupt, CHANGE);
    Serial.begin(115200);

    for (int i = 0; i < AVG_LEN; ++i) {
        pwmPcts[i] = 0;
    }

    pwmPct = 0;
    potPct = 0;
    outputPct = 0;
}

void setOutput(float pct) {


    // We're emulating a 5K pot with a
    // 10K digi-pot, so we're
    // only going to use 1/2 the rang

    uint8_t value = 0x80 + pct * 0x7f;
    SPI.beginTransaction(spiSet);
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(0x11); // Write potentiometer 0
    SPI.transfer(value); // Set wiper position
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}

void loop() {

    static int pctIndex = 0;
    long curOnTime, curOffTime;
    bool flag = false;
    float cur = digitalRead(PWM_IN_PIN);

    noInterrupts();
    long now = micros();
    long elapsed = now - lastTime;
    if (elapsed < 100000) { // 100ms
        curOnTime = onTime;
        curOffTime = offTime;
        flag = true;
    } else {
        onTime = offTime = 0;
    }
    interrupts();

    if (flag && curOnTime && curOffTime) {
        cur = (float)curOnTime / (curOffTime + curOnTime);
    }
    pwmPcts[pctIndex++] = cur;

    if (pctIndex >= AVG_LEN) {
        pctIndex = 0;
    }


    if (outputTimer > 50) {
        pwmPct = 0;

        for (int i = 0; i < AVG_LEN; ++i) {
            pwmPct += pwmPcts[i];
        }
        pwmPct /= AVG_LEN;
	potPct = analogRead(POT_IN_PIN) / 1023.0f;

        float delta = outputPct - pwmPct * potPct;
        float aDelta = fabs(delta);
        if (delta) {
            // No more than 2% per 50ms
            if (delta < 0) {
                outputPct += min(aDelta, .02);
            } else {
            // Slow down fast
                outputPct -= delta;
            }
        }
        outputTimer = 0;
	setOutput(outputPct);
    }

#ifdef DEBUG
    if (printTimer > 75) {
        Serial.print("PWM in: ");
        Serial.print(pwmPct*100);
        Serial.print("%");
        Serial.print(" POT in: ");
        Serial.print(potPct*100);
        Serial.print("  out: ");
        Serial.print(outputPct * 100);
        Serial.println();
        printTimer = 0;
    }
#endif
}
