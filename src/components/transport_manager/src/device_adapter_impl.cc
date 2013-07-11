/**
 * \file device_adapter_impl.cpp
 * \brief Class DeviceAdapterImpl.
 * Copyright (c) 2013, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "transport_manager/device_adapter_impl.h"
#include "transport_manager/device_adapter_listener.h"
#include "transport_manager/device_handle_generator.h"

namespace transport_manager {

namespace device_adapter {

log4cxx::LoggerPtr logger_ = log4cxx::LoggerPtr(
    log4cxx::Logger::getLogger("TransportManager"));

Device::Device(const char* name)
    : name_(name),
      unique_device_id_() {
}

Device::~Device() {
}

Connection::Connection() {
}

Connection::~Connection() {
}

DeviceAdapterImpl::DeviceAdapterImpl()
    : listener_(0),
      handle_generator_(0),
      initialised_(0),
      devices_(),
      devices_mutex_(),
      connections_(),
      connections_mutex_(),
      device_scanner_(0),
      server_connection_factory_(0),
      client_connection_listener_(0) {
  pthread_mutex_init(&devices_mutex_, 0);
  pthread_mutex_init(&connections_mutex_, 0);
}

DeviceAdapterImpl::~DeviceAdapterImpl() {
  pthread_mutex_destroy(&connections_mutex_);
  pthread_mutex_destroy(&devices_mutex_);
}

Error DeviceAdapterImpl::init(DeviceAdapterListener* listener,
                              DeviceHandleGenerator* handle_generator,
                              Configuration* configuration) {

  if (listener == 0)
    return BAD_PARAM;
  if (handle_generator == 0)
    return BAD_PARAM;

  LOG4CXX_INFO(logger_, "Initializing device adapter");

  listener_ = listener;
  handle_generator_ = handle_generator;

  Error error = OK;

  if ((error == OK) && device_scanner_)
    error = device_scanner_->init();
  if ((error == OK) && server_connection_factory_)
    error = server_connection_factory_->init();
  if ((error == OK) && client_connection_listener_)
    error = client_connection_listener_->init();

  initialised_ = error == OK;
  return error;
}

Error DeviceAdapterImpl::searchDevices() {
  if (!initialised_)
    return BAD_STATE;
  if (device_scanner_ == 0)
    return NOT_SUPPORTED;
  return device_scanner_->scan();
}

Error DeviceAdapterImpl::connect(const DeviceHandle device_handle,
                                 const ApplicationHandle app_handle,
                                 const SessionID session_id) {
  if (!initialised_)
    return BAD_STATE;
  if (server_connection_factory_ == 0)
    return NOT_SUPPORTED;

  return server_connection_factory_->createConnection(device_handle, app_handle,
                                                      session_id);
}

Error DeviceAdapterImpl::disconnect(const SessionID session_id) {
  if (!initialised_)
    return BAD_STATE;
  ConnectionSptr connection = findEstablishedConnection(session_id);
  if (connection.get() != 0) {
    return connection->disconnect();
  } else {
    return BAD_PARAM;
  }
}

Error DeviceAdapterImpl::disconnectDevice(const DeviceHandle device_handle) {
  if (!initialised_)
    return BAD_STATE;

  Error error = OK;
  pthread_mutex_lock(&connections_mutex_);
  for (ConnectionMap::iterator it = connections_.begin();
      it != connections_.end(); ++it) {
    ConnectionInfo& info = it->second;
    if (info.device_handle == device_handle
        && info.state == ConnectionInfo::ESTABLISHED) {
      if (OK != info.connection->disconnect()) {
        error = FAIL;
      }
    }
  }
  pthread_mutex_unlock(&connections_mutex_);

  return OK;
}

Error DeviceAdapterImpl::sendData(const int session_id,
                                  const RawMessageSptr data) {
  if (!initialised_)
    return BAD_STATE;

  ConnectionSptr connection = findEstablishedConnection(session_id);
  if (connection.get() != 0) {
    connection->sendData(data);
    return OK;
  } else {
    return BAD_PARAM;
  }
}

DeviceList DeviceAdapterImpl::getDeviceList() const {
  DeviceList devices;
  pthread_mutex_lock(&devices_mutex_);
  for (DeviceMap::const_iterator it = devices_.begin(); it != devices_.end();
      ++it) {
    devices.push_back(it->first);
  }
  pthread_mutex_unlock(&devices_mutex_);
  return devices;
}

DeviceAdapterListener* DeviceAdapterImpl::getListener() {
  return listener_;
}

DeviceHandleGenerator* DeviceAdapterImpl::getDeviceHandleGenerator() {
  return handle_generator_;
}

void DeviceAdapterImpl::addDevice(DeviceSptr device) {
  DeviceHandle handle;

  bool same_device_found = false;
  pthread_mutex_lock(&devices_mutex_);
  for (DeviceMap::const_iterator i = devices_.begin(); i != devices_.end();
      ++i) {
    const DeviceSptr other_device = i->second;
    if (device->isSameAs(other_device.get())) {
      same_device_found = true;
      break;
    }
  }
  if (!same_device_found) {
    DeviceHandle handle = handle_generator_->generate();
    devices_[handle] = device;
  }
  pthread_mutex_unlock(&devices_mutex_);
}

void DeviceAdapterImpl::searchDeviceDone(const DeviceVector& devices) {
  DeviceMap new_devices;
  for (DeviceVector::const_iterator it = devices.begin(); it != devices.end();
      ++it) {
    DeviceSptr device = *it;
    DeviceHandle device_handle;
    bool device_found = false;

    pthread_mutex_lock(&devices_mutex_);
    for (DeviceMap::iterator it = devices_.begin(); it != devices_.end();
        ++it) {
      DeviceSptr existing_device = it->second;
      if (device->isSameAs(existing_device.get())) {
        device_handle = it->first;
        device_found = true;
        break;
      }
    }
    pthread_mutex_unlock(&devices_mutex_);

    if (!device_found) {
      device_handle = handle_generator_->generate();
      LOG4CXX_INFO(
          logger_,
          "Adding new device " << device_handle << " (\"" << device->name() << "\")");
    }

    new_devices[device_handle] = device;
  }

  pthread_mutex_lock(&connections_mutex_);
  std::set<DeviceHandle> connected_devices;
  for (ConnectionMap::const_iterator it = connections_.begin();
      it != connections_.end(); ++it) {
    const ConnectionInfo& info = it->second;
    if (info.state != ConnectionInfo::FINALISING)
      connected_devices.insert(info.device_handle);
  }
  pthread_mutex_unlock(&connections_mutex_);

  pthread_mutex_lock(&devices_mutex_);
  for (DeviceMap::iterator it = devices_.begin(); it != devices_.end(); ++it) {
    DeviceSptr existing_device = it->second;

    if (new_devices.end() == new_devices.find(it->first)) {
      if (connected_devices.end() != connected_devices.find(it->first)) {
        new_devices[it->first] = existing_device;
      }
    }
  }
  devices_ = new_devices;
  pthread_mutex_unlock(&devices_mutex_);

  listener_->onSearchDeviceDone(this);
}

void DeviceAdapterImpl::searchDeviceFailed(const SearchDeviceError& error) {
  listener_->onSearchDeviceFailed(this, error);
}

void DeviceAdapterImpl::setDeviceScanner(DeviceScanner* device_scanner) {
  device_scanner_ = device_scanner;
}

void DeviceAdapterImpl::setServerConnectionFactory(
    ServerConnectionFactory* server_connection_factory) {
  server_connection_factory_ = server_connection_factory;
}

void DeviceAdapterImpl::setClientConnectionListener(
    ClientConnectionListener* client_connection_listener) {
  client_connection_listener_ = client_connection_listener;
}

bool DeviceAdapterImpl::isSearchDevicesSupported() const {
  return device_scanner_ != 0;
}

bool DeviceAdapterImpl::isServerOriginatedConnectSupported() const {
  return server_connection_factory_ != 0;
}

bool DeviceAdapterImpl::isClientOriginatedConnectSupported() const {
  return client_connection_listener_ != 0;
}

void DeviceAdapterImpl::connectionCreated(ConnectionSptr connection,
                                          const SessionID session_id,
                                          const DeviceHandle device_handle,
                                          const ApplicationHandle app_handle) {
  pthread_mutex_lock(&connections_mutex_);
  ConnectionInfo& info = connections_[session_id];
  info.app_handle = app_handle;
  info.device_handle = device_handle;
  info.connection = connection;
  info.state = ConnectionInfo::NEW;
  pthread_mutex_unlock(&connections_mutex_);
}

void DeviceAdapterImpl::disconnnectDone(const SessionID session_id) {
  pthread_mutex_lock(&connections_mutex_);
  connections_.erase(session_id);
  pthread_mutex_unlock(&connections_mutex_);
  listener_->onDisconnectDone(this, session_id);
}

void DeviceAdapterImpl::dataReceiveDone(const SessionID session_id,
                                        RawMessageSptr message) {
  listener_->onDataReceiveDone(this, session_id, message);
}

void DeviceAdapterImpl::dataReceiveFailed(const SessionID session_id,
                                          const DataReceiveError& error) {
  listener_->onDataReceiveFailed(this, session_id, error);
}

void DeviceAdapterImpl::dataSendDone(const SessionID session_id,
                                     RawMessageSptr message) {
  listener_->onDataSendDone(this, session_id, message);
}

void DeviceAdapterImpl::dataSendFailed(const SessionID session_id,
                                       RawMessageSptr message,
                                       const DataSendError& error) {
  listener_->onDataSendFailed(this, session_id, message, error);
}

DeviceSptr DeviceAdapterImpl::findDevice(DeviceHandle device_handle) const {
  DeviceSptr ret;
  pthread_mutex_lock(&devices_mutex_);
  DeviceMap::const_iterator it = devices_.find(device_handle);
  if(it != devices_.end())
    ret = it->second;
  pthread_mutex_unlock(&devices_mutex_);
  return ret;
}

void DeviceAdapterImpl::connectDone(const SessionID session_id) {
  pthread_mutex_lock(&connections_mutex_);
  ConnectionInfo& info = connections_[session_id];
  info.state = ConnectionInfo::ESTABLISHED;
  pthread_mutex_unlock(&connections_mutex_);
  listener_->onConnectDone(this, session_id);
}

void DeviceAdapterImpl::connectFailed(const SessionID session_id,
                                      const ConnectError& error) {
  pthread_mutex_lock(&connections_mutex_);
  connections_.erase(session_id);
  pthread_mutex_unlock(&connections_mutex_);
  listener_->onConnectFailed(this, session_id, error);
}

ConnectionSptr DeviceAdapterImpl::findEstablishedConnection(
    const SessionID session_id) {
  ConnectionSptr connection;
  pthread_mutex_lock(&connections_mutex_);
  ConnectionMap::iterator it = connections_.find(session_id);
  if (it != connections_.end()) {
    ConnectionInfo& info = it->second;
    if (info.state == ConnectionInfo::ESTABLISHED)
      connection = info.connection;
  }
  pthread_mutex_unlock(&connections_mutex_);
  return connection;
}

}  // namespace device_adapter

}  // namespace transport_manager

