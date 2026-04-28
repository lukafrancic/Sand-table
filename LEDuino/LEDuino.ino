
#include <Arduino.h>
#include <FastLED.h>

#define READIN_PIN 3
#define LED_PIN 5
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_LED 273
#define UPDATE_TIME 100
#define MIN_PULSE 160
#define MAX_PULSE 7950
#define LED_OFFSET 38
#define GAUSS_SIZE 51
#define RANDOM_SEED 123

CRGB leds[NUM_LED];

constexpr long TIMEOUT = 10000; // 255*32 = 8160, 1 pwm step about 32 us
constexpr unsigned long PULSE_LEN = 32; // 120 Hz -> 1 pwm step = 32 micro sec
unsigned long t, traw;
unsigned long t1 = 0, t2 = 0;
long loopCountSpeed = 0, numLoopSpeed = 0;
long loopCountPalette = 0, numLoopPalette = 0;
long step = 0;
long cNew;
int idx;
int a_new, b_new, dist, i_count;
constexpr int x0 = 0, xc = 25, x1 = 50;
int16_t speed = 5;
uint16_t moveSpeed = 0;

CRGBPalette16 currentPalette, targetPalette;
TBlendType    currentBlending;
 
constexpr uint8_t DEFAULT_BRIGHTNESS = 77;
constexpr uint8_t GAUSS[GAUSS_SIZE] = {
    84,  86,  88,  92,  95, 100, 105, 111, 118, 125, 133, 143, 152,
    162, 173, 184, 195, 205, 215, 225, 233, 241, 247, 251, 254, 255,
    254, 251, 247, 241, 233, 225, 215, 205, 195, 184, 173, 162, 152,
    143, 133, 125, 118, 111, 105, 100,  95,  92,  88,  86,  84
};
CRGB temp;

DEFINE_GRADIENT_PALETTE(desert) {
    0,   255,  84,  0, 
    64,  249,  160, 20,
    128, 198,  120, 26,  
    192, 249,  160, 20,
    255, 255,  84,  0
};

DEFINE_GRADIENT_PALETTE(oasis) {
    0,   10,  193,  22, 
    64,  35,  185,  219,
    128, 35,  219,  177,
    192, 35,  185,  219,
    255, 10,  193,  22
};


void setup() {
    delay(3000);
    pinMode(READIN_PIN, INPUT);
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LED).setCorrection(TypicalLEDStrip);

    // currentPalette = LavaColors_p;
    currentPalette = desert;
    currentBlending = LINEARBLEND;

    randomSeed(RANDOM_SEED);

    numLoopSpeed = random(50, 500);
}

void loop() {
    t1 = millis();
    
    t = pulseIn(READIN_PIN, HIGH, TIMEOUT);

    if ((t1 - t2 > UPDATE_TIME) && (t > 120)) {
        traw = t;
        t = constrain(t, MIN_PULSE, MAX_PULSE);
        cNew = map(t, MIN_PULSE, MAX_PULSE, 0, NUM_LED-1);
        cNew -= LED_OFFSET;
        if (cNew < 0) {cNew += NUM_LED - 1;}
        idx = static_cast<int>(cNew);
        
        periodicPaletteChange();
        fillLEDfromPalette(idx);

        FastLED.show();

        t2 = t1;
    }
}





void fillLEDfromPalette(int idx){
    uint8_t brightness;
    uint8_t spatial_idx;
    dist = xc - idx;
    a_new = (x0 - dist + NUM_LED) % NUM_LED;
    b_new = (x1 - dist + NUM_LED) % NUM_LED;

    if (a_new < b_new) {
        i_count = 0;
        
        for (int i = 0; i < NUM_LED; ++i) {
            if ((i >= a_new) && (i <= b_new)){
                brightness = GAUSS[i_count];
                i_count++;
            } else {
                brightness = DEFAULT_BRIGHTNESS;
            }
            spatial_idx = (moveSpeed >> 8) + (uint8_t)((i*256)/NUM_LED);
            leds[i] = ColorFromPalette(currentPalette, spatial_idx, brightness, currentBlending);
            // leds[i] = CRGB(brightness, brightness, brightness);
        }
    } else {
        i_count = GAUSS_SIZE - b_new;
        
        for (int i = 0; i < NUM_LED; ++i) {
            if ((i <= b_new) || (i >= a_new)){
                brightness = GAUSS[i_count];
                i_count = (i_count + 1) % GAUSS_SIZE;
            } else {
                brightness = DEFAULT_BRIGHTNESS;
            }
            
            spatial_idx = (moveSpeed >> 8) + (uint8_t)((i*256)/NUM_LED);
            leds[i] = ColorFromPalette(currentPalette, spatial_idx, brightness, currentBlending);
            // leds[i] = CRGB(brightness, brightness, brightness);
        }
    }
}


void periodicPaletteChange() {
    if (loopCountSpeed >= numLoopSpeed) {
        loopCountSpeed = 0;
        speed += random(-10,11);
        speed *= 8;
        speed = constrain(speed, -500, 500);
        numLoopSpeed = random(5, 50);
    }

    if (loopCountPalette >= numLoopPalette) {
        loopCountPalette = 0;
        numLoopPalette = random(300, 600);
        step = random(0, 2);
        switch (step){
            case 0:
                targetPalette = desert;
                break;    
            case 1:
                targetPalette = oasis;
                break;
            default:
                break;
        }
    }

    nblendPaletteTowardPalette(currentPalette, targetPalette, 48);
    moveSpeed += speed;
    loopCountSpeed += 1;
    loopCountPalette += 1;
}