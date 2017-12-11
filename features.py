def case1():
    remote_device = RemoteDevice()
    device = Device()
    assert device.status() == DeviceStatus.STOPPED
    assert device.model.empty()

def case2():
    case1()

    remote_device.connect()
    device.start()
    assert device.status() == DeviceStatus.CONNECTED
    assert device.model.loaded()

    remote_device.disconnect()
    assert device.status() == DeviceStatus.DISCONNECTED
    assert device.model.empty()

    remote_device.connect()
    assert device.status() == DeviceStatus.CONNECTED
    assert device.model.loaded()


def case3():
    case1()

    remote_device.disconnect()
    device.start()
    assert device.status() == DeviceStatus.DISCONNECTED
    assert device.model.empty()

    remote_device.connect()
    assert device.status() == DeviceStatus.CONNECTED
    assert device.model.loaded()
