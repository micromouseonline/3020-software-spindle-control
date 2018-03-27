
#include <elapsedMillis.h>
#include <SPI.h>

#define PWM_IN_PIN 2
#define POL_PIN 3
#define POT_IN_PIN 0
#define CS_PIN 10
#define SCK_PIN 15
#define MOSI_PIN 16
#define AVG_LEN 15
#define LED_PIN 17

#define MAX(x,y) (x) > (y) ? (x) : (y)
#define MIN(x,y) (x) < (y) ? (x) : (y)

//#define DEBUG

SPISettings spiSet(5000000, MSBFIRST, SPI_MODE0);

volatile int onTime = 0;
volatile int offTime = 0;
volatile long lastTime = 0;
elapsedMillis outputTimer;
elapsedMillis printTimer;
elapsedMillis ledTimer;
uint16_t ledState = 200;

volatile bool invertPolarity;

float pwmPcts[AVG_LEN];
float outputPct;
float pwmPct;
float potPct;

void interrupt () {
    long now = micros();
    long elapsed = lastTime - now;
    bool state = digitalRead(PWM_IN_PIN) ^ invertPolarity; 

    // this is backwards because the opto inverts
    if (state) { 
        offTime = elapsed;
    } else {
        onTime = elapsed;
    }

    lastTime = now;
}

void setup() {

    pinMode(PWM_IN_PIN, INPUT_PULLUP);
    pinMode(POT_IN_PIN, INPUT);
    pinMode(CS_PIN, OUTPUT);
    pinMode(POL_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

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
    // only going to use 1/2 the range

    uint8_t value = 0x80 + pct * 0x7f;
    SPI.beginTransaction(spiSet);
    digitalWrite(CS_PIN, LOW);
    SPI.transfer(0x11); // Write potentiometer 0
    SPI.transfer(value); // Set wiper position
    digitalWrite(CS_PIN, HIGH);
    SPI.endTransaction();
}

void setLedPct(float pct) {
    if (pct <=0) {
        ledState = 0;
    } else if (pct >= 1) {
        ledState = 1;
    } else {
        ledState = (1.0 - pct) * 1000;
    }
}

void loop() {

    static int pctIndex = 0;
    invertPolarity = digitalRead(POL_PIN);

    float cur = digitalRead(PWM_IN_PIN);
    cur = invertPolarity ? 1.0 - cur : cur;

    noInterrupts();
    // copy values out fast.
    long l = lastTime;
    int o = onTime;
    int f = offTime;
    interrupts();

    // BEGIN
    //
    // If we put this code in the 
    // "nointerrupts" block above, 
    // then narrow PWM times become
    // unstable
    long now = micros();
    long elapsed = now - l;
    if (elapsed < 100000) { // 100ms
        cur = (float)o / (float)(o + f);
    } 
    // END


    if (ledState < 2) {
        digitalWrite(LED_PIN, !ledState);
        ledTimer = 0;
    } else if (ledTimer > ledState) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        ledTimer = 0;
    }

    pwmPcts[pctIndex++] = cur;

    if (pctIndex >= AVG_LEN) {
        pctIndex = 0;
    }

    if (outputTimer > 10) {
        pwmPct = 0;

        for (int i = 0; i < AVG_LEN; ++i) {
            pwmPct += pwmPcts[i];
        }

        pwmPct /= AVG_LEN;
        potPct = analogRead(POT_IN_PIN) / 1023.0f;
        outputPct = pwmPct * potPct;

        setLedPct(outputPct);
        setOutput(outputPct);
        outputTimer = 0;
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
