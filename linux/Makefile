BIN_DIR=./bin

CORE_SRC_DIR=./core/src
CORE_OBJ_DIR=./core/obj

COMMON_SRC_DIR=./common/src
COMMON_OBJ_DIR=./common/obj

DEV_SRC_DIR=./device/src
DEV_OBJ_DIR=./device/obj

POLICY_SRC_DIR=./policy/src
POLICY_OBJ_DIR=./policy/obj

CC=g++
FLAG=-std=c++11 -DLOG_LEVEL=0 -g
LIB=-lbluetooth -lpthread
MKDIR_P=mkdir -p

$(CORE_OBJ_DIR)/%.o: $(CORE_SRC_DIR)/%.cpp
	$(CC) -o $@ -c $< $(FLAG) $(LIB)

_CORE_OBJ=API.o Core.o Stats.o ProtocolManager.o SegmentManager.o \
					EventLogging.o NetworkSwitcher.o NetworkMonitorThread.o MonitorLoggingThread.o \
					Device.o P2PServer.o ServerAdapter.o \
					ServerSocket.o  \
					ControlMessageReceiver.o ControlMessageSender.o SenderThread.o ReceiverThread.o
CORE_OBJ=$(patsubst %, $(CORE_OBJ_DIR)/%, $(_CORE_OBJ))

$(POLICY_OBJ_DIR)/%.o: $(POLICY_SRC_DIR)/%.cpp
	$(CC) -o $@ -c $< $(FLAG) $(LIB)

_POLICY_OBJ=NMPolicyManual.o NMPolicyAppAware.o AppAwareTPT.o EnergyPredictor.o NMPolicyAppManual.o NMPolicyBtOnly.o NMPolicyWfdOnly.o
POLICY_OBJ=$(patsubst %, $(POLICY_OBJ_DIR)/%, $(_POLICY_OBJ))

$(COMMON_OBJ_DIR)/%.o: $(COMMON_SRC_DIR)/%.cpp
	$(CC) -o $@ -c $< $(FLAG) $(LIB)

_COMMON_OBJ=ChildProcess.o DebugLog.o Counter.o
COMMON_OBJ=$(patsubst %, $(COMMON_OBJ_DIR)/%, $(_COMMON_OBJ))

$(DEV_OBJ_DIR)/%.o: $(DEV_SRC_DIR)/%.cpp
	$(CC) -o $@ -c $< $(FLAG) $(LIB) 
_DEV_OBJ=BtServerAdapter.o BtDevice.o BtP2PServer.o RfcommServerSocket.o \
          WfdServerAdapter.o TcpServerSocket.o WfdDevice.o WfdP2PServer.o \
					EthServerAdapter.o \
					NetworkInitializer.o CommandRfkill.o CommandIwpriv.o CommandWpaCli.o
DEV_OBJ=$(patsubst %, $(DEV_OBJ_DIR)/%, $(_DEV_OBJ))

all : outdir exp

outdir :
	$(MKDIR_P) $(BIN_DIR) $(CORE_OBJ_DIR) $(POLICY_OBJ_DIR) $(COMMON_OBJ_DIR) $(DEV_OBJ_DIR)

exp: app_aware_runner bt_only_runner wfd_only_runner app_manual_runner

app_aware_runner: exp/app_aware_runner.cpp exp/TraceRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

app_manual_runner: exp/app_manual_runner.cpp exp/TraceRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

bt_only_runner: exp/bt_only_runner.cpp exp/TraceRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

wfd_only_runner: exp/wfd_only_runner.cpp exp/TraceRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

tests: comm_init_test wpa_cli_test udhcpd_test network_tester idle_tester switch_tester

wpa_cli_test: tests/wpa_cli.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB) 

udhcpd_test: tests/udhcpd.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB) 

comm_init_test: tests/comm_init.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB) 

network_tester: tests/network_tester.cpp tests/TestRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

idle_tester: tests/idle_tester.cpp tests/IdleTestRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

switch_tester: tests/switch_tester.cpp tests/SwitchTestRunner.cpp $(CORE_OBJ) $(POLICY_OBJ) $(COMMON_OBJ) $(DEV_OBJ)
	$(CC) -o $(BIN_DIR)/$@ $^ $(FLAG) $(LIB)

clean:
	rm -rf ./obj/*.o ./bin/* ./device/obj/*.o || true