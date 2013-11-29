#include <AFMotor.h>

/***********************************************************************************************/
/* CONFIG */
/***********************************************************************************************/

#define BAUD_RATE   (9600UL)
int battery_voltage_pin = A2;
int power_up_pin = A3;
int ampmeter_pin = A4;
int buttons_pin = A5;

/***********************************************************************************************/
/* UTILITIES */
/***********************************************************************************************/
#define CONCAT_TOKENS( TokenA, TokenB )       TokenA ## TokenB
#define EXPAND_THEN_CONCAT( TokenA, TokenB )  CONCAT_TOKENS( TokenA, TokenB )
#define ASSERT( Expression )                  enum{ EXPAND_THEN_CONCAT( ASSERT_line_, __LINE__ ) = 1 / !!( Expression ) }
#define ASSERTM( Expression, Message )        enum{ EXPAND_THEN_CONCAT( Message ## _ASSERT_line_, __LINE__ ) = 1 / !!( Expression ) }
#define assert(x)                             if (!(x)) { Serial.print("Failure in: "); Serial.println(__LINE__); }

typedef unsigned long timestamp_t;
#define NOW     micros()
#define VALID_DELAY(_d) ((_d) > 0 && (_d) < ((-1UL) >> 2))

#define ANALOG_TO_MILLIVOLTS(_a)     ((_a) * 5000UL / 1024UL)
#define ABS(_v)  ((_v) < 0 ? -(_v) : (_v))

void delay_microseconds(unsigned long /* timestamp_t */ delay_us)
{
    if (delay_us < 16000UL)
        delayMicroseconds(delay_us);
    else
        delay(delay_us / 1000UL);
}

struct stat_accumulator {
    long min;
    long max;
    long nr_samples;
    long sum_samples;
    long sum_square_samples;
};

void stat_accumulator_init(struct stat_accumulator *accumulator)
{
    memset(accumulator, 0, sizeof(struct stat_accumulator));
    accumulator->min = 0x7FFFFFFFUL;
    accumulator->max = 0x80000000UL;
}

void stat_accumulator_sample(struct stat_accumulator *accumulator, long sample)
{
    accumulator->min = min(accumulator->min, sample);
    accumulator->max = max(accumulator->max, sample);
    accumulator->nr_samples++;
    accumulator->sum_samples += sample;
    accumulator->sum_square_samples += sample * sample;
}

void stat_accumulator_print(struct stat_accumulator *accumulator)
{
    long average, average_squares;
    double standard_deviation;

    if (accumulator->nr_samples == 0) {
        Serial.println("#0");
        return;
    }
    average = accumulator->sum_samples / accumulator->nr_samples;
    average_squares = accumulator->sum_square_samples / accumulator->nr_samples;
    standard_deviation = sqrt(average_squares - average * average);

    Serial.print("[");
    Serial.print(accumulator->min);
    Serial.print(",");
    Serial.print(average);
    Serial.print(",");
    Serial.print(accumulator->max);
    Serial.print("] #");
    Serial.print(accumulator->nr_samples);
    Serial.print(", sd: ");
    Serial.print(standard_deviation);
    Serial.println("");
}

/***********************************************************************************************/
/* COMMAND QUEUE */
/***********************************************************************************************/

AF_Stepper motor1(200, 1);
AF_Stepper motor2(200, 2);

typedef unsigned char command_type_t;
typedef unsigned long command_data_t;
struct command {
    timestamp_t timestamp;
    command_type_t type;
    command_data_t data;
};
typedef struct command command_t;
typedef unsigned int command_id_t;

/* Use 1KB for the command structures, half mem available. */
#define COMMANDS_QUEUE_SIZE (512 / sizeof(command_t))
command_t commands[COMMANDS_QUEUE_SIZE];
command_id_t commands_used = 0;

/* Make sure command type is wide enough to store all commands.
   On top of that we need space for calculations (which often
   involve multiplying by 2). */
ASSERT(1UL << (8 * sizeof(command_id_t)) > 2UL * COMMANDS_QUEUE_SIZE);

#define swap_commands(_i, _j) do {                                 \
    command_t _tmp;                                                  \
    memcpy(&_tmp, &commands[(_i)], sizeof(command_t));               \
    memcpy(&commands[(_i)], &commands[(_j)], sizeof(command_t));     \
    memcpy(&commands[(_j)], &_tmp, sizeof(command_t));               \
} while(0)

/* Call if the last element in the heap needs placing in the right position. */
void heapify_up() {
    command_id_t child_id, parent_id;
    //Serial.print("Heapifying with # commands: ");
    //Serial.println(commands_used);
    /* Corner case, when there are no commands. */
    if (commands_used == 0)
        return;
    child_id = commands_used - 1;
    //Serial.print("Child id: ");
    //Serial.println(child_id);
    while (child_id != 0) {
        parent_id = (child_id - 1)  / 2;
        //Serial.print("Parent id: ");
        //Serial.print(parent_id);
        //Serial.print(" with timestamps: parent: ");
        //Serial.print(commands[parent_id].timestamp);
        //Serial.print(", child: ");
        //Serial.println(commands[child_id].timestamp);
        /* If parent's timestamp is smaller/equal, the perculation must stop. */
        if (commands[parent_id].timestamp <= commands[child_id].timestamp)
            return;
        //Serial.println("Swapping.");
        /* Othewise, swap and continue. */
        swap_commands(parent_id, child_id);
        child_id = parent_id;
    }
}


/* Call if the first element in the heap needs placing in the right position. */
void heapify_down() {
    command_id_t parent_id, smallest_id, child_id;

    /* Corner case, when there are no commands. */
    if (commands_used == 0)
        return;

    /* Start off with the root. */
    smallest_id = 0;
    do {
        /* Parent id is whatever item was used to swap at the end of last iteration. */
        parent_id = smallest_id;

        child_id = 2 * parent_id + 1;
        if (child_id < commands_used &&
                commands[child_id].timestamp < commands[smallest_id].timestamp)
            smallest_id = child_id;

        child_id = 2 * parent_id + 2;
        if (child_id < commands_used &&
                commands[child_id].timestamp < commands[smallest_id].timestamp)
            smallest_id = child_id;

        swap_commands(parent_id, smallest_id);
    } while (smallest_id != parent_id);
}

struct command pop_command() {
    command_t out_command;
    assert(commands_used > 0);
    memcpy(&out_command, &commands[0], sizeof(command_t));
    swap_commands(0, commands_used-1);
    commands_used--;
    heapify_down();

    return out_command;
}

void push_command(struct command command) {
    assert(commands_used + 1 < COMMANDS_QUEUE_SIZE);
    memcpy(&commands[commands_used], &command, sizeof(command_t));
    commands_used++;
    heapify_up();
}

void reset_queue() {
    commands_used = 0;
}

int queue_empty() {
    return (commands_used == 0);
}
/***********************************************************************************************/
/* COMMANDS */
/***********************************************************************************************/
static stat_accumulator serial_stats;

typedef void (*command_handler_t)(command_t command);

enum {
    START_COMMAND,
    POWER_UP_COMMAND,
    READ_SERIAL_COMMAND,
    MOTOR_TEST_COMMAND,
    SAMPLE_PINS_COMMAND,
    MOTOR_MOVEMENT_COMMAND,
    NR_COMMAND_TYPES
};

void power_up_command_handler(struct command command)
{
    pinMode(power_up_pin, OUTPUT);
    digitalWrite(power_up_pin, HIGH);
}

struct {
    unsigned long step_delay_us;
    int forward;
    unsigned int steps_left;
} motor_test_active;

void motor_test_init(unsigned long step_delay_us, int forward)
{
    command_t command;

    motor_test_active.step_delay_us = step_delay_us;
    motor_test_active.forward = forward;
    motor_test_active.steps_left = 600;

    command.timestamp = NOW;
    command.type = MOTOR_TEST_COMMAND;
    push_command(command);
}

void motor_test_step(struct command command)
{
    int dirMotor1, dirMotor2;

    if (motor_test_active.steps_left == 0)
    {
        if (motor_test_active.forward)
            motor_test_init(motor_test_active.step_delay_us, 0);
        return;
    }

    dirMotor1 = motor_test_active.forward ? FORWARD : BACKWARD;
    dirMotor2 = motor_test_active.forward ? FORWARD : BACKWARD;

    motor1.onestep(dirMotor1, INTERLEAVE);
    motor2.onestep(dirMotor2, INTERLEAVE);

    motor_test_active.steps_left--;
    command.timestamp += motor_test_active.step_delay_us;
    assert(command.type == MOTOR_TEST_COMMAND);
    push_command(command);
}

struct motor_movement {
    unsigned long command_sequence_number;
    long left_step_delay;
    long right_step_delay;
    unsigned long next_step_left;
    unsigned long next_step_right;
    unsigned int left_steps_left;
    unsigned int right_steps_left;
} motor_movement;

static int command_count = 0;
void motor_movement_command_init(long left_steps, long right_steps, unsigned long duration)
{
    command_t command;
    timestamp_t now;
    long left_step_delay, right_step_delay;

    //command_count++;
    //if (command_count % 10 == 0) {
    //    Serial.print("mm:");
    //    Serial.print(left_steps);
    //    Serial.print(",");
    //    Serial.print(right_steps);
    //    Serial.print(",");
    //    Serial.print(duration);
    //    Serial.print(",a");
    //    Serial.print(motor_movement.active);
    //    Serial.println("");
    //}

    left_step_delay = 10UL * 1000UL * 1000UL;
    right_step_delay = 10UL * 1000UL * 1000UL;
    if (left_steps != 0) {
        left_step_delay = (long)duration / left_steps;
    }
    if (right_steps != 0) {
        right_step_delay = (long)duration / right_steps;
    }

    if (ABS(left_step_delay) < 1000 || ABS(right_step_delay) < 1000) {
        Serial.println("Too quick");
        return;
    }

    if (left_steps == 0 && right_steps == 0) {
        Serial.println("No movement requested");
        return;
    }

    motor_movement.left_steps_left = ABS(left_steps);
    motor_movement.right_steps_left = ABS(right_steps);
    motor_movement.left_step_delay = left_step_delay;
    motor_movement.right_step_delay = right_step_delay;

    /* Push a new command into the queue, making sure to match sequence numbers */
    motor_movement.command_sequence_number = motor_movement.command_sequence_number + 1;
    now = NOW;
    /* Assign when is the next step going to happen, respect current delay if it's earlier. */
    if (now + ABS(left_step_delay) < motor_movement.next_step_left) {
        motor_movement.next_step_left = now + ABS(left_step_delay);
    }
    if (now + ABS(right_step_delay) < motor_movement.next_step_right) {
        motor_movement.next_step_right = now + ABS(right_step_delay);
    }
    command.timestamp = min(motor_movement.next_step_left, motor_movement.next_step_right);
    command.type = MOTOR_MOVEMENT_COMMAND;
    command.data = motor_movement.command_sequence_number;
    push_command(command);
}

struct motor_movement_debug {
    unsigned long left_steps_done;
    unsigned long right_steps_done;
    timestamp_t last_print;
} motor_movement_debug;

void motor_movement_command_handler(struct command command)
{
    int motor_choice; /* true for left, false for right */
    int direction;

    /* Check whether the sequence number is correct, if not, this command is obsolete, return. */
    if (command.data != motor_movement.command_sequence_number) {
        return;
    }

    /* Otherwise execute movement. */
    assert(motor_movement.left_steps_left != 0 || motor_movement.right_steps_left != 0);

    /* Work out which motor is earlier. */
    if (motor_movement.left_steps_left == 0) {
        motor_choice = 0;
    } else {
        if (motor_movement.right_steps_left == 0) {
            motor_choice = 1;
        } else {
            motor_choice = motor_movement.next_step_left <= motor_movement.next_step_right;
        }
    }

    motor_choice = motor_movement.next_step_left <= motor_movement.next_step_right;

    /* Perform the step. */
    direction = motor_choice ?
                    (motor_movement.left_step_delay > 0 ? FORWARD : BACKWARD) :
                    (motor_movement.right_step_delay > 0 ? FORWARD : BACKWARD);
    if (motor_choice) {
        motor1.onestep(direction, INTERLEAVE);
    } else {
        motor2.onestep(direction, INTERLEAVE);
    }

    /* Update the movement structure */
    if (motor_choice) {
        motor_movement_debug.left_steps_done++;
        motor_movement.left_steps_left--;
        motor_movement.next_step_left += ABS(motor_movement.left_step_delay);
        if (motor_movement.left_steps_left == 0) {
            motor_movement.next_step_left += 10UL * 1000UL * 1000UL;
        }
    } else {
        motor_movement_debug.right_steps_done++;
        motor_movement.right_steps_left--;
        motor_movement.next_step_right += ABS(motor_movement.right_step_delay);
        if (motor_movement.right_steps_left == 0) {
            motor_movement.next_step_right += 10UL * 1000UL * 1000UL;
        }
    }

    /* Check whether we are done with everything. */
    if (motor_movement.left_steps_left == 0 &&
        motor_movement.right_steps_left == 0) {
        //Serial.println("Deactivating movement");
        return;
    }

    /* Push fresh command */
    command.type = MOTOR_MOVEMENT_COMMAND;
    command.timestamp = min(motor_movement.next_step_left,
                            motor_movement.next_step_right);
    command.data = motor_movement.command_sequence_number;
    push_command(command);
}


#define BUFFER_LENGTH   32
struct serial_data {
    char buffer[BUFFER_LENGTH];
    int buffer_offset;
};
static struct serial_data read_serial_data = {{0}, 0};

long parse_long(char *buffer, char *end, char **new_buffer)
{
    int idx;
    long out;

    //Serial.print(buffer);
    idx = 0;
    for (idx=0; buffer + idx < end; idx++) {
        if (idx == 0 && buffer[idx] == '-') {
            continue;
        }
        if (buffer[idx] < '0' || buffer[idx] > '9') {
            buffer[idx] = '\0';
            break;
        }
    }
    *new_buffer = (buffer + idx + 1);
    if (idx == 0) {
        return 0;
    }
    out = atol(buffer);

    //Serial.print("Parsing \"");
    //Serial.print(buffer);
    //Serial.print("\", got: ");
    //Serial.print(out);
    //Serial.println("");

    return out;
}

void process_serial_command(void)
{
    char *b = read_serial_data.buffer;

    switch (*b) {
        case 't':
        {
            unsigned long delay_us;
            int rpm;
            char c;

            c = *(++b);
            if (c > 'z') c = 'z';
            if (c < 'a') c = 'a';
            rpm = 16*16 * (c - 'a' + 1) / ('z' - 'a');
            delay_us = (60000UL / 200UL) * 1000UL / rpm / 2;
            motor_test_init(delay_us, 1);
            break;
        }

        case 'm':
        {
            long steps_l, steps_r, duration;
            char *end;

            b++;
            end = read_serial_data.buffer + read_serial_data.buffer_offset;
            steps_l = parse_long(b, end, &b);
            steps_r = parse_long(b, end, &b);
            duration = parse_long(b, end, &b);
            //Serial.println("=========");
            //Serial.println(steps_l);
            //Serial.println(steps_r);
            //Serial.println(duration);
            //Serial.println("=========");
            motor_movement_command_init(steps_l, steps_r, 1000UL * duration);
            break;
        }
        default:
            Serial.print("Unknown command: \"");
            Serial.print(b);
            Serial.println("\"");
            break;
    }
    read_serial_data.buffer_offset = 0;
}

void read_serial_command_handler(struct command command)
{
    timestamp_t now = NOW;
    if (now - motor_movement_debug.last_print > 1000UL * 1000UL) {
        Serial.print(motor_movement_debug.left_steps_done);
        Serial.print(" ");
        Serial.print(motor_movement_debug.right_steps_done);
        Serial.println("");
        motor_movement_debug.last_print = now;
    }
    while (Serial.available() > 0) {
        char b;
        int idx;

        idx = read_serial_data.buffer_offset;
        b = Serial.read();
        assert(idx < BUFFER_LENGTH);
        read_serial_data.buffer[idx] = b;
        read_serial_data.buffer_offset++;
        if (b == '\n' || b == '$') {
            read_serial_data.buffer[idx] = '\0';
            process_serial_command();
        }
    }

    /* Finally, requeue to check serial line at the appropriate time. */
    command.type = READ_SERIAL_COMMAND;
    command.timestamp = NOW + 1000UL * 1000UL / BAUD_RATE;
    push_command(command);
}

static int buttons_standard_voltage = -1;
static timestamp_t buttons_last_notification = 0;
static timestamp_t buttons_last_print = 0;
void sample_pins_command_handler(struct command command)
{
    int battery, amp, buttons;

    battery = analogRead(battery_voltage_pin);
    amp = analogRead(ampmeter_pin);
    buttons = analogRead(buttons_pin);

    /* If not initialised, initialise. */
    if (buttons_standard_voltage < 0) {
        buttons_standard_voltage = buttons;
    } else
    /* If voltage at least 5% higher than standard, button pressed. */
    if (100UL * buttons > 105UL * buttons_standard_voltage) {
        if (NOW - buttons_last_notification > 500UL * 1000UL) {
            /* Close to max reading of 1024, i.e. secondary button. */
            if (buttons > 1000) {
                Serial.println("Secondary button");
            } else {
                //Serial.println("Primary button");
                digitalWrite(power_up_pin, LOW);
            }
            buttons_last_notification = NOW;
        }
    /* Fall through: calculate standard voltage.
       Note that at the start of day standard voltage will approx
       the primary button voltage. But this will fix itself with
       enough samples after the button is released. This is of no
       major impact to button press detection since it relies on
       positive edge.
     */
    } else {
        buttons_standard_voltage = (9 * buttons_standard_voltage + 1 * buttons) / 10;
    }

    if (NOW - buttons_last_print > 1000UL * 1000UL) {
        battery = ANALOG_TO_MILLIVOLTS(battery);
        amp = ANALOG_TO_MILLIVOLTS(amp);
        buttons = ANALOG_TO_MILLIVOLTS(buttons);
#if 0
        Serial.print(battery);
        Serial.print(",");
        Serial.print(amp);
        Serial.print(",");
        Serial.print(buttons);
        Serial.print(" ");
        Serial.print(buttons_standard_voltage);
        Serial.println("");
#endif
        buttons_last_print = NOW;
    }

    /* Finally requeue another sample in 10ms. */
    command.type = SAMPLE_PINS_COMMAND;
    command.timestamp = NOW + 10UL * 1000UL;
    push_command(command);
}

void start_command_handler(struct command command)
{
    command_t power_up;
    command_t serial_input;
    command_t sample_pins;

    Serial.println("Start command");
    stat_accumulator_init(&serial_stats);
    memset(&motor_movement, 0, sizeof(struct motor_movement));
    memset(&motor_movement_debug, 0, sizeof(struct motor_movement_debug));

    power_up.type = POWER_UP_COMMAND;
    power_up.timestamp = NOW;
    push_command(power_up);

    serial_input.type = READ_SERIAL_COMMAND;
    serial_input.timestamp = NOW;
    push_command(serial_input);

    sample_pins.type = SAMPLE_PINS_COMMAND;
    sample_pins.timestamp = NOW;
    push_command(sample_pins);
}

void (*command_handlers[NR_COMMAND_TYPES])(struct command command) = {
    /* [START_COMMAND] =          */ start_command_handler,
    /* [POWER_UP_COMMAND] =       */ power_up_command_handler,
    /* [READ_SERIAL_COMMAND] =    */ read_serial_command_handler,
    /* [MOTOR_TEST_COMMAND] =     */ motor_test_step,
    /* [SAMPLE_PINS_COMMAND] =    */ sample_pins_command_handler,
    /* [MOTOR_MOVEMENT_COMMAND] = */ motor_movement_command_handler,
};



/***********************************************************************************************/
/* MAIN LOOP */
/***********************************************************************************************/
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

void setup() {
    command_t start_command;

    pinMode(led, OUTPUT);
    Serial.begin(BAUD_RATE);
    Serial.print("setup");
    /* Reset the queue and initiate operation by queueing the start command. */
    reset_queue();
    start_command.type = START_COMMAND;
    start_command.timestamp = NOW;
    push_command(start_command);
}

// the loop routine runs over and over again forever:
void loop() {
    command_t current_command;
    command_handler_t handler;
    timestamp_t wait_time;

    timestamp_t start_t;
    timestamp_t current_t;
   /* If there are no commands left, idle. */
    if (queue_empty()) {
        Serial.println("WARNING: Empty queue");
        delay(1000);
        return;
    }

    /* Normal case: there is at least one command, pop it off the queue and exec. */
    current_command = pop_command();
    handler = command_handlers[current_command.type];
    wait_time = current_command.timestamp - NOW;

    if (VALID_DELAY(wait_time))
        delay_microseconds(wait_time);
    handler(current_command);
}



/***********************************************************************************************/
/* OLD CODE, NO LONGER IN ACTIVE USE */
/***********************************************************************************************/
#if 0

//// Testing heap
void loop() {
    command_t command;
    unsigned char i, j;

    Serial.println(commands_used);
    digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(1000);               // wait for a second
    digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);               // wait for a second

    reset_queue();

    for (i=0; i<50; i++) {
        command.type = i;
        command.timestamp = random(1000);
        Serial.print("Adding type, idx: ");
        Serial.print(command.type);
        Serial.print(", timestamp: ");
        Serial.println(command.timestamp);
        push_command(command);
    }

    for (i=0; i<commands_used/2; i++) {
        command = pop_command();
        Serial.print("Popped type, idx: ");
        Serial.print(command.type);
        Serial.print(", timestamp: ");
        Serial.println(command.timestamp);
    }

    Serial.println("Done all");
    delay(10000);
}


#endif





