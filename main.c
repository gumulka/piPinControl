#include <MQTTClient.h>
#include <pigpio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define ADDRESS "tcp://192.168.0.2:1883"
#define CLIENTID "ExampleClientPub"
#define PAYLOAD1 "OPEN"
#define PAYLOAD2 "CLOSED"
#define QOS 2
#define TIMEOUT 10000L
#define BASE_TOPIC "/openhab/gpio"

#define BUFFER_SIZE 16
#define PIN_OUTPUT 5
#define STATE_FILE "/home/pi/light_state"
#define LOG_FILE "/home/pi/light_log"

static const int num_pins = 10;
static const int observe_pins[] = {6, 15, 18, 22, 13, 23, 24, 27, 17, 25};

int initPinforObserve(int pin);
void alertFunction(int gpio, int level, uint32_t tick);

static MQTTClient client;
static int spiHandle;
static char buffer[BUFFER_SIZE];
static volatile int keepRunning = 1;

/** the interrupt handler */
void intHandler(int dummy) { keepRunning = 0; }

int initPinforObserve(int pin) {
  /* set pin mode */
  int rc = gpioSetMode(pin, PI_INPUT);
  /* return error code if there was any */
  if (rc != 0) {
    return rc;
  }

  /* read and publish the current state */
  int level = gpioRead(pin);
  alertFunction(pin, level, 0);

  /* activate pin observation */
  gpioSetAlertFunc(pin, alertFunction);

  return 0;
}

void alertFunction(int gpio, int level, uint32_t tick) {
  int rc;
  MQTTClient_deliveryToken token;
  char topic[100];

  /* create the topic from the pin number */
  sprintf(topic, "%s/%d/state", BASE_TOPIC, gpio);

  /* after receiving the interrupt wait for 100 ms to compensate bouncing */
  usleep(150 * 1000);
  int level2 = gpioRead(gpio);
  if (level2 != level) {
    printf("Pin %d changed during bouncing time.\n", gpio);
    fflush(stdout);
    return;
  }
  printf("Pin %3d changed to %2d\n", gpio, level);
  fflush(stdout);

  /* select the message based on pin state
   * translation:
   *  1->"OPEN"
   *  0->"CLOSED"
   */
  MQTTClient_message m = MQTTClient_message_initializer;
  if (level == 0) {
    m.payload = PAYLOAD1;
    m.payloadlen = strlen(PAYLOAD1);
  } else if (level == 1) {
    m.payload = PAYLOAD2;
    m.payloadlen = strlen(PAYLOAD2);
  } else {
    /* ignore all other states
     * Could be 2 if timeout hits */
    return;
  }
  m.qos = QOS;
  m.retained = 0;

  /* publish the message and wait for completion */
  MQTTClient_publishMessage(client, topic, &m, &token);
  /*    printf("Waiting for up to %d seconds for publication of %s\n"
              "on topic %s for client with ClientID: %s\n",
              (int)(TIMEOUT/1000),(char*) m.payload, topic, CLIENTID); // */
  /// this might be not necessary. Should be evaluated!
  rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

  /* report to the user about errors */
  if (rc != 0) {
    printf("Error while sending message! - %d\n", rc);
    /* don't stop the program. Just report.
    keepRunning = 0; */
  }
}

int spiSendCommand(int pin, int state) {

  int i = 0;
  while (pin > 8) {
    pin -= 8;
    i++;
  }
  if (state) {
    buffer[i] |= (1 << pin);
  } else {
    buffer[i] &= ~(1 << pin);
  }
  /// TODO change the 1 to i, but evaluate if it is working
  spiWrite(spiHandle, buffer, 1);
  //    printf("Setting SPI pin %d to %d\n",pin,state);
  gpioWrite(PIN_OUTPUT, 1);
  usleep(500);
  gpioWrite(PIN_OUTPUT, 0);
  usleep(500);

  return 0;
}

int msgarrvd(void *context, char *topicName, int topicLen,
             MQTTClient_message *message) {
  int i;
  char *payloadptr;

  /* Getting rid of the first part of the topic */
  char *ptr = strtok(topicName, "/");
  for (i = 0; i < 2 && ptr != NULL; i++) {
    ptr = strtok(NULL, "/");
  }
  int pin = -1;
  /* parsing the pin number */
  if (ptr != NULL) {
    pin = atoi(ptr);
  }

  /* parsing the command. ON and OFF are the only possiblities. just checking
   * the second char is enough */
  int state = -1;
  payloadptr = message->payload;
  payloadptr++;
  if (*payloadptr == 'N' || *payloadptr == 'n') {
    state = 1;
  } else if (*payloadptr == 'F' || *payloadptr == 'f') {
    state = 0;
  } else {
    payloadptr = message->payload;
    if (*payloadptr >= '0' && *payloadptr <= '1')
      state = *payloadptr - '0';
  }

  printf("Changing pin %d to %d\n", pin, state);
  fflush(stdout);
  if (pin >= 100) {
    spiSendCommand(pin - 100, state);
  } else {
    // TODO Write code for non SPI-Pins.
    // First check if pin is output, then set value
  }

  MQTTClient_freeMessage(&message);
  MQTTClient_free(topicName);
  return 1;
}

void connlost(void *context, char *cause) {
  printf("\nConnection lost\n");
  printf("     cause: %s\n", cause);
  keepRunning = 0;
}

int main(int argc, char *argv[]) {

  /* Read config */
  /*
* while(there is input) {
get one line;
if (it is empty line || it beings with spaces followed by a '#') {
  skip this line, either empty or it's a comment;
}
find the position of the token that splits option name and its value;
copy the option name and its value to separate variables;
removing spaces before and after these variables if necessary;
if (option == option1) {
   parse value for option 1;
} else if (option == option2) {
   parse value for option 2;
} else {
   handle unknown option name;
}
check consistency of options if necessary;
} */

  /* pigpio init */
  if (gpioInitialise() < 0) {
    // pigpio initialisation failed.
    printf("GPIO Initialisation failed");
    exit(EXIT_FAILURE);
  }
  spiHandle = spiOpen(1, 500000, 0);
  if (spiHandle < 0) {
    printf("SPI Initialisation failed");
    // pigpio initialisation failed.
    exit(EXIT_FAILURE);
  }
  int rc = gpioSetMode(PIN_OUTPUT, PI_OUTPUT);
  if (rc != 0) {
    printf("Setting Output pin for SPI did not work\n");
    exit(EXIT_FAILURE);
  }

  FILE *file_ptr;
  file_ptr = fopen(STATE_FILE, "rb");
  if (!file_ptr) {
    printf("Unable to open state file!\n");
    exit(EXIT_FAILURE);
  }

  /* Read in 256 8-bit numbers into the buffer */
  int bytes_read = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, file_ptr);
  fclose(file_ptr);
  if (bytes_read != BUFFER_SIZE) {
    printf("Error while reading in data! %d != %d\n", bytes_read, BUFFER_SIZE);
    //        exit(EXIT_FAILURE);
  }

  printf("Connecting to mqtt\n");
  /* MQTT init */
  MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE,
                    NULL);
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;

  MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, NULL);

  if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to connect to mqtt broker, return code %d\n", rc);
    gpioTerminate();
    exit(EXIT_FAILURE);
  }

  /* Subscribe to topic */
  char topicbuffer[100];
  sprintf(topicbuffer, "%s/+/command", BASE_TOPIC);
  MQTTClient_subscribe(client, topicbuffer, QOS);

  /* set pins for observation */
  for (int i = 0; i < num_pins; i++) {
    initPinforObserve(observe_pins[i]);
  }

  /* init signal handling */
  signal(SIGINT, intHandler);

  /* spin around and do nothing */
  while (keepRunning) {
    sleep(1);
  }

  printf("\rShutting Down\n");
  /* cleanup */
  file_ptr = fopen(STATE_FILE, "wb");
  if (!file_ptr) {
    printf("Unable to open file for write!\n");
  }
  fwrite(buffer, 1, bytes_read, file_ptr);
  fclose(file_ptr);
  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
  spiClose(spiHandle);
  gpioTerminate();
  exit(EXIT_FAILURE);
}
