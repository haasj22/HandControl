#include <Arduino.h>
#include <Adafruit_MCP3008.h>
#include "PCA9685.h"
#include <iostream>
#include <string>
#include <chrono>

Adafruit_MCP3008 adc;
PCA9685 multi;

#define RX2 16
#define TX2 17

double cycle = 0.0;
double shift = 0.0;

int zeroes[8];
int reading[8];
int targets[8];
int output_pins[8][2];

//for DEMO only
std::chrono::steady_clock::time_point begin;
int swap;

void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RX2, TX2);
    
    adc.begin();
    //pinMode(33, OUTPUT);
    //pinMode(25,OUTPUT);

    //thumb
    zeroes[0] = 640;
    output_pins[0][0] = 14;
    output_pins[0][1] = 27;
    targets[0] = 150;

    //pointer
    zeroes[1] = 160;
    output_pins[1][0] = 12;
    output_pins[1][1] = 13;
    targets[1] = 150;

    //middle
    zeroes[2] = 747;
    output_pins[2][0] = 15;
    output_pins[2][1] = 2;
    targets[2] = 150;

    //ring
    zeroes[3] = 463;
    output_pins[3][0] = 32;
    output_pins[3][1] = 33;
    targets[3] = 150;

    //pinky
    zeroes[4] = 369;
    output_pins[4][0] = 26;
    output_pins[4][1] = 25;
    targets[4] = 150;

    //makes all pins outputs
    for(int i=0; i < 5; i++) {
        pinMode(output_pins[i][0], OUTPUT);
        pinMode(output_pins[i][1], OUTPUT);
    }


    multi.setupSingleDevice(Wire,0x40);
    multi.setToFrequency(60);

    multi.setAllChannelsDutyCycle(cycle, shift);
    
    swap = 0;
    begin = std::chrono::steady_clock::now();
}

//Gets the pid result for position
//only use P part of PID for now
double Kp = 1.;
double calculatePID(int actual, int desired) {
    return Kp * (desired - actual);
}

void loop() {
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    long int duration = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    
    //for testing just switches targets 
    if (duration >= 2000000) {
        //printf("Long\n");
        if (swap == 0) {
            swap = 1;
            for (int x = 0; x < 5; x++) {
                targets[x] = 850;
            }
        } else {
            swap = 0;
            for (int x = 0; x < 5; x++) {
                targets[x] = 150;
            }
        }
        begin = end;
    }

    //CODE FOR GRABBING INPUTS FROM UART

    /*
    for(int x = 0; x < 5; x++) {
        printf("Targets %d: %d", x, targets[x]);
    }
    printf("\n");*/
    //multi.getChannelDutyCycle(0, cycle, shift);
    //Serial.println(cycle);
    //Serial2.write("1");
    
    //Serial.println(Serial2.available());
    

    //SERIAL COMMUNICATION UNUSED FOR DEMO
    /*
    if (Serial2.available() > 0) {
        //Serial.println("In");
        String command = Serial2.readStringUntil('\n');
        Serial.println(command);
        //checks for NULL
        
        if (command.length() >= 1 && command[0] - '0' != -48) {
            targets[0] = 0;
            for ( int i = command.length() -1 ; i >= 0 ; i-- ) {
                int power = command.length() - i -1;
                targets[0] += (std::pow( 10.0,  power) * (command[i] - '0'));
            }
        }
        Serial.print("Target value: ");
        Serial.println(targets[0]);
    }*/


    //gets encoder readings from every encoder
    for (int chan=0; chan<8; chan++) {
        Serial.print("Encoder: ");
        Serial.println(chan);
        reading[chan] = (adc.readADC(chan) + zeroes[chan]) % 1024;
        Serial.println(reading[chan]);
    }

    //updates the pid for each of the fingers
    //does not work for wrist or weird thumb movement
    for(int i = 0; i < 5; i++){
        Serial.print("Channel: ");
        Serial.println(i);
        double result = calculatePID(reading[i], targets[i]);
        Serial.print("Result: ");
        Serial.println(result);
        
        if (result <= 0) {
            digitalWrite(output_pins[i][0], LOW);
            digitalWrite(output_pins[i][1], HIGH);

            int percent = 75 * (1-(1/(1 + (abs(result)/50))));
            Serial.print("Percent: ");
            Serial.println(percent);
            if (percent < 15) {
                multi.setChannelDutyCycle(i, 0, shift);
            } else {
                multi.setChannelDutyCycle(i, 25 + percent, shift);
            }
            Serial.print("Cycle: ");
            multi.getChannelDutyCycle(i, cycle, shift);
            Serial.println(cycle);
        }
        else {
            digitalWrite(output_pins[i][0], HIGH);
            digitalWrite(output_pins[i][1], LOW);

            int percent = 75 * (1-(1/(1 + (abs(result)/50))));
            Serial.print("Percent: ");
            Serial.println(percent);
            if (percent < 15) {
                multi.setChannelDutyCycle(i, 0, shift);
            } else {
                multi.setChannelDutyCycle(i, 25 + percent, shift);
            }
            Serial.print("Cycle: ");
            multi.getChannelDutyCycle(i, cycle, shift);
            Serial.println(cycle);
        }
    }

    delay(50);
}