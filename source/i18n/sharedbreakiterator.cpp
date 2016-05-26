/*
*******************************************************************************
* Copyright (C) 2016 and later: Unicode, Inc. and others.     
* License & terms of use: http://www.unicode.org/copyright.html                                                
*******************************************************************************
*
* File RELDATEFMTTEST.CPP
*
*******************************************************************************
*/
#include "sharedbreakiterator.h"
#include "unicode/brkiter.h"

#if !UCONFIG_NO_BREAK_ITERATION

U_NAMESPACE_BEGIN

SharedBreakIterator::SharedBreakIterator(
        BreakIterator *biToAdopt) : ptr(biToAdopt) { }

SharedBreakIterator::~SharedBreakIterator() {
  delete ptr;
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_BREAK_ITERATION */
