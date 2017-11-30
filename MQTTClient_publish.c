/*******************************************************************************
 * Copyright (c) 2012, 2017 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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



int initPinforObserve(int pin) {
	int rc = gpioSetMode(pin,PI_INPUT);
	if(rc!=0) {
		return rc;
	}

	int level = gpioRead(pin);
	alertFunction(pin,level,0);

	gpioSetAlertFunc(pin,alertFunction);
}

void alertFunction(int gpio, int level, uint32_t tick) {
	int rc;
    MQTTClient_deliveryToken token;
    char topic[100];

    sprintf(topic,"/openhab/iopins/%d/state",gpio);

	MQTTClient_message m;
	if(level==0) {
		m = open;
	} else if(level==1){
		m = close;
	} else {
		return ;
	}

    MQTTClient_publishMessage(client, topic, &m, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), m.payload, topic, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    if(rc!=0) {
		printf("Error while sending message!\n");
	}
}

int main(int argc, char* argv[]) {

    /* Variables */
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc,i;

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
        return EXIT_FAILURE;
    }


	/* MQTT init */
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        gpioTerminate();
        exit(EXIT_FAILURE);
    }

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


	/* spin around and do nothing */
	while(1) {
        sleep(5);
	}

    for(i=0;i<4;i++) {
        MQTTClient_publishMessage(client, TOPIC, &open, &token);
        printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD1, TOPIC, CLIENTID);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        if(rc!=0)
            return rc;
        sleep(1);
        MQTTClient_publishMessage(client, TOPIC, &close, &token);
        printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD2, TOPIC, CLIENTID);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        if(rc!=0)
            return rc;
        sleep(5);
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    gpioTerminate();
    return EXIT_SUCCESS;
}
