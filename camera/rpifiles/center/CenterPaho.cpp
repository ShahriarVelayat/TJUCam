#include "MQTTAsync.h"
#include "MQTTClientPersistence.h"

#include <stdio.h>
#include <signal.h>
#include <memory.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <string>
#include <time.h>
#include <map>
#include <cstring>
#include "globals.cpp"

#if defined(WIN32)
#define sleep Sleep
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#include "mqttutil.cpp"
void initialState() {
	CamName.clear();
	TargetName.clear();
	ifGetCamTime.clear();
	ifGetTargetTime.clear();
	ifSendMessage2Cam.clear();
	ifSendMessage2Target.clear();
	ifSendControl2Target.clear();
	CamTime.clear();
	TargetTime.clear();
	for (int i = 0; i < 60; i++) {
		CamValue[i].clear();
		TargetValue[i].clear();
		CamTimeList[i].clear();
		TargetTimeList[i].clear();
	}
	getCamTime = false;
	sendMessage2CamComplete = false;
	sendMessage2TargetComplete = false;
	getTargetTime = false;
	ifFirstTargetTime = true;
	ifHelloComplete = false;
	offlight[0] = 0;
	offlight[1] = 0;
}
int messageArrived(void *context, char *topicName, int topicLen, MQTTAsync_message *message)
{
	string line;
	string word;
	string nodeName;
	string operationName;
	string type;
    //cout << "centerMode" << centerMode << endl;
    //cout << "opts.nodelimiter:" << opts.nodelimiter << endl;
	//if (message->)
	//cout << "payloda:" <<(char*)message->payload << endl;
	//if (opts.showtopics)
	//	printf("%s\t:\n", topicName);
	if (opts.nodelimiter){
        //string topicS(topicName);
		string payloadST((char*)message->payload);
        string payloadS = payloadST.substr(0, message->payloadlen);
		cout << "centerMode: " << centerMode << endl;
		cout << topicName << ": " <<  payloadS << endl;
		string line = payloadST.substr(0, message->payloadlen);
		stringstream messageStream(line);
		switch (centerMode) {
			case WAITFORHELLO: {
				switch (topicByString.at(topicName)) {
					case HELLO: {
						messageStream >> type;
						messageStream >> nodeName;
						if (type == Type.at(TARGET)
	                    && ifGetTargetTime.count(nodeName) == 0
	                    && ifSendMessage2Target.count(nodeName) == 0
	                    && ifSendControl2Target.count(nodeName) == 0) {
	                        TargetName.push_back(nodeName);
	                        ifGetTargetTime[nodeName] = false;
	                        ifSendMessage2Target[nodeName] = false;
	                        ifSendControl2Target[nodeName] = false;
	                        for (int i = 0; i < 60; i++) {
	                            TargetValue[i].insert(make_pair(nodeName, make_pair(false, "")));
								TargetTimeList[i].insert(make_pair(nodeName, 0));
	                        }
	                    }
	                    else if (type == Type.at(CAMERA)
	                        && ifGetCamTime.count(nodeName) == 0
	                        && ifSendMessage2Cam.count(nodeName) == 0) {
	                        CamName.push_back(nodeName);
	                        ifGetCamTime[nodeName] = false;
	                        ifSendMessage2Cam[nodeName] = false;
	                        for (int i = 0; i < 60; i++) {
	                            CamValue[i][nodeName] = make_pair(false, "");
								CamTimeList[i][nodeName] = 0;
	                        }
	                    }
						if ((TargetName.size() == TargetNumber) && (CamName.size() == CamNumber)) {
		                    //centerMode = SYNCHRONOUS;
		                    //cout << "CenterMode change into SYNCHRONOUS" << endl;
		                    ifHelloComplete = true;
		                }
						break;
					}
					default: break;
				}
				break;
			}
			case SYNCHRONOUS: {
				unsigned long long camTime;
                unsigned long long targetTime;
				switch (topicByString.at(topicName)) {
					case SYNC: {
						messageStream >> type;
						//cout << "Type:" << type << endl;
						messageStream >> nodeName;
						//cout << "nodeName:" << nodeName << endl;
						if (type == Type.at(TARGET)) {
                            messageStream >> targetTime;
                            //cout << "ifGetTargetTime.count(nodeName):" << ifGetTargetTime.count(nodeName) << endl;
                            //cout << "ifSendControl2Target[nodeName]:" << ifSendControl2Target[nodeName] << endl;
                            //cout << "ifGetTargetTime[nodeName]" << ifGetTargetTime.at(nodeName) << endl;
                            if (ifSendControl2Target[nodeName] && (ifGetTargetTime.count(nodeName) == 1) && !ifGetTargetTime[nodeName]) {
                                ifGetTargetTime.at(nodeName) = true;
                                TargetTime[nodeName] = targetTime;
                                //cout << "log:" << "Center get targetTime from " + nodeName << endl;
                                if (ifFirstTargetTime) {
    					            firstTargetTimeForSync = targetTime;
    						        ifFirstTargetTime = false;
    					        }
                            }
                        }
                        else if (type == Type.at(CAMERA)) {
                            messageStream >> camTime;
                            //cout << "log:" << "Center get camTime from " + nodeName << endl;
                            ifGetCamTime.at(nodeName) = true;
                            CamTime[nodeName] = camTime;
                        }
						break;
					}
					case ACK: {
						string tt;
                        messageStream >> tt;
                        messageStream >> tt;
                        if (tt == TargetName[0]) {
                            offlight[0]=0;
                        }
                        /*
                        if (tt == TargetName[1]) {
                            offlight[1]=0;
                        }
                        */
                        break;
					}
					case FORDEBUG:
                        break;
                    default:
                        break;
				}
				break;

			}
			case MEASURE: {
				switch(topicByString.at(topicName)) {
					case TAR: {
						unsigned long long timeT;
		                messageStream >> timeT;
		                messageStream >> nodeName;
		                cout << "in TAR" << endl;
		                TargetValue[timeT/(100*1000)%60].at(nodeName) = make_pair(true, payloadS);
						TargetTimeList[timeT/(100*1000)%60].at(nodeName) = timeT;
						cout << timeT/(100*1000)%60 << endl;
						break;
					}
					case CAM: {
						unsigned long long timeT;
						messageStream >> timeT;
						messageStream >> nodeName;
						cout << "in CAM" << endl;
						CamValue[timeT/(100*1000)%60].at(nodeName) = make_pair(true, payloadS);
						CamTimeList[timeT/(100*1000)%60].at(nodeName) = timeT;
						cout << timeT/(100*1000)%60 << endl;
						break;
					}
					default: break;
				}
				break;
			}
			default: break;

		}
	}
		//printf("%.*s", message->payloadlen, (char*)message->payload);
	else
		printf("%.*s%c", message->payloadlen, (char*)message->payload, opts.delimiter);
	fflush(stdout);
	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);
	return 1;
}



void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions ropts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	if (opts.showtopics){
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic1, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic2, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic3, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic4, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic5, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic5, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic6, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic7, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic8, opts.clientid, opts.qos);
        printf("Subscribing to topic %s with client %s at QoS %d\n", topic9, opts.clientid, opts.qos);
    }


	ropts.onSuccess = onSubscribe;
	ropts.onFailure = onSubscribeFailure;
	ropts.context = client;
	if ((rc = MQTTAsync_subscribe(client, topic1, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic1, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic2, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic2, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic3, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic3, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic4, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic4, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic5, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic5, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic6, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic6, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic7, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic7, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic8, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic8, return code %d\n", rc);
		finished = 1;
	}
    if ((rc = MQTTAsync_subscribe(client, topic9, opts.qos, &ropts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe topic9, return code %d\n", rc);
		finished = 1;
	}
}



int main(int argc , char **argv)
{
    ofstream fout;
	fout.open("data.txt");

	MQTTAsync client;
	char* buffer = NULL;
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
	MQTTAsync_responseOptions pub_opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_createOptions create_opts = MQTTAsync_createOptions_initializer;
	int rc = 0;
	char url[100];

	//if (argc < 2)
		//usage();

	topic1 = "control";
    topic2 = "sync";
    topic3 = "hello";
    topic4 = "log";
    topic5 = "ack";
    topic6 = "target";
    topic7 = "cam";
    topic8 = "measurement";
    topic9 = "name";



	if (strchr(topic1, '#') || strchr(topic1, '+'))
		opts.showtopics = 1;
	if (opts.showtopics)
		printf("topic1 is %s\n", topic1);

    if (strchr(topic2, '#') || strchr(topic2, '+'))
    	opts.showtopics = 1;
    if (opts.showtopics)
    	printf("topic2 is %s\n", topic2);

    if (strchr(topic3, '#') || strchr(topic3, '+'))
    	opts.showtopics = 1;
    if (opts.showtopics)
    	printf("topic3 is %s\n", topic3);

    if (strchr(topic4, '#') || strchr(topic4, '+'))
        opts.showtopics = 1;
    if (opts.showtopics)
    	printf("topic4 is %s\n", topic4);

	getopts(argc, argv);
	sprintf(url, "%s:%s", opts.host, opts.port);
	printf("%s\n", url);
	rc = MQTTAsync_create(&client, url, opts.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, client, connectionLost, messageArrived, NULL);

	//signal(SIGINT, cfinish);
	//signal(SIGTERM, cfinish);

	conn_opts.keepAliveInterval = opts.keepalive;
	conn_opts.cleansession = 1;
	conn_opts.username = opts.username;
	conn_opts.password = opts.password;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	while (!subscribed)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif


/*
if (finished)
	goto exit;
	while (!finished)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
*/
buffer = (char*)malloc(4096);
char delimiter[2]="\n";

pub_opts.onSuccess = onPublish;
pub_opts.onFailure = onPublishFailure;
do
{
	rc = MQTTAsync_send(client, topic3, sizeof("areyouok"), (char*)"areyouok", opts.qos, 0, &pub_opts);
}
while (rc != MQTTASYNC_SUCCESS);
while (!finished)

	disc_opts.onSuccess = onDisconnect;
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	while	(!disconnected)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

exit:
	MQTTAsync_destroy(&client);

	return EXIT_SUCCESS;
}