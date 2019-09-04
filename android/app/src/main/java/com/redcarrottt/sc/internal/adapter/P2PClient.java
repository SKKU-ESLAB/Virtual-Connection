package com.redcarrottt.sc.internal.adapter;

/* Copyright (c) 2017-2018. All rights reserved.
 *  Gyeonghwan Hong (redcarrottt@gmail.com)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import com.redcarrottt.testapp.Logger;

interface P2PDiscoverResultListener {
    void onDiscoverResult(boolean isSuccess);
}

interface P2PConnectResultListener {
    void onConnectResult(boolean isSuccess);
}

interface DisconnectResultListener {
    void onDisconnectResult(boolean isSuccess);
}

public abstract class P2PClient {
    private final String kTag = "P2PClient";

    // Main function 1: Discover
    public void discover(P2PDiscoverResultListener resultListener) {
        if (this.getState() != State.kDisconnected) {
            Logger.ERR(kTag, "It's already discovering is in progress");
            doneDiscoverTx(resultListener, false);
            return;
        }
        runDiscoverTx(resultListener);
    }

    private void runDiscoverTx(P2PDiscoverResultListener resultListener) {
        if (sOngoingDiscoverTx == null) {
            sOngoingDiscoverTx = new DiscoverTransaction();
            setState(State.kDiscovering);
            sOngoingDiscoverTx.start(resultListener);
        } else {
            Logger.ERR(kTag, "Discover has already been in progress.");
            doneDiscoverTx(resultListener, false);
        }
    }

    private void doneDiscoverTx(P2PDiscoverResultListener resultListener, boolean isSuccess) {
        sOngoingDiscoverTx = null;
        if (isSuccess) {
            this.setState(State.kDiscovered);
        } else {
            this.setState(State.kDisconnected);
        }

        if (resultListener != null) {
            resultListener.onDiscoverResult(isSuccess);
        }
    }

    private static DiscoverTransaction sOngoingDiscoverTx = null;

    class DiscoverTransaction {
        private P2PDiscoverResultListener mResultListener;

        DiscoverTransaction() {
        }

        void start(P2PDiscoverResultListener resultListener) {
            this.mResultListener = resultListener;
            discoverImpl(onDiscoverResult);
        }

        private OnDiscoverResult onDiscoverResult = new OnDiscoverResult() {
            @Override
            public void onDoneDiscover(boolean isSuccess) {
                doneDiscoverTx(mResultListener, isSuccess);
            }
        };
    }

    // Main function 2: Connect
    public void connect(P2PConnectResultListener resultListener) {
        if (this.getState() != State.kDiscovered) {
            Logger.ERR(kTag, "It's already connecting is in progress");
            doneConnectTx(resultListener, false);
            return;
        }
        runConnectTx(resultListener);
    }

    private void runConnectTx(P2PConnectResultListener resultListener) {
        if (sOngoingConnectTx == null) {
            sOngoingConnectTx = new ConnectTransaction();
            setState(State.kConnecting);
            sOngoingConnectTx.start(resultListener);
        } else {
            Logger.ERR(kTag, "Connect has already been in progress.");
            doneConnectTx(resultListener, false);
        }
    }

    private void doneConnectTx(P2PConnectResultListener resultListener, boolean isSuccess) {
        sOngoingConnectTx = null;
        if (isSuccess) {
            this.setState(State.kConnected);
        } else {
            this.setState(State.kDisconnected);
        }

        if (resultListener != null) {
            resultListener.onConnectResult(isSuccess);
        }
    }

    private static ConnectTransaction sOngoingConnectTx = null;

    class ConnectTransaction {
        private P2PConnectResultListener mResultListener;

        ConnectTransaction() {
        }

        void start(P2PConnectResultListener resultListener) {
            this.mResultListener = resultListener;
            connectImpl(onConnectResult);
        }

        private OnConnectResult onConnectResult = new OnConnectResult() {
            @Override
            public void onDoneConnect(boolean isSuccess) {
                doneConnectTx(mResultListener, isSuccess);
            }
        };
    }

    // Main function 3: Disconnect
    public void disconnect(DisconnectResultListener resultListener) {
        int state = this.getState();
        if (state == State.kDisconnected || state == State.kDisconnecting) {
            Logger.ERR(kTag, "It's already disconnected or disconnecting is in progress");
            doneDisconnectTx(resultListener, false);
            return;
        }
        runDisconnectTx(resultListener);
    }

    private void runDisconnectTx(DisconnectResultListener resultListener) {
        if (sOngoingDisconnectTx == null) {
            sOngoingDisconnectTx = new DisconnectTransaction();
            sOngoingDisconnectTx.start(resultListener);
        } else {
            Logger.ERR(kTag, "Disconnection has already been in progress.");
            doneDisconnectTx(resultListener, false);
        }
    }

    private void doneDisconnectTx(DisconnectResultListener resultListener, boolean isSuccess) {
        sOngoingDisconnectTx = null;
        if (isSuccess) {
            this.setState(State.kDisconnected);
        } else {
            this.setState(State.kConnected);
        }
        if (resultListener != null) {
            resultListener.onDisconnectResult(isSuccess);
        }
    }

    private static DisconnectTransaction sOngoingDisconnectTx = null;

    class DisconnectTransaction {
        private DisconnectResultListener mResultListener;

        DisconnectTransaction() {
        }

        void start(DisconnectResultListener resultListener) {
            this.mResultListener = resultListener;

            setState(State.kDisconnecting);
            disconnectImpl(onDisconnectResult);
        }

        private OnDisconnectResult onDisconnectResult = new OnDisconnectResult() {
            @Override
            public void onDoneDisconnect(boolean isSuccess) {
                doneDisconnectTx(mResultListener, isSuccess);
            }
        };

    }

    // Implemented by child classes
    protected abstract void discoverImpl(OnDiscoverResult onDiscoverResult);

    protected abstract void connectImpl(OnConnectResult onConnectResult);

    protected abstract void disconnectImpl(OnDisconnectResult onDisconnectResult);

    // State
    class State {
        public static final int kDisconnected = 0;
        public static final int kDiscovering = 1;
        public static final int kDiscovered = 2;
        public static final int kConnecting = 3;
        public static final int kConnected = 4;
        public static final int kDisconnecting = 5;
    }

    @SuppressWarnings("SynchronizeOnNonFinalField")
    protected int getState() {
        int state;
        synchronized (this.mState) {
            state = this.mState;
        }
        return state;
    }

    @SuppressWarnings("SynchronizeOnNonFinalField")
    private void setState(int newState) {
        synchronized (this.mState) {
            this.mState = newState;
        }
    }

    protected P2PClient() {
        this.mState = State.kDisconnected;
    }

    // State
    private Integer mState;
}