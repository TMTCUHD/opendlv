/**
 * Copyright (C) 2016 Chalmers REVERE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include <opendavinci/odcore/data/Container.h>
#include <opendavinci/odcore/data/TimeStamp.h>
#include <opendavinci/odtools/recorder/Recorder.h>
#include <opendavinci/odcore/reflection/Message.h>
#include <opendavinci/odcore/reflection/MessageFromVisitableVisitor.h>
#include <opendavinci/odcore/strings/StringToolbox.h>
#include <odcantools/MessageToCANDataStore.h>
#include <odcantools/CANDevice.h>
#include <automotivedata/generated/automotive/GenericCANMessage.h>

#include <opendlvdata/generated/opendlv/proxy/reverefh16/AccelerationRequest.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/BrakeRequest.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/SteeringRequest.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/VehicleState.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/Wheels.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/Axles.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/ManualControl.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/Propulsion.h>
#include <opendlvdata/generated/opendlv/proxy/reverefh16/Steering.h>

#include "can/can.hpp"
#include "can/canmessagedatastore.hpp"

namespace opendlv {
namespace proxy {
namespace can {

using namespace std;
using namespace odcore::base;
using namespace odcore::data;
using namespace odcore::reflection;
using namespace odtools::recorder;
using namespace automotive::odcantools;


Can::Can(int32_t const &a_argc, char **a_argv)
    : odcore::base::module::TimeTriggeredConferenceClientModule(
      a_argc, a_argv, "proxy-can")
    , GenericCANMessageListener()
    , m_fifoGenericCanMessages()
    , m_recorderGenericCanMessages()
    , m_fifoMappedCanMessages()
    , m_recorderMappedCanMessages()
    , m_device()
    , m_canMessageDataStore()
    , m_revereFh16CanMessageMapping()
    , m_startOfRecording()
    , m_ASCfile()
    , m_mapOfCSVFiles()
    , m_mapOfCSVVisitors()
{
}

Can::~Can()
{
}


void Can::setUp()
{
  string const DEVICE_NODE =
  getKeyValueConfiguration().getValue<string>("proxy-can.devicenode");

  // Try to open CAN device and register this instance as receiver for
  // GenericCANMessages.
  m_device = shared_ptr<CANDevice>(new CANDevice(DEVICE_NODE, *this));

  // If the device could be successfully opened, create a recording file with a
  // dump of the data.
  if (m_device.get() && m_device->isOpen()) {
    cout << "Successfully opened CAN device '" << DEVICE_NODE << "'." << endl;

    // Automatically record all received raw CAN messages.
    m_startOfRecording = TimeStamp();
    vector<string> timeStampNoSpace = odcore::strings::StringToolbox::split(m_startOfRecording.getYYYYMMDD_HHMMSS(), ' ');
    stringstream strTimeStampNoSpace;
    strTimeStampNoSpace << timeStampNoSpace.at(0);
    if (timeStampNoSpace.size() == 2) {
      strTimeStampNoSpace << "_" << timeStampNoSpace.at(1);
    }
    const string TIMESTAMP = strTimeStampNoSpace.str();

    const bool RECORD_GCM =
    (getKeyValueConfiguration().getValue<int>("proxy-can.record_gcm") == 1);
    if (RECORD_GCM) {
      setUpRecordingGenericCANMessage(TIMESTAMP);
    }

    const bool RECORD_MAPPED =
    (getKeyValueConfiguration().getValue<int>("proxy-can.record_mapped_data") == 1);
    if (RECORD_MAPPED) {
      setUpRecordingMappedGenericCANMessage(TIMESTAMP);
    }

    // Create a data sink that automatically receives all Containers and
    // selectively relays them based on the Container type to the CAN device.
    m_canMessageDataStore =
    unique_ptr<CanMessageDataStore>(new CanMessageDataStore(m_device));
    addDataStoreFor(*m_canMessageDataStore);

    // Start the wrapped CAN device to receive CAN messages concurrently.
    m_device->start();
  }
}


void Can::setUpRecordingGenericCANMessage(const string &timeStampForFileName)
{
  // URL for storing containers containing GenericCANMessages.
  stringstream recordingUrl;
  recordingUrl << "file://"
               << "CID-" << getCID() << "_"
               << "can_gcm_" << timeStampForFileName << ".rec";
  // Size of memory segments (not needed for recording GenericCANMessages).
  const uint32_t MEMORY_SEGMENT_SIZE = 0;
  // Number of memory segments (not needed for recording
  // GenericCANMessages).
  const uint32_t NUMBER_OF_SEGMENTS = 0;
  // Run recorder in asynchronous mode to allow real-time recording in
  // background.
  const bool THREADING = true;
  // Dump shared images and shared data (not needed for recording
  // GenericCANMessages)?
  const bool DUMP_SHARED_DATA = false;

  // Create a recorder instance.
  m_recorderGenericCanMessages = unique_ptr<Recorder>(new Recorder(recordingUrl.str(),
  MEMORY_SEGMENT_SIZE, NUMBER_OF_SEGMENTS, THREADING, DUMP_SHARED_DATA));

  // Create a file to dump CAN data in ASC format.
  stringstream fileName;
  fileName << "CID-" << getCID() << "_"
           << "can_data_" << timeStampForFileName << ".asc";
  m_ASCfile = shared_ptr<fstream>(new fstream(fileName.str(), ios::out));
  (*m_ASCfile) << "Time (s) Channel ID RX/TX d Length Byte 1 Byte 2 Byte 3 Byte 4 Byte 5 Byte 6 Byte 7 Byte 8" << endl;
}


void Can::setUpRecordingMappedGenericCANMessage(const string &timeStampForFileName)
{
  // URL for storing containers containing GenericCANMessages.
  stringstream recordingUrl;
  recordingUrl << "file://"
               << "CID-" << getCID() << "_"
               << "can_mapped_data_" << timeStampForFileName << ".rec";
  // Size of memory segments (not needed for recording GenericCANMessages).
  const uint32_t MEMORY_SEGMENT_SIZE = 0;
  // Number of memory segments (not needed for recording
  // GenericCANMessages).
  const uint32_t NUMBER_OF_SEGMENTS = 0;
  // Run recorder in asynchronous mode to allow real-time recording in
  // background.
  const bool THREADING = true;
  // Dump shared images and shared data (not needed for recording
  // mapped containers)?
  const bool DUMP_SHARED_DATA = false;

  // Create a recorder instance.
  m_recorderMappedCanMessages = unique_ptr<Recorder>(new Recorder(recordingUrl.str(),
  MEMORY_SEGMENT_SIZE, NUMBER_OF_SEGMENTS, THREADING, DUMP_SHARED_DATA));

  {
    {
      stringstream fileName;
      fileName << "CID-" << getCID() << "_"
               << "can_mapped_data_id-" << opendlv::proxy::reverefh16::ManualControl::ID() << "_" << timeStampForFileName << ".csv";

      // Create map of CSV transformers.
      fstream *f = new fstream(fileName.str(), ios::out);

      // Log ManualControl.
      m_mapOfCSVFiles[opendlv::proxy::reverefh16::ManualControl::ID()] = shared_ptr<fstream>(f);
      m_mapOfCSVVisitors[opendlv::proxy::reverefh16::ManualControl::ID()] =
      shared_ptr<CSVFromVisitableVisitor>(new CSVFromVisitableVisitor(*f));
    }

    {
      stringstream fileName;
      fileName << "CID-" << getCID() << "_"
               << "can_mapped_data_id-" << opendlv::proxy::reverefh16::Steering::ID() << "_" << timeStampForFileName << ".csv";

      // Create map of CSV transformers.
      fstream *f = new fstream(fileName.str(), ios::out);

      // Log Steering.
      f = new fstream(fileName.str(), ios::out);
      m_mapOfCSVFiles[opendlv::proxy::reverefh16::Steering::ID()] = shared_ptr<fstream>(f);
      m_mapOfCSVVisitors[opendlv::proxy::reverefh16::Steering::ID()] =
      shared_ptr<CSVFromVisitableVisitor>(new CSVFromVisitableVisitor(*f));
    }

    {
      stringstream fileName;
      fileName << "CID-" << getCID() << "_"
               << "can_mapped_data_id-" << opendlv::proxy::reverefh16::Propulsion::ID() << "_" << timeStampForFileName << ".csv";

      // Create map of CSV transformers.
      fstream *f = new fstream(fileName.str(), ios::out);

      // Log Propulsion.
      f = new fstream(fileName.str(), ios::out);
      m_mapOfCSVFiles[opendlv::proxy::reverefh16::Propulsion::ID()] = shared_ptr<fstream>(f);
      m_mapOfCSVVisitors[opendlv::proxy::reverefh16::Propulsion::ID()] =
      shared_ptr<CSVFromVisitableVisitor>(new CSVFromVisitableVisitor(*f));
    }
  }
}


void Can::disableCANRequests()
{
  opendlv::proxy::reverefh16::BrakeRequest brakeRequest;
  brakeRequest.setEnableRequest(false);
  brakeRequest.setBrake(0.0);
  Container brakeRequestContainer(brakeRequest);

  canmapping::opendlv::proxy::reverefh16::BrakeRequest brakeRequestMapping;
  automotive::GenericCANMessage genericCanMessage = brakeRequestMapping.encode(brakeRequestContainer);
  m_device->write(genericCanMessage);


  opendlv::proxy::reverefh16::AccelerationRequest accelerationRequest;
  accelerationRequest.setEnableRequest(false);
  accelerationRequest.setAccelerationPedalPosition(0.0);
  Container accelerationRequestContainer(accelerationRequest);

  canmapping::opendlv::proxy::reverefh16::AccelerationRequest accelerationRequestMapping;
  genericCanMessage = accelerationRequestMapping.encode(accelerationRequestContainer);
  m_device->write(genericCanMessage);


  opendlv::proxy::reverefh16::SteeringRequest steeringRequest;
  steeringRequest.setEnableRequest(false);
  steeringRequest.setSteeringRoadWheelAngle(0.0);
  steeringRequest.setSteeringDeltaTorque(0.0);
  Container steeringRequestContainer(steeringRequest);

  canmapping::opendlv::proxy::reverefh16::SteeringRequest steeringRequestMapping;
  genericCanMessage = steeringRequestMapping.encode(steeringRequestContainer);
  m_device->write(genericCanMessage);
}


void Can::tearDown()
{
  // Stop the wrapper CAN device.
  if (m_device.get()) {
    m_device->stop();
  }

  // Flush output to CSV files.
  for (auto it = m_mapOfCSVFiles.begin(); it != m_mapOfCSVFiles.end(); it++) {
    it->second->flush();
    it->second->close();
  }

  // Flush output to ASC file.
  if (m_ASCfile.get() != NULL) {
    m_ASCfile->flush();
    m_ASCfile->close();
  }
}


Container Can::addCANTimeStamp(Container &c, const TimeStamp &ts)
{
  Container retVal = c;

  if (c.getDataType() == opendlv::proxy::reverefh16::ManualControl::ID()) {
    opendlv::proxy::reverefh16::ManualControl temp = c.getData<opendlv::proxy::reverefh16::ManualControl>();
    temp.setFromSensor(ts);
    retVal = Container(temp);
  }
  else if (c.getDataType() == opendlv::proxy::reverefh16::VehicleState::ID()) {
    opendlv::proxy::reverefh16::VehicleState temp = c.getData<opendlv::proxy::reverefh16::VehicleState>();
    temp.setFromSensor(ts);
    retVal = Container(temp);
  }
  else if (c.getDataType() == opendlv::proxy::reverefh16::Wheels::ID()) {
    opendlv::proxy::reverefh16::Wheels temp = c.getData<opendlv::proxy::reverefh16::Wheels>();
    temp.setFromSensor(ts);
    retVal = Container(temp);
  }
  else if (c.getDataType() == opendlv::proxy::reverefh16::Axles::ID()) {
    opendlv::proxy::reverefh16::Axles temp = c.getData<opendlv::proxy::reverefh16::Axles>();
    temp.setFromSensor(ts);
    retVal = Container(temp);
  }
  else if (c.getDataType() == opendlv::proxy::reverefh16::Steering::ID()) {
    opendlv::proxy::reverefh16::Steering temp = c.getData<opendlv::proxy::reverefh16::Steering>();
    temp.setFromSensor(ts);
    retVal = Container(temp);
  }
  else if (c.getDataType() == opendlv::proxy::reverefh16::Propulsion::ID()) {
    opendlv::proxy::reverefh16::Propulsion temp = c.getData<opendlv::proxy::reverefh16::Propulsion>();
    temp.setFromSensor(ts);
    retVal = Container(temp);
  }

  return retVal;
}


void Can::dumpASCData(const automotive::GenericCANMessage &gcm)
{
  if (m_ASCfile.get() != NULL) {
    TimeStamp now;
    TimeStamp ts = (now - m_startOfRecording);
    (*m_ASCfile) << (ts.getSeconds() + (static_cast<double>(ts.getFractionalMicroseconds()) / (1000.0 * 1000.0)))
                 << " 1"
                 << " " << gcm.getIdentifier()
                 << " Rx"
                 << " d"
                 << " " << static_cast<uint32_t>(gcm.getLength());
    uint64_t data = gcm.getData();
    for (uint8_t i = 0; i < gcm.getLength(); i++) {
      (*m_ASCfile) << " " << hex << (data & 0xFF);
      data = data >> 8;
    }
    (*m_ASCfile) << endl;
  }
}


void Can::nextGenericCANMessage(const automotive::GenericCANMessage &gcm)
{
  static int counter = 0;
  const int CAN_MESSAGE_COUNTER_WHEN_TO_SEND = 1;
  const int CAN_MESSAGES_TO_IGNORE = 10;
  counter++;
  if (counter > CAN_MESSAGES_TO_IGNORE) {
    counter = 0;
  }

  // Log raw CAN data in ASC format.
  dumpASCData(gcm);

  // Map CAN message to high-level data structures as defined in the ODVD file.
  vector<Container> result = m_revereFh16CanMessageMapping.mapNext(gcm);

  for (auto c : result) {
    // Add CAN device driver time stamp to message.
    Container cTimeStamped = addCANTimeStamp(c, gcm.getDriverTimeStamp());

    // Enqueue mapped container for direct recording.
    if (m_recorderMappedCanMessages.get()) {
      cTimeStamped.setReceivedTimeStamp(gcm.getDriverTimeStamp());
      m_fifoMappedCanMessages.add(cTimeStamped);
    }

    // Send container to conference only every tenth message.
    {
      if (counter == CAN_MESSAGE_COUNTER_WHEN_TO_SEND) {
        getConference().send(cTimeStamped);
      }
    }
  }

  // Enqueue CAN message wrapped as Container to be recorded if we have a valid
  // recorder.
  if (m_recorderGenericCanMessages.get()) {
    Container c(gcm);
    m_fifoGenericCanMessages.add(c);
  }
}


void Can::handleBeacons()
{
  // Send messages to the beacon relays
  if (m_canMessageDataStore->IsAutonomousEnabled() && !m_canMessageDataStore->IsOverridden()) {
    // Activate green
    string deviceIdGreen = "0";
    uint32_t relayIndexGreen = 0;
    bool relayValueGreen = false;
    opendlv::proxy::RelayRequest relayRequestGreen(relayValueGreen, relayIndexGreen, deviceIdGreen);
    Container objectContainerGreen(relayRequestGreen);
    getConference().send(objectContainerGreen);


    string deviceIdRed = "0";
    uint32_t relayIndexRed = 1;
    bool relayValueRed = true;
    opendlv::proxy::RelayRequest relayRequestRed(relayValueRed, relayIndexRed, deviceIdRed);
    Container objectContainerRed(relayRequestRed);
    getConference().send(objectContainerRed);
  }
  else {
    // Activate red
    string deviceIdGreen = "0";
    uint32_t relayIndexGreen = 0;
    bool relayValueGreen = true;
    opendlv::proxy::RelayRequest relayRequestGreen(relayValueGreen, relayIndexGreen, deviceIdGreen);
    Container objectContainerGreen(relayRequestGreen);
    getConference().send(objectContainerGreen);


    string deviceIdRed = "0";
    uint32_t relayIndexRed = 1;
    bool relayValueRed = false;
    opendlv::proxy::RelayRequest relayRequestRed(relayValueRed, relayIndexRed, deviceIdRed);
    Container objectContainerRed(relayRequestRed);
    getConference().send(objectContainerRed);
  }
}


void Can::dumpCSVData(Container &c)
{
  // Add time stamps for CSV output.
  const uint64_t receivedFromCAN = c.getReceivedTimeStamp().toMicroseconds();

  shared_ptr<Field<uint64_t>> m_receivedTS_ptr = shared_ptr<Field<uint64_t>>(new Field<uint64_t>(receivedFromCAN));
  m_receivedTS_ptr->setFieldIdentifier(0);
  m_receivedTS_ptr->setLongFieldName("Received_TimeStamp");
  m_receivedTS_ptr->setShortFieldName("Received_TimeStamp");
  m_receivedTS_ptr->setFieldDataType(reflection::AbstractField::UINT64_T);
  m_receivedTS_ptr->setSize(sizeof(uint64_t));

  if ((m_mapOfCSVFiles.count(c.getDataType()) == 1) &&
  (m_mapOfCSVVisitors.count(c.getDataType()) == 1)) {
    // We have a CSV file and a transformation available.
    if (c.getDataType() == opendlv::proxy::reverefh16::ManualControl::ID()) {
      opendlv::proxy::reverefh16::ManualControl temp = c.getData<opendlv::proxy::reverefh16::ManualControl>();
      MessageFromVisitableVisitor mfvv;
      temp.accept(mfvv);
      Message m = mfvv.getMessage();
      m.addField(m_receivedTS_ptr);
      m.accept(*m_mapOfCSVVisitors[c.getDataType()]);
    }
    if (c.getDataType() == opendlv::proxy::reverefh16::Steering::ID()) {
      opendlv::proxy::reverefh16::Steering temp = c.getData<opendlv::proxy::reverefh16::Steering>();
      MessageFromVisitableVisitor mfvv;
      temp.accept(mfvv);
      Message m = mfvv.getMessage();
      m.addField(m_receivedTS_ptr);
      m.accept(*m_mapOfCSVVisitors[c.getDataType()]);
    }
    if (c.getDataType() == opendlv::proxy::reverefh16::Propulsion::ID()) {
      opendlv::proxy::reverefh16::Propulsion temp = c.getData<opendlv::proxy::reverefh16::Propulsion>();
      MessageFromVisitableVisitor mfvv;
      temp.accept(mfvv);
      Message m = mfvv.getMessage();
      m.addField(m_receivedTS_ptr);
      m.accept(*m_mapOfCSVVisitors[c.getDataType()]);
    }
  }
}


odcore::data::dmcp::ModuleExitCodeMessage::ModuleExitCode Can::body()
{
  while (getModuleStateAndWaitForRemainingTimeInTimeslice() ==
  odcore::data::dmcp::ModuleStateMessage::RUNNING) {
    // Record GenericCANMessages.
    if (m_recorderGenericCanMessages.get()) {
      const uint32_t ENTRIES = m_fifoGenericCanMessages.getSize();
      for (uint32_t i = 0; i < ENTRIES; i++) {
        Container c = m_fifoGenericCanMessages.leave();

        // Store container to dump file.
        m_recorderGenericCanMessages->store(c);
      }
    }

    // Record mapped messages from GenericCANMessages.
    if (m_recorderMappedCanMessages.get()) {
      const uint32_t ENTRIES = m_fifoMappedCanMessages.getSize();
      for (uint32_t i = 0; i < ENTRIES; i++) {
        Container c = m_fifoMappedCanMessages.leave();

        // Store container to dump file.
        m_recorderMappedCanMessages->store(c);

        // Transform container to CSV file.
        dumpCSVData(c);
      }
    }

    // Indicate with lamps the truck's status.
    handleBeacons();
  }
  return odcore::data::dmcp::ModuleExitCodeMessage::OKAY;
}

} // can
} // proxy
} // opendlv
