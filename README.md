# industrial-light-monitoring-fault-detection
Adaptive industrial light monitoring system with fault detection. Continuously measures ambient light, detects abnormal changes, analyzes sensor stability, and provides visual/audio alerts using LEDs and a buzzer. Supports EEPROM persistence, day/night adaptive thresholds, and event-driven button calibration.


## Industrial Light Monitoring & Fault Detection System
	
**Author:** Ali Sbeity  
**Version:** 1.0  
**Copyright:** (c) 2026 Ali Sbeity  
**License:** MIT License


## Overview

This project implements an industrial-style Light Monitoring and Fault Detection System using Arduino.

The system continuously measures ambient light intensity through an LDR sensor, applies digital filtering, evaluates deviation from a calibrated reference, and detects abnormal variations using hysteresis-based decision logic.

A structured Finite State Machine (FSM) ensures deterministic operation, non-blocking timing, and robust event-driven behavior.



## System Architecture

The system is composed of four functional subsystems:

### Sensor Subsystem
- LDR voltage divider
- Analog signal acquisition (ADC)
- Optional RC hardware filtering
- Raw signal sampling

### Processing Subsystem
- Exponential Moving Average (EMA) filtering
- Adaptive threshold calculation(Day/Night mode)
- Hysteresis-based alarm decision
- Sudden change detection
- Noise window analysis
- FSM-based control logic
- Non-blocking timing using millis()
- Event-driven button handling
- Software debounce implementation

### Output Subsystem
- 4-LED bar graph (intensity visualization)
- Alarm pattern signaling
- Noise alert signaling
- Passive buzzer output

### Memory Subsystem
- EEPROM storage of: 
 - Calibration reference
 - Sensitivity level
 - Alarm counter
 - Noise counter
 - This ensures parameter
 - retention after power cycling.


## Hardware Design

### Components
- Arduino Uno (or compatible board)
- LDR + fixed resistor (voltage divider configuration)
- 4 LEDs
- Passive buzzer
- 2 push buttons (Mode / Calibration)
- Optional RC filter capacitor

### Signal Conditioning

An Exponential Moving Average (EMA) is applied:

y(k) = (1-\alpha)y(k-1) + \alpha x(k)

Where:
Alpha = 0.1
X(k) = raw ADC reading
Y(k) = filtered value

This reduces measurement noise and prevents unstable decision behavior.


## Pin Configuration

|Function|  |pin|
|--------|--|---|
|LDR input| |A0|
|Mode Button| |D2|
|Calibration Button| |D3|
|LED1-LED4| |D8-D11|
|Buzzer| |D6|


## Finite State Machine (FSM)

The system operates in four states:

### MONITORING
- Continuous filtered light measurement
- Bar graph visualization
- Hysteresis evaluation
- Sudden change detection

### CALIBRATION
- 100-sample averaging
- Reference value update
- EEPROM storage

### ALARM

Triggered when: 
- Deviation exceeds hysteresis ON threshold
- Sudden change threshold is exceeded

Action:
- 2-second alarm pattern
- Event counter increment
- EEPROM update

### NOISE
Triggered:
- when signal fluctuation exceeds noise window limit

Action:
- Fast blinking LED
- high-frequency buzzer
- Counter increment
- Returns to MONITORING when stable


## Demonstration Video

A short demonstration of the system in operation is available here:

[LinkedIn]

[Youtube]


## Hardware Schematic

The complete circuit schematic, is available here: [industrial-light-monitoring-fault-detection-schematic]


## Code snippet

```cp
Float errorRef  = abs(filteredValue – referenceLight);
Float errorRate = abs(filteredValue – previousFiltered);

Switch (currentState) {
  Case MONITORING:
    displayBar(filteredValue);  // LED bar graph
    if (errorRate > suddenThreshold && errorRef > alarmOnThreshold) {
      currentState = ALARM;
      alarmStartTime = millis();
    }
    Break;

  Case ALARM:
    alarmPattern();  // LE“ blink + Buzzer
    if (millis() - alarmStartTime > alarmDuration) {
      currentState = MONITORING;
    }
    Break;

  Case NOISE:
    noisePattern();  // Noise alert
    if (!noiseDetected && errorRef < alarmOffThreshold) {
      currentState = MONITORING;
    }
    Break;

  Case CALIBRATION:
    startCalibration(); // Measure environment & store reference
    break;
}
```

Full source code:
The complete Arduino sketch for this project is provided below. You can view or download the code from the file: [industrial-light-monitoring-fault-detection]


## Control Logic

### Hysteresis Decision Mechanism

Two thresholds are defined:
- Alarm ON Threshold
- Alarm OFF Threshold

Alarm(ON) > Alarm(OFF)

This creates a dead-band region that prevents oscillatory behavior near boundary values.

An internal latch (alarmLatched) preserves decision stability until the error falls below the OFF threshold.

### Adaptive Threshold (Day/Night Mode)
Threshold sensitivity is automatically scaled depending on ambient light conditions, improving robustness under low-light operation.

### Sudden Change Detection
Rate of change is evaluated:

|Filtered(k) - Filtered(k-1)|

Alarm triggers only when:
- Deviation from reference exceeds ON threshold
- Sudden change exceeds rate threshold

This prevents slow drift from causing false alarms.

### Noise Window Analysis
Within a 1-second window:

Noise = Max - Min

If noise exceeds limit:
- System enters NOISE state
- Event counter increments

### Non-Blocking Timing	
All timing operations use:

millis()

This ensures:
- Responsive buttons
- Stable FSM transitions
- Continuous monitoring without delay()


## Key Software Concepts
- Finite State Machine (FSM)
- Exponential Moving Average filtering
- Hysteresis-based decision stabilization
- Adaptive thresholding
- Event-driven alarm logic
- Rate-of-change detection
- EEPROM persistence
- Software debounce
- Non-blocking real-time timing


## Learning Outcomes

After completing this project, the following competencies are demonstrated:

- Structured embedded system architecture
- Industrial fault detection logic
- Signal conditioning and filtering
- Decision stabilization using hysteresis
- Robust FSM implementation
- Persistent parameter management
- Real-time non-blocking embedded programming


## Future Improvements

- Serial data logging for long-term analysis
- Adaptive noise threshold
- Dynamic alarm duration scaling
- LCD diagnostic interface
- IoT remote monitoring integration


**Copyright:** (c) 2026 Ali Sbeity  
**License:** MIT License
