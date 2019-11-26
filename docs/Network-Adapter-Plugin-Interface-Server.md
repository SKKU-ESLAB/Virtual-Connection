# Network Adapter Plug-in Interface (Server-side)
If you want to implement your network adapter, you should three sub-objects of a network adapter(NMPolicy).
* ServerSocket
* P2PServer
* Device

# ServerSocket Interface

* open_impl()
  * Open a socket with peer network adapter
  * If you implement socket-less network, this function's implementation is not necessary.
  * `bool open_impl(bool is_send_request)`
* close_impl()
  * Close a socket with peer network adapter
  * If you implement socket-less network, this function's implementation is not necessary.
  * `bool close_impl(void)`
* send_impl()
  * Send data to peer network adapter
  * `int send_impl(const void *data_buffer, size_t data_length)`
* receive_impl()
  * Receive data from peer network adapter
  * `int receive_impl(void *data_buffer, size_t data_length)`

# P2PServer Interface
* allow_discover_impl()
  * Allow discover for P2P communication
  * `bool allow_discover_impl(void)`
* disallow_discover_impl()
  * Stop allowing discover for P2P communication
  * bool disallow_discover_impl(void)`

# Device Interface
* turn_on_impl()
  * Power on the network device
  * `bool turn_on_impl(void)`
* turn_off_impl()
  * Power off the network device
  * `bool turn_off_impl(void)`
