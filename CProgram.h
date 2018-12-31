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

    bool NoDelay = false;
    uint16_t TicksPerCycle;
    const char* Name;
    bool IncludeInAutoProgram;
  protected:
};

#endif //CPROGRAM_H
