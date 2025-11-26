#ifndef STEPPER_MANAGER_H
#define STEPPER_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Default stepper pins (can be changed via config)
#define STEPPER_PIN1_DEFAULT 25
#define STEPPER_PIN2_DEFAULT 26
#define STEPPER_PIN3_DEFAULT 27
#define STEPPER_PIN4_DEFAULT 14

// Stepper configuration
#define STEPPER_STEPS_PER_REV 2048  // 28BYJ-48 has 2048 steps per revolution
#define STEPPER_MIN_SPEED 1
#define STEPPER_MAX_SPEED 15

class StepperManager {
public:
    struct StepperConfig {
        int pin1;
        int pin2;
        int pin3;
        int pin4;
        int speed;           // RPM (1-15 for 28BYJ-48)
        int stepsPerRev;     // Steps per revolution
        bool enabled;
    };

    StepperManager();
    ~StepperManager();

    // Initialize stepper motor
    void begin();

    // Load configuration from JSON
    bool loadConfig(const JsonDocument& doc);

    // Save configuration to JSON
    void saveConfig(JsonDocument& doc);

    // Get current configuration
    const StepperConfig& getConfig() const { return config; }

    // Update configuration
    void updateConfig(int pin1, int pin2, int pin3, int pin4, int speed, int stepsPerRev, bool enabled);

    // Update only pins
    void updatePins(int pin1, int pin2, int pin3, int pin4);

    // Update speed
    void setSpeed(int speed);

    // Motor control
    void step(int steps);           // Move specific number of steps (positive = CW, negative = CCW)
    void rotate(float degrees);     // Rotate by degrees
    void rotateRevolutions(float revs);  // Rotate by full revolutions
    void stop();                    // Stop motor and release coils

    // Status
    bool isMoving() const { return moving; }
    bool isEnabled() const { return config.enabled; }
    int getCurrentPosition() const { return currentPosition; }
    void resetPosition();           // Reset position counter to 0

private:
    StepperConfig config;
    int stepSequence;               // Current step in sequence (0-7)
    int currentPosition;            // Current position in steps
    bool moving;
    unsigned long stepDelay;        // Delay between steps in microseconds

    void applyConfig();
    void initPins();
    void setStep(int step);
    void releaseCoils();
    void calculateStepDelay();
};

#endif // STEPPER_MANAGER_H
