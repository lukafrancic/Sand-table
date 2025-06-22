import serial
import time
import threading
import numpy as np

class Communication:
    """
    A class with methods that handles encoded messagges for the sand table.

    Available methods
    - update_pos()
    - update_speed()
    - pause()

    :param COM: current COM port to communicate with the Arduino
    :param path_maker: a generator that yields the next point to go to
    """
    BAUDRATE = 115200
    
    HEADER = b"ab"
    POSITION_MSG = b"c"
    UPDATE_SPEED = b"d"
    TOGGLE_MSG = b"e"
    FINISH_MOVE = b"f"


    def __init__(self, COM: str, path_maker):
        self.serial = serial.Serial(COM, baudrate=self.BAUDRATE)
        self.pos = [0, 0]
        self.path_maker = path_maker
        self._event = threading.Event()
        self._is_running = False


    def update_pos(self, pos: list[int, int]) -> None:
        """
        Send new position to Arduino.

        :param pos: list of [r, phi] as position in steps. R and Phi must be
            as int32!

        :return: a bynaryarray that can be directly sent over serial
        """
        if len(pos) != 2:
            raise ValueError("Invalid position")
        # if not (isinstance(pos[0], (int, np.int32)) and 
        # isinstance(pos[1], (int, np.int32))):
        #     raise ValueError("Values must be ints")
        r = int(pos[0])
        phi = int(pos[1])

        pos_r = r.to_bytes(4, "big", signed=True)
        pos_phi = phi.to_bytes(4, "big", signed=True)

        msg = self.HEADER + self.POSITION_MSG + pos_r + pos_phi

        # return msg
        self.serial.write(msg)


    def update_speed(self, speed: int) -> bytearray:
        """
        Update the speed value on the sand table

        :param speed: speed is defined as steps per second as a uint16

        :return: a bynaryarray that can be directly sent over serial
        """

        if not isinstance(speed, int):
            raise TypeError(f"Speed must be an int not {type(speed)}!")
        
        if speed < 0 and speed > 2**16:
            raise ValueError("Speed must be a uint16!")
        
        val = speed.to_bytes(2, "big", signed=False)

        msg = self.HEADER + self.UPDATE_SPEED + val

        # return msg
        self.serial.write(msg)


    def toggle(self) -> bytearray:
        """
        Send a toggle msg to the sand table. The table will start/stop based
        on current condition.

        :return: a bytearray that can be directly sent over serial
        """
        
        msg = self.HEADER + self.TOGGLE_MSG

        # return msg
        self.serial.write(msg)


    def clear_buffer(self):
        #TODO
        pass
    

    def _loop(self):

        while self._event.is_set():
            if self.serial.in_waiting > 0:
                rec = self.serial.read()
                print("rec")
                if rec == self.FINISH_MOVE:
                    pos = next(self.path_maker)
                    self.update_pos(pos)
                    print("\tupdated")

                time.sleep(0.1)
                # self.serial.flushInput()

            time.sleep(0.1)

    
    def begin(self):
        if not self._is_running:
            self._event.set()
            # clear the serial buffer
            # self.serial.flushInput()
            time.sleep(0.1)
            self.toggle()
            time.sleep(0.1)
            self._thread = threading.Thread(target = self._loop)
            self._thread.start()
            self._is_running = True
            print("Starting")

        else:
            print("Thread is already active")

    
    def stop(self):
        if self._is_running:
            self._event.clear()
            self._thread.join()
            self._is_running = False


    def wait_for_response(self, timeout: float = 10) -> bytearray:
        """
        Blocking function that waits on a response from the Arduino

        :param timeout: if no response after N seconds, exit the function.
        """
        t1 = time.monotonic()
        rec = None
        
        while self.serial.in_waiting < 1:
            time.sleep(0.1)
            t2 = time.monotonic()
            if t2-t1 > timeout:
                break
                
        if self.serial.in_waiting >= 1:
            rec = self.serial.read()

        if rec == self.FINISH_MOVE:
            return True
        
        return False

