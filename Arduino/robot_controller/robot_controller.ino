#include<assert.h>

#define CONCAT_TOKENS( TokenA, TokenB )       TokenA ## TokenB
#define EXPAND_THEN_CONCAT( TokenA, TokenB )  CONCAT_TOKENS( TokenA, TokenB )
#define ASSERT( Expression )                  enum{ EXPAND_THEN_CONCAT( ASSERT_line_, __LINE__ ) = 1 / !!( Expression ) }
#define ASSERTM( Expression, Message )        enum{ EXPAND_THEN_CONCAT( Message ## _ASSERT_line_, __LINE__ ) = 1 / !!( Expression ) } 

struct command {
  unsigned long timestamp;
  unsigned char operation;
};
typedef struct command command_t;
typedef unsigned int command_id_t;

/* Use 1KB for the command structures, half mem available. */
#define NR_COMMANDS (1024 / sizeof(command_t))
command_t commands[NR_COMMANDS];
command_id_t commands_used = 0;

/* Make sure command type is wide enough to store all commands.
   On top of that we need space for calculations (which often
   involve multiplying by 2). */
ASSERT(1UL << (8 * sizeof(command_id_t)) > 2UL * NR_COMMANDS);

/***********************************************************************************************/
/* UTILITIES */
/***********************************************************************************************/

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
  assert(commands_used + 1 < NR_COMMANDS);
  memcpy(&commands[commands_used], &command, sizeof(command_t));
  commands_used++;
  heapify_up();
}

void drain_queue() {
  commands_used = 0;
}

/***********************************************************************************************/
/* MAIN LOOP */
/***********************************************************************************************/
// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
int led = 13;

// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  command_t command;
  unsigned char i, j;

  Serial.println(commands_used);
  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);               // wait for a second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);               // wait for a second

  drain_queue();

  for (i=0; i<50; i++) {
    command.operation = i;
    command.timestamp = random(1000);
    Serial.print("Adding operation, idx: ");
    Serial.print(command.operation);
    Serial.print(", timestamp: ");
    Serial.println(command.timestamp);
    push_command(command);
  }

  for (i=0; i<commands_used/2; i++) {
    command = pop_command();
    Serial.print("Popped operation, idx: ");
    Serial.print(command.operation);
    Serial.print(", timestamp: ");
    Serial.println(command.timestamp);
  }

  Serial.println("Done all");
  delay(10000);
}











