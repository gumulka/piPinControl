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

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg1 = MQTTClient_message_initializer;
    MQTTClient_message pubmsg2 = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc,i;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    pubmsg1.payload = PAYLOAD1;
    pubmsg1.payloadlen = strlen(PAYLOAD1);
    pubmsg1.qos = QOS;
    pubmsg1.retained = 0;

    pubmsg2.payload = PAYLOAD2;
    pubmsg2.payloadlen = strlen(PAYLOAD2);
    pubmsg2.qos = QOS;
    pubmsg2.retained = 0;
    for(i=0;i<4;i++) {
        MQTTClient_publishMessage(client, TOPIC, &pubmsg1, &token);
        printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD1, TOPIC, CLIENTID);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        if(rc!=0)
            return rc;
        sleep(1);
        MQTTClient_publishMessage(client, TOPIC, &pubmsg2, &token);
        printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD2, TOPIC, CLIENTID);
        rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        if(rc!=0)
            return rc;
        sleep(5);
    }
    printf("Message with delivery token %d delivered\n", token);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
