#include <cstdlib>
#include <mbed.h>
#include <MQTTmbed.h>
#include <MQTTClientMbedOs.h>
#include <Thread.h>  
#include <UnbufferedSerial.h>
#include <cstdio>
#include <string.h>
#include <stdlib.h>

#define MQTT_BROKER        "192.168.30.88"
#define MQTT_USERNAME      "smartglass"
#define MQTT_PASSWORD      "aiushnik6"
#define MQTT_TOPIC_POWER   "Power"
#define MQTT_TOPIC_MODE    "Mode"
#define MQTT_TOPIC_SENSOR  "LightSensor/LightIntensity/x"
#define MQTT_TOPIC_PUBL    "Publish"
#define MQTT_ID            "2e7b1cf6cf724b81b6b2cdf1a0765479"
#define MQTT_PORT          1883
#define SIG_NEED_PUBLISH   0x01

bool Auto_mode = false;
bool Toning = false;
int Value;
DigitalOut OutToning(PB_10);

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    MQTTString  &topic = md.topicName; 

    printf("Payload %.*s in topic %.*s\r\n", message.payloadlen, (char*)message.payload, topic.lenstring.len, topic.lenstring.data);

    if (strncmp(topic.lenstring.data, MQTT_TOPIC_SENSOR, topic.lenstring.len) == 0) 
    {
        Value = atoi((char*)message.payload);
    }

    if (strncmp(topic.lenstring.data, MQTT_TOPIC_POWER, topic.lenstring.len) == 0) 
    {
        if (strncmp((char*)message.payload, "On", message.payloadlen) == 0) 
        {
            Toning = true;
        }
        if (strncmp((char*)message.payload, "Off", message.payloadlen) == 0) 
        {
            Toning = false;
        }
    }

    if (strncmp(topic.lenstring.data, MQTT_TOPIC_MODE, topic.lenstring.len) == 0)
    {
        if (strncmp((char*)message.payload, "Auto", message.payloadlen) == 0)
        {
            Auto_mode = true;
        } 
        else 
        {
            Auto_mode = false;
        }
    }

    if (Auto_mode == false)
    {
        OutToning = Toning;
    }
    else 
    {
        OutToning = (Value < 1000)?false:true;
    }
}

int main(int argc, char *argv[]) 

{
    printf("Starting work SmartGlass:\n");

    TCPSocket socket;
    NetworkInterface *net = NetworkInterface::get_default_instance();
    if (!net) 
        {
            printf("Error! Network not found.\n");
            return 0;
        }
    printf("Connecting to network...\r\n");

/* connect will perform the action to connect to the network */
    nsapi_size_or_error_t rc = net->connect();
    if (rc != 0) 
        {
            printf("Error! _net->connect() returned: %d\r\n", rc);
            return -1;
        }

/* opening the socket only allocates resources */
    rc = socket.open(net);
    if (rc != 0) 
        {
            printf("Error! _socket.open() returned: %d\r\n", rc);
            return -1;
        }

/* now we have to find where to connect */
    SocketAddress address;
    address.set_ip_address("192.168.30.88");
    address.set_port(1883);

/* we are connected to the network but since we're using a connection oriented
* protocol we still need to open a connection on the socket */
    printf("Opening connection to remote %s port %d\r\n",
        address.get_ip_address(), address.get_port());
    rc = socket.connect(address);
    if (rc != 0) 
        {
            printf("Error! _socket.connect() returned: %d\r\n", rc);
            return -1;
        }
    printf("Connected socket\n");

/* MQTT connection */
    MQTTClient client(&socket);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.username.cstring = (MQTT_USERNAME);
    data.clientID.cstring = (MQTT_ID);
    data.password.cstring = (MQTT_PASSWORD);
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);

    if ((rc = client.subscribe(MQTT_TOPIC_POWER, MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);
    if ((rc = client.subscribe(MQTT_TOPIC_MODE, MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);
    if ((rc = client.subscribe(MQTT_TOPIC_SENSOR, MQTT::QOS2, messageArrived)) != 0)
        printf("rc from MQTT subscribe is %d\r\n", rc);

    while (1)
    {
        client.yield(100);
    }

    MQTT::Message message;
    char *topic = (MQTT_TOPIC_PUBL);
    char buf[100];
    int lux = 0;
    sprintf(buf,
        "{\"d\":{\"Illumination\":%d}}", lux);
    printf("Sending message: \n%s\n", buf);
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void *)buf;
    message.payloadlen = strlen(buf);
    printf("Message sent\n");
    printf("Demo concluded successfully \r\n");

    return 0;
}