#pragma once

#include <stdio.h>
#include "pico/stdlib.h"

#include "../include/properties.h"

class Gpio
    { 
	/*************************************************************************\
    |* Public Enums in this namespace
    \*************************************************************************/
    public:
        typedef enum 
            {
            PIN0        = 0,
            PIN1,
            PIN2,
            PIN3,
            PIN4,
            PIN5,
            PIN6,
            PIN7,
            PIN8,
            PIN9,
            PIN10,
            PIN11,
            PIN12,
            PIN13,
            PIN14,
            PIN15,
            PIN16,
            PIN17,
            PIN18,
            PIN19,
            PIN20,
            PIN21,
            PIN22,
            PIN23,
            PIN24,
            PIN25,
            PIN26,
            PIN27,
            PIN28,
            PIN29,
            PIN_MAX
            } Pin;

        typedef enum 
            {
            LO    = 0,
            HI
            } State;

        typedef enum 
            {
            INPUT  = 0,
            OUTPUT
            } Level;
  
    private:
        
    public:
        /*********************************************************************\
        |* Constructors and Destructor
        \*********************************************************************/
        explicit Gpio(void);

        /*********************************************************************\
        |* Initialise the set of GPIOs to a known state
        \*********************************************************************/
        void init(void);

        /*********************************************************************\
        |* Set a configured-to-be-output pin to a given state
        \*********************************************************************/
        void set(int pin, State state);

	};

