# FreeRTOS Timeline Configuration Framework

This directory contains tools to configure and generate a FreeRTOS timeline scheduler application.

## Files

- `scheduler_generator.py`: Python script to validate the schedule and generate C code.
- `system_config.json`: JSON configuration file defining the system parameters and tasks.
- `generated_main.c`: Output file containing the FreeRTOS `main()` and task stubs.

## Usage

1. **Edit Configuration**: Modify `system_config.json` to define your tasks.
   - `timeline_period`: Generally 1000 ticks.
   - `tick_rate_hz`: System tick frequency in Hz (e.g., 1000 implies 1 tick = 1 ms). Functionaly defines the time unit for all other parameters.
   - `tasks`: List of tasks with:
     - `name`: Task name (valid C identifier).
     - `type`: "HARD" or "SOFT".
     - `start_tick`: Start of the execution window.
     - `end_tick`: End of the execution window.
     - `stack_size`: Stack size in words (default 256).

2. **Generate Code**: Run the generator script.
   ```bash
   python3 scheduler_generator.py -c system_config.json -o generated_main.c
   ```
   The script performs Schedulability Analysis:
   - Validates that tasks fit within the Major Frame Period.
   - Checks for overlaps between HARD tasks.

3. **Integrate**: Copy the generated file to your project's source directory (e.g., replacing `Demo/test_main.c` or adding as a new main file).
   ```bash
   cp generated_main.c ../Demo/main_generated.c
   ```
   (You may need to adjust your Makefile to compile the new file).

## Schedulability Analysis
The tool enforces the following rules for a Timeline Scheduler on a single core:
- **HARD Tasks**: Must be strictly sequential. No time window overlaps are allowed.
- **SOFT Tasks**: May overlap with HARD tasks (preemptible) but must fit within the major frame.
- **Boundaries**: All tasks must complete by `timeline_period`.
