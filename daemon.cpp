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

#define SCAN_FIB_INTERVAL 20

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>
#include <ndn-cxx/security/key-chain.hpp>

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
  Daemon(ndn::Face& face, std::string ownIP, std::string otherIP, uint64_t go)
    : m_face(face)
    , m_controller(m_face, m_keyChain)
    , m_baseName("/localhop/wifidirect")
    , m_scheduler(m_face.getIoService())
    , m_ownIP(std::move(ownIP))
    , m_otherIP(std::move(otherIP))
    , m_announcePrefixes(std::move(""))
    , m_go(go)
    , m_counter(0)
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
    this->addRoute(m_otherIP);

    if(m_go == 0)
      m_scheduler.scheduleEvent(ndn::time::seconds(SCAN_FIB_INTERVAL), std::bind(&Daemon::sendProbe, this));

    // std::cerr << "\n===========================================================" << std::endl;    
    // std::cerr << "\n\nAdding face:" << std::endl;    
    // this->addFace();
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

  void
  addRoute(std::string IP)
  {
    ndn::nfd::ControlParameters params;
    std::string remoteHostIP = "/localhop/wifidirect/"+IP;
    std::cerr << "Remote host IP is " << remoteHostIP << std::endl;
    params.setName(remoteHostIP);

    //set the FaceID of the face created towards the WiFi direct interface
    //params.setFaceId(1);
    params.setExpirationPeriod(ndn::time::seconds(100));

    ndn::nfd::CommandOptions options;
    //options.setPrefix("");

    m_controller.start<ndn::nfd::RibRegisterCommand>
      (params, [&] (const ndn::nfd::ControlParameters&) 
      {
        std::cerr << "Successfully created a route" << std::endl;

        m_scheduler.scheduleEvent(ndn::time::seconds(100), [] 
        {
            std::cerr << "DONE" << std::endl;
        });
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
      // if (fibEntry.getPrefix() == m_localhopFibEntry) {
      //   isConnectedToHub = true;
      // }
      // else if (fibEntry.getPrefix() == m_adhocFibEntry) {
      //   isConnectedToAdhoc = true;
      // }
    }
  }

  void 
  onProbeFIB(const std::vector<ndn::nfd::FibEntry>& status)
  {
    std::cerr << "\nonFibStatusRetrieved called" << std::endl;

    std::vector<std::string> nonLocalPrefixesVector;
    std::string nonLocalPrefixes = "";
    std::string local1 = "/localhost";
    std::string local2 = "/localhop/wifidirect";

    for (auto const& fibEntry : status) {
      std::stringstream sstream;  
      sstream << fibEntry.getPrefix();
      std::string s = sstream.str();
      if (s.compare(0, local1.length(), local1) == 0 || s.compare(0, local2.length(), local2) == 0) {
        std::cerr << "Skipping local entry " << fibEntry.getPrefix() << std::endl;
      }
      else {
        std::stringstream sstream;  
        sstream << fibEntry.getPrefix();
        std::string s = sstream.str();
        nonLocalPrefixesVector.push_back(s);
      }
    }

    nonLocalPrefixes += std::to_string(nonLocalPrefixesVector.size());
    nonLocalPrefixes += "\n";

    for ( auto &i : nonLocalPrefixesVector ) {
      nonLocalPrefixes += i;
      nonLocalPrefixes += "\n";
    }
    //std::cerr << "Interest packet looks like this now:\n" << nonLocalPrefixes << std::endl;
    m_announcePrefixes = nonLocalPrefixes;
    std::cerr << "Interest packet looks like this now:\n " << m_announcePrefixes << std::endl;
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
  }

  void
  onInterest(const ndn::Interest& interest)
  {
    std::cerr << "<< Interest received: " << interest << std::endl;

    m_controller.fetch<ndn::nfd::FibDataset>(std::bind(&Daemon::onProbeFIB, this, _1),
                                             std::bind(&Daemon::onStatusTimeout, this));
    
    sleep(1);   
    // create data packet with the same name as interest
    std::shared_ptr<ndn::Data> data = std::make_shared<ndn::Data>(interest.getName());

    std::cerr << "Interest packet data = " << m_announcePrefixes << std::endl;
    // prepare and assign content of the data packet
    std::ostringstream os;
    os << m_announcePrefixes << std::endl;
    std::string content = os.str();
    data->setContent(reinterpret_cast<const uint8_t*>(content.c_str()), content.size());

    // set metainfo parameters
    data->setFreshnessPeriod(ndn::time::seconds(10));

    // sign data packet
    m_keyChain.sign(*data);

    // make data packet available for fetching
    m_face.put(*data);
  }

  void
  onData(const ndn::Data& data)
  {
    std::cerr << "<< Received data: \n"
              << std::string(reinterpret_cast<const char*>(data.getContent().value()),
                                                           data.getContent().value_size())
              << std::endl;

    if (data.getName().get(-1).toSequenceNumber() >= 10) {
      return;
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
  std::string m_announcePrefixes;
  uint64_t m_go;
  uint64_t m_counter;
};


/*************************************************** UTIL FUNCTIONS ***************************************************/

std::string 
getOwnIP() 
{
  //char *IP = "fe80::76da:38ff:fe8d:89ab";
  //std::string IP = "fe80::76da:38ff:fe8f:5319";
  std::string IP = "GOaaaaaaaaaaaaaaaaaa";
  return IP;
}

std::string 
getOtherIP() 
{
  //char *IP = "fe80::76da:38ff:fe8d:89ab";
  //std::string IP = "fe80::76da:38ff:fe8d:89ab";
  std::string IP = "NonGObbbbbbbbbbbbbbb";
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
  system("/Users/amar/Desktop/a.out 1 2 3");

  return go;
}

/*************************************************** MAIN ***************************************************/
int
main(int argc, char** argv)
{
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
    Daemon daemon(face, ownIP, otherIP, go);

    // start processing loop (it will block forever)
    face.processEvents();
  }
  catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
  }
  return 0;
}
