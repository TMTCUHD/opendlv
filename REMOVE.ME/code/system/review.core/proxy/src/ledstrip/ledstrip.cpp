/**
 * Copyright (C) 2015 Chalmers REVERE
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

#define _USE_MATH_DEFINES

#include <ctype.h>
#include <cstring>
#include <cmath>
#include <iostream>

#include "opendavinci/odcore/base/KeyValueConfiguration.h"
#include "opendavinci/odcore/data/Container.h"
#include "opendavinci/odcore/data/TimeStamp.h"

#include "opendlvdata/GeneratedHeaders_opendlvdata.h"

#include "ledstrip/ledstrip.hpp"
#include "ledstrip/device.hpp"

namespace opendlv {
namespace proxy {
namespace ledstrip {


using namespace std;

// We add some of OpenDaVINCI's namespaces for the sake of readability.
using namespace odcore;
using namespace odcore::wrapper;

/**
  * Constructor.
  */
Ledstrip::Ledstrip(int32_t const &a_argc, char **a_argv)
    : DataTriggeredConferenceClientModule(
      a_argc, a_argv, "proxy-ledstrip")
    , m_device(),
    m_angle(0.0f),
    m_R(0),
    m_G(255),
    m_B(0),
    m_timeStamp()
{
}

Ledstrip::~Ledstrip()
{
}

// This method will do the main data processing job.
odcore::data::dmcp::ModuleExitCodeMessage::ModuleExitCode Ledstrip::body()
{
    const string SERIAL_PORT = "/dev/ttyUSB0";
    //const string SERIAL_PORT = "/dev/ttyACM0"; // this is for the Arduino Uno with spare LED strip
    const uint32_t BAUD_RATE = 115200;
    const float pi=M_PI;
    
    std::shared_ptr<SerialPort> serial;
    try {
        serial = std::shared_ptr<SerialPort>(SerialPortFactory::createSerialPort(SERIAL_PORT, BAUD_RATE));
    }
    catch(string &exception) {
        cerr << "Serial port could not be created: " << exception << endl;
        return odcore::data::dmcp::ModuleExitCodeMessage::OKAY;
    }

    // variable for debugging purposes
    //const float increment=0.05;
    // boolean flags for debugging purposes
    //bool test=false, sign=false;

    uint8_t focus=1;

    while (getModuleStateAndWaitForRemainingTimeInTimeslice() == odcore::data::dmcp::ModuleStateMessage::RUNNING) {
    
    CLOG2<<endl<<"Start while loop"<<endl;
    
    /*
    // debug code
    if(test) // this is for the Arduino Uno with spare LED strip
    {
        CLOG2<<"angle: "<<m_angle<<" rad"<<std::endl;
        if(sign) {m_angle+=increment;CLOG2<<"adding "<<increment<<endl;}
        else {m_angle-=increment;CLOG2<<"subtracting "<<increment<<endl;}
    }
    */
    
    // capping the max angle at 45 deg = 0.785398 rad
    if(std::fabs(m_angle) >= 0.785398f){
        if(m_angle >= 0.0f){
            m_angle = 0.785398f;
            // for debugging purposes
            //sign=false;
        } else {
            m_angle = -0.785398f;
            // for debugging purposes
            //sign=true;
        }
    }
    
    CLOG2<<"angle: "<<m_angle<<" rad"<<std::endl;

    // Construct Arduino frame to control the LED strip
    std::vector<uint8_t> ledRequest;
    
    /* 
    * "focus" represents the centre of the LED section to be powered. 
    * It is obtained through the transformation of the direction 
    * of the movement angle (capped between [-45,45] deg) 
    * into a percentage value
    */
    focus=round(-m_angle/(45.0f/180.0f*pi)*50.0f)+50.0f;
    
    uint8_t section_size=50; // the size of the LED section to be powered
    uint8_t fade=10; // the number of LEDs to be dimmed at the edge of the lighted section
    uint8_t checksum=0; // checksum resulting from the bitwise xor of the payload bytes
    /*
    if(sign) focus--;
    else focus++;

    if(focus>=146)
    {
        sign=true;  
    }
    else if(focus<1)
    {
        sign=false;
    }*/

    checksum=focus^section_size^fade^m_R^m_G^m_B;
    
    uint8_t R=m_R, G=m_G, B=m_B;
    
    // set the color to green
    if(std::fabs(focus-50)<=10)
    {
        R=0; G=255; B=0;
    }
    else // set the color to orange
    {
        R=200; G=50; B=5;
        // light orange: (180,65,10)
    }

    // Message header: 0xFEDE
    // Message size: 9 bytes
    ledRequest.push_back(0xFE);
    ledRequest.push_back(0xDE);
    ledRequest.push_back(focus);
    ledRequest.push_back(section_size);
    ledRequest.push_back(fade);
    ledRequest.push_back(R);
    ledRequest.push_back(G);
    ledRequest.push_back(B);
    ledRequest.push_back(checksum);
    
    std::string command(ledRequest.begin(),ledRequest.end());
    CLOG2 << "Frame : ";
    for(uint8_t i=0;i<command.size();++i) std::cout<<(uint16_t)command.at(i)<<" ";
    CLOG2 << " : frame size = " << command.size()
          << " {focus: "<< (uint16_t)focus << ", section size: " << (uint16_t)section_size << ", fade: " << (uint16_t)fade
          << ", R: " << (uint16_t)m_R << ", G: " << (uint16_t)m_G << ", B: " << (uint16_t)m_B << "}" << ", checksum: " << (uint16_t)checksum << std::endl;

    serial->send(command);
    std::cout<<"Frame sent."<<std::endl;  
  }
  return odcore::data::dmcp::ModuleExitCodeMessage::OKAY;
}

void Ledstrip::nextContainer(odcore::data::Container &a_container)
{ 
//  odcore::data::TimeStamp now;
  
  if (a_container.getDataType() == opendlv::perception::StimulusDirectionOfMovement::ID()) {
    opendlv::perception::StimulusDirectionOfMovement stimulusDirectionOfMovement = a_container.getData<opendlv::perception::StimulusDirectionOfMovement>();

    opendlv::model::Direction direction = stimulusDirectionOfMovement.getDesiredDirectionOfMovement();

    m_angle = direction.getAzimuth();
    std::cout << "Got an angle!" << std::endl;
    CLOG2 << "Direction azimuth: " << m_angle << std::endl;
  }
  
/*
  // code for testing purposes
    if (a_container.getDataType() == opendlv::proxy::reverefh16::Steering::ID()) {
opendlv::proxy::reverefh16::Steering steering = 
        a_container.getData<opendlv::proxy::reverefh16::Steering>();
 
        m_angle = steering.getRoadwheelangle();
        //CLOG2<<"RoadWheelAngle: "<<m_angle<<std::endl;
    }
*/
// code from competition
//  if (a_container.getDataType() == (opendlv::perception::Object::ID() + 300)){
//    opendlv::perception::Object inputObject = a_container.getData<opendlv::perception::Object>();
//    
//    if(inputObject.getDistance() < 10){
//      m_tooClose = true;
//      m_timeStamp = now;
//    }
//  }

}

void Ledstrip::setUp()
{
//  odcore::base::KeyValueConfiguration kv = getKeyValueConfiguration();

//  std::string const type =
//  kv.getValue<std::string>("proxy-ledstrip.type");
  /*  std::string const port = kv.getValue<std::string>(
   *      "proxy-ledstrip.port");
  */
//  if (type.compare("victor") == 0) {
//    //      m_device = std::unique_ptr<Device>(new VictorDevice());
//  }

  // if (m_device.get() == nullptr) {
  //   std::cerr << "[proxy-ledstrip] No valid device driver defined."
  //             << std::endl;
  // }
}

void Ledstrip::tearDown()
{

}

} // ledstrip
} // proxy
} // opendlv
