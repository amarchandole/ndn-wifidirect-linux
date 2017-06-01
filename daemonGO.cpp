/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define SCAN_FIB_INTERVAL 15

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#include <ndn-cxx/lp/tags.hpp>
#include <ndn-cxx/name.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/mgmt/nfd/fib-entry.hpp>
#include <ndn-cxx/mgmt/nfd/rib-entry.hpp>
#include <ndn-cxx/mgmt/nfd/controller.hpp>
#include <ndn-cxx/mgmt/nfd/status-dataset.hpp>
#include <boost/thread.hpp>
#include <unistd.h>

#include <iostream>
#include <string>

class Daemon
{
public:
  Daemon(ndn::Face& face, std::string ownIP, std::string otherIP, uint64_t go, int tempFaceId)
    : m_face(face)
    , m_controller(m_face, m_keyChain)
    , m_baseName("/localhop/wifidirect")
    , m_scheduler(m_face.getIoService())
    , m_ownIP(std::move(ownIP))
    , m_otherIP(std::move(otherIP))
    , m_go(go)
    , m_counter(0)
    , m_tempFaceId(tempFaceId)
  {
    std::cerr << "\n===========================================================" << std::endl;    
    std::cerr << "Daemon constructor:" << std::endl;
    this->printRole(m_go);

    // Register own IP as a prefix as soon as daemon starts running, so that you can receive the data sent to you.
    std::cerr << "\n===========================================================" << std::endl;    
    std::cerr << "Registering prefix:" << std::endl;    
    this->registerPrefix(m_ownIP);

    // Add route to other node's IP as soon as daemon starts running (assume that connection is done at this stage.
    // This allows you to send data to this node, if any application has data for this name.
    std::cerr << "\n===========================================================" << std::endl;    
    std::cerr << "Adding route:" << std::endl;    
    std::string remoteHostIP = "/localhop/wifidirect/"+m_otherIP;
    std::cerr << "Remote host IP is " << remoteHostIP << std::endl;
    this->addRoute(remoteHostIP, m_tempFaceId);

    m_controller.start<ndn::nfd::FaceUpdateCommand>(
      ndn::nfd::ControlParameters()
        .setFlagBit(ndn::nfd::BIT_LOCAL_FIELDS_ENABLED, true),
      std::bind(&Daemon::onEnableLocalFieldsSuccess, this),
      std::bind(&Daemon::onEnableLocalFieldsError, this, _1));

    //if(m_go == 0)
      m_scheduler.scheduleEvent(ndn::time::seconds(SCAN_FIB_INTERVAL), std::bind(&Daemon::sendProbe, this));

    // std::cerr << "\n===========================================================" << std::endl;    
    // std::cerr << "\n\nAdding face:" << std::endl;    
    // this->addFace();
  }

  void
  onEnableLocalFieldsSuccess()
  {
  }

  void
  onEnableLocalFieldsError(const ndn::nfd::ControlResponse& response)
  {
    std::cerr << "Couldn't enable local fields (code: " +
                                std::to_string(response.getCode()) + ", info: " + response.getText() +
                                ")" << std::endl;
  }

  void
  printRole(int go) {
    if(go==0) { 
      std::cerr << "\nROLE: Group Owner" << std::endl;
    }
    else { 
      std::cerr << "\nROLE: Non Group Owner" << std::endl;
    }
  }

  void
  registerPrefix(std::string IP)
  {
    std::cerr << "\nsetInterestFilter (and also set a prefix for the interest to be received):" << std::endl;

    std::stringstream ss;
    ss << m_baseName << "/" << IP;
    std::string prefix = ss.str();
    
    m_face.setInterestFilter(prefix,
                             std::bind(&Daemon::onInterest, this, _2),
                             std::bind([] {
                                 std::cerr << "Prefix registered: " << std::endl;
                               }),
                             [] (const ndn::Name& prefix, const std::string& reason) {
                               std::cerr << "Failed to register prefix: " << reason << std::endl;
                             });
  }

  void
  addFace()
  {
    ndn::nfd::ControlParameters params;
    params.setUri("udp4://111.222.33.44:6363");

    m_controller.start<ndn::nfd::FaceCreateCommand>(
      params, [this] (const ndn::nfd::ControlParameters& result) 
      {
        std::cerr << "Successfully created a face" << std::endl;

      },[&] (const ndn::nfd::ControlResponse& resp) 
      {
         std::cerr << "FAILURE: " << resp.getText() << std::endl;
      });
  }

  //uses prefix to register and the face ID
  void
  addRoute(std::string prefix, uint64_t faceId)
  {
    ndn::nfd::ControlParameters params;
    params.setName(prefix);

    //set the FaceID of the face created towards the WiFi direct interface
    params.setFaceId(faceId);
    params.setExpirationPeriod(ndn::time::seconds(100));

    ndn::nfd::CommandOptions options;
    //options.setPrefix("");

    m_controller.start<ndn::nfd::RibRegisterCommand>
      (params, [&] (const ndn::nfd::ControlParameters&) 
      {
        //std::cerr << "Successfully created a route" << std::endl;
      },[&] (const ndn::nfd::ControlResponse& resp) 
      {
         std::cerr << "FAILURE: " << resp.getText() << std::endl;
      });

    m_controller.fetch<ndn::nfd::ForwarderGeneralStatusDataset>(std::bind(&Daemon::onStatusRetrieved, this, _1),
                                                                std::bind(&Daemon::onStatusTimeout, this));
  }

  void
  onStatusRetrieved(const ndn::nfd::ForwarderStatus& status)
  {
    std::cerr << "\nonStatusRetrieved called" << std::endl;
    m_controller.fetch<ndn::nfd::FibDataset>(std::bind(&Daemon::onFibStatusRetrieved, this, _1),
                                             std::bind(&Daemon::onStatusTimeout, this));

    //m_scheduler.scheduleEvent(ndn::time::seconds(SCAN_FIB_INTERVAL), std::bind(&Daemon::requestNfdStatus, this));
  }

  void
  onFibStatusRetrieved(const std::vector<ndn::nfd::FibEntry>& status)
  {
    std::cerr << "\nonFibStatusRetrieved called" << std::endl;

    for (auto const& fibEntry : status) {
      std::cerr << fibEntry.getPrefix() << std::endl;
    }
  }

  void 
  onProbeRIB(const std::vector<ndn::nfd::RibEntry>& status, const ndn::Name& interestName, uint64_t incomingFaceId)
  {
    std::vector<std::string> prefixesToReturn;
    std::string prefixesToReturnStr = "";
    std::string local1 = "/localhost";
    std::string local2 = "/localhop/wifidirect";

    for (auto const& ribEntry : status) {
      if(ribEntry.getRoutes()[0].getFaceId() != incomingFaceId) {

        std::stringstream sstream;  
        sstream << ribEntry.getName();
        std::string s = sstream.str();

        if (!(s.compare(0, local1.length(), local1) == 0) && !(s.compare(0, local2.length(), local2) == 0)) {
          prefixesToReturn.push_back(s);
        }
      }
    }

    for ( auto &i : prefixesToReturn ) 
    {
      prefixesToReturnStr += i;
      prefixesToReturnStr += "\n";
    }

    // create data packet with the same name as interest
    std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(interestName);
    

    // prepare and assign content of the data packet
    std::ostringstream os;
    os << prefixesToReturnStr << std::endl;
    std::string content = os.str();

    //std::cerr << "\nInterest packet now looks like: " << content << std::endl;
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    // set metainfo parameters
    data->setFreshnessPeriod(ndn::time::seconds(10));

    // sign data packet
    m_keyChain.sign(*data);

    // make data packet available for fetching
    m_face.put(*data);
  }

  void
  onStatusTimeout()
  {
    std::cerr << "\nonStatusTimeout called" << std::endl;

    std::cerr << "Should not really happen, most likely a serious problem" << std::endl;
    m_scheduler.scheduleEvent(ndn::time::seconds(SCAN_FIB_INTERVAL), std::bind(&Daemon::requestNfdStatus, this));
  }

private:
  void
  requestNfdStatus()
  {
    std::cerr << "\nrequestNfdStatus called" << std::endl;
    m_controller.fetch<ndn::nfd::ForwarderGeneralStatusDataset>(std::bind(&Daemon::onStatusRetrieved, this, _1),
                                                                std::bind(&Daemon::onStatusTimeout, this));
  }

  void
  sendProbe()
  {
    // if GO: probe all the clients connected
    // else : probe only the group owner
    // /?MustBeFresh=True
    std::stringstream ss;
    ss << m_baseName << "/" << m_otherIP << "/" << m_ownIP << "/probe" << m_counter;
    std::string probeFormat = ss.str();

    m_counter++;

    ndn::Name nextName = ndn::Name(probeFormat);
    std::cerr << ">> Sending probe interest: " << probeFormat << std::endl;

    m_face.expressInterest(ndn::Interest(nextName).setMustBeFresh(true),
                           std::bind(&Daemon::onData, this, _2),
                           std::bind(&Daemon::onTimeout, this, _1));

    //if(m_go == 0)
      m_scheduler.scheduleEvent(ndn::time::seconds(SCAN_FIB_INTERVAL), std::bind(&Daemon::sendProbe, this));
  }

  void
  onInterest(const ndn::Interest& interest)
  {
    std::cerr << "<< Interest received: " << interest << std::endl;

    const ndn::Name& name = interest.getName();
    auto incomingFaceIdTag = interest.getTag<ndn::lp::IncomingFaceIdTag>();

    if(incomingFaceIdTag != NULL)
        m_controller.fetch<ndn::nfd::RibDataset>(std::bind(&Daemon::onProbeRIB, this, _1, name, *incomingFaceIdTag),
                                             std::bind(&Daemon::onStatusTimeout, this));
  }

  void
  onData(const ndn::Data& data)
  {
    //std:: string dataReceived;
    //dataReceived = reinterpret_cast<const char*>(data.getContent().value());
    //reinterpret_cast<const char*>(data.getContent().value()), data.getContent().value_size()

    const ndn::Block& content = data.getContent();
    std::istringstream newPrefixesFromNeighbor(ndn::encoding::readString (content));
    std::string line;
    auto incomingFaceIdTag = data.getTag<ndn::lp::IncomingFaceIdTag>();
    
    std::cerr << "<< Received data: \n";
    while (std::getline(newPrefixesFromNeighbor, line)) {
      std::cout << line << std::endl;
      if(line[0]=='/')
        addRoute(line, *incomingFaceIdTag);
    }
  }

  void
  onTimeout(const ndn::Interest& interest)
  {
    // re-express interest
    std::cerr << "<< Timeout for " << interest << std::endl;
    m_face.expressInterest(interest.getName(),
                           std::bind(&Daemon::onData, this, _2),
                           std::bind(&Daemon::onTimeout, this, _1));
  }

private:
  ndn::Face& m_face;
  ndn::KeyChain m_keyChain;
  ndn::nfd::Controller m_controller;
  ndn::Name m_baseName;
  ndn::Scheduler m_scheduler;
  std::string m_ownIP;
  std::string m_otherIP;
  uint64_t m_go;
  uint64_t m_counter;
  uint64_t m_tempFaceId;
};


/*************************************************** UTIL FUNCTIONS ***************************************************/

std::string 
getOwnIP() 
{
  //char *IP = "fe80::76da:38ff:fe8d:89ab";
  //std::string IP = "fe80::76da:38ff:fe8f:5319";
  //std::string IP = "GOaaaaaaaaaaaaaaaaaa";

  std::string IP = "fe80::76da:38ff:fe8d:89ab";
  return IP;
}

std::string 
getOtherIP() 
{
  //char *IP = "fe80::76da:38ff:fe8d:89ab";
  //std::string IP = "fe80::76da:38ff:fe8d:89ab";
  //std::string IP = "NonGObbbbbbbbbbbbbbb";
  
  std::string IP = "fe80::76da:38ff:fe8f:5319";
  return IP;
}

int connectDevices() {
  int go;

  while(true) {
    std::cerr << "\n===========================================================" << std::endl;    
    std::cerr << "Do you wish to connect as a Group Owner (0) or a Non Group Owner (1)?" << std::endl;
    std::cin >> go;

    if(go==0) {
      std::cerr << "You chose to be Group Owner!" << std::endl;
      break;
    } else if (go==1) {
      std::cerr << "You chose to be Non group owner!" << std::endl;
      break;
    } else {
      std::cerr << "Incorrect option selected. Reselect!" << std::endl;
    }
  } 

  //system("/home/ubuntu/wpa_supplicant-2.6/wpa_supplicant/wpa_supplicant -Dnl80211 -c p2p.conf -i wlan1");
  //sleep(3);
  //system("/home/ubuntu/wpa_supplicant-2.6/wpa_supplicant/wpa_cli");
  return go;
}

/*************************************************** MAIN ***************************************************/
int
main(int argc, char** argv)
{
  int tempFaceId;

  std::cerr << "Enter face ID for interface wlx74da388f5319: " << std::endl;
  std::cin >> tempFaceId;

  try {
    //go is 1 if this device is a Group Owner
    int go = connectDevices();
    std::string ownIP = getOwnIP();
    std::string otherIP = getOtherIP();

    ndn::Face face;

    if(go==1) {
      std::string temp = ownIP;
      ownIP = otherIP;
      otherIP = temp;
    }

    // create daemon instance
    Daemon daemon(face, ownIP, otherIP, go, tempFaceId);

    // start processing loop (it will block forever)
    face.processEvents();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
