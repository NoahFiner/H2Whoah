// Include libraries
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Buffer sizes
#define MAX_RECV_SIZE 128
#define MAX_RESPONSE_SIZE 1500
#define MAX_ITER 20
#define MAX_MEASUREMENT_SIZE 50
#define MAX_DATA_SIZE 200

// Client to communicate to influx
TCPClient client;

// Connection info
const char *host = "ec2-18-219-54-175.us-east-2.compute.amazonaws.com";
const char *db = "H2WOAH";
const char *user = "h2woah";
const char *password = "asdfasdf";
const char *lab_group = "0";
const int port = 8086;

// Variables to store changing data
float soil_moisture = 0;
float new_height = 0;
float current_height = 0;
int status = 0;

// Pin assignments
int val = 0;
int soilPin = A3;
int soilPower = D7;

int echoPin = D3;
int trigPin = D4;

int motorPin = D6;

float distance = 0;

float maxSoilVal = 3365.0;
float minSoilVal = 100.0;

float average_reading(int n) {
  int counter = 0;
  float avg = 0;
  digitalWrite(soilPower, HIGH);
  delay(10);
  for(counter=0; counter < n; counter++) {
    avg += analogRead(soilPin);
    delay(20);
  }
  avg /= n;
  digitalWrite(soilPower, LOW);
  delay(10);
  return avg;
}

float getDistance()
{
  float echoTime;                   //variable to store the time it takes for a ping to bounce off an object
  float calcualtedDistance;         //variable to store the distance calculated from the echo time

  //send out an ultrasonic pulse that's 10ms long
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10); 
  digitalWrite(trigPin, LOW);

  echoTime = pulseIn(echoPin, HIGH);      //use the pulsein command to see how long it takes for the
                                          //pulse to bounce back to the sensor

  calcualtedDistance = echoTime / 148.0;  //calculate the distance of the object that reflected the pulse (half the bounce time multiplied by the speed of sound)

  return calcualtedDistance;              //send back the distance that was calculated
}

float average_ultrasonic(int n) {
  int counter = 0;
  float avg = 0;
  for(counter=0; counter < n; counter++) {
    avg += getDistance();
    delay(100);
  }
  avg /= n;
  delay(10);
  return avg;
}

// Buffers
char measurement_name[MAX_MEASUREMENT_SIZE] = {'\0'};
char data_buffer[MAX_DATA_SIZE] = {'\0'};
char response_buffer[MAX_RESPONSE_SIZE] = {'\0'};
uint8_t recv[MAX_RECV_SIZE] = {0};

void setup()
{
    Serial.begin(9600);
    pinMode(2, OUTPUT);
    pinMode(3, OUTPUT);
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    pinMode(7, OUTPUT);
    
    
    pinMode(soilPower, OUTPUT);
    digitalWrite(soilPower, LOW);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    pinMode(motorPin, OUTPUT);
}

void loop() {
    Serial.println("Woke up!");
    
    Serial.println("Getting most previous new_height from the server...");
    // Read GET request from server
    readData(response_buffer, measurement_name, 0);
    // Parse value from GET request and save result as a variable
    
    Serial.println(response_buffer);
    new_height = (float)parseReading(response_buffer);
    Serial.print("Done! new_height is ");
    Serial.println(new_height);
    
    //measure things
    float soil = average_reading(10);
    float adjusted_soil = (float)((soil-minSoilVal)/maxSoilVal);
    Serial.print("Soil moisture = ");
    Serial.println(adjusted_soil);
    
  
    current_height = getDistance();

    Serial.print("Current height = ");
    Serial.println(current_height);
    
    // current_height = new_height; //delete this later
    
    if(new_height < 1) {
        Serial.println("ERROR: NEW_HEIGHT NOT READ CORRECTLY FROM DB!");
        Serial.println("Setting new_height to 20 to be safe");
        new_height = 20;
    }
    if(current_height < new_height || current_height > 50) {
        digitalWrite(motorPin, HIGH);
        Serial.println("Opening valve");
    } else {
        delay(1000);
    }
    while(current_height < new_height || current_height > 50) {
        current_height = getDistance();
        Serial.print(current_height);
        Serial.print(" is smaller than ");
        Serial.println(new_height);
        delay(500);
    }
    // soil_moisture = soil_moisture_reading(soilPower, soilPin, 20);

    // srand(time(NULL));
    // soil_moisture = (float)(rand() % 100)/200.0; //delete this line later
    
    // send height, soil, and 1/0 for verbose/not verbose
    Serial.println("Writing measured values to the database");
    sendMeasurements(roundf(1000 * current_height) / 1000, adjusted_soil, 0);
    Serial.println("Done!");
    // Serial.print("current_height: ");
    // Serial.println(current_height);
    // Serial.print("soil moisture: ");
    // Serial.println(adjusted_soil);
    
    Serial.println("Closing valve\n\n\n");

    digitalWrite(motorPin, LOW);
    delay(2000);
    
    
    
    // Go to sleep
    // System.sleep(SLEEP_MODE_DEEP, 90);
    
    // delay(90000);
}

void light_led(int pin_number, int status)
{
    if (status)
        {
            digitalWrite(pin_number, HIGH);
        }
    else
        {
            digitalWrite(pin_number, LOW);
        }
}

float soil_moisture_reading(int soilPower, int soilPin, int n)
{
  int counter = 0;
  float avg = 0;
  digitalWrite(soilPower, HIGH);
  delay(10);
  for(counter=0;counter<n;counter++)
  {
      avg += analogRead(soilPin);
      delay(20);
  }
  avg /= n;
  digitalWrite(soilPower, LOW);
  delay(10);
  return avg;
}

/////////////////////
// Here be dragons...
/////////////////////

int readData(char *response_buffer, char *measurement_name, int verbose){
    //function that reads a measurement from influxdb
    // Send data over to influx
    memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
    if (client.connect(host, port)){
        // Write GET request to client
        client.printlnf("GET /query?q=SELECT%%20%%2A%%20FROM%%20h2whoah%%20WHERE%%20%%22value%%22%%20%%3D%%20%%27a%%27%%20OR%%20%%22value%%22%%20%%3D%%20%%27m%%27%%20ORDER%%20BY%%20DESC%%20LIMIT%%201&db=%s&u=%s&p=%s HTTP/1.1", db, user, password);
        // client.printlnf("GET /query?q=SELECT%%20last%%28value%%29%%20FROM%%20%s%%20WHERE%%20lab_group%%3D%%27%s%%27&db=%s&u=%s&p=%s HTTP/1.1", measurement_name, lab_group, db, user, password);
        client.printlnf("Host: %s:%d", host, port);
        client.println("Connection: close");
        client.println();
        // Read response
        readResponse(response_buffer);
        // If verbose, echo all output to serial
        if (verbose)
        {
            Serial.println("############ BEGIN HTTP GET REQUEST ############");
            Serial.printlnf("GET /query?q=SELECT%%20%%2A%%20FROM%%20h2whoah%%20ORDER%%20BY%%20DESC%%20LIMIT%%201&db=%s&u=%s&p=%s HTTP/1.1", db, user, password);
            Serial.printlnf("Host: %s:%d", host, port);
            Serial.println("Connection: close");
            Serial.println();
            Serial.printlnf("############ END HTTP GET REQUEST ############");
            Serial.println();
            // Print response
            Serial.println("############ BEGIN HTTP GET RESPONSE ############");
            Serial.printlnf("%s", response_buffer);
            Serial.printlnf("############ END HTTP GET RESPONSE ############");
            Serial.println();            
        }
        client.stop();
        memset(measurement_name, '\0', MAX_MEASUREMENT_SIZE);
        return 1;
    }
    else
    {
        Serial.println("Connection Failed!");
    }
    memset(measurement_name, '\0', MAX_MEASUREMENT_SIZE);
    return 0;
}

int readResponse(char *response_buffer)
{
    int recv_len = 0;
    int current_response_len = 0;
    char *cursor = response_buffer;
    char *search_ptr = response_buffer;
    char *header_end = 0;
    const char *section_break = "\r\n\r\n";
    // Keep reading until packet is exhausted
    delay(100);
    for (int i=0; i < MAX_ITER; i++)
    {
        // Read from TCP buffer
        recv_len = client.read(recv, MAX_RECV_SIZE);
        // If nothing was read, set recv_len to 0
        if (recv_len == -1){recv_len = 0;}
        // Prevent buffer overflow
        if (MAX_RESPONSE_SIZE - current_response_len - recv_len <= 0){break;}
        // Copy chunk into response buffer
        memcpy(response_buffer + current_response_len, recv, recv_len);
        // Update response length
        current_response_len += recv_len;
        // Reset receive buffer
        memset(recv, 0, sizeof(recv));
        // Look for section break
        cursor = strstr(search_ptr, section_break);
        // If section break found...
        if (cursor)
        {
            // Update the search pointer
            search_ptr = cursor + strlen(section_break);
            // If it's the first section break, demarcate end of header
            if (!header_end)
            {
                header_end = search_ptr;
            }
            // If its the second section break, stop reading
            else
            {
                break;
            }
        }
        delay(100);
    }
    return 1;
}

int writeInflux(char *data_buffer, char *response_buffer, int verbose){
    //function that writes a measurement given as a string to influx and returns 1 if successful and 0 if failed
    // Send data over to influx
    if(client.connect(host, port)){
        // Request will look like this:
        // ----------------------------
        // POST /write?db=ENGR100&u=engr100_student&p=goblue HTTP/1.1
        // Host: ec2-18-219-54-175.us-east-2.compute.amazonaws.com:8086
        // User-Agent: Photon/1.0
        // Content-Length: 32
        // Content-Type: application/x-www-form-urlencoded
        //
        // soil_moisture,group=0 value=0.86
        // ----------------------------
        client.printlnf("POST /write?db=%s&u=%s&p=%s&precision=s HTTP/1.1", db, user, password);
        client.printlnf("Host: %s:%d", host, port);
        client.println("User-Agent: Photon/1.0");
        client.printlnf("Content-Length: %d", strlen(data_buffer));
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.println();
        client.print(data_buffer);
        client.println();
        if (verbose)
        {
            Serial.println("############ BEGIN HTTP POST REQUEST ############");
            Serial.printlnf("POST /write?db=%s&u=%s&p=%s&precision=s HTTP/1.1", db, user, password);
            Serial.printlnf("Host: %s:%d", host, port);
            Serial.println("User-Agent: Photon/1.0");
            Serial.printlnf("Content-Length: %d", strlen(data_buffer));
            Serial.println("Content-Type: application/x-www-form-urlencoded");
            Serial.println();
            Serial.print(data_buffer);
            Serial.println();
            Serial.printlnf("############ END HTTP POST REQUEST ############");
            Serial.println();
            readResponse(response_buffer);
            Serial.println("############ BEGIN HTTP POST RESPONSE ############");
            Serial.printlnf("%s", response_buffer);
            Serial.printlnf("############ END HTTP POST RESPONSE ############");
            Serial.println();
        }
        client.flush();
        client.stop();
        memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
        return 1;
    }
    else
    {
        Serial.println("Connection Failed!");
    }
    memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
    return 0;
}

int getCurrentTime() {
    return Time.now();
}

void sendMeasurements(float height, float soil, int verbose){
    // function to create the data string to send to influx 
    // Create your data string for the parameter to pass into write influx
    memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
    memset(data_buffer, '\0', MAX_DATA_SIZE);
    //for some reason, %c wants an int. so i'm giving it 97, which is decimal for a
    sprintf(data_buffer, "h2whoah height=%f,new_height=%f,soil=%f,value=\"%c%\" %d", height, height, soil, 112, getCurrentTime());
    //write to influx and will try for 5 times before just taking a new reading
    for(int j=0; j<5; j++){
        if (writeInflux(data_buffer, response_buffer, verbose)){
            break;
        }
        else{
            Serial.println("Trying again");}
        }
    // Clear all buffers
    memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
    memset(data_buffer, '\0', MAX_DATA_SIZE);
}



void sendData(float value, char *measurement_name, char *lab_group, char *data_buffer, char *response_buffer, int verbose){
    // function to create the data string to send to influx 
    // Create your data string for the parameter to pass into write influx
    memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
    memset(data_buffer, '\0', MAX_DATA_SIZE);
    if (strlen(lab_group) > 0){
        sprintf(data_buffer, "%s,lab_group=%s value=%f", measurement_name, lab_group, value);
    }
    else
    {
        
        sprintf(data_buffer, "%s value=%f", measurement_name, value);
    }
    //write to influx and will try for 5 times before just taking a new reading
    for(int j=0; j<5; j++){
        if (writeInflux(data_buffer, response_buffer, verbose)){
            break;
        }
        else{
            Serial.println("Trying again");}
        }
    // Clear all buffers
    memset(response_buffer, '\0', MAX_RESPONSE_SIZE);
    memset(data_buffer, '\0', MAX_DATA_SIZE);
}


//results will be like this:
// {"results":[{"statement_id":0,"series":[{"name":"h2whoah","columns":["time","height","new_height","soil","type"],"values":[["2018-11-21T01:51:52.474563836Z",1,1,1,null]]}]}]}
// we're trying to get the 3rd value in the values array (yes, i know)
float parseReading(char *buffer){
    char *a = buffer;
    char *b = buffer;
    char value_str[10] = {'\0'};
    float value = 0;
    // Search HTTP response for values
    a = strstr(a, "values");
    // If found...
    if (a)
    {
        // Set pointer at first character after "values"
        a += strlen("values");
        // Search remaining string for "," to get to value after timestamp
        a = strstr(a, ",");
        // If found...
        if (a)
        {
            // Set pointer at first character after ","
            a += strlen(",");
            Serial.println(a);
            // Search remaining string for "," again to get new_height
            a = strstr(a, ",");
            // If found...
            if (a)
            {
                // Set pointer at first character after ","
                a += strlen(",");
                // Search for next ","
                b = strstr(a, ",");
                // If found...
                if (b)
                {
                    // Copy all characters between "," and "," into value_str
                    strncpy(value_str, a, b - a);
                    // Convert value_str to a float
                    value = strtof(value_str, NULL);
                    // Return parsed value
                    return value;
                }
            }
        }
    }
    return 0;
}
 

// float parseReading(char *buffer, char *measurement_name){
//     char *a = buffer;
//     char *b = buffer;
//     char value_str[10] = {'\0'};
//     float value = 0;
    
//     // Search HTTP response for search term
//     a = strstr(a, search_term);
//     // If found...
//     if (a)
//     {
//         // Set pointer at first character after search term
//         a += strlen(search_term);
//         // Search remaining string for "values"
//         a = strstr(a, "values");
//         // If found...
//         if (a)
//         {
//             // Set pointer at first character after "values"
//             a += strlen("values");
//             // Search remaining string for ","
//             a = strstr(a, ",");
//             // If found...
//             if (a)
//             {
//                 // Set pointer at first character after ","
//                 a += strlen(",");
//                 // Search for closing bracket
//                 b = strstr(a, "]");
//                 // If found...
//                 if (b)
//                 {
//                     // Copy all characters between "," and "]" into value_str
//                     strncpy(value_str, a, b - a);
//                     // Convert value_str to a float
//                     value = strtof(value_str, NULL);
//                     // Return parsed value
//                     return value;
//                 }
//             }
//         }
//     }
//     return 0;
// }
 
