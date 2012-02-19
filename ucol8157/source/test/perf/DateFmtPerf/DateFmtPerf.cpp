/*
**********************************************************************
* Copyright (c) 2002-2011,International Business Machines
* Corporation and others.  All Rights Reserved.
**********************************************************************
**********************************************************************
*/

#include "DateFmtPerf.h"
#include "uoptions.h"
#include <stdio.h>
#include <fstream>

#include <iostream>
using namespace std;

DateFormatPerfTest::DateFormatPerfTest(int32_t argc, const char* argv[], UErrorCode& status)
: UPerfTest(argc,argv,status) {

    if (locale == NULL){
        locale = "en_US";   // set default locale
    }
}

DateFormatPerfTest::~DateFormatPerfTest()
{
}

UPerfFunction* DateFormatPerfTest::runIndexedTest(int32_t index, UBool exec,const char* &name, char* par) {

	//exec = true;

    switch (index) {
        TESTCASE(0,DateFmt250);
        TESTCASE(1,DateFmt10000);
        TESTCASE(2,DateFmt100000);
        TESTCASE(3,BreakItWord250);
        TESTCASE(4,BreakItWord10000);
        TESTCASE(5,BreakItChar250);
        TESTCASE(6,BreakItChar10000);
        TESTCASE(7,NumFmt10000);
        TESTCASE(8,NumFmt100000);
        TESTCASE(9,Collation10000);
        TESTCASE(10,Collation100000);

        default: 
            name = ""; 
            return NULL;
    }
    return NULL;
}


UPerfFunction* DateFormatPerfTest::DateFmt250(){
    DateFmtFunction* func= new DateFmtFunction(1, locale);
    return func;
}

UPerfFunction* DateFormatPerfTest::DateFmt10000(){
    DateFmtFunction* func= new DateFmtFunction(40, locale);
    return func;
}

UPerfFunction* DateFormatPerfTest::DateFmt100000(){
    DateFmtFunction* func= new DateFmtFunction(400, locale);
    return func;
}

UPerfFunction* DateFormatPerfTest::BreakItWord250(){
    BreakItFunction* func= new BreakItFunction(250, true);
    return func;
}

UPerfFunction* DateFormatPerfTest::BreakItWord10000(){
    BreakItFunction* func= new BreakItFunction(10000, true);
    return func;
}
 
UPerfFunction* DateFormatPerfTest::BreakItChar250(){
    BreakItFunction* func= new BreakItFunction(250, false);
    return func;
}

UPerfFunction* DateFormatPerfTest::BreakItChar10000(){
    BreakItFunction* func= new BreakItFunction(10000, false);
    return func;
}

UPerfFunction* DateFormatPerfTest::NumFmt10000(){
    NumFmtFunction* func= new NumFmtFunction(10000, locale);
    return func;
}

UPerfFunction* DateFormatPerfTest::NumFmt100000(){
    NumFmtFunction* func= new NumFmtFunction(100000, locale);
    return func;
}

UPerfFunction* DateFormatPerfTest::Collation10000(){
    CollationFunction* func= new CollationFunction(40, locale);
    return func;
}

UPerfFunction* DateFormatPerfTest::Collation100000(){
    CollationFunction* func= new CollationFunction(400, locale);
    return func;
}



int main(int argc, const char* argv[]){

    // -x Filename.xml
    if((argc>1)&&(strcmp(argv[1],"-x") == 0))
    {
        if(argc < 3) {
			fprintf(stderr, "Usage: %s -x <outfile>.xml\n", argv[0]);
			return 1;
			// not enough arguments
		}

		cout << "ICU version - " << U_ICU_VERSION << endl;
        UErrorCode status = U_ZERO_ERROR;

#define FUNCTION_COUNT 6
        // Declare functions
        UPerfFunction *functions[FUNCTION_COUNT];

        functions[0] = new DateFmtFunction(40, "en");
        functions[1] = new BreakItFunction(10000, true); // breakIterator word
        functions[2] = new BreakItFunction(10000, false); // breakIterator char
        functions[3] = new NumFmtFunction(100000, "en");
        functions[4] = new CollationFunction(400, "en");
        functions[5] = new StdioNumFmtFunction(100000, "en");

        // Perform time recording
        double t[FUNCTION_COUNT];
        for(int i = 0; i < FUNCTION_COUNT; i++) t[i] = 0;

#define ITER_COUNT 10
#ifdef U_DEBUG
        cout << "Doing " << ITER_COUNT << " iterations:" << endl;
        cout << "__________| Running...\r";
        cout.flush();
#endif
        for(int i = 0; i < ITER_COUNT; i++) {
#ifdef U_DEBUG
          cout << '*' << flush;
#endif
          for(int j = 0; U_SUCCESS(status)&& j < FUNCTION_COUNT; j++)
            t[j] += (functions[j]->time(1, &status) / ITER_COUNT);
        }
#ifdef U_DEBUG
        cout << " Done                   " << endl;
#endif

        if(U_SUCCESS(status)) {

          // Output results as .xml
          ofstream out;
          out.open(argv[2]);

          out << "<perfTestResults icu=\"c\" version=\"" << U_ICU_VERSION << "\">" << endl;

          for(int i = 0; i < FUNCTION_COUNT; i++)
            {
              out << "    <perfTestResult" << endl;
              out << "        test=\"";
              switch(i)
                {
                case 0: out << "DateFormat"; break;
                case 1: out << "BreakIterator Word"; break;
                case 2: out << "BreakIterator Char"; break;
                case 3: out << "NumbFormat"; break;
                case 4: out << "Collation"; break;
                case 5: out << "StdioNumbFormat"; break;
                default: out << "Unknown "  << i; break;
                }
              out << "\"" << endl;
              out << "        iterations=\"" << functions[i]->getOperationsPerIteration() << "\"" << endl;
              out << "        time=\"" << t[i] << "\" />" << endl;
            }
          out << "</perfTestResults>" << endl;
          out.close();
          cout << " Wrote to " << argv[2] << endl;
        }

        if(U_FAILURE(status)) {
          cout << "Error! " << u_errorName(status) << endl;
          return 1;
        }

        return 0;
    }
    
    
    // Normal performance test mode
    UErrorCode status = U_ZERO_ERROR;

    DateFormatPerfTest test(argc, argv, status);


    if(U_FAILURE(status)){   // ERROR HERE!!!
		cout << "initialize failed! " << status << endl;
        return status;
    }
	//cout << "Done initializing!\n" << endl;
    
    if(test.run()==FALSE){
		cout << "run failed!" << endl;
        fprintf(stderr,"FAILED: Tests could not be run please check the arguments.\n");
        return -1;
    }
	cout << "done!" << endl;

    return 0;
}
