#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <FastLED.h>
#include <time.h>

class LEDStrip {
    public:
        LEDStrip(int LEDCount, int minBrightness, int _maxBrightness);
        
        // template <ESPIChipsets CHIPSET,  uint8_t DATA_PIN, EOrder RGB_ORDER>
        int init();

        // Utility Functions:
        void setPixel(int pixel, byte red, byte green, byte blue);
        void setPixelTargets(int pixel, byte red, byte green, byte blue);
        void setStrip(byte red, byte green, byte blue);
        void setStripTargets(byte red, byte green, byte blue);
        void setSubStrip(int p_low, int p_high, byte red, byte green, byte blue);
        void setSubStripTargets(int p_low, int p_high, byte red, byte green, byte blue);
        void update();
        void computeSlew();
        void slew();

        // Strip Operations:
        void leftshift(bool wrap);
        void rightshift(bool wrap);

        // Animations:
        void randomize();
        void randomizeSubStrip(int p_low, int p_high);
        bool randomWarmup(byte r, byte g, byte b, byte dl);
        void twinkle(byte r, byte g, byte b, float dr, float dg, float db, float dl);

    private:
        uint8_t _LEDPin;
        int _LEDCount;

        int _minBrightness;
        int _maxBrightness;

        unsigned long _lastUpdate;
        unsigned int _updateIntervalMS = 50;
        
        CRGB* _leds;
        CRGB* _ledTargets;
        struct rgbInt{
          int r = 0;
          int g = 0;
          int b = 0;
        }*_slewRates;

        bool _slewEnable;
        bool _slewComputed;
        bool _slewing;
        int _slewTicks = 10;
};

#endif