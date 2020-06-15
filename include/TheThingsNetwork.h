/*******************************************************************************
 * 
 * ttn-esp32 - The Things Network device library for ESP-IDF / SX127x
 * 
 * Copyright (c) 2018 Manuel Bleichenbacher
 * 
 * Licensed under MIT License
 * https://opensource.org/licenses/MIT
 *
 * High-level API for ttn-esp32.
 *******************************************************************************/

#ifndef _THETHINGSNETWORK_H_
#define _THETHINGSNETWORK_H_

#include <stdint.h>
#include "driver/spi_master.h"

/**
 * @brief Constant for indicating that a pin is not connected
 */
#define TTN_NOT_CONNECTED 0xff

typedef uint8_t port_t;

/**
 * @brief Response codes
 */
enum TTNResponseCode
{
  kTTNErrorTransmissionFailed = -1,
  kTTNErrorUnexpected = -10,
  kTTNSuccessfulTransmission = 1,
  kTTNSuccessfulReceive = 2
};

/**
 * @brief Callback for recieved messages
 * 
 * @param payload  pointer to the received bytes
 * @param length   number of received bytes
 * @param port     port the message was received on
 */
typedef void (*TTNMessageCallback)(const uint8_t* payload, size_t length, port_t port);

/**
 * @brief TTN device
 * 
 * The 'TheThingsNetwork' class enables ESP32 devices with SX1272/73/76/77/78/79 LoRaWAN chips
 * to communicate via The Things Network.
 * 
 * Only one instance of this class must be created.
 */
class TheThingsNetwork
{
public:
    /**
     * @brief Construct a new The Things Network device instance.
     */
    TheThingsNetwork();

    /**
     * @brief Destroy the The Things Network device instance.
     */
    ~TheThingsNetwork();

    /**
     * @brief Reset the LoRaWAN radio.
     * 
     * Does not clear provisioned keys.
     */
    void reset();

    /**
     * @brief Configures the pins used to communicate with the LoRaWAN radio chip.
     * 
     * 
     * The SPI bus must be first configured using spi_bus_initialize(). Then it is passed as the first parameter.
     * Additionally, 'gpio_install_isr_service()' must be called to initialize the GPIO ISR handler service.
     * 
     * @param spi_host  The SPI bus/peripherial to use (SPI_HOST, HSPI_HOST or VSPI_HOST).
     * @param nss       The GPIO pin number connected to the radio chip's NSS pin (serving as the SPI chip select)
     * @param rxtx      The GPIO pin number connected to the radio chip's RXTX pin (TTN_NOT_CONNECTED if not connected)
     * @param rst       The GPIO pin number connected to the radio chip's RST pin (TTN_NOT_CONNECTED if not connected)
     * @param dio0      The GPIO pin number connected to the radio chip's DIO0 pin
     * @param dio1      The GPIO pin number connected to the radio chip's DIO1 pin
     */
    void configurePins(spi_host_device_t spi_host, uint8_t nss, uint8_t rxtx, uint8_t rst, uint8_t dio0, uint8_t dio1);

    /**
     * @brief Sets the information needed to activate the device via OTAA, without actually activating.
     * 
     * The provided device EUI, app EUI and app key are saved in non-volatile memory. Before
     * this function is called, 'nvs_flash_init' must have been called once.
     * 
     * Call join() without arguments to activate.
     * 
     * @param devEui  Device EUI (16 character string with hexadecimal data)
     * @param appEui  Application EUI of the device (16 character string with hexadecimal data)
     * @param appKey  App Key of the device (32 character string with hexadecimal data)
     * @return true   if the provisioning was successful
     * @return false  if the provisioning failed
     */
    bool provision(const char *devEui, const char *appEui, const char *appKey);

    /**
     * @brief Sets the information needed to activate the device via OTAA, using the MAC to generate the device EUI
     * and without actually activating.
     * 
     * The generated device EUI and the provided app EUI and app key are saved in non-volatile memory. Before
     * this function is called, 'nvs_flash_init' must have been called once.
     * 
     * The device EUI is generated by retrieving the ESP32's WiFi MAC address and expanding it into a device EUI
     * by adding FFFE in the middle. So the MAC address A0:B1:C2:01:02:03 becomes the EUI A0B1C2FFFE010203.
     * This hexadecimal data can be entered into the Device EUI field in the TTN console.
     * 
     * Generating the device EUI from the MAC address allows to flash the same app EUI and app key to a batch of
     * devices. However, using the same app key for multiple devices is insecure. Only use this approach if
     * it is okay for that the LoRa communication of your application can easily be intercepted and that
     * forged data can be injected.
     * 
     * Call join() without arguments to activate.
     * 
     * @param appEui  Application EUI of the device (16 character string with hexadecimal data)
     * @param appKey  App Key of the device (32 character string with hexadecimal data)
     * @return true   if the provisioning was successful
     * @return false  if the provisioning failed
     */
    bool provisionWithMAC(const char *appEui, const char *appKey);

    /**
     * @brief Start task that listens on configured UART for AT commands.
     * 
     * Run 'make menuconfig' to configure it.
     */
    void startProvisioningTask();

    /**
     * @brief Wait until the device EUI, app EUI and app key have been provisioned
     * via the provisioning task.
     * 
     * If device is already provisioned (stored data in NVS, call to 'provision()'
     * or call to 'join(const char*, const char*, const char*)', this function
     * immediately returns.
     */
    void waitForProvisioning();

     /**
     * @brief Activate the device via OTAA.
     * 
     * The app EUI, app key and dev EUI must already have been provisioned by a call to 'provision()'.
     * Before this function is called, 'nvs_flash_init' must have been called once.
     * 
     * The function blocks until the activation has completed or failed.
     * 
     * @return true   if the activation was succeful
     * @return false  if the activation failed
     */
    bool join();

   /**
     * @brief Set the device EUI, app EUI and app key and activate the device via OTAA.
     * 
     * The device EUI, app EUI and app key are NOT saved in non-volatile memory.
     * 
     * The function blocks until the activation has completed or failed.
     * 
     * @param devEui  Device EUI (16 character string with hexadecimal data)
     * @param appEui  Application EUI of the device (16 character string with hexadecimal data)
     * @param appKey  App Key of the device (32 character string with hexadecimal data)
     * @return true   if the activation was succeful
     * @return false  if the activation failed
     */
    bool join(const char *devEui, const char *appEui, const char *appKey);

    /**
     * @brief Transmit a message
     * 
     * The function blocks until the message could be transmitted and a message has been received
     * in the subsequent receive window (or the window expires). Additionally, the function will
     * first wait until the duty cycle allows a transmission (enforcing the duty cycle limits).
     * 
     * @param payload  bytes to be transmitted
     * @param length   number of bytes to be transmitted
     * @param port     port (default to 1)
     * @param confirm  flag indicating if a confirmation should be requested. Default to 'false'
     * @return TkTTNSuccessfulTransmission   Successful transmission
     * @return kTTNErrorTransmissionFailed   Transmission failed
     * @return TkTTNErrorUnexpected          Unexpected error
     */
    TTNResponseCode transmitMessage(const uint8_t *payload, size_t length, port_t port = 1, bool confirm = false);

    /**
     * @brief Set the function to be called when a message is received
     * 
     * When a message is received, the specified function is called. The
     * message, its length and the port number are provided as
     * parameters. The values are only valid during the duration of the
     * callback. So they must be immediately processed or copied.
     * 
     * Messages are received as a result of 'transmitMessage' or 'poll'. The callback is called
     * in the task that called any of these functions and it occurs before these functions
     * return control to the caller.
     * 
     * @param callback  the callback function
     */
    void onMessage(TTNMessageCallback callback);

    /**
     * @brief Checks if device EUI, app EUI and app key have been stored in non-volatile storage
     * or have been provided as by a call to 'join(const char*, const char*, const char*)'.
     * 
     * @return true   if they are stored, complete and of the correct size
     * @return false  otherwise
     */
    bool isProvisioned();

    /**
     * @brief Sets the RSSI calibration value for LBT (Listen Before Talk).
     * 
     * This value is added to RSSI measured prior to decision. It must include the guardband.
     * Ignored in US, EU, IN and other countries where LBT is not required.
     * Default to 10 dB.
     * 
     * @param rssiCal RSSI calibration value, in dB
     */
    void setRSSICal(int8_t rssiCal);

    /**
     * @brief Disables a channel via the underlying LMIC library.
     *
     *.Note that it's return value is triggered via the *CHANGE* in state from 
     * *ENABLED->DISABLED*.  A repeat call will lead to a return value of 'false' 
     * until the channel has been 'enabled' inbetween.
     * This will fail to build if this component has not been configured
     * (idf.py menuconfig / make menuconfig )
     *
     * @param channel unsigned integer indicating the channel number to disable
     * @return true The channel was originally enabled and has now been disabled
     * @return false otherwise
     */
    bool disableChannel (uint8_t channel);

    /**
     * @brief Enables a sub band (group of 8 channels).
     *
     * This function is used to enable a consecutive group of 8 predefined channels.  This
     * function works through the underlying LMIC library.
     * This will fail to build if this component has not been configured 
     * (idf.py menuconfig / make menuconfig )
     *
     * @param band unsigned integer indicating which block of channels to enable
     * @return true success, at least one of the channels in the sub-band has been enabled
     * @return false otherwise
     */
    bool enableSubBand(uint8_t band);

    /**
     * @brief Enables a channel via the underlying LMIC library.
     *
     *.Note that it's return value is triggered via the *CHANGE* in state from 
     * *DISABLED->ENABLED*.  A repeat call will lead to a return value of 'false' 
     * until the channel has been 'disabled' inbetween.
     * This will fail to build if this component has not been configured 
     * (idf.py menuconfig / make menuconfig )
     *
     * @param channel unsigned integer indicating which channel to enable
     * @return true The channel was originally disabled and has now been enabled
     * @return false otherwise
     */
    bool enableChannel(uint8_t channel);

    /**
     * @brief Disables a sub band (group of 8 channels).
     *
     * This function is used to disable a consecutive group of 8 predefined channels.  This
     * function works through the underlying LMIC library.
     * This will fail to build if this component has not been configured 
     * (idf.py menuconfig / make menuconfig )
     *
     *
     * @param band unsigned integer indicating which block of channels to disable
     * @return true success, at least one of the channels in the sub-band was disabled
     * @return false otherwise
     */
    bool disableSubBand(uint8_t band);

    /**
     * @brief Selects a single sub band (group of 8 channels) to be active at a time.
     *
     * This function is used to exclusively enable a block of consecutive channels.  It operates
     * via LMIC_disableSubBand() and LMIC_enableSubBand(), enabling only the selected band.
     * This works well when being used with gateways that only support a subset of all LoRa
     * channels (many consumer gateways only support up to 8 channels at a time).
     * This will fail to build if this component has not been configured 
     * (idf.py menuconfig / make menuconfig )
     *
     *
     * @param band unsigned integer indicating which block of channels to use
     * @return true success
     * @return false otherwise
     */
    bool selectSubBand(uint8_t band);

    //void  setDrTxpow   (dr_t dr, s1_t txpow);  // set default/start DR/txpow
    //void  setAdrMode   (bit_t enabled);        // set ADR mode (if mobile turn off)

private:
    TTNMessageCallback messageCallback;

    bool joinCore();
};

#endif
