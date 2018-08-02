#!/user/bin/env python

import subprocess
import asyncio
import websockets

program_name = './bin/audioPlatformv2'

def switch(argument):
    switcher = {
            1: "test",
            2: "test2"
        }
    print(switcher.get(argument, "Invalid Message"))


async def react(websocket, path):
    message = await websocket.recv()
    print("received: {}".format(message))

    switch(message)

    p = subprocess.Popen(program_name, shell=True)
    p.wait()
    p.kill()

start_server = websockets.serve(react, "192.168.137.73" , 8765)

asyncio.get_event_loop().run_until_complete(start_server)
asyncio.get_event_loop().run_forever()

