package com.redcarrottt.sc.internal;

/* Copyright (c) 2017-2018. All rights reserved.
 *  Gyeonghwan Hong (redcarrottt@gmail.com)
 *  Eunsoo Park (esevan.park@gmail.com)
 *  Injung Hwang (sinban04@gmail.com)
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
import com.redcarrottt.testapp.MainActivity;

import java.util.Date;
import java.util.LinkedList;
import java.util.ListIterator;

import static com.redcarrottt.sc.internal.Segment.kSegPayloadSize;

@SuppressWarnings("SynchronizeOnNonFinalField")
public class SegmentManager {
    static private Object sSingletonLock = new Object();
    static private SegmentManager sSingleton = null;

    private static final int kSegFreeThreshold = 256;

    public static final int kSQSendData = 0;
    public static final int kSQRecvData = 1;
    public static final int kSQSendControl = 2;
    public static final int kSQRecvControl = 3;
    public static final int kNumSQ = 4;
    static final int kSQUnknown = 999;

    static public final int kDeqSendControlData = 0;
    static public final int kDeqRecvData = 1;
    static public final int kDeqRecvControl = 2;
    private static final int kNumDeq = 3;

    static public final int kSNData = 0;
    static public final int kSNControl = 1;
    private static final int kNumSN = 2;

    private Integer[] mNextSendSeqNo;

    private Integer[] mExpectedSeqNo;

    private LinkedList[] mQueues;
    private Object[] mDequeueCond;
    private final LinkedList<Segment> mFailedSendingQueue;
    private LinkedList[] mPendingQueue;
    private int[] mQueueLengths;

    private static String kTag = "SegmentManager";

    private final LinkedList<Segment> mFreeSegments;
    private int mFreeSegmentsSize;

    private SegmentManager() {
        this.mQueues = new LinkedList[kNumSQ];
        for (int i = 0; i < kNumSQ; i++) {
            this.mQueues[i] = new LinkedList<Segment>();
        }

        this.mDequeueCond = new Object[kNumDeq];
        for (int i = 0; i < kNumDeq; i++) {
            this.mDequeueCond[i] = new Object();
        }

        this.mFailedSendingQueue = new LinkedList<Segment>();
        this.mPendingQueue = new LinkedList[kNumSQ];
        for (int i = 0; i < kNumSQ; i++) {
            this.mPendingQueue[i] = new LinkedList<Segment>();
        }

        this.mQueueLengths = new int[kNumSQ];
        for (int i = 0; i < kNumSQ; i++) {
            this.mQueueLengths[i] = 0;
        }

        this.mExpectedSeqNo = new Integer[kNumSQ];
        for (int i = 0; i < kNumSQ; i++) {
            this.mExpectedSeqNo[i] = 0;
        }

        this.mNextSendSeqNo = new Integer[kNumSN];
        for (int i = 0; i < kNumSN; i++) {
            this.mNextSendSeqNo[i] = 0;
        }

        this.mFreeSegments = new LinkedList<Segment>();

        QueueWatcherThread queueWatcherThread = new QueueWatcherThread();
        queueWatcherThread.start();
    }


    static public SegmentManager singleton() {
        synchronized (sSingletonLock) {
            if (sSingleton == null) sSingleton = new SegmentManager();
        }

        return sSingleton;
    }

    private int getNextSeqNo(int seq_num_type, int length) {
        int ret = mNextSendSeqNo[seq_num_type];
        mNextSendSeqNo[seq_num_type] += length;
        return ret;
    }

    // Wait data before disconnection
    public void waitReceiving(int wait_seq_no_control, int wait_seq_no_data) {
        synchronized (this.mWaitReceiving) {
            this.mIsWaitReceiving = true;
            this.mWaitSeqNoControl = wait_seq_no_control;
            this.mWaitSeqNoData = wait_seq_no_data;
            try {
                this.mWaitReceiving.wait();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    public int getFinalSendSeqNo(int seq_no_type) {
        synchronized (this.mNextSendSeqNo[seq_no_type]) {
            return this.mNextSendSeqNo[seq_no_type] - 1;
        }
    }

    private int getExpectedSeqNo(int queue_type) {
        synchronized (this.mExpectedSeqNo[queue_type]) {
            return this.mExpectedSeqNo[queue_type];
        }
    }

    private int getLastSeqNo(int queue_type) {
        synchronized (this.mExpectedSeqNo[queue_type]) {
            return this.mExpectedSeqNo[queue_type] - 1;
        }
    }

    private void checkReceivingDone() {
        boolean is_wakeup = false;
        synchronized (this.mWaitReceiving) {
            if (this.mIsWaitReceiving && this.getExpectedSeqNo(kSQRecvControl) > this.mWaitSeqNoControl && this.getExpectedSeqNo(kSQRecvData) > this.mWaitSeqNoData) {
                is_wakeup = true;
            }

            if (is_wakeup) {
                this.mWaitReceiving.notifyAll();
                this.mIsWaitReceiving = false;
            }
        }
    }

    public void wakeUpDequeueWaiting(int dequeueType) {
        synchronized (mDequeueCond[dequeueType]) {
            this.mDequeueCond[dequeueType].notifyAll();
        }
    }

    private boolean mIsWaitReceiving = false;
    private int mWaitSeqNoControl = 0;
    private int mWaitSeqNoData = 0;
    private Object mWaitReceiving = new Object();

    int send_to_segment_manager(byte[] data_bytes, int data_length, boolean isControl) {
        if (data_bytes == null || data_length <= 0) throw new AssertionError();

        int offset = 0;
        int num_of_segments = (data_length + Segment.kSegPayloadSize - 1) / Segment.kSegPayloadSize;
        int seq_num_type = (isControl) ? kSNControl : kSNData;
        int allocated_seq_no = getNextSeqNo(seq_num_type, num_of_segments);
        int seg_idx;

        for (seg_idx = 0; seg_idx < num_of_segments; seg_idx++) {
            Segment segmentToEnqueue = popFreeSegment();

            // Calculate segment metadata fields
            int seq_no = allocated_seq_no++;
            int payload_length = (data_length - offset < kSegPayloadSize) ?
                    (data_length - offset) : kSegPayloadSize;

            // Setting segment metadata
            segmentToEnqueue.initByteArray();
            segmentToEnqueue.setMetadataNormal(seq_no, payload_length);
            if (offset + payload_length < data_length) segmentToEnqueue.setMoreFragmentFlag();
            if (isControl) segmentToEnqueue.setControlFlag();

            // Setting segment payload data
            segmentToEnqueue.setPayloadData(data_bytes, offset, payload_length);
            offset += payload_length;

            // Enqueue the segment
            if (isControl) {
                enqueue(kSQSendControl, segmentToEnqueue);
            } else {
                enqueue(kSQSendData, segmentToEnqueue);
            }
        }

        return 0;
    }

    byte[] recv_from_segment_manager(ProtocolData protocolData, boolean isControl) {
        if (protocolData == null) throw new AssertionError();

        byte[] serialized;
        int offset = 0;
        int data_size;
        boolean isMoreFragment;

        Segment segmentDequeued;
        do {
            if (isControl) {
                segmentDequeued = dequeue(kDeqRecvControl);
            } else {
                segmentDequeued = dequeue(kDeqRecvData);
            }
        } while (segmentDequeued == null);

        ProtocolManager.parse_header(segmentDequeued.getPayloadData(), protocolData);
        if (protocolData.len == 0) return null;

        //Logger.DEBUG(kTag, "pd.len is " + pd.len);
        serialized = new byte[protocolData.len];

        // Handle the first segment of the data bulk, because it contains protocol data
        data_size = segmentDequeued.getLength() - ProtocolManager.kProtocolHeaderSize;
        System.arraycopy(segmentDequeued.getPayloadData(), ProtocolManager.kProtocolHeaderSize,
                serialized, offset, data_size);
        offset += data_size;

        isMoreFragment = segmentDequeued.isMoreFragment();
        addFreeSegmentToList(segmentDequeued);

        while (isMoreFragment) {
            do {
                if (isControl) {
                    segmentDequeued = dequeue(kDeqRecvControl);
                } else {
                    segmentDequeued = dequeue(kDeqRecvData);
                }
            } while (segmentDequeued == null);

            // Copy each segments' payload to the serialized data buffer
            byte[] segment_payload_data = segmentDequeued.getPayloadData();
            data_size = segmentDequeued.getLength();
            System.arraycopy(segment_payload_data, 0, serialized, offset, data_size);

            // Update MF and offset
            isMoreFragment = segmentDequeued.isMoreFragment();
            offset += data_size;
            addFreeSegmentToList(segmentDequeued);
        }

        return serialized;
    }

    @SuppressWarnings("unchecked")
    public void enqueue(int queueType, Segment segment) {
        if (queueType >= kNumSQ) throw new AssertionError();

        int dequeueType;
        switch (queueType) {
            case kSQRecvControl:
                dequeueType = kDeqRecvControl;
                break;
            case kSQRecvData:
                dequeueType = kDeqRecvData;
                break;
            case kSQSendControl:
            case kSQSendData:
                dequeueType = kDeqSendControlData;
                break;
            default:
                Logger.ERR(kTag, "Enqueue: Unknown queue type: " + queueType);
                return;
        }

//        if (queueType == kSQRecvData || queueType == kSQRecvControl) {
//            Logger.VERB(kTag, "Received type=" + queueType + " seq_no=" + segment.getSeqNo());
//        }


        boolean segmentEnqueued = false;

        synchronized (this.mQueues[queueType]) {
            synchronized (this.mExpectedSeqNo[queueType]) {
                int seqNo = segment.getSeqNo();
                boolean isPending = false;
                if (seqNo == this.mExpectedSeqNo[queueType]) {
                    // Case 1. this seq no. = expected seq no.
                    // In-order segments -> enqueue to the target queue
//                    Logger.DEBUG(kTag,
//                            "Normal: ExpectedRecvSeqNo[" + queueType + "]=" + this
// .mExpectedSeqNo[queueType]);
                    this.mExpectedSeqNo[queueType]++;
                    this.mQueues[queueType].offerLast(segment);
                    this.mQueueLengths[queueType]++;
                    segmentEnqueued = true;
//                    Logger.DEBUG(kTag,
//                            "Normal done: ExpectedRecvSeqNo[" + queueType + "]=" + this
// .mExpectedSeqNo[queueType]);
                } else if (seqNo < this.mExpectedSeqNo[queueType]) {
                    // Case 2. this seq no. < expected seq no.
                    // Duplicated segments -> ignore
//                    Logger.DEBUG(kTag,
//                            "Duplicated: ExpectedRecvSeqNo[" + queueType + "]=" + this
// .mExpectedSeqNo[queueType]);
                    return;
                } else {
                    // Case 3. this seq no. > expected seq no.
                    // Out-of-order segments -> insert at the proper position of pending queue
                    ListIterator it = this.mPendingQueue[queueType].listIterator();
                    while (it.hasNext()) {
                        Segment walker = (Segment) it.next();
                        if (walker.getSeqNo() > seqNo) break;
                    }
                    it.add(segment);
//                    Logger.DEBUG(kTag, "Pending Queue: (" + queueType + ") incoming=" + seqNo +
//                            " " + "/" + " " + "expected_next=" + this.mExpectedSeqNo[queueType]);
                    isPending = true;
                }

                // Update Receive-Pending UI
                if (queueType == kSQRecvControl || queueType == kSQRecvData) {
                    if (isPending) {
                        MainActivity.setPendingState(true);
                    } else {
                        MainActivity.setPendingState(false);
                        MainActivity.setNextSeqNo(this.mExpectedSeqNo[kSQRecvControl],
                                this.mExpectedSeqNo[kSQRecvData]);
                    }
                }

                ListIterator it = this.mPendingQueue[queueType].listIterator();
                while (it.hasNext()) {
                    Segment walker = (Segment) it.next();
                    int expected_seq_no = this.mExpectedSeqNo[queueType];
//                    Logger.DEBUG(kTag,
//                            "Pending Queue -> Continuous Queue?: expected=" + expected_seq_no + " " + "!=" + " " + walker.getSeqNo() + "?");
                    if (expected_seq_no > walker.getSeqNo()) {
                        // Obsolete segment in pending queue
                        it.remove();
                    } else if(expected_seq_no < walker.getSeqNo()) {
                        // Future segment in pending queue
                        break;
                    } else {
                        // Continuous segment in pending queue
                        this.mQueues[queueType].offerLast(walker);
                        this.mQueueLengths[queueType]++;
                        this.mExpectedSeqNo[queueType]++;
                        segmentEnqueued = true;
                        it.remove();
                    }
                }
            }
        }

        if (segmentEnqueued) {
            this.wakeUpDequeueWaiting(dequeueType);
        }

        this.checkReceivingDone();
    }

    public Segment dequeue(int dequeueType) {
        assert (dequeueType < kNumDeq);
        synchronized (this.mDequeueCond[dequeueType]) {
            // If queue is empty, wait until some segment is enqueued
            boolean isWaitRequired = false;
            switch (dequeueType) {
                case kDeqSendControlData:
                    isWaitRequired =
                            ((this.mQueueLengths[kSQSendControl] == 0) && (this.mQueueLengths[kSQSendData] == 0));
                    break;
                case kDeqRecvControl:
                    isWaitRequired = (this.mQueueLengths[kSQRecvControl] == 0);
                    break;
                case kDeqRecvData:
                    isWaitRequired = (this.mQueueLengths[kSQRecvData] == 0);
                    break;
            }
            if (isWaitRequired) {
                try {
                    this.mDequeueCond[dequeueType].wait();
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }

            // Set target queue type
            int targetQueueType = kSQUnknown;
            switch (dequeueType) {
                case kDeqSendControlData:
                    if (this.mQueueLengths[kSQSendControl] != 0) {
                        // Priority 1. Dequeue send control queue
                        targetQueueType = kSQSendControl;
                    } else if (this.mQueueLengths[kSQSendData] != 0) {
                        // Priority 2. Dequeue send data queue
                        targetQueueType = kSQSendData;
                    } else {
                        return null;
                    }
                    break;
                case kDeqRecvControl:
                    targetQueueType = kSQRecvControl;
                    break;
                case kDeqRecvData:
                    targetQueueType = kSQRecvData;
                    break;
                default:
                    Logger.ERR(kTag,
                            "Dequeue failed: invalid dequeue type (Dequeue=" + dequeueType + ")");
                    return null;
            }

            // Check queue type
            if (targetQueueType >= kNumSQ) {
                Logger.ERR(kTag, "Dequeue failed: invalid queue type (Dequeue=" + dequeueType +
                        ")");
                return null;
            }

            // Dequeue from queue
            synchronized (this.mQueues[targetQueueType]) {
                // Check the dequeued segment
                Segment segmentDequeued = (Segment) this.mQueues[targetQueueType].pollFirst();
                if (segmentDequeued == null) {
//                    Logger.DEBUG(kTag, "Dequeue interrupted: empty queue (queue=" +
//                            targetQueueType + ", dequeue=" + dequeueType + ")");
                    return null;
                }
                mQueueLengths[targetQueueType]--;
                return segmentDequeued;
            }
        }
    }

    public Segment popFreeSegment() {
        Segment ret;
        synchronized (this.mFreeSegments) {
            if (this.mFreeSegmentsSize == 0) {
                ret = new Segment();
            } else {
                ret = this.mFreeSegments.pop();
                this.mFreeSegmentsSize--;
            }

            if (ret == null) throw new AssertionError();
        }
        return ret;
    }

    public void addFreeSegmentToList(Segment seg) {
        synchronized (this.mFreeSegments) {
            this.mFreeSegments.push(seg);
            this.mFreeSegmentsSize++;

            if (this.mFreeSegmentsSize > kSegFreeThreshold) {
                deallocateAllTheFreeSegments(kSegFreeThreshold / 2);
            }
        }
    }

    private void deallocateAllTheFreeSegments(int threshold) {
        while (this.mFreeSegmentsSize > threshold) {
            this.mFreeSegments.pop();
            this.mFreeSegmentsSize--;
        }
    }

    public void addFailedSegmentToList(Segment seg) {
        synchronized (mFailedSendingQueue) {
            mFailedSendingQueue.offerLast(seg);
            this.wakeUpDequeueWaiting(kDeqSendControlData);
        }
    }

    public Segment popFailedSegment() {
        Segment ret;
        synchronized (mFailedSendingQueue) {
            ret = mFailedSendingQueue.pollFirst();
        }

        return ret;
    }

    void checkAndRequestRetransmitMissingSegment() {
        this.__checkAndRequestRetransmitMissingSegment(kSQRecvControl);
        this.__checkAndRequestRetransmitMissingSegment(kSQRecvData);
    }

    private void __checkAndRequestRetransmitMissingSegment(int queueType) {
        synchronized (this.mQueues[queueType]) {
            int nextSeqNo;
            if (queueType == kSQRecvControl) {
                nextSeqNo = this.getExpectedSeqNo(kSQRecvControl);
            } else if (queueType == kSQRecvData) {
                nextSeqNo = this.getExpectedSeqNo(kSQRecvData);
            } else {
                return;
            }

            Logger.DEBUG(kTag,
                    "Check Retransmit: QueueType=" + queueType + " NextSeqNo=" + nextSeqNo);

            // Check queue length
            if (this.mPendingQueue[queueType].size() == 0) {
                return;
            }

            // Find missing segments and send retransmission request on them
            for (int i = 0; i < this.mPendingQueue[queueType].size(); i++) {
                Segment segment = (Segment) this.mPendingQueue[queueType].get(i);
                int seqNo = segment.getSeqNo();
                if (seqNo > nextSeqNo) {
                    // Missing segments detected (nextSeqNo ~ seqNo-1)
                    ControlMessageSender controlMsgSender =
                            Core.singleton().getControlMessageSender();
                    int seg_type = (queueType == kSQRecvControl) ? kSNControl : kSNData;
                    controlMsgSender.sendRequestRetransmit(seg_type, nextSeqNo, seqNo - 1);
                    nextSeqNo = seqNo + 1;
                } else if (seqNo == nextSeqNo) {
                    // Continuous segments
                    nextSeqNo = seqNo + 1;
                } else {
                    // Continuous segments
                }
            }
        }
    }

    private int mPrevLastSeqNoControl = 0;
    private int mPrevLastSeqNoData = 0;
    private Date mPrevLastChangeTS = null;
    private final long kUnchangedThresholdFirstMS = 1000;
    private final long kUnchangedThresholdAgainMS = 5000;
    private long mUnchangedThresholdMS = kUnchangedThresholdFirstMS;

    private void checkAndNotifyQueueStatus() {
        if (Core.singleton().isAdaptersSwitching()) {
            return;
        }

        int lastSeqNoControl = this.getLastSeqNo(kSQRecvControl);
        int lastSeqNoData = this.getLastSeqNo(kSQRecvData);
        Logger.DEBUG(kTag,
                "checkAndNotifyQueueStatus: control=" + lastSeqNoControl + " data=" + lastSeqNoData);
        if (lastSeqNoControl < 0 || lastSeqNoData < 0) {
            return;
        }

        if (this.mPrevLastSeqNoControl != lastSeqNoControl || this.mPrevLastSeqNoData != lastSeqNoData) {
            this.mPrevLastSeqNoControl = lastSeqNoControl;
            this.mPrevLastSeqNoData = lastSeqNoData;

            ControlMessageSender controlMsgSender = Core.singleton().getControlMessageSender();
            controlMsgSender.sendRequestQueueStatusMessage(lastSeqNoControl, lastSeqNoData);

            this.mUnchangedThresholdMS = kUnchangedThresholdFirstMS;
            this.mPrevLastChangeTS = new Date();
        } else {
            if (this.mPrevLastChangeTS != null) {
                Date presentTS = new Date();
                long unchangedMS = presentTS.getTime() - this.mPrevLastChangeTS.getTime();
                if (unchangedMS > this.mUnchangedThresholdMS) {
                    this.checkAndRequestRetransmitMissingSegment();
                    this.mPrevLastChangeTS = new Date();
                    this.mUnchangedThresholdMS = kUnchangedThresholdAgainMS;
                }
            }
        }
    }

    class QueueWatcherThread extends Thread {
        private final long kSleepMS = 1000;

        @SuppressWarnings("InfiniteLoopStatement")
        @Override
        public void run() {
            while (true) {
                try {
                    checkAndNotifyQueueStatus();
                    Thread.sleep(kSleepMS);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        }
    }
}