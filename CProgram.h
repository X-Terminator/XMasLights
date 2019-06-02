#ifndef CPROGRAM_H
#define CPROGRAM_H

class CLEDProgram 
{
  protected:
    CLEDProgram(const char *inName = "") : IncludeInAutoProgram(true) { Name = inName; TicksPerCycle = NUM_LEDS; };
  public:
    virtual bool Start() { return true; };
    virtual bool Update() = 0;
    bool Stop() { return true; };
    virtual int GetUpdatePeriod(uint8_t inSpeed) {
        unsigned long lvUpdatePeriodMs = ((MAX_CYCLE_TIME_MS * (255 - g_GlobalSettings.Speed)) / 255);
        if (TicksPerCycle > 0)
        {
          lvUpdatePeriodMs /= TicksPerCycle;
        }
        return lvUpdatePeriodMs;
      };
    
    bool NoDelay = false;
    uint16_t TicksPerCycle;
    const char* Name;
    bool IncludeInAutoProgram;
  protected:
};

#endif //CPROGRAM_H
