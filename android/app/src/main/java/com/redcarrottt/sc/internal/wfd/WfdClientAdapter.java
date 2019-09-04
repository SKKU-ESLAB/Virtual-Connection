package com.redcarrottt.sc.internal.wfd;

import android.app.Activity;

import com.redcarrottt.sc.internal.adapter.ClientAdapter;
import com.redcarrottt.sc.internal.ControlMessageListener;
import com.redcarrottt.sc.internal.ControlMessageProtocol;
import com.redcarrottt.sc.internal.Core;
import com.redcarrottt.testapp.Logger;

public class WfdClientAdapter extends ClientAdapter implements ControlMessageListener {
    private static final String kTag = "WfdClientAdapter";

    // Singleton
    static public WfdClientAdapter singleton(int id, String name, int targetPort,
                                             Activity ownerActivity) {
        if (sSingleton == null) {
            sSingleton = new WfdClientAdapter(id, name, targetPort, ownerActivity);
        }
        return sSingleton;
    }

    static private WfdClientAdapter sSingleton;

    // Constructor
    private WfdClientAdapter(int id, String name, int targetPort, Activity ownerActivity) {
        super(id, name);

        // Components
        WfdDevice device = new WfdDevice(ownerActivity);
        this.mP2pClient = new WfdP2PClient(ownerActivity, false);
        this.mClientSocket = new TcpClientSocket(targetPort);

        // Initialize
        this.initialize(device, this.mP2pClient, this.mClientSocket);

        // Add WfdP2PClient as a control message listener
        Core.singleton().getControlMessageReceiver().addControlMessageListener(this);
    }

    @Override
    public void onReceiveControlMessage(int privateType, String privateMessage) {
        if (privateType == ControlMessageProtocol.PrivType.kWFDInfo) {
            Logger.DEBUG(kTag, "Received WFD info message: " + privateMessage);
            String wfdInfoLines[] = privateMessage.split("\\r?\\n");
            if (wfdInfoLines.length < 5) {
                Logger.ERR(kTag, "WFD info is insufficient: " + wfdInfoLines.length + " < 5");
                return;
            }
            String targetMacAddress = wfdInfoLines[0];
            String ssid = wfdInfoLines[1];
            String passphrase = wfdInfoLines[2];
            String wpsPin = wfdInfoLines[3];
            String targetIpAddress = wfdInfoLines[4];

            mP2pClient.setWfdP2PInfo(targetMacAddress, ssid, passphrase, wpsPin);
            mClientSocket.setTcpClientInfo(targetIpAddress);
        }
    }

    // Components
    private WfdP2PClient mP2pClient;
    private TcpClientSocket mClientSocket;
}
