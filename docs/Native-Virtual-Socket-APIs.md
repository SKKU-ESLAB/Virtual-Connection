# Virtual Socket APIs (C++ Native)

## Data Transfer
* send()
  * Send data to target peer device with any P2P connection
  * `int send(const void *dataBuffer, uint32_t dataLength)`

* receive()
  * Receive data from target peer device with any P2P connection
  * `int receive(void **dataBuffer)`

## Preparation
### Handling Selective Connection Library
* start_sc()
  * Start Selective Connection
  * Overloaded Functions
    * `void start_sc(StartCallback startCallback)`
    * `void start_sc(StartCallback startCallback, bool is_monitor, bool is_logging, bool is_append)`
  * Handler: `typedef void (*StartCallback)(bool is_success)`

* stop_sc()
  * Stop Selective Connection
  * Overloaded Functions
    * `void stop_sc(StopCallback stopCallback)`
    * `void stop_sc(StopCallback stopCallback, bool is_monitor, bool is_logging)`
  * Handler: `typedef void (*StopCallback)(bool is_success)`

### Handling Network Adapters
* register_adapter()
  * Register a network adapter to selective connection
  * `void register_adapter(ServerAdapter *adapter)`
  
### Handling Network Selection Policy
* set_nm_policy()
  * Configure network selection policy of this system
  * `void set_nm_policy(NMPolicy* nm_policy)`
* get_nm_policy()
  * Get current network selection policy of this system
  * `NMPolicy* get_nm_policy(void)`
