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

/*
TODO:
1. Remove references to K64F & use NUCLEO_F429ZI only.
2. Tested with Ethernet. Remove references to wireless technologies.
3. More formatting / code clean up when I find some free time.
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

#include "mbed.h"
#include "rtos.h"

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

/************************************************************BLE Stuff from here *********************************/
BLE &ble = BLE::Instance();
static DiscoveredCharacteristic optCharacteristic;
static bool triggerLedCharacteristic;
static const char PEER_NAME[] = "OPT3001";
uint16_t payload_length = 0;
uint8_t dataforClient[] = {0};
uint32_t final_dataforClient = 0;

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

        if(type == GapAdvertisingData::COMPLETE_LOCAL_NAME) {
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
        printf("S UUID-%x attrs[%u %u]\r\n", service->getUUID().getShortUUID(), service->getStartHandle(), service->getEndHandle());
    } else {
        printf("S UUID-");
        const uint8_t *longUUIDBytes = service->getUUID().getBaseUUID();
        for (unsigned i = 0; i < UUID::LENGTH_OF_LONG_UUID; i++) {
            printf("%02x", longUUIDBytes[i]);
        }
        printf(" attrs[%u %u]\r\n", service->getStartHandle(), service->getEndHandle());
    }
}

//This function is not used. ToDo: get rid of it and any references.
void updateLedCharacteristic(void) {
    if (!BLE::Instance().gattClient().isServiceDiscoveryActive()) {
        optCharacteristic.read();
    }
}


void characteristicDiscoveryCallback(const DiscoveredCharacteristic *characteristicP) {
    printf("  C UUID-%x valueAttr[%u] props[%x]\r\n", characteristicP->getUUID().getShortUUID(), characteristicP->getValueHandle(), (uint8_t)characteristicP->getProperties().broadcast());
    if (characteristicP->getUUID().getShortUUID() == 0xA001) { /* !ALERT! Alter this filter to suit your device. */
        optCharacteristic        = *characteristicP;
        triggerLedCharacteristic = true;
	}
}

void discoveryTerminationCallback(Gap::Handle_t connectionHandle) {
    printf("terminated SD for handle %u\r\n", connectionHandle);
	if(connectionHandle != 0) {
//		led2 = 1;
	}

    if (triggerLedCharacteristic) {
        triggerLedCharacteristic = false;
        eventQueue.call(updateLedCharacteristic);
    }
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *params) {
	printf("Connected to OPT3001 now...\r\n");
    if (params->role == Gap::CENTRAL) {
        BLE &ble = BLE::Instance();
        ble.gattClient().onServiceDiscoveryTermination(discoveryTerminationCallback);
        ble.gattClient().launchServiceDiscovery(params->handle, serviceDiscoveryCallback, characteristicDiscoveryCallback, 0xA000, 0xA001);
    }
}

//ASHOK's triggerRead function

void triggerRead(const GattReadCallbackParams *response) {
	if (response->handle == optCharacteristic.getValueHandle()) {
        optCharacteristic.read();
//		printf("payload length  = %d\r\n", response-> len);
payload_length = response-> len;
		//printf("************OPT Characteristic value = ", optCharacteristic);
		for(int i=0; i< response-> len; i++) {
			//printf("%02x", response->data[i]);
			dataforClient[i] = response -> data[i];
	//		printf("%d", dataforClient[i]);
		}
//BLE packet contains 4 bytes of 8 bit int's each. Combine all of them to form a single 32-bit int.
		final_dataforClient = ((uint32_t)dataforClient[3]<<24) | (dataforClient[2]<<16) | (dataforClient[1] << 8) | (dataforClient[0]);
		printf("Final data for cleint = %x\r\n", final_dataforClient);
	//	printf("\r\n");
		optCharacteristic.read();
    }
}

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *) {
    printf("disconnected\r\n");
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
	printf("I'm inside BLE init\r\n");
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;
	ble_error_t error1 = params->error;

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


// In case of K64F board , there is button resource available
// to change resource value and unregister
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
 * Arguments for running "blink" in it's own thread.
 */
class BlinkArgs {
public:
    BlinkArgs() {
        clear();
    }
    void clear() {
        position = 0;
        blink_pattern.clear();
    }
    uint16_t position;
    std::vector<uint32_t> blink_pattern;
};

/*
 * The Led contains one property (pattern) and a function (blink).
 * When the function blink is executed, the pattern is read, and the LED
 * will blink based on the pattern.
 */
class LedResource {
public:
    LedResource() {
        // create ObjectID with metadata tag of '3201', which is 'digital output'
        led_object = M2MInterfaceFactory::create_object("3201");
        M2MObjectInstance* led_inst = led_object->create_object_instance();

        // 5853 = Multi-state output
        M2MResource* pattern_res = led_inst->create_dynamic_resource("5853", "Pattern",
            M2MResourceInstance::STRING, false);
        // read and write
        pattern_res->set_operation(M2MBase::GET_PUT_ALLOWED);
        // set initial pattern (toggle every 200ms. 7 toggles in total)
        pattern_res->set_value((const uint8_t*)"500:500:500:500:500:500:500", 27);

        // there's not really an execute LWM2M ID that matches... hmm...
        M2MResource* led_res = led_inst->create_dynamic_resource("5850", "Blink",
            M2MResourceInstance::OPAQUE, false);
        // we allow executing a function here...
        led_res->set_operation(M2MBase::POST_ALLOWED);
        // when a POST comes in, we want to execute the led_execute_callback
        led_res->set_execute_function(execute_callback(this, &LedResource::blink));
        // Completion of execute function can take a time, that's why delayed response is used
        led_res->set_delayed_response(true);
        blink_args = new BlinkArgs();
    }

    ~LedResource() {
        delete blink_args;
    }

    M2MObject* get_object() {
        return led_object;
    }

    void blink(void *argument) {
        // read the value of 'Pattern'
        status_ticker.detach();
        red_led = 1;

        M2MObjectInstance* inst = led_object->object_instance();
        M2MResource* res = inst->resource("5853");
        // Clear previous blink data
        blink_args->clear();

        // values in mbed Client are all buffers, and we need a vector of int's
        uint8_t* buffIn = NULL;
        uint32_t sizeIn;
        res->get_value(buffIn, sizeIn);

        // turn the buffer into a string, and initialize a vector<int> on the heap
        std::string s((char*)buffIn, sizeIn);
        free(buffIn);
        output.printf("led_execute_callback pattern=%s\r\n", s.c_str());

        // our pattern is something like 500:200:500, so parse that
        std::size_t found = s.find_first_of(":");
        while (found!=std::string::npos) {
            blink_args->blink_pattern.push_back(atoi((const char*)s.substr(0,found).c_str()));
            s = s.substr(found+1);
            found=s.find_first_of(":");
            if(found == std::string::npos) {
                blink_args->blink_pattern.push_back(atoi((const char*)s.c_str()));
            }
        }
        // check if POST contains payload
        if (argument) {
            M2MResource::M2MExecuteParameter* param = (M2MResource::M2MExecuteParameter*)argument;
            String object_name = param->get_argument_object_name();
            uint16_t object_instance_id = param->get_argument_object_instance_id();
            String resource_name = param->get_argument_resource_name();
            int payload_length = param->get_argument_value_length();
            uint8_t* payload = param->get_argument_value();
            output.printf("Resource: %s/%d/%s executed\r\n", object_name.c_str(), object_instance_id, resource_name.c_str());
            output.printf("Payload: %.*s\r\n", payload_length, payload);
        }
        // do_blink is called with the vector, and starting at -1
        blinky_thread.start(this, &LedResource::do_blink);
    }

private:
    M2MObject* led_object;
    Thread blinky_thread;
    BlinkArgs *blink_args;
    void do_blink() {
        // blink the LED
        red_led = !red_led;
        // up the position, if we reached the end of the vector
        if (blink_args->position >= blink_args->blink_pattern.size()) {
            // send delayed response after blink is done
            M2MObjectInstance* inst = led_object->object_instance();
            M2MResource* led_res = inst->resource("5850");
            led_res->send_delayed_post_response();
            red_led = 1;
            status_ticker.attach_us(blinky, 250000);
            return;
        }
        // Invoke same function after `delay_ms` (upping position)
        Thread::wait(blink_args->blink_pattern.at(blink_args->position));
        blink_args->position++;
        do_blink();
    }
};

/*
 * The button contains one property (click count).
 * When `handle_button_click` is executed, the counter updates.
 */
class ButtonResource {
public:
    ButtonResource(): counter(0) {
        // create ObjectID with metadata tag of 'OPT', which is 'digital input'
        btn_object = M2MInterfaceFactory::create_object("OPT");
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
        //counter++;
	//	for(int i=0; i< payload_length; i++) {
			//printf("%02x", response->data[i]);
			//dataforClient[i] = response -> data[i];
	//		printf("%x", dataforClient[i]);
	//	}
	//	printf("\r\n");

/***************************  Implement OPT data copy to payload here instead of counter ******************/


#ifdef TARGET_K64F
        printf("handle_button_click, new value of counter is %d\r\n", counter);
#else
        printf("OPT3001 value sent to connector =  %x\r\n", final_dataforClient);
#endif
        // serialize the value of counter as a string, and tell connector
        char buffer[20];
        int size = sprintf(buffer,"%x",final_dataforClient);
        res->set_value((uint8_t*)buffer, size);
    }

private:
    M2MObject* btn_object;
    uint16_t counter;
};

class BigPayloadResource {
public:
    BigPayloadResource() {
        big_payload = M2MInterfaceFactory::create_object("1000");
        M2MObjectInstance* payload_inst = big_payload->create_object_instance();
        M2MResource* payload_res = payload_inst->create_dynamic_resource("1", "BigData",
            M2MResourceInstance::STRING, true /* observable */);
        payload_res->set_operation(M2MBase::GET_PUT_ALLOWED);
        payload_res->set_value((uint8_t*)"0", 1);
        payload_res->set_incoming_block_message_callback(
                    incoming_block_message_callback(this, &BigPayloadResource::block_message_received));
        payload_res->set_outgoing_block_message_callback(
                    outgoing_block_message_callback(this, &BigPayloadResource::block_message_requested));
    }

    M2MObject* get_object() {
        return big_payload;
    }

    void block_message_received(M2MBlockMessage *argument) {
        if (argument) {
            if (M2MBlockMessage::ErrorNone == argument->error_code()) {
                if (argument->is_last_block()) {
                    output.printf("Last block received\r\n");
                }
                output.printf("Block number: %d\r\n", argument->block_number());
                // First block received
                if (argument->block_number() == 0) {
                    // Store block
                // More blocks coming
                } else {
                    // Store blocks
                }
            } else {
                output.printf("Error when receiving block message!  - EntityTooLarge\r\n");
            }
            output.printf("Total message size: %d\r\n", argument->total_message_size());
        }
    }

    void block_message_requested(const String& resource, uint8_t *&/*data*/, uint32_t &/*len*/) {
        output.printf("GET request received for resource: %s\r\n", resource.c_str());
        // Copy data and length to coap response
    }

private:
    M2MObject*  big_payload;
};

// Network interaction must be performed outside of interrupt context
Semaphore updates(0);
volatile bool registered = false;
volatile bool clicked = false;
osThreadId mainThread;

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
	printf("I'm inside BLE thread.....\r\n");
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

    // we create our button and LED resources
    ButtonResource button_resource;
    LedResource led_resource;
    BigPayloadResource big_payload_resource;

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
    object_list.push_back(led_resource.get_object());
    object_list.push_back(big_payload_resource.get_object());

    // Set endpoint registration object
    mbed_client.set_register_object(register_object);

    // Register with mbed Device Connector
    mbed_client.test_register(register_object, object_list);
    registered = true;

//Start BLE thread after connection is established to device connector. Else, there is conflict.
	BLE_thread.start(BLE_thread_init);

    while (true) {

	printf("inside main for client\n");
    triggerLedCharacteristic = false;

        updates.wait(25000);
        if(registered) {
            if(!clicked) {
				//printf("Inside registered ... clicked \r\n");
                mbed_client.test_update_register();
            }
        }else {
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
