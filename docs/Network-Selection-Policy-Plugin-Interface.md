# Network Selection Policy Plug-in Interface

* decide()
  * Get the result of the policy (selected network adapter)
  * `SwitchBehavior decide(const Stats &stats, bool is_increasable, bool is_decreasable)`
* on_custom_event()
  * Handle custom event sent from applications
  * `void on_custom_event(std::string &event_string)`
* get_stats_string()
  * Get status string of the network selection policy
  * `std::string get_stats_string(void)`
* get_name()
  * Get the name of the network selection policy
  * `std::string get_name(void)`
