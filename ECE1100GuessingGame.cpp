* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "Nav_Switch.h"
#include "uLCD_4DGL.h"
#include <cstdio>

// Component setup
Nav_Switch myNav(p12, p15, p14, p16, p13);  // Nav switch
uLCD_4DGL uLCD(p9, p10, p11);               // uLCD display
BusOut mbedleds(LED1, LED2, LED3, LED4);    // LEDs for feedback
DigitalIn button1(p21);                      // Push buttons
DigitalIn button2(p22);
DigitalIn button3(p23);
DigitalIn button4(p24);
PwmOut speaker(p25);                         // Speaker for sound feedback
AnalogOut DACout(p18);                       // Analog output

// Game variables
int target_number = 0;                       // Target number to guess
int guess = 1;                               // Current guess
int attempts = 0;                            // Number of attempts

// Generate a new random target number between 1 and 10
void generate_random_number() {
    srand(time(NULL));
    target_number = rand() % 10 + 1;
}

// Display the initial game instructions
void draw_start_screen() {
    uLCD.cls();
    uLCD.text_width(2);
    uLCD.text_height(2);
    uLCD.color(WHITE);
    uLCD.locate(1, 1);
    uLCD.printf("Number\nMatch!");
    uLCD.locate(0, 7);
    uLCD.text_width(1);
    uLCD.printf("Use Nav to Guess");
}

// Display feedback for the player's guess
void display_hint() {
    uLCD.cls();
    uLCD.text_width(2);
    uLCD.text_height(2);
    uLCD.color(WHITE);
    uLCD.locate(1, 0);
    uLCD.printf("Guess: %d", guess);
    uLCD.locate(1, 2);

    if (guess < target_number) {
        uLCD.color(BLUE);
        uLCD.printf("Try Higher!");
    } else if (guess > target_number) {
        uLCD.color(RED);
        uLCD.printf("Try Lower!");
    } else {
        uLCD.color(GREEN);
        uLCD.printf("Correct!\nAttempts: %d", attempts);
        speaker.period(1.0 / 500.0); // Success sound
        speaker = 0.5;               // 50% duty cycle
        ThisThread::sleep_for(200ms);
        speaker = 0.0;
    }
}

// Play a buzz sound for incorrect guess feedback
void play_buzz() {
    speaker.period(1.0 / 250.0);  // Set frequency to 250 Hz
    speaker = 0.5;                // 50% duty cycle
    ThisThread::sleep_for(100ms); // Play buzz for 100 ms
    speaker = 0.0;                // Turn off speaker
}

// Main program
int main() {
    // Initial setup
    uLCD.baudrate(3000000);
    button1.mode(PullUp);
    button2.mode(PullUp);
    button3.mode(PullUp);
    button4.mode(PullUp);

    generate_random_number();      // Generate the initial random number
    draw_start_screen();

    while (1) {
        // Update guess with nav switch
        if (myNav.up()) {
            guess = (guess < 10) ? guess + 1 : 1; // Increment guess with wrap-around
            display_hint();
            ThisThread::sleep_for(200ms);         // Debounce
        } else if (myNav.down()) {
            guess = (guess > 1) ? guess - 1 : 10; // Decrement guess with wrap-around
            display_hint();
            ThisThread::sleep_for(200ms);         // Debounce
        }

        // Submit guess with center button
        if (myNav.center()) {
            attempts++;                           // Increment attempts counter
            display_hint();

            if (guess == target_number) {
                // Show success message and reset game after a short delay
                mbedleds = 0xF;                   // Light up all LEDs for success
                ThisThread::sleep_for(2s);        // Display success for 2 seconds
                mbedleds = 0x0;                   // Turn off LEDs

                generate_random_number();         // Generate new target
                guess = 1;                        // Reset guess to 1
                attempts = 0;                     // Reset attempts
                draw_start_screen();              // Reset the screen
            } else {
                play_buzz();                      // Play buzz for incorrect guess
                mbedleds = 1 << ((attempts - 1) % 4); // Light up LEDs as progress indicator
            }
            ThisThread::sleep_for(500ms);         // Debounce
        }

        // Check button actions
        if (!button1) { // Button 1: reset game
            generate_random_number();
            guess = 1;
            attempts = 0;
            mbedleds = 0x0;                       // Turn off LEDs
            draw_start_screen();
            ThisThread::sleep_for(500ms);         // Debounce
        }
        if (!button2) { // Button 2: hint mode (higher/lower only)
            uLCD.cls();
            uLCD.locate(1, 1);
            if (guess < target_number) {
                uLCD.printf("Hint: Higher");
            } else if (guess > target_number) {
                uLCD.printf("Hint: Lower");
            }
            ThisThread::sleep_for(1s);
            display_hint();                       // Return to hint display
        }
        if (!button3) { // Button 3: Increase difficulty (1-15 range)
            target_number = rand() % 15 + 1;
            uLCD.cls();
            uLCD.locate(1, 1);
            uLCD.printf("Range: 1-15");
            ThisThread::sleep_for(1s);
            display_hint();
        }
        if (!button4) { // Button 4: Easy mode (1-5 range)
            target_number = rand() % 5 + 1;
            uLCD.cls();
            uLCD.locate(1, 1);
            uLCD.printf("Range: 1-5");
            ThisThread::sleep_for(1s);
            display_hint();
        }
    }
}
