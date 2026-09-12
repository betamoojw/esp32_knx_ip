# Task: Audit and Implement Complete KNX Datapoint Type (DPT) Support

## 1. Objective

Review, understand, analyze, and modify the current codebase:

The objective is to ensure that the project supports all required KNX Datapoint Types (DPTs) used by the KNX protocol.

You must:

1. Inspect the existing KNX DPT implementation.
2. Identify every supported and unsupported DPT.
3. Compare the implementation against the official KNX DPT specifications.
4. Implement all missing DPT support that is applicable to this project.
5. Fix incorrect, incomplete, or inconsistent existing DPT implementations.
6. Ensure that all implemented DPTs are integrated with the KNX communication, configuration, and MCP layers where applicable.
7. Build and test the complete implementation.

Do not stop after adding a few missing DPTs. The final implementation must provide comprehensive KNX DPT support within the scope of the project's supported KNX communication and application features.

---

## 2. Repository Inspection

Before modifying any code:

- Inspect the complete repository structure.
- Understand the ESP32 firmware architecture.
- Identify the KNX/IP communication implementation.
- Identify all KNX datapoint type definitions.
- Identify KNX datapoint encoding and decoding functions.
- Identify KNX group address read/write handling.
- Identify KNX configuration loading and persistence.
- Identify the KNX MCP tools and their datapoint type handling.
- Identify all relevant unit tests and build configurations.

Pay special attention to:

- KNX-related source and header files.
- KNX/IP protocol implementation.
- KNX datapoint encoding/decoding code.
- KNX configuration files.
- MCP tool implementations.
- Build and test files.

Do not assume that the existing DPT implementation is complete.

---

## 3. Complete KNX DPT Audit

Create a comprehensive inventory of KNX Datapoint Types.

At minimum, audit the following DPT families:

### DPT 1 — 1-bit Boolean

Audit all relevant DPT 1 subtypes, including:

- DPT 1.001 — Switch
- DPT 1.002 — Boolean
- DPT 1.003 — Enable
- DPT 1.004 — Ramp
- DPT 1.005 — Alarm
- DPT 1.006 — Binary Value
- DPT 1.007 — Step
- DPT 1.008 — Up/Down
- DPT 1.009 — Open/Close
- DPT 1.010 — Start/Stop
- DPT 1.011 — State
- DPT 1.012 — Invert
- DPT 1.013 — Dim Send
- DPT 1.014 — Input
- DPT 1.015 — Dimming Control
- DPT 1.016 — Heating/Cooling
- DPT 1.017 — Occupancy
- DPT 1.018 — Window/Door
- DPT 1.019 — Logical Function
- DPT 1.020 — Scene A/B
- DPT 1.021 — Shutter/Blinds
- DPT 1.022 — Day/Night
- DPT 1.023 — Heat/Cool
- DPT 1.100 — Cooling/Heating

Verify whether the project supports the relevant 1-bit DPT subtypes or treats them as generic boolean values.

### DPT 5 — 8-bit Unsigned

Audit:

- DPT 5.001 — Scaling
- DPT 5.003 — Angle
- DPT 5.004 — Percent U8
- DPT 5.005 — Decimal Factor
- DPT 5.006 — Tariff
- DPT 5.010 — Unsigned Counter
- DPT 5.100 — Decimal Factor

Verify scaling, range, rounding, and byte encoding.

### DPT 7 — 2-byte Unsigned

Audit all relevant DPT 7 subtypes, including:

- DPT 7.001 — Unsigned Counter
- DPT 7.002 — Unsigned Integer
- DPT 7.003 — Time Period in Milliseconds
- DPT 7.004 — Time Period in Microseconds
- DPT 7.005 — Time Period in Seconds
- DPT 7.006 — Time Period in Minutes
- DPT 7.007 — Time Period in Hours
- DPT 7.010 — Time Period in Days
- DPT 7.011 — Time Period in Weeks
- DPT 7.012 — Time Period in Months
- DPT 7.013 — Time Period in Years
- DPT 7.600 — Unsigned Integer

Verify correct 16-bit encoding, decoding, ranges, and units.

### DPT 9 — 2-byte Floating Point

Audit:

- DPT 9.001 — Temperature
- DPT 9.002 — Temperature Difference
- DPT 9.003 — Kelvin/Hour
- DPT 9.004 — Illumination
- DPT 9.005 — Wind Speed
- DPT 9.006 — Pressure
- DPT 9.007 — Humidity
- DPT 9.008 — Air Quality
- DPT 9.010 — Time
- DPT 9.011 — Wind Speed
- DPT 9.020 — Volt
- DPT 9.021 — Current
- DPT 9.022 — Power Density
- DPT 9.023 — Kelvin/Percent
- DPT 9.024 — Power
- DPT 9.025 — Volume Flow
- DPT 9.026 — Rain Amount
- DPT 9.027 — Temperature
- DPT 9.028 — Wind Speed
- DPT 9.029 — Pressure
- DPT 9.030 — Humidity

Verify the KNX 16-bit floating-point format, sign handling, exponent, mantissa, resolution, and range.

### DPT 12 — 4-byte Unsigned

Audit:

- DPT 12.001 — Unsigned Counter
- DPT 12.100 — Unsigned Integer

Verify correct 32-bit encoding and decoding.

### DPT 13 — 4-byte Signed Integer

Audit:

- DPT 13.001 — Counter
- DPT 13.002 — Flow Rate
- DPT 13.010 — Active Energy
- DPT 13.011 — Apparent Energy
- DPT 13.012 — Reactive Energy
- DPT 13.013 — Active Energy
- DPT 13.014 — Apparent Energy
- DPT 13.015 — Reactive Energy
- DPT 13.100 — Long Delta Time
- DPT 13.120 — Acceleration

Verify signed 32-bit encoding and unit conversions.

### DPT 14 — 4-byte IEEE 754 Floating Point

Audit all relevant DPT 14 subtypes, including:

- DPT 14.000 — Acceleration
- DPT 14.001 — Acceleration Angular
- DPT 14.002 — Activation Energy
- DPT 14.003 — Activity
- DPT 14.004 — Amount of Substance
- DPT 14.005 — Amplitude
- DPT 14.006 — Angle
- DPT 14.007 — Angle in Radians
- DPT 14.008 — Angular Velocity
- DPT 14.009 — Area
- DPT 14.010 — Capacitance
- DPT 14.011 — Charge
- DPT 14.012 — Conductance
- DPT 14.013 — Electrical Current
- DPT 14.014 — Electrical Current Density
- DPT 14.015 — Electrical Dipole Moment
- DPT 14.016 — Electrical Field Strength
- DPT 14.017 — Electrical Charge Density
- DPT 14.018 — Electrical Flux
- DPT 14.019 — Electrical Flux Density
- DPT 14.020 — Electrical Polarization
- DPT 14.021 — Electrical Resistance
- DPT 14.022 — Electrical Resistivity
- DPT 14.023 — Electric Voltage
- DPT 14.024 — Energy
- DPT 14.025 — Force
- DPT 14.026 — Frequency
- DPT 14.027 — Heat Capacity
- DPT 14.028 — Heat Flow Rate
- DPT 14.029 — Humidity
- DPT 14.030 — Kelvin Temperature
- DPT 14.031 — Length
- DPT 14.032 — Light
- DPT 14.033 — Luminous Flux
- DPT 14.034 — Luminous Intensity
- DPT 14.035 — Magnetic Field Strength
- DPT 14.036 — Magnetic Flux
- DPT 14.037 — Magnetic Flux Density
- DPT 14.038 — Magnetic Moment
- DPT 14.039 — Magnetic Polarization
- DPT 14.040 — Mass
- DPT 14.041 — Mass Flux
- DPT 14.042 — Power
- DPT 14.043 — Power Factor
- DPT 14.044 — Pressure
- DPT 14.045 — Reactance
- DPT 14.046 — Relative Humidity
- DPT 14.047 — Relative Humidity
- DPT 14.048 — Resistance
- DPT 14.049 — Resistivity
- DPT 14.050 — Self Inductance
- DPT 14.051 — Solid Angle
- DPT 14.052 — Sound Intensity
- DPT 14.053 — Temperature
- DPT 14.054 — Temperature Difference
- DPT 14.055 — Thermal Capacity
- DPT 14.056 — Thermal Conductivity
- DPT 14.057 — Thermoelectric Power
- DPT 14.058 — Time
- DPT 14.059 — Torque
- DPT 14.060 — Volume
- DPT 14.061 — Volume Flow
- DPT 14.062 — Weight
- DPT 14.063 — Work
- DPT 14.064 — Power Density
- DPT 14.065 — Kelvin per Percent
- DPT 14.066 — Volume
- DPT 14.067 — Wind Speed
- DPT 14.068 — Pressure
- DPT 14.069 — Humidity
- DPT 14.070 — Air Flow
- DPT 14.071 — Volume
- DPT 14.072 — Concentration
- DPT 14.073 — Concentration
- DPT 14.074 — Concentration
- DPT 14.075 — Concentration
- DPT 14.076 — Concentration
- DPT 14.077 — Concentration
- DPT 14.078 — Concentration
- DPT 14.079 — Concentration
- DPT 14.080 — Concentration
- DPT 14.081 — Concentration
- DPT 14.082 — Concentration
- DPT 14.083 — Concentration
- DPT 14.084 — Concentration
- DPT 14.085 — Concentration
- DPT 14.086 — Concentration
- DPT 14.087 — Concentration
- DPT 14.088 — Concentration
- DPT 14.089 — Concentration
- DPT 14.090 — Concentration
- DPT 14.091 — Concentration
- DPT 14.092 — Concentration
- DPT 14.093 — Concentration
- DPT 14.094 — Concentration
- DPT 14.095 — Concentration
- DPT 14.096 — Concentration
- DPT 14.097 — Concentration
- DPT 14.098 — Concentration
- DPT 14.099 — Concentration

Do not blindly implement every possible subtype without checking the official DPT definitions and whether the subtype is valid.

### DPT 17 — 3-byte Scene Number

Audit:

- DPT 17.001 — Scene Number

### DPT 19 — 8-byte Date/Time

Audit:

- DPT 19.001 — Date/Time

Verify weekday, date, time, flags, and special values.

### DPT 20 — 1-byte HVAC/Status

Audit relevant DPT 20 subtypes, including:

- DPT 20.001 — Occupancy
- DPT 20.002 — Window/Door
- DPT 20.003 — Heating/Cooling
- DPT 20.004 — Gain
- DPT 20.005 — Wind Speed
- DPT 20.006 — Humidity
- DPT 20.007 — Air Quality
- DPT 20.008 — Temperature
- DPT 20.009 — Wind Direction
- DPT 20.010 — Energy Mode
- DPT 20.011 — Building Mode
- DPT 20.012 — Occupancy Mode
- DPT 20.013 — Room Heating Controller Mode
- DPT 20.014 — Room Cooling Controller Mode
- DPT 20.015 — HVAC Mode
- DPT 20.016 — DPT HVAC Status
- DPT 20.017 — DPT HVAC Status
- DPT 20.100 — HVAC Mode

Verify enum values, reserved values, and correct one-byte encoding.

### DPT 7, 12, 13, 14, 17, 19, 20 and Other Families

Also inspect all other valid KNX DPT families relevant to the implementation, including but not limited to:

- DPT 6 — 8-bit signed
- DPT 8 — 2-byte signed
- DPT 10 — Time of Day
- DPT 11 — Date
- DPT 15 — Access Control
- DPT 16 — String
- DPT 18 — Scene Control
- DPT 21 — Status
- DPT 22 — HVAC Control
- DPT 23 — 2-bit control
- DPT 24 — 8-bit text
- DPT 25 — Alarm Info
- DPT 26 — Scene Info
- DPT 27 — Combined status
- DPT 28 — UTF-8 string
- DPT 29 — 8-byte signed
- DPT 30 — 1-byte bitfield
- DPT 31 — 3-byte bitfield
- DPT 232 — RGB color
- DPT 234 — Scene number
- DPT 251 — RGBW color

For each family, verify the actual official DPT subtypes and the required data length.

Do not claim complete support based only on a partial list.

---

## 4. DPT Support Matrix

Create a support matrix documenting:

| DPT | Name | Data Length | Encoding | Decoding | Range | Status |
|---|---|---:|---|---|---|---|
| DPT 1.xxx | ... | ... | Supported/Missing | Supported/Missing | ... | ... |

The matrix must distinguish:

- Fully supported.
- Partially supported.
- Missing.
- Incorrect implementation.
- Unsupported by design.
- Not applicable to current KNX stack.

For every missing or incorrect DPT:

- Explain the problem.
- Identify affected files.
- Describe the required implementation.
- Implement the fix.

Do not mark a DPT as supported simply because a generic byte container exists. Verify that the actual encoding and decoding behavior is correct.

---

## 5. Implementation Requirements

Implement the missing DPT support using the existing project architecture.

### Encoding and Decoding

For every implemented DPT:

- Use the correct KNX datapoint format.
- Use the correct byte length.
- Implement correct encoding.
- Implement correct decoding.
- Validate ranges.
- Handle signed and unsigned values correctly.
- Handle floating-point values correctly.
- Handle enum and bitfield values correctly.
- Handle special values where required.
- Preserve endianness.
- Avoid data truncation.
- Avoid incorrect scaling.
- Avoid loss of precision beyond the DPT specification.

### Architecture

Follow the existing code style and design.

- Reuse existing DPT abstractions where appropriate.
- Avoid duplicated encoding logic.
- Avoid unnecessary new dependencies.
- Do not break existing KNX/IP communication.
- Do not break existing MCP tools.
- Do not break configuration persistence.
- Keep backward compatibility with existing configuration formats where possible.

If the project uses a registry or factory for DPT types, update it consistently.

If the project uses string names such as `DPT 1.001`, ensure all supported DPTs are correctly registered and resolvable.

---

## 6. KNX Communication Integration

Verify that the supported DPTs work correctly throughout the full data path:

```text
KNX Group Address
        ↓
KNX/IP Telegram
        ↓
APCI / TPDU Processing
        ↓
DPT Decode
        ↓
Internal KNX Value
        ↓
MCP / Application Layer
```

And for outgoing values:

```text
MCP / Application Value
        ↓
DPT Encode
        ↓
APCI / TPDU
        ↓
KNX/IP Telegram
        ↓
KNX Group Address
```

Verify:

- Group read requests.
- Group write requests.
- Group response handling.
- DPT encoding.
- DPT decoding.
- Internal value conversion.
- MCP tool input validation.
- MCP tool output formatting.
- Error handling.
- Unsupported DPT behavior.

If the project supports KNX configuration through JSON, verify that all supported DPTs can be represented correctly.

---

## 7. MCP Integration

Inspect all KNX MCP tools.

Ensure that:

- DPT types can be specified correctly.
- DPT type validation is complete.
- All supported DPTs can be used where applicable.
- MCP values are converted correctly to KNX payloads.
- KNX payloads are decoded correctly into MCP responses.
- Invalid DPT values are rejected with useful errors.
- Unsupported DPT types do not silently produce incorrect data.

Do not change existing MCP tool names or JSON formats unnecessarily.

---

## 8. Testing Requirements

Add or update automated tests for every supported DPT family.

At minimum, test:

### Encoding/Decoding

- Minimum valid value.
- Maximum valid value.
- Zero.
- Positive values.
- Negative values where applicable.
- Boundary values.
- Invalid values.
- Round-trip encode/decode.

### KNX Telegram Integration

- Correct payload length.
- Correct encoded bytes.
- Correct decoded values.
- Group write handling.
- Group response handling.

### MCP Integration

- Valid DPT type.
- Valid value.
- Invalid DPT type.
- Invalid value.
- Correct error handling.
- Correct returned value.

### Regression

Ensure all existing tests still pass.

If a test framework already exists, use it.

If no test framework exists for the DPT implementation, create a lightweight test suite appropriate for the project.

---

## 9. Sikp this step - Build and Validation

Build the project using the existing ESP-IDF build process.

For example:

```bash
idf.py build
```

Run all available tests.

Check for:

- Compilation errors.
- Linker errors.
- Warnings.
- Runtime issues.
- Memory problems.
- Incorrect type conversions.
- Stack or heap problems.
- ESP32 compatibility issues.

Do not consider the task complete if the project does not compile.

If the full firmware build cannot be executed in the current environment, clearly report the limitation and perform all available host-side validation.

---

## 10. Final Completeness Verification

Before finishing, perform a final audit.

Answer these questions with evidence from the code:

1. Which DPT families are currently supported?
2. Which DPT subtypes are supported?
3. Which DPT subtypes were missing?
4. Which missing DPTs were implemented?
5. Were any existing DPT implementations incorrect?
6. Were all incorrect implementations fixed?
7. Are all implemented DPTs registered?
8. Are all implemented DPTs accessible through the KNX communication layer?
9. Are all applicable DPTs accessible through the MCP layer?
10. Are all DPT encoders and decoders tested?
11. Does the firmware compile successfully?
12. Do all tests pass?
13. Are there any remaining unsupported DPTs?
14. If any remain, why are they unsupported?

Do not claim “all KNX DPTs are supported” unless the audit and implementation justify that statement.

---

## 11. Deliverables

Provide the following final deliverables:

### A. Implementation Summary

Summarize all changes made.

### B. Modified Files

List every modified and newly created file with a short explanation.

### C. DPT Support Matrix

Provide the final supported DPT family and subtype matrix.

### D. Missing DPTs

List all missing DPTs that were implemented.

### E. Test Results

Report:

- Build result.
- Unit test result.
- Integration test result.
- Any remaining failures.

### F. Remaining Limitations

Clearly document any DPTs that are not supported and explain why.


## Important Rules

- Do not modify unrelated features.
- Do not remove existing KNX functionality.
- Do not assume generic byte handling equals complete DPT support.
- Do not implement fake or placeholder DPT support.
- Do not silently ignore unsupported DPTs.
- Do not change public APIs unnecessarily.
- Do not claim success without compiling and testing.
- Use official KNX DPT specifications as the reference for encoding and decoding.
- Ensure the final result is maintainable, complete, and production-ready.

**Final Goal:**

Deliver a fully audited, correctly implemented, registered, integrated, and tested KNX DPT implementation for the current branch, covering all applicable KNX datapoint types supported by the project's architecture.