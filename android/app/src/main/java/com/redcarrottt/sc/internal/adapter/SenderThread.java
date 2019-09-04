package com.redcarrottt.sc.internal.adapter;

import com.redcarrottt.sc.internal.Segment;
import com.redcarrottt.sc.internal.SegmentManager;
import com.redcarrottt.testapp.Logger;

import static com.redcarrottt.sc.internal.SegmentManager.kDeqSendControlData;

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
public class SenderThread extends Thread {
    private final String kTag = "SenderThread";

    void waitUntilFinish() {
        try {
            synchronized (this.mWaitObject) {
                if (this.isOn()) {
                    Logger.DEBUG(kTag,
                            "Waiting for sender thread... (" + this.mMotherAdapter.getName() + ")");
                    synchronized (this.mIsWaiting) {
                        this.mIsWaiting = true;
                    }
                    this.mWaitObject.wait();
                    Logger.DEBUG(kTag,
                            "Sender thread's end is detected... (" + this.mMotherAdapter.getName() + ")");
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

    boolean isSuspended() {
        synchronized (this.mIsSuspended) {
            return this.mIsSuspended;
        }
    }

    void suspendSending() {
        this.mIsSuspended = true;
    }

    void wakeupSending() {
        synchronized (this.mIsSuspended) {
            this.mIsSuspended = false;
            this.mIsSuspended.notifyAll();
        }
    }

    @Override
    public void run() {
        this.setName(this.mMotherAdapter.getName() + "/Sender");
        synchronized (this.mIsOn) {
            this.mIsOn = true;
        }
        synchronized (this.mIsWaiting) {
            this.mIsWaiting = false;
        }
        synchronized (this.mIsSuspended) {
            this.mIsSuspended = false;
        }

        this.senderThreadLoop();

        Logger.VERB(kTag, this.mMotherAdapter.getName() + "'s Sender thread ends");

        this.notifyFinished();
    }

    private void senderThreadLoop() {
        Logger.VERB(kTag, this.mMotherAdapter.getName() + "'s Sender thread starts");
        while (this.mIsOn) {
            SegmentManager sm = SegmentManager.singleton();
            Segment segmentToSend;

            // If this sender is set to be suspended, wait until it wakes up
            {
                boolean suspended;
                do {
                    suspended = this.isSuspended();
                    if (!suspended) {
                        break;
                    }
                    Logger.VERB(kTag, "Sender thread suspended: " + this.getName());
                    this.mMotherAdapter.onSuspendSender();
                    synchronized (this.mIsSuspended) {
                        try {
                            this.mIsSuspended.wait();
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                    this.mMotherAdapter.onWakeupSender();
                } while (true);
            }

            // Dequeue from a queue (one of the three queues)
            // Priority 1. Failed sending queue
            segmentToSend = sm.popFailedSegment();
            // Priority 2. Send control queue
            // Priority 3. Send data queue
            if (segmentToSend == null) {
                segmentToSend = sm.dequeue(kDeqSendControlData);
            }

            if (segmentToSend == null) {
                // Nothing to send.
                // SegmentManager::wake_up_dequeue_waiting() function may make this case
                continue;
            }

            int state = this.mMotherAdapter.getState();
            if (state == ClientAdapter.State.kDisconnecting || state == ClientAdapter.State.kDisconnected || this.mMotherAdapter.isDisconnectingOnPurpose()) {
                sm.addFailedSegmentToList(segmentToSend);
                continue;
            }

            // If it is suspended, push the segment to the send-fail queue
            if (this.isSuspended()) {
                Logger.VERB(kTag,
                        "Sending segment is pushed to failed queue at " + this.getName() + " " +
                                "(suspended)");
                sm.addFailedSegmentToList(segmentToSend);
                continue;
            }

            Logger.DEBUG(this.mMotherAdapter.getName(),
                    "SEND Segment " + segmentToSend.getSeqNo() + " / " + segmentToSend.getLength() + "" + " / " + segmentToSend.getFlag());

            int res = this.mMotherAdapter.send(segmentToSend.getByteArray(),
                    segmentToSend.getByteArraySize());
            if (res < 0) {
                Logger.WARN(kTag, "Sending failed at " + this.mMotherAdapter.getName());
                sm.addFailedSegmentToList(segmentToSend);
                break;
            }
            sm.addFreeSegmentToList(segmentToSend);
        }
    }

    void finish() {
        synchronized (this.mIsOn) {
            this.mIsOn = false;
        }
        synchronized (this.mIsSuspended) {
            this.mIsSuspended = false;
        }
    }

    SenderThread(ClientAdapter adapter) {
        this.mMotherAdapter = adapter;
        this.mIsOn = false;
        this.mIsSuspended = false;

        this.mIsWaiting = false;
    }

    public boolean isOn() {
        return this.mIsOn;
    }

    private ClientAdapter mMotherAdapter;

    private Boolean mIsOn;
    private Boolean mIsSuspended;
}