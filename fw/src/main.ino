
#include <elapsedMillis.h>
#define PIN 2

volatile int onTime = 0;
volatile int offTime = 0;
volatile long lastTime = 0;
elapsedMillis outputTimer;
elapsedMillis printTimer;

#define AVG_LEN 15

float pcts[AVG_LEN];
float outputPct;
float pct;

void interrupt () {
    long now = micros();
    long elapsed = lastTime - now;
    bool state = digitalRead(PIN);

    if (state) {
        offTime = elapsed;
    } else {
        onTime = elapsed;
    }

    lastTime = now;
}

void setup() {

    pinMode(PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN), interrupt, CHANGE);
    Serial.begin(115200);
    
    for (int i = 0; i < AVG_LEN; ++i) {
        pcts[i] = 0;
    }
    pct = 0;
    outputPct = 0;
}

void loop() {

    static int pctIndex = 0;
    long curOnTime, curOffTime;
    bool flag = false;
    float cur = digitalRead(PIN);

    noInterrupts();
    long now = micros();
    long elapsed = now - lastTime;
    if (elapsed < 100000) {
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
    pcts[pctIndex++] = cur;

    if (pctIndex >= AVG_LEN) {
        pctIndex = 0;
    }


    if (outputTimer > 50) {
        pct = 0;
        for (int i = 0; i < AVG_LEN; ++i) {
            pct += pcts[i];
        }
        pct /= AVG_LEN;

        float delta = outputPct - pct;
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

    }
    if (printTimer > 75) {
        Serial.print(pct*100); 
        Serial.print("%");
        Serial.print("  out: ");
        Serial.print(outputPct * 100); 
        Serial.println();
        printTimer = 0;
    }



}
