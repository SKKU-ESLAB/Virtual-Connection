package com.redcarrottt.sc.internal.adapter;

import android.util.Log;

import com.redcarrottt.sc.internal.Segment;
import com.redcarrottt.sc.internal.SegmentManager;
import com.redcarrottt.testapp.Logger;

import java.util.Date;

import static com.redcarrottt.sc.internal.ExpConfig.VERBOSE_CLIENT_ADAPTER;
import static com.redcarrottt.sc.internal.ExpConfig.VERBOSE_RECEIVER_TIME;
import static com.redcarrottt.sc.internal.ExpConfig.VERBOSE_SEGMENT_DEQUEUE_CTRL;
import static com.redcarrottt.sc.internal.ExpConfig.VERBOSE_SEGMENT_DEQUEUE_DATA;
import static com.redcarrottt.sc.internal.SegmentManager.kSQRecvControl;
import static com.redcarrottt.sc.internal.SegmentManager.kSQRecvData;

/* Copyright (c) 2019, contributors. All rights reserved.
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

@SuppressWarnings("SynchronizeOnNonFinalField")
class ReceiverThread extends Thread {
    private final String kTag = "ReceiverThread";

    void waitUntilFinish() {
        try {
            synchronized (this.mWaitObject) {
                if (this.isOn()) {
                    Logger.DEBUG(kTag,
                            "Waiting for receiver thread... (" + this.mMotherAdapter.getName() +
                                    ")");
                    synchronized (this.mIsWaiting) {
                        this.mIsWaiting = true;
                    }
                    this.mWaitObject.wait();
                    Logger.DEBUG(kTag,
                            "Receiver thread's end is detected... (" + this.mMotherAdapter.getName() + ")");
                }
            }
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void notifyFinished() {
        synchronized (this.mIsWaiting) {
            if (this.mIsWaiting) {
                this.mWaitObject.notifyAll();
            }
        }
    }

    private Boolean mIsWaiting;
    private Object mWaitObject = new Object();

    @Override
    public void run() {
        this.setName(this.mMotherAdapter.getName() + "/Receiver");
        synchronized (this.mIsOn) {
            this.mIsOn = true;
        }
        synchronized (this.mIsWaiting) {
            this.mIsWaiting = false;
        }

        this.receiverThreadLoop();

        this.notifyFinished();
    }

    private int mReceiveCount = 0;
    private Date mDates[] = new Date[5];
    private long mIntervals[] = new long[4];

    private void receiverThreadLoop() {
        Logger.VERB(kTag, this.mMotherAdapter.getName() + "'s Receiver thread starts");

        while (this.isOn()) {
            if (VERBOSE_RECEIVER_TIME) this.mDates[0] = new Date();

            SegmentManager sm = SegmentManager.singleton();

            int length_to_receive = Segment.kSegHeaderSize + Segment.kSegPayloadSize;
            byte[] receive_buffer = new byte[length_to_receive];

            if (VERBOSE_RECEIVER_TIME) this.mDates[1] = new Date();

            if (VERBOSE_CLIENT_ADAPTER) {
                Logger.DEBUG(kTag, this.mMotherAdapter.getName() + ": Receiving...");
            }
            int res = this.mMotherAdapter.receive(receive_buffer, length_to_receive);
            if (res < length_to_receive) {
                Logger.WARN(kTag, "Receiving failed at " + this.mMotherAdapter.getName());
                break;
            }

            if (VERBOSE_RECEIVER_TIME) this.mDates[2] = new Date();

            // Set segment
            Segment segmentToReceive = sm.popFreeSegment();
            segmentToReceive.setByteArray(receive_buffer);

            if (VERBOSE_RECEIVER_TIME) this.mDates[3] = new Date();

            if (segmentToReceive.isAck()) {
                // For now, Android-side does not handle ACK message.
                continue;
            }

            if (VERBOSE_SEGMENT_DEQUEUE_CTRL && segmentToReceive.isControl()) {
                Logger.DEBUG(getName(),
                        "Receive Segment: seqno=" + segmentToReceive.getSeqNo() + " / " + "type" + "=ctrl");
            }
            if (VERBOSE_SEGMENT_DEQUEUE_DATA && !segmentToReceive.isControl()) {
                Logger.DEBUG(getName(),
                        "Receive Segment: seqno=" + segmentToReceive.getSeqNo() + " / " + "type" + "=data");
            }

            if (segmentToReceive.isControl()) {
                sm.enqueue(kSQRecvControl, segmentToReceive);
            } else {
                sm.enqueue(kSQRecvData, segmentToReceive);
            }

            // Send ACK message
            sendAckSegment(segmentToReceive.getSeqNo(), segmentToReceive.getFlag(),
                    segmentToReceive.getSendStartTSSec(), segmentToReceive.getSendStartTSUsec(),
                    segmentToReceive.getMediaStartTSSec(), segmentToReceive.getMediaStartTSUsec());

            if (VERBOSE_RECEIVER_TIME) this.mDates[4] = new Date();

            if (VERBOSE_RECEIVER_TIME) {
                this.mReceiveCount++;
                for (int i = 0; i < 4; i++) {
                    this.mIntervals[i] += this.mDates[i + 1].getTime() - this.mDates[i].getTime();
                }
                if (this.mReceiveCount % 500 == 0) {
                    Log.d(kTag,
                            "Receive Time " + this.mIntervals[0] + " / " + this.mIntervals[1] +
                                    " / " + this.mIntervals[2] + " / " + this.mIntervals[3]);
                    for (int i = 0; i < 4; i++) {
                        this.mIntervals[i] = 0;
                    }
                }
            }
        }
        Logger.VERB(kTag, this.mMotherAdapter.getName() + "'s Receiver thread ends");
    }

    private void sendAckSegment(int seq_no, int flag, int send_start_ts_sec,
                                int send_start_ts_usec, int media_start_ts_sec,
                                int media_start_ts_usec) {
        // Filter ACK messages
        if (seq_no % 50 != 0) return;

        String message = "ACK";
        byte[] messageBytes = message.getBytes();

        SegmentManager sm = SegmentManager.singleton();
        Segment ackSegment = sm.popFreeSegment();
        ackSegment.initByteArray();
        ackSegment.setMetadataAck(seq_no, messageBytes.length, flag, send_start_ts_sec,
                send_start_ts_usec, media_start_ts_sec, media_start_ts_usec);
        ackSegment.setPayloadData(messageBytes, 0, messageBytes.length);

        this.mMotherAdapter.send(ackSegment.getByteArray(), ackSegment.getByteArraySize());
        sm.addFreeSegmentToList(ackSegment);
    }

    void finish() {
        synchronized (this.mIsOn) {
            this.mIsOn = false;
        }
    }

    ReceiverThread(ClientAdapter adapter) {
        this.mMotherAdapter = adapter;
        this.mIsOn = false;
        this.mIsWaiting = false;
    }

    public boolean isOn() {
        return this.mIsOn;
    }

    private ClientAdapter mMotherAdapter;

    private Boolean mIsOn;
}