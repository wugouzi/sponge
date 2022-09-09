#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"
#include "ipv4_datagram.hh"
#include "parser.hh"

#include <iostream>
#include <optional>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
  cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
       << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
  // convert IP address of next hop to raw 32-bit representation (used in ARP header)
  const uint32_t next_hop_ip = next_hop.ipv4_numeric();
  EthernetFrame frame;
  EthernetHeader &head = frame.header();
    
  frame.payload() = dgram.serialize();
    
  head.type = EthernetHeader::TYPE_IPv4;
  head.src = _ethernet_address;
  if (_arp_table.find(next_hop_ip) == _arp_table.end()) {
    if (_arp_time.find(next_hop_ip) == _arp_time.end() ||
        _arp_time[next_hop_ip] + 5000 <= _cur_time) {
      _arp_time[next_hop_ip] = _cur_time;
      _waiting_frame[next_hop_ip] = frame;
      // now send arp
      EthernetFrame arp;
      EthernetHeader &arp_head = arp.header();
      arp_head.type = EthernetHeader::TYPE_ARP;
      arp_head.src = _ethernet_address;
      arp_head.dst = ETHERNET_BROADCAST;

      ARPMessage arp_message;
      arp_message.sender_ip_address = _ip_address.ipv4_numeric();
      arp_message.sender_ethernet_address = _ethernet_address;
      arp_message.target_ip_address = next_hop_ip;
      arp_message.opcode = 1;

      arp.payload() = arp_message.serialize();

      _frames_out.push(arp);
    }
    return;
  }
  head.dst = _arp_table[next_hop_ip];
  _frames_out.push(frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
  if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST)
    return nullopt;
  const EthernetHeader &head = frame.header();
  if (head.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram idgram;
    if (idgram.parse(frame.payload()) != ParseResult::NoError)
      return nullopt;
    return idgram;
  }

  ARPMessage arp;
  if (arp.parse(frame.payload()) != ParseResult::NoError)
    return nullopt;
  // remember
  _arp_table[arp.sender_ip_address] = arp.sender_ethernet_address;
  _arp_time[arp.sender_ip_address] = _cur_time;
  if (frame.header().dst == ETHERNET_BROADCAST) {
    if (arp.target_ip_address == _ip_address.ipv4_numeric()) {
      ARPMessage arp_reply;
      arp_reply.target_ip_address = arp.sender_ip_address;
      arp_reply.target_ethernet_address = arp.sender_ethernet_address;
      arp_reply.sender_ethernet_address = _ethernet_address;
      arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
      arp_reply.opcode = 2;

      EthernetFrame frame_reply;
      frame_reply.payload() = arp_reply.serialize();
      frame_reply.header().type = EthernetHeader::TYPE_ARP;
      frame_reply.header().dst = arp.sender_ethernet_address;
      frame_reply.header().src = _ethernet_address;
      _frames_out.push(frame_reply);
    }
    return nullopt;
  }
  if (_waiting_frame.find(arp.sender_ip_address) != _waiting_frame.end()) {
    EthernetFrame &wait_frame = _waiting_frame[arp.sender_ip_address];
    wait_frame.header().dst = arp.sender_ethernet_address;
    _frames_out.push(wait_frame);
    _waiting_frame.erase(arp.sender_ip_address);
  }
  return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
  _cur_time += ms_since_last_tick;

  // arp cache timeout
  for (auto it = _arp_time.begin(); it != _arp_time.end();) {
    if (_cur_time >= 30 * 1000 + it->second) {
      _arp_table.erase(it->first);
      it = _arp_time.erase(it);
    }
    else
      break;
  }
}
