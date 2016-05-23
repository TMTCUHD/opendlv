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

#include <cmath>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "opendavinci/odcore/data/Container.h"
#include "opendavinci/odcore/data/TimeStamp.h"

#include "opendlv/data/environment/Point3.h"

#include "geolocation/geolocation.hpp"



namespace opendlv {
namespace sensation {
namespace geolocation {

/**
  * Constructor.
  *
  * @param a_argc Number of command line arguments.
  * @param a_argv Command line arguments.
  */
Geolocation::Geolocation(int32_t const &a_argc, char **a_argv)
    : TimeTriggeredConferenceClientModule(
      a_argc, a_argv, "sensation-geolocation"),
    m_ekf()
{
}

Geolocation::~Geolocation()
{
}

odcore::data::dmcp::ModuleExitCodeMessage::ModuleExitCode Geolocation::body()
{
  opendlv::data::environment::WGS84Coordinate gpsReference;
  bool hasGpsReference = false;

  odcore::data::TimeStamp previousTimestep;
  odcore::data::TimeStamp previousDataTimestamp;
  double timestamp = 0.0;

  Control<double> control;

  SystemModel<double> systemModel;

   State<double> state;
   state.setZero();
  m_ekf.init(state);

  KinematicObservationModel<double> kinematicObservationModel(
      0.0, 0.0,  0.0, 0.0);


  // To dump data structures into a CSV file, you create an output file first.
  // std::ofstream fout("../Exp_data/output.csv");
  bool   saveToFile = true;
  std::ofstream fout_ekfState("./output_ekf.csv");
  fout_ekfState
      << "%HEADER: Output of the Extended Kalman Filter, data format : \n"
      << "%timestamp (s), ground truth: x (m),  y (m), theta (rad), "
      << "theta_dot(rad/s), commands : velocity (m/s) steering angle (rad), "
      << "noisy data: x (m), y (m), theta (rad), theta_dot (rad/s), "
      << "ekf estimation vector: x (m), x_dot (m/s), y (m), y_dot (ms), "
      << "theta (rad), theta_dot(rad/s)  \n"
      << "%t lat long yaw long_vel wheels_angle Z_x Z_y Z_theta Z_theta_dot "
      << "HAS_DATA X_x X_x_dot X_y X_y_dot X_theta X_theta_dot sent_lat "
      << "sent_long sent_alt sent_heading positionConfidence headingConfidence yawRateConfidence"
      << endl;

  cout << getName() << " Geolocation module started " << endl;
  while (getModuleStateAndWaitForRemainingTimeInTimeslice() ==
      odcore::data::dmcp::ModuleStateMessage::RUNNING) {

    auto gpsReadingContainer =
        getKeyValueDataStore().get(opendlv::proxy::GpsReading::ID());
    auto gpsReading = gpsReadingContainer.getData<opendlv::proxy::GpsReading>();

//    std::cout << getName() << "\tLatidude  =  " << gpsReading.getLatitude()
//              << "  Longitude  =  " << gpsReading.getLongitude() << std::endl;

    if (gpsReadingContainer.getReceivedTimeStamp().toMicroseconds() > 0) {
      if (!hasGpsReference) {
        gpsReference = opendlv::data::environment::WGS84Coordinate(
            gpsReading.getLatitude(),
            opendlv::data::environment::WGS84Coordinate::NORTH,
            gpsReading.getLongitude(),
            opendlv::data::environment::WGS84Coordinate::EAST);
        hasGpsReference = true;

        // TODO: now this variable is set only once using the first data, it is
        // necessary to write a function able to reset this value to a new
        // reference frame.
        previousTimestep = gpsReadingContainer.getReceivedTimeStamp();
      }

      odcore::data::TimeStamp thisTimestep;
      odcore::data::TimeStamp duration = thisTimestep - previousTimestep;
      previousTimestep = thisTimestep;


      auto propulsionContainer = getKeyValueDataStore().get(
          opendlv::proxy::reverefh16::Propulsion::ID());
      auto propulsion = propulsionContainer.getData<
          opendlv::proxy::reverefh16::Propulsion>();

      if (propulsionContainer.getReceivedTimeStamp().getSeconds() > 0) {
        control.v() = propulsion.getPropulsionShaftVehicleSpeed();// /3.6;
        // TODO: to m/s --- get the message in si unit
      }


      if (propulsion.getPropulsionShaftVehicleSpeed() < 0.001) {
          control.v() = 0.0;
      // if we don't get any data from the CAN,
      // we try to fill the speed from GPS data
          //auto gpsSpeed = gpsReading.getSpeed();
          //          if (gpsSpeed > 1.0 ){
          //          control.v() = gpsSpeed;}
      }

std::cout << "\n\n the vehicle speed is : " << gpsReading.getSpeed() << endl;
      auto steeringContainer = getKeyValueDataStore().get(
          opendlv::proxy::reverefh16::Steering::ID());
      auto steering = steeringContainer.getData<
          opendlv::proxy::reverefh16::Steering>();

      auto vehicleStateContainer = getKeyValueDataStore().get(
                  opendlv::proxy::reverefh16::VehicleState::ID());
      auto vehicleState = vehicleStateContainer.getData<
              opendlv::proxy::reverefh16::VehicleState>();
      double vehicleYawRate = vehicleState.getYawRate();


      if (steeringContainer.getReceivedTimeStamp().getSeconds() > 0) {
        control.phi() = steering.getSteeringwheelangle()/m_steeringToWheelRatio;
      }

      std::cout   << getName() << "\t" << "timestamp="
        << timestamp << "\t control:  steering.getRoadwheelangle = "
        << steering.getRoadwheelangle()
        << " steering.getSteeringwheelangle "
        << steering.getSteeringwheelangle()
        << "  propulsion.getPropulsionShaftVehicleSpeed "
        << propulsion.getPropulsionShaftVehicleSpeed()
        << std::endl;

      std::cout   << getName() << "\t" << "timestamp="
        << timestamp << "\t original GPS data  "
        << std::setprecision(19) << gpsReading.getLatitude() << "  vel "
        << std::setprecision(19) << gpsReading.getLongitude() << " altitude "
        << gpsReading.getAltitude() << std::endl;

      opendlv::data::environment::WGS84Coordinate currentLocation(
          gpsReading.getLatitude(),
          opendlv::data::environment::WGS84Coordinate::NORTH,
          gpsReading.getLongitude(),
          opendlv::data::environment::WGS84Coordinate::EAST);

      opendlv::data::environment::Point3 currentCartesianLocation =
          gpsReference.transform(currentLocation);

      //kinematic kalman block
      KinematicObservationVector<double> observationVector =
          kinematicObservationModel.h(state);
      observationVector.Z_x() = currentCartesianLocation.getX();
      observationVector.Z_y() = currentCartesianLocation.getY();
      if (gpsReading.getHasHeading()) {
        observationVector.Z_theta() = gpsReading.getNorthHeading();
      }
      else {
        observationVector.Z_theta() = state.theta();
      }
      observationVector.Z_theta_dot() = vehicleYawRate;
      // TODO: Put yaw rate here...


      double deltaTime = duration.toMicroseconds() / 1000000.0;
      systemModel.updateDeltaT(deltaTime);


      state = m_ekf.predict(systemModel, control);
      std::cout   << getName() << "\t"
                  << "timestamp = "  << timestamp
                  << " deltaTime = " << deltaTime
                  << "\t prediction x = " << state.x()
                  << ", y = " << state.y()
                  << ", theta = " << state.theta() << std::endl;


      bool gpsHasData = false;
      if (gpsReadingContainer.getReceivedTimeStamp().toMicroseconds() >
          previousDataTimestamp.toMicroseconds()) {

          std::cout << "observation vector "
                    << observationVector.Z_x() << " "
                    << observationVector.Z_y() << " "
                    << observationVector.Z_theta() << " "
                    << observationVector.Z_theta_dot() << " "
                    << endl;
        state = m_ekf.update(kinematicObservationModel, observationVector);
        previousDataTimestamp = gpsReadingContainer.getReceivedTimeStamp();
        gpsHasData = true;
      }

      timestamp += systemModel.getDeltaT();

      std::cout   << getName() << "\t" << "timestamp="
        << timestamp << "\t hasData=" << gpsHasData << "\t estimation x = " << state.x()
        << ", y = " <<
        state.y() << ", theta = " << state.theta() << std::endl;


//     std::cout << "covariance matrix  Kalman : \n " << m_ekf.getCovariance() << "\n" << std::endl;
//       std::cout << "covariance matrix  systemModel : \n " << systemModel.getCovariance()<< "\n" << std::endl;
//       std::cout << "covariance matrix  kinematicObservationModel : \n " << kinematicObservationModel.getCovariance()<< "\n" << std::endl;

//TODO: convert here x and y to get the position of the rear axle
      opendlv::data::environment::WGS84Coordinate currentWGS84CoordinateEstimation;
      bool ekfSuccess=true;
      // if one of the coordinates is nan, it gives the last GPS available measure
      if (!std::isnan(state.x()) && !std::isnan(state.y()) )
      {
          // Build the proper GPS coordinates to send
          opendlv::data::environment::Point3 currentStateEstimation
                  (state.x() + m_gpsToCoGDisplacement_x,
                   state.y() + m_gpsToCoGDisplacement_y,
                   currentCartesianLocation.getZ() + m_gpsToCoGDisplacement_z);
          currentWGS84CoordinateEstimation = gpsReference.transform(currentStateEstimation);
      }
      else
      {
       currentWGS84CoordinateEstimation = currentLocation;
       std::cout << "NAN - sending gps data instead"<<std::endl;
       filterReset( currentCartesianLocation, gpsReading);
       state = m_ekf.getState();
       ekfSuccess = false;
      }

      double heading = 0;
      if (!std::isnan(state.theta()))
      {
          heading = std::asin(std::sin(state.theta()));  // asin(sin(theta)) make sure that the heading is in [-pi,+pi]
      }
      else
      {
          heading = gpsReading.getNorthHeading();
          ekfSuccess = false;
      }



      double positionConfidence = calculatePositionConfidence(ekfSuccess);

      double headingConfidence = calculateHeadingConfidence(ekfSuccess);

      double speedConfidence = calculateSpeedConfidence(ekfSuccess);

      double yawRate = state.theta_dot();

      double yawRateConfidence = calculateHeadingConfidence(ekfSuccess);

      double longitudinalAcceleration = 0.0;

      double longitudinalAccelerationConfidence = 0.0;


      std::cout   << getName() << "\t" << "timestamp="
        << timestamp << "\t "
        << "\tlat=" << currentWGS84CoordinateEstimation.getLatitude()
        << ", long="
        << currentWGS84CoordinateEstimation.getLongitude()
        << ", theta=" << state.theta() << std::endl;

      // Send the message
      opendlv::sensation::Geolocation geoLocationEstimation(currentWGS84CoordinateEstimation.getLatitude(),
                                                            positionConfidence,
                                                            currentWGS84CoordinateEstimation.getLongitude(),
                                                            positionConfidence,
                                                            gpsReading.getAltitude(),
                                                            heading,
                                                            headingConfidence
                                                            );
      
      odcore::data::Container msg(geoLocationEstimation);
      getConference().send(msg);



      // TODO: This should be sent from another module = sensation/geolocation 
      // should be split in two as discussed previously
      opendlv::model::Cartesian3 velocity(control.v(), 0.0f, 0.0f);
      opendlv::model::Cartesian3 velocityConfidence(speedConfidence, 0.0f, 
          0.0f);

      opendlv::model::Cartesian3 acceleration(longitudinalAcceleration, 0.0f, 
          0.0f);
      opendlv::model::Cartesian3 accelerationConfidence(
          longitudinalAccelerationConfidence, 0.0f, 0.0f);
      
      opendlv::model::Cartesian3 angularVelocity(0.0f, 0.0f, yawRate);
      opendlv::model::Cartesian3 angularVelocityConfidence(0.0f, 0.0f,
          yawRateConfidence);

      opendlv::model::Cartesian3 angularAcceleration(0.0f, 0.0f, 0.0f);
      opendlv::model::Cartesian3 angularAccelerationConfidence(0.0f, 0.0f,
          0.0f);

      int16_t frameId = 0;

      opendlv::model::DynamicState dynamicState(velocity, velocityConfidence,
          acceleration, accelerationConfidence, angularVelocity,
          angularVelocityConfidence, angularAcceleration,
          angularAccelerationConfidence, frameId);

      odcore::data::Container dynamicStateContainer(dynamicState);
      getConference().send(dynamicStateContainer);



      //save data to file
      if (  saveToFile){
      fout_ekfState
          << std::setprecision(19) << timestamp << " "
          << gpsReading.getLatitude() << " "
          << gpsReading.getLongitude() << " "
          << gpsReading.getNorthHeading() <<  " "
          << control.v() << " "
          << control.phi() << " "
          << observationVector.Z_x() << " "
          << observationVector.Z_y() << " "
          << observationVector.Z_theta() << " "
          << observationVector.Z_theta_dot() << " "
          << gpsHasData << " "
          << state.x() << " "
          << state.x_dot() << " "
          << state.y() << " "
          << state.y_dot() << " "
          << state.theta() << " "
          << state.theta_dot() << " "
          << currentWGS84CoordinateEstimation.getLatitude() << " "
          << currentWGS84CoordinateEstimation.getLongitude() << " "
          << gpsReading.getAltitude() << " "
          << heading << " "
          << positionConfidence << " "
          << headingConfidence << " "
          << yawRateConfidence << " "
          << endl;
      }
    }
    else { //TODO: remove this if not necessary anymore
        //std::cout << getName() << "\t"<< " NO DATA " << std::endl;
    }
  }
  return odcore::data::dmcp::ModuleExitCodeMessage::OKAY;
}

void Geolocation::setUp()
{
}

void Geolocation::tearDown()
{
}


double Geolocation::calculatePositionConfidence(bool a_filterSuccess)
{
    if (!a_filterSuccess){
        return -1;
    }
    auto covSR = m_ekf.getCovariance();

    double positionConfidence_x = std::sqrt(covSR(0,0)*covSR(0,0));
    double positionConfidence_y = std::sqrt(covSR(2,2)*covSR(2,2));
    return std::max(std::sqrt(positionConfidence_x),
                    std::sqrt(positionConfidence_y));
}

double Geolocation::calculateHeadingConfidence(bool a_filterSuccess)
{
    if (!a_filterSuccess){
        return -1;
    }
    auto covSR = m_ekf.getCovariance();
    double confidence = std::sqrt(covSR(4,4)*covSR(4,4));
    return std::sqrt(confidence);
}

double Geolocation::calculateHeadingRateConfidence(bool a_filterSuccess)
{
    if (!a_filterSuccess){
        return -1;
    }
    auto covSR = m_ekf.getCovariance();
    double confidence = std::sqrt(covSR(5,5)*covSR(5,5));
    return std::sqrt(confidence);
}

double Geolocation::calculateSpeedConfidence(bool a_filterSuccess)
{
    if (!a_filterSuccess){
        return -1;
    }
return 0.0; // if information is not available
}


void Geolocation::filterReset(opendlv::data::environment::Point3 a_currentCartesianLocation,
                              opendlv::proxy::GpsReading a_currentWGS84Location )
{


State<double> state;
state <<  a_currentCartesianLocation.getX(),
          0.0, //a_currentWGS84Location.getSpeed() * std::cos (a_currentWGS84Location.getNorthHeading()),
          a_currentCartesianLocation.getY(),
          0.0, //a_currentWGS84Location.getSpeed() * std::sin (a_currentWGS84Location.getNorthHeading()),
          a_currentWGS84Location.getNorthHeading(),
          0.0, //(a_currentWGS84Location.getSpeed() / 3.4 ) * std::tan(a_currentWGS84Location.getNorthHeading());

m_ekf.init(state);
m_ekf.setCovariance(Eigen::Matrix<double, 6, 6>::Identity());

}

} // geolocation
} // sensation
} // opendlv
