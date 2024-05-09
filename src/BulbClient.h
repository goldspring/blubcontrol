#pragma once

class BulbClient {
public:
    /**/virtual bool GetState() = 0;
    /**/virtual bool TurnOn() = 0;
    /**/virtual bool TurnOff() = 0;
    
    /**/virtual bool SetRelativeLuminance(int relativeLevel) = 0;
    /**/virtual bool SetRgb(int r, int g, int b) = 0;
    /**/virtual bool SetTemperatureByAltitude(double altitude) = 0;
    /**/virtual bool SetTemperature(int temperature) = 0;
    virtual ~BulbClient(){};
};
