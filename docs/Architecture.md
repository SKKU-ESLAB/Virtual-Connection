# The Architecture of Selective Connection

Selective connection has well-defined modular architecture of which each module is programmable.

* **Virtual Socket APIs**: APIs allowing applications to specify "what to transfer" without worrying "how to transfer"
* **Network Adapter Plug-ins**: Plug-in modules to transfer data or give hints to network selection policy
* **Network Selection Plug-in**: Module to select network adapter to use according to the system status
* **Segment Queues**: Module to handle seamless handover over multiple network adapters and provide prioritized transfer
* **Network Switcher**: Module to switch network adapters of both self device and peer device.


<img src="https://github.com/SKKU-ESLAB/selective-connection/blob/master/docs/Architecture.png?raw=true" width="500px" />
