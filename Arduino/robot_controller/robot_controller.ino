#include<assert.h>

/***********************************************************************************************/
/* UTILITIES */
/***********************************************************************************************/
#define CONCAT_TOKENS( TokenA, TokenB )       TokenA ## TokenB
#define EXPAND_THEN_CONCAT( TokenA, TokenB )  CONCAT_TOKENS( TokenA, TokenB )
#define ASSERT( Expression )                  enum{ EXPAND_THEN_CONCAT( ASSERT_line_, __LINE__ ) = 1 / !!( Expression ) }
#define ASSERTM( Expression, Message )        enum{ EXPAND_THEN_CONCAT( Message ## _ASSERT_line_, __LINE__ ) = 1 / !!( Expression ) } 

typedef unsigned long timestamp_t;
#define NOW     micros()
#define VALID_DELAY(_d) ((_d) < ((-1UL) >> 2))

void delay_microseconds(unsigned long /* timestamp_t */ delay_us) {
    if (delay_us < 16000)
        delayMicroseconds(delay_us);
    else
        delay(delay_us / 1000);
}

#define BAUD_RATE   (9600)

/***********************************************************************************************/
/* COMMAND QUEUE */
/***********************************************************************************************/
typedef unsigned char command_type_t;
struct command {
    timestamp_t timestamp;
    command_type_t type;
};
typedef struct command command_t;
typedef unsigned int command_id_t;

/* Use 1KB for the command structures, half mem available. */
#define COMMANDS_QUEUE_SIZE (1024 / sizeof(command_t))
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
typedef void (*command_handler_t)(command_t command);

enum {
    START_COMMAND,
    READ_SERIAL_COMMAND,
    NR_COMMAND_TYPES
};

void start_command_handler(struct command command)
{
    command_t serial_input;

    Serial.println("Start command");
    serial_input.type = READ_SERIAL_COMMAND;
    serial_input.timestamp = NOW;
    push_command(serial_input);
}

void read_serial_command_handler(struct command command)
{
    static int read_serial_exec_cnt = 0;

    read_serial_exec_cnt++;
    if (read_serial_exec_cnt % BAUD_RATE == 0)
        Serial.println(read_serial_exec_cnt);

    while (Serial.available() > 0) {
        Serial.read();
    }

    /* Finally requeue to check serial line at the appropriate time. */
    command.type = READ_SERIAL_COMMAND;
    command.timestamp = NOW + 1000UL * 1000UL / BAUD_RATE;
    push_command(command);
}

void (*command_handlers[NR_COMMAND_TYPES])(struct command command) = {
    /* [START_COMMAND] =       */ start_command_handler,
    /* [READ_SERIAL_COMMAND] = */ read_serial_command_handler
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

    /* If there are no commands left, idle. */
    if (queue_empty()) {
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





