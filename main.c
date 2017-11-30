#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "MQTTClient.h"

#define ADDRESS     "tcp://192.168.0.4:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "/openhab/Flur_Switch_v/state"
#define PAYLOAD1     "OPEN"
#define PAYLOAD2     "CLOSED"
#define QOS         2
#define TIMEOUT     10000L

MQTTClient_message open, close;
MQTTClient client;

int initPinforObserve(int pin);
void alertFunction(int gpio, int level, uint32_t tick);

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

int initPinforObserve(int pin) {
    /* set pin mode */
    int rc = gpioSetMode(pin,PI_INPUT);
    /* return error code if there was any */
    if(rc!=0) {
        return rc;
    }

    /* read and publish the current state */
    int level = gpioRead(pin);
    alertFunction(pin,level,0);

    /* activate pin observation */
    gpioSetAlertFunc(pin,alertFunction);
}

void alertFunction(int gpio, int level, uint32_t tick) {
    int rc;
    MQTTClient_deliveryToken token;
    char topic[100];

    /* create the topic from the pin number */
    sprintf(topic,"/openhab/iopins/%d/state",gpio);

    /* select the message based on pin state
     * translation:
     *  1->"OPEN"
     *  0->"CLOSED"
     */
    MQTTClient_message m;
    if(level==0) {
        m = open;
    } else if(level==1){
        m = close;
    } else {
        /* ignore all other states
         * Could be 2 if timeout hits */
        return ;
    }

    /* publish the message and wait for completion */
    MQTTClient_publishMessage(client, topic, &m, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), m.payload, topic, CLIENTID);
    /// this might be not necessary. Should be evaluated!
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

    /* report to the user about errors */
    if(rc!=0) {
        printf("Error while sending message!\n");
        /* don't stop the program. Just report.
        keepRunning = 0; */
    }
}

int main(int argc, char* argv[]) {

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
        exit(EXIT_FAILURE);
    }


    /* MQTT init */
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        gpioTerminate();
        exit(EXIT_FAILURE);
    }

    /* Initialize the two possible messages */
    open = MQTTClient_message_initializer;
    open.payload = PAYLOAD1;
    open.payloadlen = strlen(PAYLOAD1);
    open.qos = QOS;
    open.retained = 0;

    close = MQTTClient_message_initializer;
    close.payload = PAYLOAD2;
    close.payloadlen = strlen(PAYLOAD2);
    close.qos = QOS;
    close.retained = 0;


    /* set pins for observation */
    initPinforObserve(23);

    /* init signal handling */
    signal(SIGINT, intHandler);

    /* spin around and do nothing */
    while(keepRunning) {
        sleep(1);
    }

    /* cleanup */
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    gpioTerminate();
    return EXIT_SUCCESS;
}
