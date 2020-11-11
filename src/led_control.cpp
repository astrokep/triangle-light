#include "led_control.hpp"

// Constructor:
LEDStrip::LEDStrip(int LEDCount, int minBrightness, int maxBrightness){
    // _LEDPin = LEDPin;
    _LEDCount = LEDCount;
    _minBrightness = minBrightness;
    _maxBrightness = maxBrightness;

    _leds = (CRGB*)malloc(_LEDCount * sizeof(CRGB));
    _ledTargets = (CRGB*)malloc(_LEDCount * sizeof(CRGB));
    _slewRates = (rgbInt*)malloc(_LEDCount * sizeof(rgbInt));
}

int LEDStrip::init(){
    FastLED.addLeds<WS2812B, 2, GRB>(_leds, _LEDCount).setCorrection(TypicalLEDStrip);
    setStrip(0, 0, 0);
    update();
    return 1;
}

// Utility Functions:
void LEDStrip::setPixel(int pixel, byte red, byte green, byte blue){
    _leds[pixel].r = red;
    _leds[pixel].g = green;
    _leds[pixel].b = blue;
}

void LEDStrip::setPixelTargets(int pixel, byte red, byte green, byte blue){
    _ledTargets[pixel].r = red;
    _ledTargets[pixel].g = green;
    _ledTargets[pixel].b = blue;
}

void LEDStrip::setStrip(byte red, byte green, byte blue){
    for(int i = 0; i < _LEDCount; ++i){
        setPixel(i, red, green, blue);
    }
}

void LEDStrip::setStripTargets(byte red, byte green, byte blue){
    for(int i = 0; i < _LEDCount; ++i){
        setPixelTargets(i, red, green, blue);
    }
    computeSlew();
    _slewEnable = true;
}

void LEDStrip::setSubStrip(int p_low, int p_high, byte red, byte green, byte blue){
    for(int i = p_low; i < p_high; ++i){
        setPixel(i, red, green, blue);
    }
}

void LEDStrip::setSubStripTargets(int p_low, int p_high, byte red, byte green, byte blue){
    for(int i = p_low; i < p_high; ++i){
        setPixelTargets(i, red, green, blue);
    }
    computeSlew();
    _slewEnable = true;
}


void LEDStrip::computeSlew(){
    for(int i = 0; i < _LEDCount; ++i){
        _slewRates[i].r = (_ledTargets[i].r - _leds[i].r)/_slewTicks;
        _slewRates[i].g = (_ledTargets[i].g - _leds[i].g)/_slewTicks;
        _slewRates[i].b = (_ledTargets[i].b - _leds[i].b)/_slewTicks;
    }
    Serial.println("Slew Rates:");
    Serial.print("R: ");
    Serial.println(_slewRates[0].r);
    Serial.print("G: ");
    Serial.println(_slewRates[0].g);
    Serial.print("B: ");
    Serial.println(_slewRates[0].b);
    _slewComputed = true;
}

void LEDStrip::slew(){
    _slewEnable = false;
    for(int i = 0; i < _LEDCount; ++i){
        if((abs(_ledTargets[i].r - _leds[i].r) > abs(_slewRates[i].r))){
            _leds[i].r += _slewRates[i].r;
            _slewEnable = true;
        }
        else{
            _leds[i].r = _ledTargets[i].r;
            _slewRates[i].r = 0;
        }

        if((abs(_ledTargets[i].g - _leds[i].g) > abs(_slewRates[i].g))){
            _leds[i].g += _slewRates[i].g;
            _slewEnable = true;
        }
        else{
            _leds[i].g = _ledTargets[i].g;
            _slewRates[i].g = 0;
        }

        if((abs(_ledTargets[i].b - _leds[i].b) > abs(_slewRates[i].b))){
            _leds[i].b += _slewRates[i].b;
            _slewEnable = true;
        }
        else{
            _leds[i].b = _ledTargets[i].b;
            _slewRates[i].b = 0;
        }
    }
}

void LEDStrip::update(){
    if((millis() - _lastUpdate) > _updateIntervalMS){
        if(_slewEnable){
            slew();
        }
        FastLED.show();
        _lastUpdate = millis();
    }
}

// Strip Operations:
void LEDStrip::leftshift(bool wrap){
    CRGB temp = _leds[_LEDCount - 1];
    for(int i = _LEDCount - 1; i > 0; --i){
        _leds[i] = _leds[i-1];
    }
    if(!wrap){
        temp.r = 0;
        temp.g = 0;
        temp.b = 0;
    }
    _leds[0] = temp;    
}

void LEDStrip::rightshift(bool wrap){
    CRGB temp = _leds[0];
    for(int i = 0; i < _LEDCount; ++i){
        _leds[i] = _leds[i+1];
    }
    if(!wrap){
        temp.r = 0;
        temp.g = 0;
        temp.b = 0;
    }
    _leds[_LEDCount - 1] = temp;    
}

// Animations:
bool LEDStrip::randomWarmup(byte r, byte g, byte b, byte dl){
  bool done = true;
  for(int i = 0; i < _LEDCount; ++i){
    if(_leds[i].r != r || _leds[i].g != g || _leds[i].b != b){
      done = false;
      byte incr = random(0, dl);
      byte r_out = constrain(_leds[i].r + incr, _minBrightness, _maxBrightness);
      byte g_out = constrain(_leds[i].g + incr, _minBrightness, _maxBrightness);
      byte b_out = constrain(_leds[i].b + incr, _minBrightness, _maxBrightness);
      setPixel(i, r_out, g_out, b_out);
    }
  }
  update();
  return done;
}

void LEDStrip::randomize(){
    byte r = random(_minBrightness, _maxBrightness);
    byte g = random(_minBrightness, _maxBrightness);
    byte b = random(_minBrightness, _maxBrightness);
    setStrip(r, g, b);
}

void LEDStrip::randomizeSubStrip(int p_low, int p_high){
    byte r = random(_minBrightness, _maxBrightness);
    byte g = random(_minBrightness, _maxBrightness);
    byte b = random(_minBrightness, _maxBrightness);
    setSubStripTargets(p_low, p_high, r, g, b);
}

void LEDStrip::twinkle(byte r, byte g, byte b, float dr, float dg, float db, float dl){
    for(int i = 0; i < _LEDCount; ++i){
        float dl_out = random(100-dl, 100)/100.0;
        float dr_coef = random(-dr, dr)/100.0;
        float dg_coef = random(-dg, dg)/100.0;
        float db_coef = random(-db, db)/100.0;
        byte r_out = constrain(r * (1 + dr_coef) * dl_out, _minBrightness, _maxBrightness);
        byte g_out = constrain(g * (1 + dg_coef) * dl_out, _minBrightness, _maxBrightness);
        byte b_out = constrain(b * (1 + db_coef) * dl_out, _minBrightness, _maxBrightness);
        setPixel(i, r_out, g_out, b_out);
    }
}