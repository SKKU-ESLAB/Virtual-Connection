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

import com.redcarrottt.sc.internal.Core;
import com.redcarrottt.sc.internal.SegmentManager;
import com.redcarrottt.testapp.Logger;

import java.util.ArrayList;
import java.util.Date;

import static com.redcarrottt.sc.internal.SegmentManager.kDeqSendControlData;

public class ClientAdapter {
    private final String kTag = "ClientAdapter";
    private final ClientAdapter self = this;

    // Main Functions: connect, disconnect, send, receive
    public void connect(ConnectResultListener listener, boolean isSendRequest) {
        int state = this.getState();
        if (state != State.kDisconnected) {
            Logger.ERR(kTag,
                    "Connect Failed: Already connected or connect/disconnection is in " +
                            "progress: " + this.getName() + " / " + this.getState());
            listener.onConnectResult(false);
            return;
        }

        if (isSendRequest) {
            Core.singleton().getControlMessageSender().sendRequestConnect(this.getId());
        }

        ConnectThread thread = new ConnectThread(listener);
        thread.start();
    }

    public void disconnectOnCommand(DisconnectResultListener listener) {
        // Check if the adapter is not sleeping
        int state = this.getState();
        if (state == State.kGoingSleep) {
            Logger.VERB(kTag, this.getName() + ": Disconnect - waiting for sleeping...");
            while (state != State.kSleeping) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
                state = this.getState();
            }
        } else if (state != State.kSleeping) {
            Logger.ERR(kTag, this.getName() + ": Disconnect fail - not sleeping - " + state);
            listener.onDisconnectResult(false);
            return;
        }

        // Set this disconnection is on purpose
        this.startDisconnectingOnPurpose();

        // Get my final seq_no
        SegmentManager sm = SegmentManager.singleton();
        int my_final_seq_no_control = sm.getFinalSendSeqNo(SegmentManager.kSNControl);
        int my_final_seq_no_data = sm.getFinalSendSeqNo(SegmentManager.kSNData);

        // Send disconnect request
        Core.singleton().getControlMessageSender().sendRequestDisconnect(this.getId(),
                my_final_seq_no_control, my_final_seq_no_data);

        disconnectInternal(listener);
    }

    public void disconnectOnPeerCommand(DisconnectResultListener listener,
                                        int peerFinalSeqNoControl, int peerFinalSeqNoData) {
        // Set this disconnection is on purpose
        this.startDisconnectingOnPurpose();
        this.peerKnowsDisconnectingOnPurpose();

        // Start wait receiving
//        SegmentManager sm = SegmentManager.singleton();
//        sm.waitReceiving(peerFinalSeqNoControl, peerFinalSeqNoData);

        // Send disconnect ack
        Core.singleton().getControlMessageSender().sendRequestDisconnectAck(this.getId());

        disconnectInternal(listener);
    }

    public void disconnectOnFailure(DisconnectResultListener listener) {
        // Check if the adapter is not sleeping
        int state = this.getState();
        if (state == State.kDisconnected || state == State.kDisconnecting) {
            Logger.VERB(kTag, this.getName() + ": Disconnect - already disconnecting or " +
                    "disconnected  - " + state);
            listener.onDisconnectResult(false);
            return;
        }

        disconnectInternal(listener);
    }

    private void disconnectInternal(DisconnectResultListener listener) {
        // Spawn disconnect thread
        DisconnectThread thread = new DisconnectThread(listener);
        thread.start();
    }

    int send(byte[] dataBuffer, int dataLength) {
        int state = this.getState();
        if (state == State.kSleeping || state == State.kGoingSleep || state == State.kWakingUp) {
            if (!this.isDisconnectingOnPurpose()) {
                Logger.ERR(kTag,
                        "Send Failed: Already sleeping: " + this.getName() + " / " + this.getState());
            }
            return -1;
        } else if (state != State.kActive) {
            Logger.ERR(kTag,
                    "Send Failed: Already disconnected or connect/disconnection is in " +
                            "progress: " + this.getName() + " / " + this.getState());
            return -1;
        }

        if (this.mClientSocket == null) {
            return -2;
        }

//        Logger.VERB(this.getName(), "Send data: length=" + dataLength);

        // Omit Implementing Statistics: SendDataSize
        return this.mClientSocket.send(dataBuffer, dataLength);
    }

    int receive(byte[] dataBuffer, int dataLength) {
        int state = this.getState();
        if (state != State.kActive && state != State.kGoingSleep && state != State.kSleeping && state != State.kWakingUp) {
            if (!this.isDisconnectingOnPurpose()) {
                Logger.ERR(kTag,
                        "Receive Failed: Already disconnected or connect/disconnection " + "is " + "in" + " progress: " + this.getName() + " / " + this.getState());
            }
            return -1;
        }

        if (this.mClientSocket == null) {
            return -2;
        }

        // Omit Implementing Statistics: ReceiveDataSize
        return this.mClientSocket.receive(dataBuffer, dataLength);
    }

    // Connect/Disconnect Threads & Callbacks
    class ConnectThread extends Thread implements TurnOnResultListener, P2PDiscoverResultListener
            , P2PConnectResultListener {
        private Date mTurnOnStartTS;
        private Date mDiscoverStartTS;
        private Date mConnectStartTS;
        private Date mConnectEndTS;

        @Override
        public void run() {
            // Measure switch latency
            this.mTurnOnStartTS = new Date();

            this.setName(self.getName() + "/Connect");
            Logger.IMP(kTag, self.getName() + " Connect Thread Start (id:" + this.getId() + ")");
            setState(ClientAdapter.State.kConnecting);

            if (self.mDevice == null || self.mP2PClient == null || self.mClientSocket == null) {
                this.onFail();
                return;
            }

            // Turn on device
            self.mDevice.turnOn(this);
        }

        @Override
        public void onTurnOnResult(boolean isSuccess) {
            // Measure switch latency
            this.mDiscoverStartTS = new Date();

            int deviceState = self.mDevice.getState();
            if (!isSuccess || deviceState != Device.State.kOn) {
                Logger.ERR(kTag,
                        "Cannot connect the server adapter - turn-on fail: " + self.getName());
                this.onFail();
                return;
            }

            Logger.VERB(kTag, "Turn on success: " + self.getName());

            // Discover and connect to server
            self.mP2PClient.discover(this);
        }

        @Override
        public void onDiscoverResult(boolean isSuccess) {
            // Measure switch latency
            this.mConnectStartTS = new Date();

            int p2pClientState = self.mP2PClient.getState();
            if (!isSuccess || p2pClientState != P2PClient.State.kDiscovered) {
                Logger.ERR(kTag,
                        "Cannot connect the server adapter - discover fail: " + self.getName());
                this.onFail();
                return;
            }

            Logger.VERB(kTag, "Discover success: " + self.getName());

            // Discover and connect to server
            self.mP2PClient.connect(this);
        }

        @Override
        public void onConnectResult(boolean isSuccess) {
            // Measure switch latency
            this.mConnectEndTS = new Date();
            long turnOnMS = this.mDiscoverStartTS.getTime() - this.mTurnOnStartTS.getTime();
            long discoverMS = this.mConnectStartTS.getTime() - this.mDiscoverStartTS.getTime();
            long connectMS = this.mConnectEndTS.getTime() - this.mConnectStartTS.getTime();
            Logger.IMP(kTag,
                    "Connect " + this.getName() + " Latency = " + turnOnMS + "ms + " + discoverMS + "ms + " + connectMS + "ms");

            // Check the result of "Discover and connect"
            int p2pClientState = self.mP2PClient.getState();
            if (!isSuccess || p2pClientState != P2PClient.State.kConnected) {
                Logger.ERR(kTag,
                        "Cannot connect the server adapter - connect fail:" + self.getName());
                self.mDevice.turnOff(null);
                this.onFail();
                return;
            }

            Logger.VERB(kTag, "P2P connect success: " + self.getName());

            Logger.IMP(kTag, self.getName() + " Connect Thread End (id:" + this.getId() + ")");

            this.mSocketOpenThread = new SocketOpenThread();
            this.mSocketOpenThread.start();
        }

        private SocketOpenThread mSocketOpenThread;

        class SocketOpenThread extends Thread {
            @Override
            public void run() {
                Logger.IMP(kTag, self.getName() + " Socket Open Thread Start (id:" + this.getId() + ")");
                // Open client socket
                this.setName(self.getName() + "/SocketOpen");
                int socketState = self.mClientSocket.getState();
                if (socketState != ClientSocket.State.kOpened) {
                    boolean res = self.mClientSocket.open();

                    socketState = self.mClientSocket.getState();
                    if (!res || socketState != ClientSocket.State.kOpened) {
                        Logger.ERR(kTag,
                                "Cannot connect the server adapter - socket open fail: " + self.getName());
                        self.mP2PClient.disconnect(null);
                        self.mDevice.turnOff(null);
                        onFail();
                        return;
                    }
                }

                Logger.VERB(kTag, "Socket connect success: " + self.getName());

                // Run sender & receiver threads
                if (self.mSenderThread != null && !self.mSenderThread.isOn()) {
                    self.mSenderThread.start();
                }

                if (self.mReceiverThread != null && !self.mReceiverThread.isOn()) {
                    self.mReceiverThread.start();
                }

                // Report result success
                self.setState(ClientAdapter.State.kActive);
                if (mResultListener != null) {
                    mResultListener.onConnectResult(true);
                    mResultListener = null;
                }
                Logger.IMP(kTag, self.getName() + " Socket Open Thread End (id:" + this.getId() + ")");
            }
        }

        private void onFail() {
            self.setState(ClientAdapter.State.kDisconnected);

            // Report result fail
            if (this.mResultListener != null) {
                this.mResultListener.onConnectResult(false);
                this.mResultListener = null;
            }
        }

        private ConnectResultListener mResultListener;

        ConnectThread(ConnectResultListener resultListener) {
            this.mResultListener = resultListener;
        }
    }

    class DisconnectThread extends Thread implements com.redcarrottt.sc.internal.adapter.DisconnectResultListener, TurnOffResultListener {
        @Override
        public void run() {
            this.setName(self.getName() + "/Disconnect");
            Logger.IMP(kTag, self.getName() + " Disconnect Thread Start (id:" + this.getId() + ")");
            int oldState = self.getState();
            setState(ClientAdapter.State.kDisconnecting);

            if (isDisconnectingOnPurpose() && !isDisconnectingOnPurposePeer()) {
                waitForDisconnectingOnPurposePeer();
            }

            boolean res = this.__disconnect_thread(oldState);

            finishDisconnectingOnPurpose();

            if (!res) {
                this.onFail();
            }

            Logger.VERB(kTag,
                    self.getName() + "'s Disconnect Thread Finished. (id:" + this.getId() + ")");
        }

        @SuppressWarnings("SynchronizeOnNonFinalField")
        private boolean __disconnect_thread(int oldState) {
            // Finish sender & receiver threads
            if (self.mSenderThread != null) {
                self.mSenderThread.finish();
                if (oldState == ClientAdapter.State.kSleeping) {
                    self.wakeUpInternal();
                }
            }
            if (self.mReceiverThread != null) {
                self.mReceiverThread.finish();
            }

            // Wake up sender thread waiting segment queue
            SegmentManager sm = SegmentManager.singleton();
            sm.wakeUpDequeueWaiting(kDeqSendControlData);

            // Close client socket
            if (self.mClientSocket == null) {
                return false;
            }
            int socketState = self.mClientSocket.getState();
            if (socketState != ClientSocket.State.kClosed) {
                boolean res = self.mClientSocket.close();

                socketState = self.mClientSocket.getState();
                if (!res || socketState != ClientSocket.State.kClosed) {
                    Logger.ERR(kTag,
                            "Cannot disconnect the server adapter - socket " + "close " + "fail: "
                                    + "" + "" + self.getName());
                    return false;
                }
            }

            // P2P Disconnect
            self.mP2PClient.disconnect(this);

            // Wait for sender/receiver thread
            self.mSenderThread.waitUntilFinish();
            self.mReceiverThread.waitUntilFinish();
            return true;
        }

        @Override
        public void onDisconnectResult(boolean isSuccess) {
            // Check the result of "P2P Disconnect"
            if (!isSuccess) {
                Logger.ERR(kTag, "Cannot disconnect the server adapter - " + "disconnect P2P " +
                        "client fail: " + self.getName());
                this.onFail();
                return;
            }

            // Turn off device
            self.mDevice.turnOff(this);
        }

        @Override
        public void onTurnOffResult(boolean isSuccess) {
            if (!isSuccess) {
                Logger.ERR(kTag, "Cannot disconnect the server adapter - turn-off " + "fail:" +
                        "" + " " + self.getName());
                this.onFail();
                return;
            }

            // Report result success
            self.setState(ClientAdapter.State.kDisconnected);
            if (this.mResultListener != null) {
                this.mResultListener.onDisconnectResult(true);
                this.mResultListener = null;
            }

            Logger.IMP(kTag, self.getName() + " Disconnect Thread End (id:" + this.getId() + ")");
        }

        private void onFail() {
            self.setState(ClientAdapter.State.kActive);

            // Report result fail
            if (this.mResultListener != null) {
                this.mResultListener.onDisconnectResult(false);
                this.mResultListener = null;
            }
        }

        private DisconnectResultListener mResultListener;

        DisconnectThread(DisconnectResultListener resultListener) {
            this.mResultListener = resultListener;
        }
    }

    public interface ConnectResultListener {
        void onConnectResult(boolean isSuccess);
    }

    public interface DisconnectResultListener {
        void onDisconnectResult(boolean isSuccess);
    }

    // Sender/Receiver Threads


    private SenderThread mSenderThread;
    private ReceiverThread mReceiverThread;

    @SuppressWarnings("SynchronizeOnNonFinalField")
    public boolean sleep(boolean isSendRequest) {
        int state = this.getState();
        if (state != State.kActive) {
            Logger.ERR(kTag, "Failed to sleep: " + this.getName() + "(state: " + state + ")");
            return false;
        }

        boolean isSenderSuspended = this.mSenderThread.isSuspended();
        if (isSenderSuspended) {
            Logger.ERR(kTag, "Sender has already been suspended!: " + this.getName());
            return false;
        } else {
            if (isSendRequest) {
                // Send Request
                Core.singleton().getControlMessageSender().sendRequestSleep(this.getId());
            }

            // Sleep
            this.setState(State.kGoingSleep);
            this.mSenderThread.suspendSending();

            // Wake up sender thread waiting send queue
            SegmentManager sm = SegmentManager.singleton();
            sm.wakeUpDequeueWaiting(kDeqSendControlData);
            return true;
        }
    }

    void onSuspendSender() {
        this.setState(ClientAdapter.State.kSleeping);
    }

    void onWakeupSender() {
        this.setState(ClientAdapter.State.kActive);
    }

    @SuppressWarnings("SynchronizeOnNonFinalField")
    public boolean wakeUp(boolean isSendRequest) {
        int state = this.getState();
        if (state != State.kSleeping) {
            Logger.ERR(kTag, "Failed to wake up: " + this.getName() + "(state: " + state + ")");
            return false;
        }
        boolean isSenderSuspended = this.mSenderThread.isSuspended();
        if (isSenderSuspended) {
            if (isSendRequest) {
                // Send Request
                Core.singleton().getControlMessageSender().sendRequestWakeup(this.getId());
            }

            // Wake up
            this.setState(State.kWakingUp);
            this.wakeUpInternal();

            return true;
        } else {
            Logger.ERR(kTag, "Sender has not been suspended!: " + this.getName());
            return false;
        }
    }

    private void wakeUpInternal() {
        this.mSenderThread.wakeupSending();
    }

    // Initialize
    public ClientAdapter(int id, String name) {
        this.mId = id;
        this.mName = name;
        this.mState = State.kDisconnected;
        this.mListeners = new ArrayList<>();
        this.mIsDisconnectingOnPurpose = false;
        this.mIsDisconnectingOnPurposePeer = false;

        this.mSenderThread = new SenderThread(this);
        this.mReceiverThread = new ReceiverThread(this);
    }

    protected void initialize(Device device, P2PClient p2pClient, ClientSocket clientSocket) {
        this.mDevice = device;
        this.mP2PClient = p2pClient;
        this.mClientSocket = clientSocket;
    }

    // Attribute getters
    public int getId() {
        return this.mId;
    }

    public String getName() {
        return this.mName;
    }

    public boolean isDisconnectingOnPurpose() {
        return this.mIsDisconnectingOnPurpose;
    }

    private boolean isDisconnectingOnPurposePeer() {
        return this.mIsDisconnectingOnPurposePeer;
    }

    // Attribute setters
    private void startDisconnectingOnPurpose() {
        this.mIsDisconnectingOnPurpose = true;
    }

    private void waitForDisconnectingOnPurposePeer() {
        synchronized (mWaitForDisconnectAck) {
            try {
                mWaitForDisconnectAck.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public void peerKnowsDisconnectingOnPurpose() {
        this.mIsDisconnectingOnPurposePeer = true;
        synchronized (this.mWaitForDisconnectAck) {
            this.mWaitForDisconnectAck.notifyAll();
        }
    }

    private void finishDisconnectingOnPurpose() {
        this.mIsDisconnectingOnPurpose = false;
        this.mIsDisconnectingOnPurposePeer = false;
    }

    // Attributes
    private int mId;
    private String mName;

    // Disconnecting on purpose by a device
    private boolean mIsDisconnectingOnPurpose;
    private boolean mIsDisconnectingOnPurposePeer;
    private Object mWaitForDisconnectAck = new Object();

    // State
    public class State {
        public static final int kDisconnected = 0;
        public static final int kConnecting = 1;
        public static final int kActive = 2;
        public static final int kDisconnecting = 3;
        public static final int kGoingSleep = 4;
        public static final int kSleeping = 5;
        public static final int kWakingUp = 6;
        public static final int kASNum = 7;
    }

    public static String stateToString(int state) {
        final String[] stateStr = {"Disconnected", "Connecting", "Active", "Disconnecting",
                "GoingSleep", "Sleeping", "WakingUp"};
        if (state >= State.kASNum || state < 0) {
            return "";
        } else {
            return stateStr[state];
        }
    }

    @SuppressWarnings("SynchronizeOnNonFinalField")
    public int getState() {
        int state;
        synchronized (this.mState) {
            state = this.mState;
        }
        return state;
    }

    @SuppressWarnings("SynchronizeOnNonFinalField")
    private void setState(int newState) {
        int oldState;
        synchronized (this.mState) {
            oldState = this.mState;
            this.mState = newState;
        }

        Logger.DEBUG(this.getName(),
                "State(" + stateToString(oldState) + "->" + stateToString(newState) + ")");

        for (ClientAdapterStateListener listener : this.mListeners) {
            listener.onUpdateClientAdapterState(this, oldState, newState);
        }
    }

    public void listenState(ClientAdapterStateListener listener) {
        synchronized (this.mListeners) {
            this.mListeners.add(listener);
        }
    }

    // State
    private Integer mState;

    // State Listener
    private final ArrayList<ClientAdapterStateListener> mListeners;

    // Main Components : Device, P2PClient, ClientSocket
    private Device mDevice;
    private P2PClient mP2PClient;

    private ClientSocket mClientSocket;
}