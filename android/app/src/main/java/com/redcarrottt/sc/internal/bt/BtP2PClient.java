package com.redcarrottt.sc.internal.bt;

import com.redcarrottt.sc.internal.adapter.OnConnectResult;
import com.redcarrottt.sc.internal.adapter.OnDisconnectResult;
import com.redcarrottt.sc.internal.adapter.OnDiscoverResult;
import com.redcarrottt.sc.internal.adapter.P2PClient;

class BtP2PClient extends P2PClient {
    @Override
    protected void discoverImpl(OnDiscoverResult onDiscoverResult) {
        // Assume that target device has already been bonded.
        onDiscoverResult.onDoneDiscover(true);
    }

    @Override
    protected void connectImpl(OnConnectResult onConnectResult) {
        // Assume that target device has already been bonded.
        onConnectResult.onDoneConnect(true);
    }

    @Override
    protected void disconnectImpl(OnDisconnectResult onDisconnectResult) {
        // Assume that target device has already been bonded.
        onDisconnectResult.onDoneDisconnect(true);
    }

    // Constructor
    public BtP2PClient() {
    }

    // Attributes
}
