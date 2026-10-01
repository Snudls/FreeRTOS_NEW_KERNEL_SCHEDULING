# Timeline

This project implements a deterministic, timeline-based scheduler designed to extend the default priority-based scheduler in FreeRTOS. By adopting this model, the system enforces task execution based on a strict global timeline divided into Major Frames and Sub-frames.


The scheduler orchestrates two distinct task categories:


- Hard Real-Time (HRT): Strictly time-critical tasks activated at a precise start time relative to the sub-frame. They must complete within a defined "Hard Window"; if an HRT task exceeds its allocated slot, it is forcibly terminated to preserve timeline integrity.



- Soft Real-Time (SRT): Best-effort tasks that utilize the idle time remaining after HRT execution. They follow a fixed, compile-time defined sequence and are simply preempted when an HRT slot begins, offering no guarantee of completion within a specific frame.

## Key Design Principles
To ensure predictability and repeatability, the system enforces the following constraints:


- Run-to-Completion: The task model avoids internal self-rescheduling logic.


- Non-Blocking Communication: Inter-task communication is restricted to polling mechanisms to avoid blocking states that could violate deterministic timing. Critical resources require explicit protection via semaphores or spinlocks.


- Cyclic Reset: The entire system state is reset at the end of every Major Frame, reinitializing all tasks for the next cycle.

## Modified or added function
 All the functions that have been modified/added are:
- configUSE_TIMELINE: it is a flag inside FreeRTOSConfig.h that if it is equal to 1 it will execute the timeline scheduler 
- configDEBUG_TIMELINE: it is a flag which shows debugging print as the cpu utilization.
- vTaskSwitchContext: it will check for every  tick instead of selecting the highest priority task.
-  xTaskIncrementTick: it will always return pdTRUE in order to call the scheduler for every tick.
- xTaskCreate: in this implementation it will require:
     - Name
     - Function
     - Stack depth
     - Parameter
     - Priority (*must* be 0 in the timeline configuration)
     - Start tick
     - End tick
     - Type (TASK_TYPE_HARD/ TASK_TYPE_SOFT)
     - vConfigureScheduler: it will configure the scheduler by using a struct called TimelineTaskConfig_t.
- TimelineTaskConfig_t:it is used to define the timeline task, which has:
     - Name
     - Function
     - Stack depth
     - Parameter
     - Start tick
     - End tick
     - Type (TASK_TYPE_HARD/ TASK_TYPE_SOFT)
     - Semaphore Handler
- xTaskReset: it will reset the stack pointer and the program counter to the initial values
- prvGCD and prvLCM: these functions will calculate Major and Minor cycle for checking schedulability.


## USAGE AND TEST CASE
These primitives represent the minimum requirements for executing a timeline-based scheduler with hard and soft real-time capabilities.

Strict regression testing is required for any code changes: the core tasks must remain functional. Crucially, the underlying system design must also remain capable of executing a priority-based scheduler alongside the timeline implementation.


### C: Priority based scheduling
It is the way to execute the default main , this code must be executed inside the /DemoPriority folder 
```c
// Priority based FreeRTOS implementation
// inside DemoPriority folder, this is main from lab 3.1
make clean
make all
make qemu_start

Test 0 - Priority Based scheduling: PASSED
========================================
PASSED!
Hello, World!
PASSED!
PASSED!
PASSED!
PASSED!
PASSED!
PASSED!
Hello, World!
PASSED!
```


### C: Timeline based scheduling
This is used to execute different test cases where different TEST_NUMBER defines different test and must execute from /DemoTest folder:

In order to execute test:
```c
// run single test case at time, inside Demo folder
make clean
make all EXTRA_CFLAGS="-DTEST_NUMBER=${TEST_NUMBER}"
make qemu_start
```

Where the tests are: 

1) Overlapping HRT Tasks - Stress Test: Two HARD tasks with overlapping windows [0-100] and [50-150]. Expected: B starts when A ended its job or deadline miss.
```c
Test 1 - Overlapping HRT Tasks: PASSED
========================================

Test complete.
[T1] Task_A [0-100] running
[T1] Task_B [50-150] running
 TASK B START AT --> 100[T1] Task_B complete
```

2) Minimal Time Gaps - Edge-case:Adjacent windows [0-100] and [100-200] with zero gap, while a CPU-bound SOFT task runs in the background to validate correct preemption by HARD tasks. Expected: Task_B starts exactly at tick 100.
If A goes to tick number 100 will raise deadline.
```c

### Test 2 - Minimal Time Gaps: PASSED
========================================

Test complete.
[T2] Task_A [0-100] running
 TASK A HARD START AT --> 0
 TASK A HARD END AT --> 100
[T2] Task_A complete
 TASK B HARD START AT --> 100
[T2] Task_B [100-200] running
 TASK B HARD END AT --> 199
[T2] Task_B complete
 TASK C SOFT AT --> 199
 TASK C SOFT AT --> 239
 TASK C SOFT AT --> 280
 TASK C SOFT AT --> 320
 TASK C SOFT AT --> 361
 TASK C SOFT AT --> 401
 TASK C SOFT AT --> 442
 TASK D HARD START AT --> 450
[T4] Task_D [450-500] running
 TASK D HARD END AT --> 500
[T4] Task_D complete
 TASK E HARD START AT --> 500
[T5] Task_E [500-600] running
 TASK E HARD END AT --> 600
[T5] Task_E complete
PASSED
 TASK C SOFT AT --> 633
 TASK C SOFT AT --> 673
```

3) SRT Preemption check: SOFT task [0-200], HARD task [100-150]. Expected: HARD preempts SOFT at tick 100.
```c

Test 3 - SRT Preemption: PASSED


Test complete.
[T3] SOFT task starting --> 0
[T3] HARD task starting --> 100
[T3] HARD task end --> 105
[T3] SOFT task end --> 105
```
4) TimeConsistency: Task [50-150] should start at exactly tick 50. Expected: Variance 0 ticks.
```c
----------------------------------------
Test 4 - Timing Consistency: PASSED
========================================

Test complete.
[T4] task starting --> 50
[T4] Timing task running
[T4] Timing task complete
```
5) Multiple_HRT - Stress test: FIVE sequential HARD tasks in windows [0-100], [100-200], etc. Expected: All FIVE complete in order.
```c
----------------------------------------
Test 5 - Multiple Sequential HRT: PASSED
========================================

Test complete.
[T5] Task_0 running --> 0
[T5] Task_0 complete --> 100
[T5] Task_1 running --> 100
[T5] Task_1 complete --> 200
[T5] Task_2 running --> 200
[T5] Task_2 complete --> 300
[T5] Task_3 running --> 300
[T5] Task_3 complete --> 400
[T5] Task_4 running --> 400
[T5] Task_4 complete --> 500
```

6) Deadline Miss - Violation Test: Task attempts to run for 40 ticks in a 20-tick window [0-20]. Expected: Task should be killed at tick 20.
```c
----------------------------------------
Test 6 - Deadline Miss: PASSED
========================================
Test complete.
[T6] Deadline Task [0-20] START (will try 40 ticks work)
# task is dead
```

7) SemaphoreHandler: Semaphore fails to release the resource at period end. A forced reset is required to free the resource for the next iteration (only if it is necessary).
```c
Timeline Scheduler Test Suite - Test 7
sem added!
[ TASK F ] SEMAPHORE TAKEN
[ TASK G ] OK SEMAPHORE SILL TAKEN
Semaphore released
[ TASK F ] SEMAPHORE TAKEN
[ TASK F ] PASSED
[ TASK G ] OK SEMAPHORE SILL TAKEN
Semaphore released
```
8) Polling: The tasks will be able to communicate through polling 
```c
Timeline Scheduler Test Suite - Test 8
[T8A]: running COUNTER VALUE --> 1
END
[T8A]: ok received correct counter, i'll decrement it[T8B]: running COUNTER VALUE --> 2
END
[LOG]: PASSED
[T8A]: running COUNTER VALUE --> 1
END
[T8A]: ok received correct counter, i'll decrement it[T8B]: running COUNTER VALUE --> 2
END
[LOG]: PASSED
[T8A]: running COUNTER VALUE --> 1
END
```

### Python:
It is also possible to configure everything by using python. To do it, it is necessary to modify system_config.json where is it possible to define the tasks :
- timeline_period: Generally 1000 ticks.
- tick_rate_hz: System tick frequency in Hz (e.g., 1000 implies 1 tick = 1 ms). Functionaly defines the time unit for all other parameters.
- tasks: List of tasks with:
- name: Task name (valid C identifier).
- type: "HARD" or "SOFT".
- start_tick: Start of the execution window.
- end_tick: End of the execution window.
- stack_size: Stack size in words (default 256).

```python
#it checks schedulability analysis and generate the c file
python3 scheduler_generator.py -c system_config.json -o generated_main.c

cp generated_main.c ../Demo/main_generated.c

# Modify Makefile with generated main
```


# BONUS
## CI/CD Pipeline
We created a pipeline running all previous tests to prevent errors and improve tracking. The pipeline runs each test case, saves the output to a log artifact, and checks for the keyword 'PASSED' to verify success.

Since the QEMU board runs in an infinite loop, we use a timeout command to force the process to exit. This was chosen over more complex exit methods for its simplicity and ease of implementation.


```c
// core part of .gitlab-ci.yml
    - make -C Demo clean
    - make -C Demo all EXTRA_CFLAGS="-DTEST_NUMBER=${TEST_NUMBER}"
    - timeout 5s make -C Demo qemu_start > "artifacts/test${TEST_NUMBER}.log" 2>&1 || true 
// this line handle outputs and errors (and put them in .log), after it always return true,
//by that ci/cd does not raise error.
    - cat "artifacts/test${TEST_NUMBER}.log"
    - grep -q "PASSED" "artifacts/test${TEST_NUMBER}.log"
```

To activate the pipeline, the .gitlab-ci.yml file is placed at the project root within the main branch. Consequently, any push from a development branch will inherit this configuration and execute the pipeline commands.

## PORTABILITY
To ensure library portability and consistent execution environments, we implemented a containerized runner. This container functions as a fully isolated Linux operating system. We chose this approach over a standard GitLab Docker extension to maximize security and simplify library management.

This encapsulation prevents malicious commands from affecting the host machine. The solution is also highly efficient, offering streamlined maintainability with low resource and skill requirements.

```make
# DOCKERFILE

FROM debian:bookworm-slim

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates \
      curl \
      bash \
      tini \
      make \
      git \
      gcc-arm-none-eabi \
      libnewlib-arm-none-eabi \
      gcc-arm-linux-gnueabi \
      qemu-user \
      qemu-system-arm \
      gdb-multiarch \
 && curl -L --output /usr/local/bin/gitlab-runner https://gitlab-runner-downloads.s3.amazonaws.com/latest/binaries/gitlab-runner-linux-amd64 \
 && chmod +x /usr/local/bin/gitlab-runner \
 && useradd --comment 'GitLab Runner' --create-home gitlab-runner --shell /bin/bash \
 && gitlab-runner install --user=gitlab-runner --working-directory=/home/gitlab-runner \
 && rm -rf /var/lib/apt/lists/*

ENTRYPOINT ["/usr/bin/tini", "--"]
CMD ["bash"]
```

This docker file contain all the minimum requirements to run our project and freertos project. 
```make
sudo docker build -t docker_runner .
sudo docker run -it docker_runner /bin/bash
```

```make
# INSIDE DOCKER_RUNNER
gitlab-runner start
gilab-runner register

# now, setup with server URL, Token ( & other info), !!! select shell as executor
```
```make
# to check the status run inside docker
gitlab-runner --debug run
```

Any type of modification at kernel level must pass all this tasks (except for the 7th that is a user choice).


## Authors

- Lorenzo Giandomenico Mat. 360387
- Alessandro Minutoli Mat. 357349
- Mattia Giosa Mat. 355121
- Federico Capitani Mat. 362023
- Ludovico Rigotti Mat. 361395


## License

[GNU GENERAL PUBLIC LICENSE](https://choosealicense.com/licenses/gpl-3.0/)