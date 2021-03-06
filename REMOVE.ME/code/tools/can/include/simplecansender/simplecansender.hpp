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

#ifndef SIMPLECANSENDER_SIMPLECANSENDER_HPP_
#define SIMPLECANSENDER_SIMPLECANSENDER_HPP_

#include <memory>

#include <opendavinci/odcore/base/FIFOQueue.h>
#include <opendavinci/odcore/base/module/TimeTriggeredConferenceClientModule.h>
#include <odcantools/GenericCANMessageListener.h>
#include <reverefh16mapping/GeneratedHeaders_reverefh16mapping.h>

namespace automotive {
    class GenericCANMessage;
}
namespace automotive {
    namespace odcantools {
        class CANDevice;
    }
}
namespace odtools {
    namespace recorder {
        class Recorder;
    }
}

namespace opendlv {
    namespace proxy {
        namespace can {
            class CanMessageDataStore;
        }
    }
}

namespace opendlv {
    namespace tools {
        namespace can {
            namespace simplecansender {
                /**
                 * This class opens a PEAK CAN device to send control commands to the vehicle.
                 */
                class SimpleCANSender
                : public odcore::base::module::TimeTriggeredConferenceClientModule,
                  public automotive::odcantools::GenericCANMessageListener {
                 public:
                  SimpleCANSender(int32_t const &, char **);
                  SimpleCANSender(SimpleCANSender const &) = delete;
                  SimpleCANSender &operator=(SimpleCANSender const &) = delete;
                  virtual ~SimpleCANSender();

                  virtual void nextGenericCANMessage(const automotive::GenericCANMessage &gcm);

                 private:
                  void setUp();
                  void tearDown();
                  odcore::data::dmcp::ModuleExitCodeMessage::ModuleExitCode body();

                 private:
                  odcore::base::FIFOQueue m_fifo;
                  std::shared_ptr<automotive::odcantools::CANDevice> m_device;
                  std::unique_ptr<opendlv::proxy::can::CanMessageDataStore> m_canMessageDataStore;
                  canmapping::CanMapping m_revereFh16CanMessageMapping;

                 private:
                    void disableCmds();
                    automotive::GenericCANMessage m_lastMessage;
                };
            } // simplecansender
        } // can
    } // tools
} // opendlv

#endif
