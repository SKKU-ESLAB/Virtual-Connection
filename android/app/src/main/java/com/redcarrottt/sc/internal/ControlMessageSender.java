package com.redcarrottt.sc.internal;

import com.redcarrottt.testapp.Logger;

/* Copyright (c) 2018, contributors. All rights reserved.
 *
 * Contributor: Gyeonghwan Hong <redcarrottt@gmail.com>
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
public class ControlMessageSender {
    private static final String kTag = "ControlMessageSender";

    public void sendRequestConnect(int adapterId) {
        this.sendRequest(ControlMessageProtocol.CMCode.kConnect, adapterId);
        Logger.VERB(kTag, "Send(Control Msg): Request(Connect " + adapterId + ")");
    }

    public void sendRequestDisconnect(int adapterId, int finalSeqNoControl, int finalSeqNoData) {
        String message =
                "" + ControlMessageProtocol.CMCode.kDisconnect + "\n" + adapterId + "\n" + finalSeqNoControl + "\n" + finalSeqNoData;
        this.sendControlMessage(message);
        Logger.VERB(kTag, "Send(Control Msg): Request(Disconnect " + adapterId + "; " +
                "final_seq_no_control=" + finalSeqNoControl + "; final_seq_no_data=" + finalSeqNoData + ")");
    }

    public void sendRequestDisconnectAck(int adapterId) {
        this.sendRequest(ControlMessageProtocol.CMCode.kDisconnectAck, adapterId);
        Logger.VERB(kTag, "Send(Control Msg): Request(DisconnectAck " + adapterId + ")");
    }

    public void sendRequestRetransmit(int segType, int seqNoStart, int seqNoEnd) {
        String message =
                "" + ControlMessageProtocol.CMCode.kRetransmit + "\n" + segType + "\n" + seqNoStart + "\n" + seqNoEnd;
        this.sendControlMessage(message);
        Logger.ERR(kTag,
                "Send(Control Msg): Request(Retransmit " + segType + "; " + "seq_no=" + seqNoStart + "~" + seqNoEnd + ")");
    }

    public void sendRequestQueueStatusMessage(int lastSeqNoControl, int lastSeqNoData) {
        if (Core.singleton().isAdaptersSwitching()) {
            return;
        }

        String message =
                "" + ControlMessageProtocol.CMCode.kQueueStatus + "\n" + lastSeqNoControl + "\n" + lastSeqNoData;
        this.sendControlMessage(message);
        Logger.WARN(kTag,
                "Send(Control Msg): Request(QueueStatus; lastSeqNoControl=" + lastSeqNoControl +
                        "; lastSeqNoData=" + lastSeqNoData + ")");
    }

    public void sendRequestSleep(int adapterId) {
        this.sendRequest(ControlMessageProtocol.CMCode.kSleep, adapterId);
        Logger.VERB(kTag, "Send(Control Msg): Request(Sleep " + adapterId + ")");
    }

    public void sendRequestWakeup(int adapterId) {
        this.sendRequest(ControlMessageProtocol.CMCode.kWakeup, adapterId);
        Logger.VERB(kTag, "Send(Control Msg): Request(WakeUp " + adapterId + ")");
    }

    public void sendNotiPrivateData(int privateType, String privateMessage) {
        String message =
                "" + ControlMessageProtocol.CMCode.kPriv + "\n" + privateType + "\n" + privateMessage;
        this.sendControlMessage(message);
    }

    private void sendRequest(int requestCode, int adapterId) {
        String message = "" + requestCode + "\n" + adapterId;
        this.sendControlMessage(message);
    }

    private void sendControlMessage(String controlMessage) {
        byte[] messageBuffer = controlMessage.getBytes();
        Core.singleton().send(messageBuffer, messageBuffer.length, true);
    }
}
