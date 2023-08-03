#

# tinyssb/simplepub/ble.py
# Bluetooth Low Energy interface

# (c) Jul 2023 <christian.tschudin@unibas.ch>

'''
Enables this central device to connect to a tinySSB peripheral
device that exposes the READFRM characteristics. Other (central)
tinySSB devices like the Tremola VoSSBoL app are ignored.
'''

try:
    import asyncio
    import queue
    import time
    import zlib

    from bleak import BleakScanner, BleakClient, BleakGATTCharacteristic
    from bleak.backends.device import BLEDevice
    from bleak.backends.scanner import AdvertisementData
    
    is_installed = True
            
    known_devices = {}
    peer_candidates = queue.Queue()

    class ConnectionGone(Exception):
        pass

    TINYSSB_SERVICE_UUID = '6e400001-7646-4b5b-9a50-71becce51558'
    TINYSSB_WRITETO_UUID = '6e400002-7646-4b5b-9a50-71becce51558'
    TINYSSB_READFRM_UUID = '6e400003-7646-4b5b-9a50-71becce51558'

    class BleSocket:

        nm = 'b'

        def __init__(self, client):
            self.client = client
            self.outgoing = queue.Queue()
            self.incoming = queue.Queue()
            self.closed = False
    
        async def send(self, buf):
            self.outgoing.put(buf)

        async def recv(self):
            while True:
                if self.closed:               raise ConnectionGone()
                if not self.incoming.empty(): return self.incoming.get()
                await asyncio.sleep(0.1)

        def _upcall(self, c: BleakGATTCharacteristic, d: bytearray):
            self.incoming.put(bytes(d))

        async def _emitter(self):
            while True:
                if self.outgoing.empty():
                    await asyncio.sleep(0.1)
                    continue
                pkt = self.outgoing.get()
                try:
                    await self.client.write_gatt_char(TINYSSB_WRITETO_UUID,
                                                      pkt, response=True)
                except Exception as e:
                    # print(e)
                    self.closed = True
                    return
                await asyncio.sleep(0.1)
        pass

    def onBleAdv(device: BLEDevice, advertisement_data: AdvertisementData):
        if not device.address in known_devices:
            known_devices[device.address] = True
            peer_candidates.put(device.address)

    async def ble_listener(onConnect):
        print("BLE listener started")

        def onDisc(client):
            known_devices[client.address].closed = True
            del known_devices[client.address]
            print(f"   BLE {client.address} - gone")

        async def ble_connection(a):
            async with BleakClient(a, disconnected_callback=onDisc) as client:
                known_devices[a] = True # client.address

                print(f"   BLE {a} - connected")
                srvc = client.services[TINYSSB_SERVICE_UUID]
                rd = None
                for c in srvc.characteristics:
                    if c.uuid == TINYSSB_READFRM_UUID and 'read' in c.properties:
                        # print('found', c.uuid)
                        rd = c
                        break
                if rd == None:
                    print(f"   BLE {a} - no READFRM, ignoring this device")
                    return

                bsock = BleSocket(client)
                known_devices[a] = bsock
                await client.start_notify(TINYSSB_READFRM_UUID,
                                          lambda c,d: bsock._upcall(c,d))
                asyncio.create_task(bsock._emitter())
                await onConnect(bsock)

        while True:
            if peer_candidates.empty():
                await asyncio.sleep(0.1)
                continue
            a = peer_candidates.get()
            asyncio.create_task(ble_connection(a))


    async def serve(onConnect):
        print("BLE scanner started")
        scanner = BleakScanner(onBleAdv, [TINYSSB_SERVICE_UUID],
                               # cb=dict(use_bdaddr=True)
        )
        asyncio.create_task(ble_listener(onConnect))
        while True: # print("(re)starting scanner")
            await scanner.start()
            await asyncio.sleep(5.0)
            await scanner.stop()
    
except Exception as e:
    # print("BLE install failed", e)
    is_installed = False

# eof
