#include "stepper_manager.h"

// Half-step sequence for 28BYJ-48 (8 steps for smoother movement)
const int STEP_SEQUENCE[8][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

StepperManager::StepperManager() : stepSequence(0), currentPosition(0), moving(false), stepDelay(2000) {
    // Default configuration
    config.pin1 = STEPPER_PIN1_DEFAULT;
    config.pin2 = STEPPER_PIN2_DEFAULT;
    config.pin3 = STEPPER_PIN3_DEFAULT;
    config.pin4 = STEPPER_PIN4_DEFAULT;
    config.speed = 10;  // Default 10 RPM
    config.stepsPerRev = STEPPER_STEPS_PER_REV;
    config.enabled = true;
}

StepperManager::~StepperManager() {
    releaseCoils();
}

void StepperManager::begin() {
    if (config.enabled) {
        initPins();
        calculateStepDelay();
        Serial.println("Stepper Manager initialized");
        Serial.printf("Pins: %d, %d, %d, %d\n", config.pin1, config.pin2, config.pin3, config.pin4);
        Serial.printf("Speed: %d RPM, Steps/Rev: %d\n", config.speed, config.stepsPerRev);
    }
}

void StepperManager::initPins() {
    pinMode(config.pin1, OUTPUT);
    pinMode(config.pin2, OUTPUT);
    pinMode(config.pin3, OUTPUT);
    pinMode(config.pin4, OUTPUT);
    releaseCoils();
}

void StepperManager::releaseCoils() {
    digitalWrite(config.pin1, LOW);
    digitalWrite(config.pin2, LOW);
    digitalWrite(config.pin3, LOW);
    digitalWrite(config.pin4, LOW);
}

void StepperManager::calculateStepDelay() {
    // Calculate delay in microseconds based on RPM
    // For half-stepping: 8 sequences per 4 steps = 4096 sequences per revolution
    // delay = (60 * 1000000) / (RPM * 4096)
    if (config.speed > 0) {
        stepDelay = (60UL * 1000000UL) / ((unsigned long)config.speed * 4096UL);
        if (stepDelay < 800) stepDelay = 800;  // Minimum delay for 28BYJ-48
    }
}

bool StepperManager::loadConfig(const JsonDocument& doc) {
    if (doc.containsKey("stepper")) {
        JsonObjectConst stepper = doc["stepper"].as<JsonObjectConst>();

        if (stepper.containsKey("pin1")) config.pin1 = stepper["pin1"];
        if (stepper.containsKey("pin2")) config.pin2 = stepper["pin2"];
        if (stepper.containsKey("pin3")) config.pin3 = stepper["pin3"];
        if (stepper.containsKey("pin4")) config.pin4 = stepper["pin4"];
        if (stepper.containsKey("speed")) config.speed = stepper["speed"];
        if (stepper.containsKey("steps_per_rev")) config.stepsPerRev = stepper["steps_per_rev"];
        if (stepper.containsKey("enabled")) config.enabled = stepper["enabled"];

        // Validate speed
        if (config.speed < STEPPER_MIN_SPEED) config.speed = STEPPER_MIN_SPEED;
        if (config.speed > STEPPER_MAX_SPEED) config.speed = STEPPER_MAX_SPEED;

        calculateStepDelay();
        return true;
    }
    return false;
}

void StepperManager::saveConfig(JsonDocument& doc) {
    JsonObject stepper = doc.createNestedObject("stepper");
    stepper["pin1"] = config.pin1;
    stepper["pin2"] = config.pin2;
    stepper["pin3"] = config.pin3;
    stepper["pin4"] = config.pin4;
    stepper["speed"] = config.speed;
    stepper["steps_per_rev"] = config.stepsPerRev;
    stepper["enabled"] = config.enabled;
}

void StepperManager::updateConfig(int pin1, int pin2, int pin3, int pin4, int speed, int stepsPerRev, bool enabled) {
    config.pin1 = pin1;
    config.pin2 = pin2;
    config.pin3 = pin3;
    config.pin4 = pin4;
    config.speed = constrain(speed, STEPPER_MIN_SPEED, STEPPER_MAX_SPEED);
    config.stepsPerRev = stepsPerRev;
    config.enabled = enabled;
    applyConfig();
}

void StepperManager::updatePins(int pin1, int pin2, int pin3, int pin4) {
    releaseCoils();  // Release old pins first
    config.pin1 = pin1;
    config.pin2 = pin2;
    config.pin3 = pin3;
    config.pin4 = pin4;
    initPins();
}

void StepperManager::setSpeed(int speed) {
    config.speed = constrain(speed, STEPPER_MIN_SPEED, STEPPER_MAX_SPEED);
    calculateStepDelay();
}

void StepperManager::applyConfig() {
    if (config.enabled) {
        initPins();
        calculateStepDelay();
    } else {
        releaseCoils();
    }
}

void StepperManager::setStep(int step) {
    // Apply the step sequence to pins
    digitalWrite(config.pin1, STEP_SEQUENCE[step][0]);
    digitalWrite(config.pin2, STEP_SEQUENCE[step][1]);
    digitalWrite(config.pin3, STEP_SEQUENCE[step][2]);
    digitalWrite(config.pin4, STEP_SEQUENCE[step][3]);
}

void StepperManager::step(int steps) {
    if (!config.enabled) return;

    moving = true;
    int direction = (steps > 0) ? 1 : -1;
    int stepsToMove = abs(steps) * 2;  // Multiply by 2 for half-stepping

    for (int i = 0; i < stepsToMove && moving; i++) {
        stepSequence += direction;
        if (stepSequence > 7) stepSequence = 0;
        if (stepSequence < 0) stepSequence = 7;

        setStep(stepSequence);
        delayMicroseconds(stepDelay);

        // Update position every 2 half-steps = 1 full step
        if (i % 2 == 1) {
            currentPosition += direction;
        }

        // Yield to prevent watchdog timeout on long movements
        if (i % 100 == 0) {
            yield();
        }
    }

    releaseCoils();
    moving = false;
}

void StepperManager::rotate(float degrees) {
    if (!config.enabled) return;

    // Calculate steps needed for the rotation
    float stepsPerDegree = (float)config.stepsPerRev / 360.0f;
    int stepsNeeded = (int)(degrees * stepsPerDegree);
    step(stepsNeeded);
}

void StepperManager::rotateRevolutions(float revs) {
    if (!config.enabled) return;

    int stepsNeeded = (int)(revs * config.stepsPerRev);
    step(stepsNeeded);
}

void StepperManager::stop() {
    moving = false;
    releaseCoils();
}

void StepperManager::resetPosition() {
    currentPosition = 0;
}
