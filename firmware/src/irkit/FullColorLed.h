#ifndef FullColorLed_h
#define FullColorLed_h

//クラス定義
class FullColorLed
{
public:
  //コンストラクタ
    FullColorLed(int R_pin, int G_pin, int B_pin);
    void SetLedColor(bool REDColor, bool BLUEColor, bool GREENColor);
    void LedOff();
  
private:
    int REDLEDpin;
    int GREENLEDpin;
    int BLUELEDpin;
};

#endif


