#include <pebble.h>
#include "test_unit.h"

#define DEC_24_2014_23_55_00 1419465300
#define FEB_12_2015_23_55_00 1423785300
#define NOV_26_2014_23_55_00 1417046100
#define JAN_01_2015_00_00_00 1420070400
  
#define TEST_COUNT 26

typedef struct {
  time_t startTime;
  uint16_t stepSeconds;
  uint16_t stepCount;
} TestData;

static TestData _testData[TEST_COUNT];

TestUnitData* CreateTestUnit() {
  TestUnitData* data = malloc(sizeof(TestUnitData));
  if (data != NULL) {
    memset(data, 0, sizeof(TestUnitData));
        
    // Friday the 13th
    uint16_t testIndex = 0;
    
    // Run up to minute 19
    _testData[testIndex].startTime = FEB_12_2015_23_55_00;
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 24;
    testIndex++;
    
    // Pause at minute 19.
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (24 * 60);
    _testData[testIndex].stepSeconds = 1;
    _testData[testIndex].stepCount = 8;
    testIndex++;
    
    // Run up to minute 30
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (25 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 10;
    testIndex++;
    
    // Pause at minute 30
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (35 * 60);
    _testData[testIndex].stepSeconds = 1;
    _testData[testIndex].stepCount = 8;
    testIndex++;
    
    // Run up to minute 41
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (36 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 10;
    testIndex++;
    
    // Pause at minute 41
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (46 * 60);
    _testData[testIndex].stepSeconds = 1;
    _testData[testIndex].stepCount = 8;
    testIndex++;
    
    // Run up to minute 51
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (47 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 9;
    testIndex++;

    // Pause at eat minute to let animation run.
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (56 * 60);
    _testData[testIndex].stepSeconds = 1;
    _testData[testIndex].stepCount = 15;
    testIndex++;
    
    _testData[testIndex].startTime = FEB_12_2015_23_55_00 + (57 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 13;
    testIndex++;

    // Christmas
    
    // Increment by 5 minutes and then effectively pause for 10 seconds to let
    // animation run.
    for (int index = 0; index < 7; index++) {
      _testData[testIndex].startTime = DEC_24_2014_23_55_00 + (5 * index * 60);
      _testData[testIndex].stepSeconds = 60;
      _testData[testIndex].stepCount = 5;
      testIndex++;
  
      _testData[testIndex].startTime = DEC_24_2014_23_55_00 + (5 * (index + 1) * 60);
      _testData[testIndex].stepSeconds = 1;
      _testData[testIndex].stepCount = 10;
      testIndex++;
    }
    
    _testData[testIndex].startTime = DEC_24_2014_23_55_00 + (35 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 35;
    testIndex++;
    
    // Thanksgiving
    _testData[testIndex].startTime = NOV_26_2014_23_55_00;
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 70;
    testIndex++;
    
    // Normal. Increment by 61 minutes so hour and minute changes.
    _testData[testIndex].startTime = JAN_01_2015_00_00_00;
    _testData[testIndex].stepSeconds = 3660;
    _testData[testIndex].stepCount = 60;
    testIndex++;
  }
  
  return data;
}

void DestroyTestUnit(TestUnitData* data) {
  if (data != NULL) {
    free(data);
  }
}

time_t TestUnitGetTime(TestUnitData* data) {
  // Check if we need to roll over to next test.
  if (data->stepIndex >= _testData[data->testIndex].stepCount) {
    data->stepIndex = 0;
    data->testIndex++;
    if (data->testIndex >= TEST_COUNT) {
      data->testIndex = 0;
    }
  }
  
  // Check if first step of TestData
  if (data->stepIndex == 0) {
    data->time = _testData[data->testIndex].startTime;
    
  } else {
    data->time += _testData[data->testIndex].stepSeconds;
  }
  
  data->stepIndex++;
    
  return data->time;
}