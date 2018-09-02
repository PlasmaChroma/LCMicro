// callback function prototypes

enum ControlMode
{
  MODE_FIXED_RPM = 0,
  MODE_MIN_MAX = 1
};

void helpCallback(char* tokens);
void resetCallback(char* tokens);
void brightCallback(char* tokens);
void statusCallback(char* tokens);
