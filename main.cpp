/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "simpleclient.h"
#include <string>
#include <sstream>
#include <vector>
#include "mbed-trace/mbed_trace.h"
#include "mbedtls/entropy_poll.h"

#include <events/mbed_events.h>
#include <mbed.h>
#include "ble/BLE.h"
#include "ble/DiscoveredCharacteristic.h"
#include "ble/DiscoveredService.h"

#include "security.h"

//#include "mbed.h"
#include "rtos.h"
#include "MiCS6814_GasSensor.h"

#if MBED_CONF_APP_NETWORK_INTERFACE == WIFI
#include "ESP8266Interface.h"
ESP8266Interface esp(MBED_CONF_APP_WIFI_TX, MBED_CONF_APP_WIFI_RX);
#elif MBED_CONF_APP_NETWORK_INTERFACE == ETHERNET
#include "EthernetInterface.h"
EthernetInterface eth;
#elif MBED_CONF_APP_NETWORK_INTERFACE == MESH_LOWPAN_ND
#define MESH
#include "NanostackInterface.h"
LoWPANNDInterface mesh;
#elif MBED_CONF_APP_NETWORK_INTERFACE == MESH_THREAD
#define MESH
#include "NanostackInterface.h"
ThreadInterface mesh;
#endif

#if defined(MESH)
#if MBED_CONF_APP_MESH_RADIO_TYPE == ATMEL
#include "NanostackRfPhyAtmel.h"
NanostackRfPhyAtmel rf_phy(ATMEL_SPI_MOSI, ATMEL_SPI_MISO, ATMEL_SPI_SCLK, ATMEL_SPI_CS,
                           ATMEL_SPI_RST, ATMEL_SPI_SLP, ATMEL_SPI_IRQ, ATMEL_I2C_SDA, ATMEL_I2C_SCL);
#elif MBED_CONF_APP_MESH_RADIO_TYPE == MCR20
#include "NanostackRfPhyMcr20a.h"
NanostackRfPhyMcr20a rf_phy(MCR20A_SPI_MOSI, MCR20A_SPI_MISO, MCR20A_SPI_SCLK, MCR20A_SPI_CS, MCR20A_SPI_RST, MCR20A_SPI_IRQ);
#endif //MBED_CONF_APP_RADIO_TYPE
#endif //MESH

#ifndef MESH
// This is address to mbed Device Connector
#define MBED_SERVER_ADDRESS "coap://api.connector.mbed.com:5684"
#else
// This is address to mbed Device Connector
#define MBED_SERVER_ADDRESS "coaps://[2607:f0d0:2601:52::20]:5684"
#endif

Serial output(USBTX, USBRX);

// Status indication
DigitalOut red_led(LED1);
DigitalOut green_led(LED2);
DigitalOut blue_led(LED3);
Ticker status_ticker;
// Ren, begin
enum charType {
	PRESSURE,
	TEMPERATURE,
	HUMIDITY
};
const char * dbg_CharType[HUMIDITY+1]={"Pressure","Temperature","Humidity"};
const char * objName[HUMIDITY+1]={"3323","3303","3304"};
static bool is_active[HUMIDITY+1];
static float dataprint[HUMIDITY+1]={0,0,0};
/*
 * The BME280 sensor contains 3 float properties.
 * Those are updated once BLE Gatt client read the values.
 */
class BME280Resource {
public:
    BME280Resource() {
    	// create Pressure object '3323'.
        for(int m=0; m<HUMIDITY+1; m++) {
        	M2MObjectInstance* tmp_inst;
            bme280[m] = M2MInterfaceFactory::create_object(objName[m]);
            tmp_inst = bme280[m] ->create_object_instance();
            tmp_res[m] = tmp_inst->create_dynamic_resource("5700", dbg_CharType[m],
                    M2MResourceInstance::STRING, true /* observable */);
            tmp_res[m]->set_operation(M2MBase::GET_ALLOWED);
            tmp_res[m]->set_value((uint8_t*)"0.0", 3);
        }
    }

    void set_bme280_value(float val, int h){
        char tmp_buf[50];
        int len;
        if (h<PRESSURE || h>HUMIDITY) {
            printf("data type h (input) is wrong!!");
            return;
        }
        if (h==HUMIDITY)
            len=sprintf(tmp_buf,"%0.2f%%",val);
        else if (h==PRESSURE)
            len=sprintf(tmp_buf,"%0.1f hPa",val);
        else
            len=sprintf(tmp_buf,"%0.2f degC",val);

        tmp_res[h]->set_value((uint8_t*)tmp_buf, len);
        //printf("set_value(tmp_bug=%s\r\n", tmp_buf);
    }


    M2MObject* get_object(int idx) {
        if (idx<PRESSURE || idx>HUMIDITY)    return NULL;
        return bme280[idx];
    }
private:
    M2MObject*  bme280[HUMIDITY+1];
//    M2MObjectInstance* tmp_inst[HUMIDITY+1];
    M2MResource* tmp_res[HUMIDITY+1];
};

static BME280Resource *demo1;
// end of BME280 resource definition

const char * dbg_gasType[C2H5OH+1]={"CO","NO2","NH3","C3H8","C4H10","CH4", "H2", "C2H5OH"};
const char * gas_objID[C2H5OH+1]={"27000","27001","27002","27003","27004","27005", "27006", "27007" };

/*
 * The  MiCS-6814 Multichannel Gas Sensor (seeed) contains 8 float properties.
 * Those are connected directly.
 */
class MiCS6814_Resource {
public:
	MiCS6814_Resource() {
    	// create CO object '27000'.
        for(int m=0; m<C2H5OH+1; m++) {
        	M2MObjectInstance* gas_inst;
        	MiCS6814_Gas[m] = M2MInterfaceFactory::create_object(gas_objID[m]);
            gas_inst = MiCS6814_Gas[m] ->create_object_instance();
            gas_res[m] = gas_inst->create_dynamic_resource("5700", dbg_gasType[m],
                    M2MResourceInstance::STRING, true /* observable */);
            gas_res[m]->set_operation(M2MBase::GET_ALLOWED);
            gas_res[m]->set_value((uint8_t*)"0.00", 3);
        }
    }

    void set_MiCS6814_value(float val, int h){
        char tmp_buf[50];
        int len;
        if (h<CO || h>C2H5OH) {
            printf("gas type h (input) is wrong!!");
            return;
        }
        len=sprintf(tmp_buf,"%0.2f ppm",val);

        gas_res[h]->set_value((uint8_t*)tmp_buf, len);
        printf("set_value(%s) = %s\r\n", dbg_gasType[h], tmp_buf);
    }


    M2MObject* get_object(int idx) {
        if (idx<CO || idx>C2H5OH)    return NULL;
        return MiCS6814_Gas[idx];
    }
private:
    M2MObject*  MiCS6814_Gas[C2H5OH+1];
//    M2MObjectInstance* gas_inst[C2H5OH+1];
    M2MResource* gas_res[C2H5OH+1];
};

static MiCS6814_Resource _gasSensor_res;

static Ticker set_gasValue_timer;

#if defined(TARGET_LPC1768)
MiCS6814_GasSensor Multi_gas_sensor(p28, p27);
#else
MiCS6814_GasSensor Multi_gas_sensor(I2C_SDA, I2C_SCL);
#endif

void read_gas_val(){
	for(int m=0; m<C2H5OH+1; m++) {
		_gasSensor_res.set_MiCS6814_value(Multi_gas_sensor.getGas((GAS_TYPE)m),m);
	}
}

// Ren, end of MiCS6814_GasResource definition


/************************************************************BLE Stuff from here *********************************/
BLE &ble = BLE::Instance();
static DiscoveredCharacteristic bme280Characteristic[HUMIDITY+1];
static bool triggerLedCharacteristic;
static const char PEER_NAME[] = "BME280";


static EventQueue eventQueue(
    /* event count */ 16 * /* event size */ 32
);

void advertisementCallback(const Gap::AdvertisementCallbackParams_t *params) {
    // parse the advertising payload, looking for data type COMPLETE_LOCAL_NAME
    // The advertising payload is a collection of key/value records where
    // byte 0: length of the record excluding this byte
    // byte 1: The key, it is the type of the data
    // byte [2..N] The value. N is equal to byte0 - 1

  	//printf("Starting advertisementCallback...\r\n");
    for (uint8_t i = 0; i < params->advertisingDataLen; ++i) {

        const uint8_t record_length = params->advertisingData[i];
        if (record_length == 0) {
            continue;
        }
        const uint8_t type = params->advertisingData[i + 1];
        const uint8_t* value = params->advertisingData + i + 2;
        const uint8_t value_length = record_length - 1;

        if (type == GapAdvertisingData::COMPLETE_LOCAL_NAME) {
            if ((value_length == sizeof(PEER_NAME)) && (memcmp(value, PEER_NAME, value_length) == 0)) {
                printf(
                    "adv peerAddr[%02x %02x %02x %02x %02x %02x] rssi %d, isScanResponse %u, AdvertisementType %u\r\n",
                    params->peerAddr[5], params->peerAddr[4], params->peerAddr[3], params->peerAddr[2],
                    params->peerAddr[1], params->peerAddr[0], params->rssi, params->isScanResponse, params->type
                );
                BLE::Instance().gap().connect(params->peerAddr, Gap::ADDR_TYPE_RANDOM_STATIC, NULL, NULL);
                break;
            }
        }
        i += record_length;
    }
}

void serviceDiscoveryCallback(const DiscoveredService *service) {
    if (service->getUUID().shortOrLong() == UUID::UUID_TYPE_SHORT) {
        printf("S type short UUID-%x attrs[%u %u]\r\n", service->getUUID().getShortUUID(), service->getStartHandle(), service->getEndHandle());
    } else {
        printf("S type long UUID-");
        const uint8_t *longUUIDBytes = service->getUUID().getBaseUUID();
        for (unsigned i = 0; i < UUID::LENGTH_OF_LONG_UUID; i++) {
            printf("%02x", longUUIDBytes[i]);
        }
        printf(" attrs[%u %u]\r\n", service->getStartHandle(), service->getEndHandle());
    }
}

/***
    This function called read() initially, following read() calls
    are repeated inside triggerRead function
***/
void updateLedCharacteristic(void) {
    if (!BLE::Instance().gattClient().isServiceDiscoveryActive()) {
        //printf("02  updateLedCharacteristic\n");
        for(int g=0; g<HUMIDITY+1; g++) {
            if (is_active[g])    bme280Characteristic[g].read();
        }
    }
}


void characteristicDiscoveryCallback(const DiscoveredCharacteristic *characteristicP) {
    int tmp_uuid;
    tmp_uuid=characteristicP->getUUID().getShortUUID();
    printf("  C UUID-%x valueAttr[%u] props[%x]\r\n", characteristicP->getUUID().getShortUUID(), characteristicP->getValueHandle(), (uint8_t)characteristicP->getProperties().broadcast());
    if ((tmp_uuid < GattCharacteristic::UUID_PRESSURE_CHAR) ||
        (tmp_uuid > GattCharacteristic::UUID_HUMIDITY_CHAR))     return;
    triggerLedCharacteristic = true;
    if (tmp_uuid == GattCharacteristic::UUID_PRESSURE_CHAR) {
        bme280Characteristic[PRESSURE] = *characteristicP;    is_active[PRESSURE] = true;
        printf(" is_active[PRESSURE] = true\r\n");
    }
    else if (tmp_uuid == GattCharacteristic::UUID_TEMPERATURE_CHAR) {
    	  bme280Characteristic[TEMPERATURE] = *characteristicP;    is_active[TEMPERATURE] = true;
    	  printf(" is_active[TEMPERATURE] = true\r\n");
   } else {
        bme280Characteristic[HUMIDITY] = *characteristicP;    is_active[HUMIDITY] = true;
        printf(" is_active[HUMIDITY] = true\r\n");
   }
}

void discoveryTerminationCallback(Gap::Handle_t connectionHandle) {
    printf("terminated SD for handle %u\r\n", connectionHandle);
    if (triggerLedCharacteristic) {
        triggerLedCharacteristic = false;
        eventQueue.call(updateLedCharacteristic);
    }
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *params) {
    int ret;
    printf("Connected to BME280 now...\r\n");
    if (params->role == Gap::CENTRAL) {
        BLE &ble = BLE::Instance();
        ble.gattClient().onServiceDiscoveryTermination(discoveryTerminationCallback);
        // Ren, connect to ENVIRONMENT service
        ret=ble.gattClient().launchServiceDiscovery(params->handle, serviceDiscoveryCallback, characteristicDiscoveryCallback, GattService::UUID_ENVIRONMENTAL_SERVICE);
        printf("ble.gattClient().launchServiceDiscovery = %d\r\n", ret);
    }
}

//ASHOK's triggerRead function

void triggerRead(const GattReadCallbackParams *response) {
    int k=0;
    uint8_t dataforClient[4] = {0,0,0,0};
    uint32_t final_dataforClient = 0;
    for(int j=0; j<HUMIDITY+1; j++) {
        if (is_active[j]) {
            if (response->handle == bme280Characteristic[j].getValueHandle()){
                if ( response-> len > 4) {
                    printf("response-> len is wrong :%d, %s, skipping read", response-> len, dbg_CharType[j]);
                    break;
                }
                for(int i=0; i< response-> len; i++) {
                    dataforClient[i] = response -> data[i];
                }
                printf("%d B, ", response->len);
                //BLE packet contains 4 bytes of 8 bit int's each. Combine all of them to form a single 32-bit int.
                final_dataforClient = ((uint32_t)dataforClient[3]<<24) | (dataforClient[2]<<16) | (dataforClient[1] << 8) | (dataforClient[0]);
                //Ren debug
                dataprint[j] = ( j==PRESSURE)?   (float) final_dataforClient/10 : (float) final_dataforClient/100;
                demo1->set_bme280_value(dataprint[j], j);
                if (j==HUMIDITY) {
                    k=0;   printf("%s  = %0.2f%%   ", dbg_CharType[j], dataprint[j]);
                } else
                {
                    k=j+1;
                    if(j==PRESSURE)    printf("%s  = %0.1f hPa   ", dbg_CharType[j], dataprint[j]);
                    else   printf("%s  = %0.2f degC   ", dbg_CharType[j], dataprint[j]);
                }
                break;
            }
        }
    }
    printf("\r\n");
    bme280Characteristic[k].read();
}

// BLE disconnected
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *) {
    printf("BLE disconnected\r\n");
    /* Start scanning and try to connect again */
    BLE::Instance().gap().startScan(advertisementCallback);
}

void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
    printf("BLE Error = %u\r\n", error);
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    printf("I'm inside BLE init Complete\r\n");
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(ble, error);
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        printf("Not the default instance\r\n");
        return;
    }

    // Ren, clear discovered Characteristic at beginning
    for(int i=0; i<HUMIDITY+1; i++)   {
    	  dataprint[i]=0.0;     is_active[i]=false;
    }

    ble.gap().onDisconnection(disconnectionCallback);
    ble.gap().onConnection(connectionCallback);

    // On reading data, call triggerRead function.
    ble.gattClient().onDataRead(triggerRead);

    // scan interval: 400ms and scan window: 400ms.
    // Every 400ms the device will scan for 400ms
    // This means that the device will scan continuously.
    ble.gap().setScanParams(400, 400);
    error =   ble.gap().startScan(advertisementCallback);
    printf("BLE Error startScan = %u\r\n", error);

}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}

/************************************************************BLE Stuff to here *********************************/

void blinky() {
    green_led = !green_led;
}

// These are example resource values for the Device Object
struct MbedClientDevice device = {
    "Manufacturer_String",      // Manufacturer
    "Type_String",              // Type
    "ModelNumber_String",       // ModelNumber
    "SerialNumber_String"       // SerialNumber
};

// Instantiate the class which implements LWM2M Client API (from simpleclient.h)
MbedClient mbed_client(device);

// Network interaction must be performed outside of interrupt context
Semaphore updates(0);
volatile bool registered = false;
volatile bool clicked = false;
osThreadId mainThread;


#ifdef TARGET_K64F
// Set up Hardware interrupt button.
InterruptIn obs_button(SW2);
InterruptIn unreg_button(SW3);
#else
//In non K64F boards , set up a timer to simulate updating resource,
// there is no functionality to unregister.
Ticker timer;
#endif

/*
 * The button contains one property (click count).
 * When `handle_button_click` is executed, the counter updates.
 */
class ButtonResource {
public:
    ButtonResource(): counter(0) {
        // create ObjectID with metadata tag of 'BTN_SW2', which is 'digital input'
        btn_object = M2MInterfaceFactory::create_object("BTN_SW2");
        M2MObjectInstance* btn_inst = btn_object->create_object_instance();
        // create resource with ID '5501', which is digital input counter
        M2MResource* btn_res = btn_inst->create_dynamic_resource("5501", "Button",
            M2MResourceInstance::INTEGER, true /* observable */);
        // we can read this value
        btn_res->set_operation(M2MBase::GET_ALLOWED);
        // set initial value (all values in mbed Client are buffers)
        // to be able to read this data easily in the Connector console, we'll use a string
        btn_res->set_value((uint8_t*)"0", 1);
    }

    ~ButtonResource() {
    }

    M2MObject* get_object() {
        return btn_object;
    }

    /*
     * When you press the button, we read the current value of the click counter
     * from mbed Device Connector, then up the value with one.
     */
    void handle_button_click() {
        M2MObjectInstance* inst = btn_object->object_instance();
        M2MResource* res = inst->resource("5501");

        // up counter
        counter++;
        printf("handle_button_click, new value of counter is %d\r\n", counter);
        // serialize the value of counter as a string, and tell connector
        char buffer[20];
        int size = sprintf(buffer,"%d",counter);
        res->set_value((uint8_t*)buffer, size);
    }

private:
    M2MObject* btn_object;
    uint16_t counter;
};


void unregister() {
    registered = false;
    updates.release();
}

void button_clicked() {
    clicked = true;
    updates.release();
}

// debug printf function
void trace_printer(const char* str) {
    printf("%s\r\n", str);
}

/****************************************************************More BLE Stuff from here****************/
//BLE thread init and further calls to other BLE methods.
void BLE_thread_init(void){
    printf("I'm inside BLE thread_init.....\r\n");
    eventQueue.call_every(500, blinky);
    //Schedule events before starting the thread since there might be some missed events while scanning / pairing.
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);
    //Loop forever the BLE thread
    eventQueue.dispatch_forever();
}

/****************************************************************More BLE Stuff  to here****************/

// Entry point to the program
int main() {

    unsigned int seed;
    size_t len;

    //Create a new thread for BLE
    Thread BLE_thread;


#ifdef MBEDTLS_ENTROPY_HARDWARE_ALT
    // Used to randomize source port
    mbedtls_hardware_poll(NULL, (unsigned char *) &seed, sizeof seed, &len);

#elif defined MBEDTLS_TEST_NULL_ENTROPY

#warning "mbedTLS security feature is disabled. Connection will not be secure !! Implement proper hardware entropy for your selected hardware."
    // Used to randomize source port
    mbedtls_null_entropy_poll( NULL,(unsigned char *) &seed, sizeof seed, &len);

#else

#error "This hardware does not have entropy, endpoint will not register to Connector.\
You need to enable NULL ENTROPY for your application, but if this configuration change is made then no security is offered by mbed TLS.\
Add MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES and MBEDTLS_TEST_NULL_ENTROPY in mbed_app.json macros to register your endpoint."

#endif

    srand(seed);
    red_led = 1;
    blue_led = 1;
    status_ticker.attach_us(blinky, 250000);
    // Keep track of the main thread
    mainThread = osThreadGetId();

    // Sets the console baud-rate
    output.baud(9600);

    output.printf("Starting mbed Client example...\r\n");

    mbed_trace_init();
    mbed_trace_print_function_set(trace_printer);
    NetworkInterface *network_interface = 0;
    int connect_success = -1;
#if MBED_CONF_APP_NETWORK_INTERFACE == WIFI
    output.printf("\n\rUsing WiFi \r\n");
    output.printf("\n\rConnecting to WiFi..\r\n");
    connect_success = esp.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD);
    network_interface = &esp;
#elif MBED_CONF_APP_NETWORK_INTERFACE == ETHERNET
    output.printf("Using Ethernet\r\n");
    connect_success = eth.connect();
    network_interface = &eth;
#endif
#ifdef MESH
    output.printf("Using Mesh\r\n");
    output.printf("\n\rConnecting to Mesh..\r\n");
    mesh.initialize(&rf_phy);
    connect_success = mesh.connect();
    network_interface = &mesh;
#endif
    if(connect_success == 0) {
        output.printf("\n\rConnected to Network successfully\r\n");
    } else {
        output.printf("\n\rConnection to Network Failed %d! Exiting application....\r\n", connect_success);
        return 0;
    }
    const char *ip_addr = network_interface->get_ip_address();
    if (ip_addr) {
        output.printf("IP address %s\r\n", ip_addr);
    } else {
        output.printf("No IP address\r\n");
    }

    // create our button resources
    ButtonResource button_resource;
#ifdef TARGET_K64F
    // On press of SW3 button on K64F board, example application
    // will call unregister API towards mbed Device Connector
    //unreg_button.fall(&mbed_client,&MbedClient::test_unregister);
    unreg_button.fall(&unregister);

    // Observation Button (SW2) press will send update of endpoint resource values to connector
    obs_button.fall(&button_clicked);
#else
    // Send update of endpoint resource values to connector every 5 seconds periodically
    timer.attach(&button_clicked, 5.0);
#endif

    // Create endpoint interface to manage register and unregister
    mbed_client.create_interface(MBED_SERVER_ADDRESS, network_interface);

    // Create Objects of varying types, see simpleclient.h for more details on implementation.
    M2MSecurity* register_object = mbed_client.create_register_object(); // server object specifying connector info
    M2MDevice*   device_object   = mbed_client.create_device_object();   // device resources object

    // Create list of Objects to register
    M2MObjectList object_list;

    // Add objects to list
    object_list.push_back(device_object);
    object_list.push_back(button_resource.get_object());
    // add bme280 data objects
    if (demo1==NULL)   {
        //	Create bme280 instance
        demo1=new BME280Resource();
    	  printf("created bme280 instance now!!\r\n");
    }
    object_list.push_back(demo1->get_object(PRESSURE));
    object_list.push_back(demo1->get_object(TEMPERATURE));
    object_list.push_back(demo1->get_object(HUMIDITY));
    object_list.push_back(_gasSensor_res.get_object(NH3));
    object_list.push_back(_gasSensor_res.get_object(CO));
    object_list.push_back(_gasSensor_res.get_object(NO2));
    object_list.push_back(_gasSensor_res.get_object(C3H8));
    object_list.push_back(_gasSensor_res.get_object(C4H10));
    object_list.push_back(_gasSensor_res.get_object(CH4));
    object_list.push_back(_gasSensor_res.get_object(H2));
    object_list.push_back(_gasSensor_res.get_object(C2H5OH));

    // Set endpoint registration object
    mbed_client.set_register_object(register_object);

    // Register with mbed Device Connector
    mbed_client.test_register(register_object, object_list);
    registered = true;

    //Start BLE thread after connection is established to device connector. Else, there is conflict.
	  BLE_thread.start(BLE_thread_init);
    // waiting for completion of BLE_thread_init
    Thread::wait(2000);

    set_gasValue_timer.attach(&read_gas_val, 6.0);
    while (true) {

	      printf("inside main for client\r\n");
        triggerLedCharacteristic = false;

        updates.wait(25000);
        if(registered) {
            if(!clicked) {
                //printf("Inside registered ... clicked \r\n");
                mbed_client.test_update_register();
            }
        } else {   // not registered, then stop;
            break;
        }
        if(clicked) {
           clicked = false;
           button_resource.handle_button_click();
        }

    }

    mbed_client.test_unregister();
    status_ticker.detach();
}
