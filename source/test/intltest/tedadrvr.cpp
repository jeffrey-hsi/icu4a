/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 2002, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

/* Created by weiv 05/09/2002 */

#include "tedadrvr.h"
#include "resbtddr.h"


TestDataDriver *TestDataDriver::createTestInstance(const char *testName, UErrorCode &status) {
  if(U_FAILURE(status)) {
    return NULL;
  }
  TestDataDriver *result = NULL;

  // TODO: probe for resource bundle and then for XML.
  // According to that, construct an appropriate driver object

  result = new ResBundTestDataDriver(testName, status);
  if(U_SUCCESS(status)) {
    return result;
  } else {
    return NULL;
  }

}

TestDataDriver::TestDataDriver(const char* testName) {
  fTestName = testName;
}
  
int32_t 
TestDataDriver::countTests(void) 
{
  return fNumberOfTests;
}
