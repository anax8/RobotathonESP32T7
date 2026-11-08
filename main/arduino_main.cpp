// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>
#include <uni.h>
#include "controller_callbacks.h"


#include <ESP32Servo.h>
#include <ESP32SharpIR.h>
#include <QTRSensors.h>

#include <Wire.h>
#include <Arduino_APDS9960.h>
#include <bits/stdc++.h>

#define APDS9960_INT 0
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FREQ 100000

const char* mode = "teleop";
const char* input_color = "blank";
const char* current_color = "blank";
const char* start_color = "no";

//labelling the pins
#define IN1 17 // motor 1
#define IN2 16
#define IN3 19 // motor 2
#define IN4 18
#define IN5 15 // servo

TwoWire I2C_0 = TwoWire(0);
APDS9960 apds = APDS9960(I2C_0, APDS9960_INT);

extern ControllerPtr myControllers[BP32_MAX_GAMEPADS]; // BP32 library allows for up to 4 concurrent controller connections, but we only need 1

void dumpGamepad(ControllerPtr ctl) {
    Console.printf(
        "DPAD: %d A: %d B: %d X: %d Y: %d LX: %d LY: %d RX: %d RY: %d L1: %d R1: %d L2: %d R2: %d\n",
        ctl->dpad(),        // D-pad
        ctl->a(),           // Letter buttons
        ctl->b(),
        ctl->x(),
        ctl->y(),
        ctl->axisX(),        // (-511 - 512) left X Axis
        ctl->axisY(),        // (-511 - 512) left Y axis
        ctl->axisRX(),       // (-511 - 512) right X axis
        ctl->axisRY(),       // (-511 - 512) right Y axis
        ctl->l1(),           // Bumpers
        ctl->r1(),
        ctl->l2(),
        ctl->r2()
    );
}

Servo servo;
ESP32SharpIR sensor1( ESP32SharpIR::GP2Y0A21YK0F, 27);

QTRSensors qtr;
uint16_t sensors[4];

void setup() {
    Serial.begin(115200);

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys(); 
    esp_log_level_set("gpio", ESP_LOG_ERROR); // Suppress info log spam from gpio_isr_service
    uni_bt_allowlist_set_enabled(true);

    // set up pin mode - AXY
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(IN5, OUTPUT);
    Console.printf("Setup complete\n");

    servo.attach(IN5);

    //sets up I2C protocol
    I2C_0.begin(I2C_SDA, I2C_SCL, I2C_FREQ);

    //sets up color sensor
    apds.setInterruptPin(APDS9960_INT);
    apds.begin();

    // set up Serial Communication and sensor pins for line sensor
    Serial.begin(115200);
    qtr.setTypeAnalog(); // or setTypeAnalog()
    qtr.setSensorPins((const uint8_t[]) {32, 33, 34, 35}, 4); // pin numbers go in the curly brackets {}, and number of sensors in use goes after

    // calibration sequence
    for (uint8_t i = 0; i < 250; i++) { 
        Console.printf("calibrating %d/250\n", i); // 250 is the number of calibrations recommended by manufacturer
        qtr.calibrate(); 
        delay(20);
    }
}

void loop() {
    vTaskDelay(1); // Ensures WDT does not get triggered when no controller is connected
    BP32.update(); 
    for (auto myController : myControllers) { // Only execute code when controller is connected
        if (myController && myController->isConnected() && myController->hasData()) {        
        
int r, g, b, a;

//for mode by controller buttons
if (myController->y()) {
    mode = "autonomous-color";
    Console.printf("Switched to color\n");
    input_color = "blank";
    start_color = "yes";
}
else if (myController->x()) {
    mode = "autonomous-line";
    Console.printf("Switched to line\n");
    start_color = "no";
}
else if (myController->a()) {
    mode = "teleop";
    Console.printf("Switched to teleop\n");
    start_color = "no";
}


if(strcmp(mode, "teleop") == 0) {
            
            Console.printf("Teleop mode\n");

            // test for consol control motors if work above
            int joy = myController->axisY();
            int sad = myController->axisX();
            if (joy<0) {
                // digitalWrite(IN1, 255);
                // digitalWrite(IN2, 0);    // forward
                // digitalWrite(IN3, 255);
                // digitalWrite(IN4, 0); 
                // Console.printf("Forward\n");
                analogWrite(IN1, 0);
                analogWrite(IN2, 255);
                analogWrite(IN3, 0);
                analogWrite(IN4, 255);
                // delay(1000);
            } else if (joy>0) {
                // digitalWrite(IN1, LOW);
                // digitalWrite(IN2, HIGH);   // reverse
                // digitalWrite(IN3, LOW);
                // digitalWrite(IN4, HIGH);
                // Console.printf("Reverse\n");
                analogWrite(IN1, 255);
                analogWrite(IN2, 0);
                analogWrite(IN3, 255);
                analogWrite(IN4, 0);
                // delay(1000);
            } else if (sad > 0) {
                    // digitalWrite(IN1, HIGH);
                    // digitalWrite(IN2, LOW);   // right
                    // digitalWrite(IN3, LOW);
                    // digitalWrite(IN4, LOW);
                // Console.printf("Right\n");
                analogWrite(IN1, 0);
                analogWrite(IN2, 255);
                analogWrite(IN3, 0);
                analogWrite(IN4, 10);
                // delay(1000);
            } else if (sad < 0) {
                // digitalWrite(IN1, LOW);
                // digitalWrite(IN2, LOW);   // left
                // digitalWrite(IN3, HIGH);
                // digitalWrite(IN4, LOW);
                // Console.printf("Left\n");
                analogWrite(IN1, 0);
                analogWrite(IN2, 10);
                analogWrite(IN3, 0);
                analogWrite(IN4, 255);
                // delay(1000);            
            } else {
                // digitalWrite(IN1, LOW);
                // digitalWrite(IN2, LOW);    // stop
                // digitalWrite(IN3, LOW);
                // digitalWrite(IN4, LOW);
                // Console.printf("Stop\n");
                analogWrite(IN1, 0);
                analogWrite(IN2, 0);
                analogWrite(IN3, 0);
                analogWrite(IN4, 0);
                // delay(1000);
            }

            int stick = myController->axisRY();
            if (stick<-10) {
                servo.write(6000);
                // Console.printf("servo forward\n");
                // delay(1000);
            } else if (stick>10) {
                servo.write(0); 
                // Console.printf("servo reverse\n");
                // delay(1000);
            } else {
                servo.write(1500);               
                // Console.printf("servo stopped\n");
                // delay(1000);
            }        
}

else if (strcmp(mode, "autonomous-color") == 0) {

    Console.printf("Autonomous color mode\n");

    while (strcmp(start_color, "yes") == 0) {
        Console.printf("Starting color detection\n");
        // while (!apds.colorAvailable()) { delay(5); } // Wait until color is read from the sensor 
        apds.readColor(r, g, b, a);
        // Console.printf("RED: %d GREEN: %d BLUE: %d AMBIENT: %d\n", r, g, b, a);
            if (r > g and r > b) {
                Console.printf("starting red\n");
                input_color = "red";
                
            } else if (g > r and g > b) {
                Console.printf("starting green\n");
                input_color = "green";
            } else if (b > r and b > g) {
                Console.printf("starting blue\n");
                input_color = "blue";
            } else {
                Console.printf("starting white\n");
                input_color = "white";
            }

        start_color = "no";

        //go forward first a bit

    }

    while (!apds.colorAvailable()) { delay(5); } // Wait until color is read from the sensor 
            apds.readColor(r, g, b, a);
            //Console.printf("RED: %d GREEN: %d BLUE: %d AMBIENT: %d\n", r, g, b, a);
            if (r > g and r > b) {
                Console.printf("current red\n");
                current_color = "red";
            } else if (g > r and g > b) {
                Console.printf("current green\n");
                current_color = "green";
            } else if (b > r and b > g) {
                Console.printf("current blue\n");
                current_color = "blue";
            }
    
    Console.printf("Input color: %s Current color: %s\n", input_color, current_color);
    // if (strcmp(input_color, current_color) == 0) {
    //     Console.printf("Colors match\n");
    //     Serial.println("Stop");
    //     analogWrite(IN1, 0);
    //     analogWrite(IN2, 0);
    //     analogWrite(IN3, 0);
    //     analogWrite(IN4, 0);
    // } else {
    //     Console.printf("Colors do not match\n");
    //     Serial.println("Straight Ahead");  
    //     analogWrite(IN1, 255);
    //     analogWrite(IN2, 0);
    //     analogWrite(IN3, 255);
    //     analogWrite(IN4, 0);
    // }

}

else if (strcmp(mode, "autonomous-line") == 0) {

    Console.printf("Autonomous line mode\n");

            qtr.readLineBlack(sensors); // Get calibrated sensor values returned into sensors[]
            Console.printf("S1: %d S2: %d S3: %d S4: %d\n", sensors[0], sensors[1], sensors[2], sensors[3]);
            delay(100);

            // Serial.println(sensor1.getDistanceFloat());

            // uint16_t sensors[4];
            // int16_t position = qtr.readLineBlack(sensors);
            // int16_t error = position - 1000;
            // if (error < 0)
            // {
            //     Serial.println("On the left");
            // }
            // if (error > 0)
            // {
            //     Serial.println("On the right");
            // }
            if(sensors[0] < 600 && sensors[3] > 900){ // highly dependent on the calibration which has some issues, idk
                Serial.println("Straight Ahead");  
                analogWrite(IN1, 0);
                analogWrite(IN2, 255);
                analogWrite(IN3, 0);
                analogWrite(IN4, 255);
            }
            else {
                Serial.println("Stop");
                analogWrite(IN1, 0);
                analogWrite(IN2, 0);
                analogWrite(IN3, 0);
                analogWrite(IN4, 0);
            }
            // vTaskDelay(1);
            // delay(100);

            // dumpGamepad(myController); // Prints the gamepad state, delete or comment if don't need

        }
    }}}