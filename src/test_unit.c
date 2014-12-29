#include <pebble.h>
#include "test_unit.h"

#define DEC_25_2014_00_00_00 1419465600
#define FEB_13_2015_00_00_00 1423785600
#define NOV_27_2014_00_00_00 1417046400
#define JAN_01_2015_00_00_00 1420070400
  
#define TEST_COUNT 24

typedef struct {
  time_t startTime;
  uint16_t stepSeconds;
  uint16_t stepCount;
  uint16_t endPauseCount;
} TestData;

static TestData _testData[TEST_COUNT];

TestUnitData* CreateTestUnit() {
  TestUnitData* data = malloc(sizeof(TestUnitData));
  if (data != NULL) {
    memset(data, 0, sizeof(TestUnitData));
        
    uint16_t testIndex = 0;
    
/*
    _testData[testIndex].startTime = JAN_01_2015_00_00_00 + (58 * 60);
    _testData[testIndex].stepSeconds = 0;
    _testData[testIndex].stepCount = 30;
    _testData[testIndex].endPauseCount = 15;
    testIndex++;
*/
    
/*
    for (int index = 0; index < 20; index++) {
      _testData[testIndex].startTime = JAN_01_2015_00_00_00 + (3 * index * 60);
      _testData[testIndex].stepSeconds = 0;
      _testData[testIndex].stepCount = 0;
      _testData[testIndex].endPauseCount = 15;
      testIndex++;
    }
*/

/*    
    // Run up to minute 50
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (45 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 5;
    _testData[testIndex].endPauseCount = SHARK_ANIMATION_DURATION / 1000;
    testIndex++;
    
    // Run minute 51 and pause for eat minute
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (51 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 0;
    _testData[testIndex].endPauseCount = 8;
    testIndex++;
    
    // Run up to minute 0 and pause. This lets the exited flag reset and the duck fly in.
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (52 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 8;
    _testData[testIndex].endPauseCount = 2;
    testIndex++;
*/    


    // Friday the 13th
    
    // Run up to minute 0 and pause for fly-in
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 - (5 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 5;
    _testData[testIndex].endPauseCount = 5;
    testIndex++;

    // Run up to minute 20 and pause
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (1 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 19;
    _testData[testIndex].endPauseCount = SHARK_ANIMATION_DURATION / 1000;
    testIndex++;
 
    // Wait for shark to pass by starting at minute 20 to 50
    for (int index = 0; index < 6; index++) {
      _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (21 * 60) + (5 * index * 60);
      _testData[testIndex].stepSeconds = 60;
      _testData[testIndex].stepCount = 4;
      _testData[testIndex].endPauseCount = SHARK_ANIMATION_DURATION / 1000;
      testIndex++;
    }
    
    // Run minute 51 and pause for eat minute
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (51 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 0;
    _testData[testIndex].endPauseCount = 10;
    testIndex++;
    
    // Run up to minute 0 and pause for fly-in
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (52 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 8;
    _testData[testIndex].endPauseCount = 5;
    testIndex++;
    
    // Run to 5 minutes past
    _testData[testIndex].startTime = FEB_13_2015_00_00_00 + (61 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 4;
    testIndex++;

    // Christmas
    
    // Start at 5 minutes before the hour, run to 30 minutes past, 
    // increment by 5 minutes, and then pause to let animation run.
    for (int index = 0; index < 7; index++) {
      _testData[testIndex].startTime = DEC_25_2014_00_00_00 - (5 * 60) + (5 * index * 60);
      _testData[testIndex].stepSeconds = 60;
      _testData[testIndex].stepCount = 5;
      _testData[testIndex].endPauseCount = SANTA_ANIMATION_DURATION / 1000;
      testIndex++;
    }
    
    // Run up to minute 0 and pause for fly-in
    _testData[testIndex].startTime = DEC_25_2014_00_00_00 + (30 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 30;
    _testData[testIndex].endPauseCount = 5;
    testIndex++;
    
    // Run to 5 minutes past
    _testData[testIndex].startTime = DEC_25_2014_00_00_00 + (61 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 4;
    testIndex++;
    
    // Thanksgiving
    
    // Start at 5 minutes before the hour
    _testData[testIndex].startTime = NOV_27_2014_00_00_00 - (5 * 60);
    _testData[testIndex].stepSeconds = 60;
    _testData[testIndex].stepCount = 70;
    testIndex++;
   
    // Normal. Increment by 61 minutes so hour and minute changes.
    
    // Start at 5 minutes before the hour and run to minute 0
    _testData[testIndex].startTime = JAN_01_2015_00_00_00 - (5 * 60);
    _testData[testIndex].stepSeconds = 3660;
    _testData[testIndex].stepCount = 5;
    _testData[testIndex].endPauseCount = 5;
   testIndex++;
    
    // Run to minute 0 and pause for fly-in
    _testData[testIndex].startTime = JAN_01_2015_00_00_00 + (1 * 60);
    _testData[testIndex].stepSeconds = 3660;
    _testData[testIndex].stepCount = 59;
    _testData[testIndex].endPauseCount = 5;
    testIndex++;
     
    // Run to 5 minutes past
    _testData[testIndex].startTime = JAN_01_2015_00_00_00 + (61 * 60);
    _testData[testIndex].stepSeconds = 3660;
    _testData[testIndex].stepCount = 4;
    testIndex++;

  }
  
  return data;
}

void DestroyTestUnit(TestUnitData *data) {
  if (data != NULL) {
    free(data);
  }
}

time_t TestUnitGetTime(TestUnitData *data) {
  // Check if we need to roll over to next test.
  if (data->stepIndex > _testData[data->testIndex].stepCount) {
    // Run through the pause count
    if (data->endPauseRemaining > 0) {
      data->endPauseRemaining--;
      return data->time;
    }
    
    data->stepIndex = 0;
    data->testIndex++;
    if (data->testIndex >= TEST_COUNT) {
      data->testIndex = 0;
    }
  }
  
  // Check if first step of TestData
  if (data->stepIndex == 0) {
    data->time = _testData[data->testIndex].startTime;
    data->endPauseRemaining = _testData[data->testIndex].endPauseCount;
    
  } else {
    data->time += _testData[data->testIndex].stepSeconds;
  }
  
  data->stepIndex++;
    
  return data->time;
}