import asyncio
import logging
from bleak import BleakScanner, BleakClient

# Event to watch for new data
battery_level_event = asyncio.Event()
battery_ampere_event = asyncio.Event()
battery_voltage_event = asyncio.Event()

ble_read_event = asyncio.Event()

# Global to resources
battery_level_value = 0
battery_ampere_value = 0
battery_voltage_value = 0

# Globals for read characteristic data
ble_read_value = ""
ble_read_uuid = ""

log = logging.getLogger('handlers')


# easy setter for globals
def change_battery_level(new_value):
    global battery_level_value
    battery_level_value = new_value


def change_battery_ampere(new_value):
    global battery_ampere_value
    battery_ampere_value = new_value


def change_battery_voltage(new_value):
    global battery_voltage_value
    battery_voltage_value = new_value


def store_read_value(new_value):
    global ble_read_value
    ble_read_value = new_value


# Flags for canceling observe
cancel_flags_observe = {"observe_3411_0_1": False, "observe_3411_0_2": False, "observe_3411_0_3": False, "disconnect_ble": False}

# Flags for if to start observe
start_flags_observe = {"battery_level": False, "battery_ampere": False, "battery_voltage": False}

# Flag if to read gatt
read_ble_value = False

def handle_factory_reset(*args, **kwargs):
    global cancel_flags_observe
    cancel_flags_observe["disconnect_ble"] = True
    log.info(f'handle_factory_reset(): {args}, {kwargs}')

# Main function to read from BLE
async def read_ble():
    global cancel_flags_observe, start_flags_observe
    devices = await BleakScanner.discover()
    device = next((d for d in devices if d.name == "BLE Battery Demo"), None)
    if device:
        async with BleakClient(device) as client:

            async def control_connection():
                if cancel_flags_observe["disconnect_ble"]:
                    await client.disconnect()

            # Reads from gatt with uuid from global
            async def read_value_from_ble():
                global read_ble_value, ble_read_uuid
                if read_ble_value:
                    data = await client.read_gatt_char(ble_read_uuid)
                    return_value = int.from_bytes(data, byteorder="little")  # Bytesarray is returned
                    store_read_value(return_value)
                    #print("UUID: ", ble_read_uuid, " returned: ", return_value)
                    ble_read_event.set()
                    read_ble_value = False

            # Reads notify return data
            def b_level_notification_handler(sender, data):
                reading_decimal = int.from_bytes(data, byteorder="little")
                change_battery_level(reading_decimal)
                battery_level_event.set()

            def b_voltage_notification_handler(sender, data):
                reading_decimal = int.from_bytes(data, byteorder="little")
                change_battery_voltage(reading_decimal / 1000.0)
                battery_voltage_event.set()

            def b_ampere_notification_handler(sender, data):
                reading_decimal = int.from_bytes(data, byteorder="little")
                change_battery_ampere(reading_decimal / 1000.0)
                battery_ampere_event.set()

            # Controls whether to run observe or not depending on globals
            async def control_observe(uuid, handler, start_flag, cancel_flag):
                global cancel_flags_observe, start_flags_observe

                if start_flags_observe[start_flag]:
                    await client.start_notify(uuid, handler)
                    start_flags_observe[start_flag] = False
                elif cancel_flags_observe[cancel_flag]:
                    await client.stop_notify(uuid)

            # Actively run threads of observes and or read (in current state of aiocoap only one obeserve at a time works correct)
            while True:
                await asyncio.gather(
                    read_value_from_ble(),
                    control_observe("00002A19-0000-1000-8000-00805F9B34FB", b_level_notification_handler,
                                    "battery_level", "observe_3411_0_1"),
                    control_observe("32B4E46D-807F-4E75-ADD7-B08A613E76F3", b_ampere_notification_handler,
                                    "battery_ampere", "observe_3411_0_2"),
                    control_observe("347BA623-F41A-4B59-A508-DE45079B4F20", b_voltage_notification_handler,
                                    "battery_voltage", "observe_3411_0_3"), control_connection()
                )

    else:
        print("BLE device not found")


# Starts read_ble right away
asyncio.ensure_future(read_ble())


# get = observe
async def get_battery_level(model, notifier):
    global cancel_flags_observe, battery_level_value
    while not cancel_flags_observe["observe_3411_0_1"]:
        await battery_level_event.wait()
        battery_level_event.clear()
        #print("New value for resource 1 = ", battery_level_value)
        await model.set_resource('3411', '0', "1", battery_level_value)
        await notifier()


# read = read gatt
async def read_battery_level() -> int:
    global read_ble_value, ble_read_uuid, ble_read_value
    ble_read_uuid = "00002A19-0000-1000-8000-00805F9B34FB"
    read_ble_value = True
    await ble_read_event.wait()
    ble_read_event.clear()
    return ble_read_value


async def get_battery_ampere(model, notifier):
    global cancel_flags_observe, battery_ampere_value
    while not cancel_flags_observe["observe_3411_0_2"]:
        await battery_ampere_event.wait()
        battery_ampere_event.clear()
        #print("New value for resource 2 = ", battery_ampere_value)
        await model.set_resource('3411', '0', "2", battery_ampere_value)
        await notifier()


async def read_battery_ampere() -> float:
    global read_ble_value, ble_read_uuid, ble_read_value
    ble_read_uuid = "32B4E46D-807F-4E75-ADD7-B08A613E76F3"
    read_ble_value = True
    await ble_read_event.wait()
    ble_read_event.clear()
    return ble_read_value / 1000.0


async def get_battery_voltage(model, notifier):
    global cancel_flags_observe, battery_voltage_value
    while not cancel_flags_observe["observe_3411_0_3"]:
        await battery_voltage_event.wait()
        battery_voltage_event.clear()
        #print("New value for resource 3 = ", battery_voltage_value)
        await model.set_resource('3411', '0', "3", battery_voltage_value)
        await notifier()


async def read_battery_voltage() -> float:
    global read_ble_value, ble_read_uuid, ble_read_value
    ble_read_uuid = "347BA623-F41A-4B59-A508-DE45079B4F20"
    read_ble_value = True
    await ble_read_event.wait()
    ble_read_event.clear()
    return ble_read_value / 1000.0


def observe_3411_0_1(*args, **kwargs):
    global cancel_flags_observe, start_flags_observe
    log.info(f'observe_3411_0_1(): {args}, {kwargs}')
    model = kwargs['model']
    notifier = kwargs['notifier']
    cancel_flags_observe["observe_3411_0_1"] = kwargs['cancel']
    start_flags_observe["battery_level"] = True

    asyncio.ensure_future(get_battery_level(model, notifier))


def observe_3411_0_2(*args, **kwargs):
    global cancel_flags_observe, start_flags_observe
    log.info(f'observe_3411_0_2(): {args}, {kwargs}')
    model = kwargs['model']
    notifier = kwargs['notifier']
    cancel_flags_observe["observe_3411_0_2"] = kwargs['cancel']
    start_flags_observe["battery_ampere"] = True

    asyncio.ensure_future(get_battery_ampere(model, notifier))


def observe_3411_0_3(*args, **kwargs):
    global cancel_flags_observe, start_flags_observe
    log.info(f'observe_3411_0_3(): {args}, {kwargs}')
    model = kwargs['model']
    notifier = kwargs['notifier']
    cancel_flags_observe["observe_3411_0_3"] = kwargs['cancel']
    start_flags_observe["battery_voltage"] = True

    asyncio.ensure_future(get_battery_voltage(model, notifier))
