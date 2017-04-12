## Testing the application ``BLEClient_mbedDevConn``

1. Flash the application.
2. Verify that the registration succeeded. You should see `Registered object successfully!` printed to the serial port.
3. On mbed Device Connector, go to [My devices > Connected devices](https://connector.mbed.com/#endpoints). Your device should be listed here.
4. Go to [Device Connector > API Console](https://connector.mbed.com/#console).
5. Click the **Endpoint directory lookups** drop down menu.
![](/img/ep_lookup.PNG)
6. In the menu, click **GET** next to **Endpoint's resource representation**. Select your _endpoint_ and _resource-path_. For example, the _endpoint_ is the identifier of your endpoint that can be found in the `security.h` file as `MBED_ENDPOINT_NAME`. Choose `/3303/0/5700`as a resource path and click **TEST API**.
7. The temperature value from BME280 is shown.

<span class="tips">**Tip:** If you get an error, for example `Server Response: 410 (Gone)`, clear your browser's cache, log out, and log back in.</span>

<span class="notes">**Note:** Only GET methods can be executed through [Device Connector > API Console](https://connector.mbed.com/#console). For other methods, check the [mbed Device Connector Quick Start](https://github.com/ARMmbed/mbed-connector-api-node-quickstart).

### Application resources

The application exposes three [resources](https://docs.mbed.com/docs/mbed-device-connector-web-interfaces/en/latest/#the-mbed-device-connector-data-model):

1. `/3303/0/5700`- Temperature
1. `/3304/0/5700`- Humidity
1. `/3323/0/5700`- Pressure

To learn how to get notifications when resource 1 changes, or how to use resources 2 and 3, read the [mbed Device Connector Quick Start](https://github.com/ARMmbed/mbed-connector-api-node-quickstart).
