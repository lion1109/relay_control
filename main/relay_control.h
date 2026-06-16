#pragma once


// This header defines data structures and allows access by unit tests.

#ifdef UNIT_TEST
#pragma message "compiling TEST, non production"
#define STATIC 
#define EXTERN extern 
#else
#pragma message "compiling non TEST, production"
#define STATIC static
#define EXTERN static 
#endif

#ifdef UNIT_TEST
int relay_get_state();
#endif